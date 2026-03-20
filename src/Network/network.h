#pragma once
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

struct ArchiveReadDeleter {
    void operator()(struct archive* a) {
      if (a) {
        archive_read_close(a);
        archive_read_free(a);
      }
    }
};
struct ArchiveWriteDeleter {
    void operator()(struct archive* ext) {
      if (ext) {
        archive_write_close(ext);
        archive_write_free(ext);
      }
    }
};

struct SemVer {
    int         major = 0, minor = 0, patch = 0;
    std::string original;

    bool operator<(const SemVer& other) const {
      if (major != other.major) return major < other.major;
      if (minor != other.minor) return minor < other.minor;
      return patch < other.patch;
    }
};

SemVer                              ParseVersion(const std::string& v);
std::string                         ExtractRepoBase(const std::string& url);
std::pair<std::string, std::string> ParseGitHubUrl(const std::string& url);
size_t CurlWriteCallback(void* ptr, size_t size, size_t nmemb, void* stream);
bool   DownloadTarball(const std::string& url, const std::string& out_path);
struct RemoteInfo {
    std::string tag;
    std::string head_oid;
};
RemoteInfo GetRemoteInfo(const std::string& github_url);
void       AddDeps(const std::string& github_url);
void       RemoveDeps(const std::string& dep_name);
