#include <cstring>
#include <filesystem>
#include <fstream>
#include <git2.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using json   = nlohmann::json;
namespace fs = std::filesystem;

bool Clone(const std::string& repo_url);

void AddDeps(const std::string& github_url);
