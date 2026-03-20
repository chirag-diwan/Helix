#include "network.h"

#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <git2.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

using json   = nlohmann::json;
namespace fs = std::filesystem;

SemVer ParseVersion(const std::string& v) {
  SemVer sv;
  sv.original       = v;
  const char* start = v.c_str();
  if (*start == 'v') start++;
  sscanf(start, "%d.%d.%d", &sv.major, &sv.minor, &sv.patch);
  return sv;
}

std::string ExtractRepoBase(const std::string& url) {
  std::string base = fs::path(url).filename().string();
  if (base.length() >= 4 && base.compare(base.length() - 4, 4, ".git") == 0) {
    base.erase(base.length() - 4);
  }
  return base;
}

std::pair<std::string, std::string> ParseGitHubUrl(const std::string& url) {
  std::regex  pattern(R"(github\.com/([^/]+)/([^/.]+))");
  std::smatch match;
  if (std::regex_search(url, match, pattern)) {
    return {match[1].str(), match[2].str()};
  }
  return {"", ""};
}

size_t CurlWriteCallback(void* ptr, size_t size, size_t nmemb, void* stream) {
  auto* out = static_cast<std::ofstream*>(stream);
  out->write(static_cast<char*>(ptr), size * nmemb);
  return size * nmemb;
}

bool DownloadTarball(const std::string& url, const std::string& out_path) {
  CURL* curl = curl_easy_init();
  if (!curl) return false;

  std::ofstream file(out_path, std::ios::binary);
  if (!file) {
    curl_easy_cleanup(curl);
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Helix-PackageManager/1.0");

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  file.close();

  if (res != CURLE_OK) {
    std::cerr << "Download failed: " << curl_easy_strerror(res) << "\n";
    fs::remove(out_path);
    return false;
  }
  return true;
}

void ExtractAndStripOptimized(const std::string& filename, const std::string& dest_dir) {
  std::unique_ptr<struct archive, ArchiveReadDeleter>  a(archive_read_new());
  std::unique_ptr<struct archive, ArchiveWriteDeleter> ext(archive_write_disk_new());

  if (!a || !ext) {
    throw std::runtime_error("Failed to allocate archive structures.");
  }

  // Dropped ARCHIVE_EXTRACT_UNLINK to stop journal thrashing.
  // Added TIME and PERM for data integrity.
  int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_SECURE_NODOTDOT;

  archive_read_support_format_tar(a.get());

  // Attempt multithreaded decompression first. Fallback to native single-threaded if pigz is missing.
  if (archive_read_support_filter_program(a.get(), "pigz -d") != ARCHIVE_OK) {
    archive_read_support_filter_gzip(a.get());
  }

  archive_write_disk_set_options(ext.get(), flags);
  // standard_lookup removed. Do not burn CPU cycles querying the OS for UIDs/GIDs you aren't using.

  struct stat st;
  size_t      block_size = (stat(filename.c_str(), &st) == 0 && st.st_blksize > 0)
                               ? st.st_blksize * 16
                               : 1048576;

  if (archive_read_open_filename(a.get(), filename.c_str(), block_size) != ARCHIVE_OK) {
    throw std::runtime_error(archive_error_string(a.get()));
  }

  std::string base_dir = dest_dir;
  if (!base_dir.empty() && base_dir.back() != '/') base_dir += '/';

  std::string target_path;
  target_path.reserve(1024);

  struct archive_entry* entry;
  while (archive_read_next_header(a.get(), &entry) == ARCHIVE_OK) {
    const char* current_path = archive_entry_pathname(entry);
    const char* first_slash  = strchr(current_path, '/');

    if (!first_slash || *(first_slash + 1) == '\0') {
      archive_read_data_skip(a.get());
      continue;
    }

    target_path.assign(base_dir).append(first_slash + 1);
    archive_entry_set_pathname(entry, target_path.c_str());

    if (archive_write_header(ext.get(), entry) != ARCHIVE_OK) {
      continue;
    }

    const void* buff;
    size_t      size;
    int64_t     offset;

    while (archive_read_data_block(a.get(), &buff, &size, &offset) == ARCHIVE_OK) {
      if (archive_write_data_block(ext.get(), buff, size, offset) != ARCHIVE_OK) {
        break; // Disk full or permission error mid-write
      }
    }
    archive_write_finish_entry(ext.get());
  }
}

RemoteInfo GetRemoteInfo(const std::string& github_url) {
  git_repository* repo   = nullptr;
  git_remote*     remote = nullptr;
  RemoteInfo      info;

  auto cleanup = [&]() {
    if (remote) git_remote_free(remote);
    if (repo) git_repository_free(repo);
  };

  if (git_repository_init(&repo, "/tmp/helix_empty_repo", 1) != 0) {
    cleanup();
    return info;
  }
  if (git_remote_create_anonymous(&remote, repo, github_url.c_str()) != 0) {
    cleanup();
    return info;
  }
  if (git_remote_connect(remote, GIT_DIRECTION_FETCH, nullptr, nullptr,
                         nullptr) != 0) {
    cleanup();
    return info;
  }

  const git_remote_head** refs     = nullptr;
  size_t                  refs_len = 0;

  if (git_remote_ls(&refs, &refs_len, remote) == 0) {
    std::regex          pattern(R"(refs/tags/(v?\d+\.\d+\.\d+))");
    std::vector<SemVer> parsed_versions;

    for (size_t i = 0; i < refs_len; i++) {
      std::string_view ref_name(refs[i]->name);

      if (ref_name == "HEAD") {
        char oid_str[GIT_OID_HEXSZ + 1] = {0};
        git_oid_fmt(oid_str, &refs[i]->oid);
        info.head_oid = oid_str;
        continue;
      }

      std::smatch match;
      std::string ref_str(ref_name);
      if (std::regex_search(ref_str, match, pattern)) {
        parsed_versions.push_back(ParseVersion(match[1].str()));
      }
    }

    if (!parsed_versions.empty()) {
      std::sort(parsed_versions.begin(), parsed_versions.end());
      info.tag = parsed_versions.back().original;
    }
  }

  cleanup();
  return info;
}

void AddDeps(const std::string& github_url) {
  if (!fs::exists("helix.json")) {
    std::cerr << "Fatal: helix.json not found. Run 'init' first.\n";
    return;
  }

  auto [owner, repo_name] = ParseGitHubUrl(github_url);
  if (owner.empty() || repo_name.empty()) {
    std::cerr << "Fatal: Invalid GitHub URL.\n";
    return;
  }

  std::string base       = ExtractRepoBase(github_url);
  std::string target_dir = "./deps/" + base;

  if (fs::exists(target_dir)) {
    std::cerr << "Dependency already exists: " << target_dir << "\n";
    return;
  }

  std::cout << "Querying remote for " << base << "...\n";
  RemoteInfo remote_info = GetRemoteInfo(github_url);

  std::string version_id;
  std::string tarball_url;

  if (!remote_info.tag.empty()) {
    version_id = remote_info.tag;
    std::cout << "Found release tag: " << version_id << "\n";
    tarball_url = "https://github.com/" + owner + "/" + repo_name +
                  "/archive/refs/tags/" + version_id + ".tar.gz";
  } else if (!remote_info.head_oid.empty()) {
    version_id = remote_info.head_oid;
    std::cout << "No release tags found. Falling back to HEAD commit: "
              << version_id.substr(0, 7) << "\n";
    tarball_url = "https://github.com/" + owner + "/" + repo_name +
                  "/archive/" + version_id + ".tar.gz";
  } else {
    std::cerr << "Fatal: Could not resolve tags or HEAD commit from remote.\n";
    return;
  }

  std::string archive_path = "./deps/" + base + ".tar.gz";
  fs::create_directories(target_dir);

  std::cout << "Downloading " << base << " archive...\n";
  if (!DownloadTarball(tarball_url, archive_path)) {
    fs::remove(target_dir);
    return;
  }

  std::cout << "Extracting archive...\n";
  try {
    ExtractAndStripOptimized(archive_path, target_dir);
    fs::remove(archive_path);
  } catch (const std::exception& e) {
    std::cerr << "Extraction failed: " << e.what() << "\n";
    fs::remove(archive_path);
    fs::remove_all(target_dir);
    return;
  }

  json          j;
  std::ifstream in_file("helix.json");
  in_file >> j;
  in_file.close();

  j["deps"][base]["url"]            = github_url;
  j["deps"][base]["version"]        = version_id;
  j["deps"][base]["tarball_url"]    = tarball_url;
  j["deps"][base]["path"]           = target_dir;
  j["deps"][base]["flags"]          = json::array();
  j["deps"][base]["link_libraries"] = json::array();

  std::ofstream out_file("helix.json");
  out_file << j.dump(4) << "\n";

  std::cout << "Successfully added " << base << ".\n";
}

void RemoveDeps(const std::string& dep_name) {
  if (!fs::exists("helix.json")) {
    std::cerr << "Fatal: helix.json not found.\n";
    return;
  }

  json          j;
  std::ifstream file("helix.json");
  if (!file) {
    std::cerr << "Fatal: Could not open helix.json.\n";
    return;
  }
  file >> j;
  file.close();

  if (!j.contains("deps") || !j["deps"].contains(dep_name)) {
    std::cerr << "Dependency '" << dep_name << "' not found in helix.json.\n";
    return;
  }

  std::string path = j["deps"][dep_name]["path"];

  std::error_code ec;
  fs::remove_all(path, ec);
  if (ec) {
    std::cerr << "Warning: Failed to cleanly delete directory " << path << " ("
              << ec.message() << ")\n";
  }

  j["deps"].erase(dep_name);

  std::ofstream config("helix.json");
  config << j.dump(4) << "\n";
  std::cout << "Successfully removed " << dep_name << ".\n";
}
