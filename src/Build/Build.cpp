#include "./Build.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;

void Build() {
  fs::create_directory("build");
  if (!fs::exists("helix.json")) {
    std::cerr << "helix.json not found. Run 'init' first.\n";
    return;
  }

  json          j;
  std::ifstream config("helix.json");
  config >> j;
  config.close();

  std::ofstream Cmake("CMakeLists.txt");
  if (!Cmake) {
    std::cerr << "Failed to write CMakeLists.txt\n";
    return;
  }

  std::string proj_name = j.value("name", "helix_project");

  Cmake << "cmake_minimum_required(VERSION 3.15)\n";
  Cmake << "project(" << proj_name << " CXX)\n\n";

  std::vector<std::string> link_targets;
  if (j.contains("deps")) {
    for (auto& [name, dep_info] : j["deps"].items()) {
      Cmake << "add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/" << name
            << " EXCLUDE_FROM_ALL)\n";
      link_targets.push_back(name);
    }
  }

  Cmake << "\nfile(GLOB_RECURSE SOURCES \"src/*.cpp\")\n";
  Cmake << "add_executable(${PROJECT_NAME} ${SOURCES})\n";

  if (!link_targets.empty()) {
    Cmake << "target_link_libraries(${PROJECT_NAME} PRIVATE ";
    for (const auto& target : link_targets) {
      Cmake << target << " ";
    }
    Cmake << ")\n";
  }

  if (j.contains("link_libraries") && !j["link_libraries"].empty()) {
    Cmake << "target_link_libraries(${PROJECT_NAME} PRIVATE ";

    for (const auto& flag : j["link_libraries"]) {
      Cmake << flag.get<std::string>() << " ";
    }
    Cmake << ")\n";
  }

  if (j.contains("build_flags") && !j["build_flags"].empty()) {
    Cmake << "target_compile_options(${PROJECT_NAME} PRIVATE ";

    for (const auto& flag : j["build_flags"]) {
      Cmake << flag.get<std::string>() << " ";
    }
    Cmake << ")\n";
  }

  Cmake.close();

  int config_res = std::system("cmake -B build -S .");
  if (config_res != 0) {
    std::cerr << "Fatal: CMake configuration failed.\n";
    return;
  }

  int build_res = std::system("cmake --build build");
  if (build_res != 0) {
    std::cerr << "Fatal: Compilation failed.\n";
    return;
  }
}
