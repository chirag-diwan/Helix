#pragma once

#include <cstring>
#include <filesystem>
#include <fstream>
#include <git2.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using json   = nlohmann::json;
namespace fs = std::filesystem;

void Build();
