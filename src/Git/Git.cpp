#include "./Git.h"

bool Clone(const std::string& repo_url) {
  std::string base = fs::path(repo_url).filename().string();
  if (base.length() >= 4 && base.compare(base.length() - 4, 4, ".git") == 0) {
    base = base.substr(0, base.length() - 4);
  }

  std::string local_path = "./deps/" + base;

  if (fs::exists(local_path)) {
    std::cerr << "Dependency already exists: " << local_path << "\n";
    return false;
  }

  git_repository* repo = nullptr;
  int error = git_clone(&repo, repo_url.c_str(), local_path.c_str(), nullptr);

  if (error != 0) {
    const git_error* e = git_error_last();
    std::cerr << "Error cloning repo: "
              << (e && e->message ? e->message : "Unknown") << "\n";
    return false;
  }

  std::cout << "Repository cloned successfully: " << base << "\n";
  git_repository_free(repo);
  return true;
}

void AddDeps(const std::string& github_url) {
  if (!fs::exists("helix.json")) {
    std::cerr << "helix.json not found. Run 'init' first.\n";
    return;
  }

  if (!Clone(github_url)) return;

  std::string base = fs::path(github_url).filename().string();
  if (base.length() >= 4 && base.compare(base.length() - 4, 4, ".git") == 0) {
    base = base.substr(0, base.length() - 4);
  }
  const std::string local_path = "./deps/" + base;

  json          j;
  std::ifstream in_file("helix.json");
  in_file >> j;
  in_file.close();

  j["deps"][base] = {{"url", github_url}, {"path", local_path}};

  std::ofstream out_file("helix.json");
  out_file << j.dump(4);
}
