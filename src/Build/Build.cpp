#include "./Build.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;

void GenerateSyntheticCMake(const std::string& dep_name,
                            const std::string& dep_path) {
  std::vector<std::string> sources;
  std::vector<std::string> include_dirs;

  include_dirs.push_back(".");

  for (const auto& entry : fs::recursive_directory_iterator(dep_path)) {
    if (!entry.is_regular_file()) continue;

    std::string path_str = entry.path().generic_string();
    std::string filename = entry.path().filename().string();
    std::string ext      = entry.path().extension().string();

    if (path_str.find("test") != std::string::npos ||
        path_str.find("example") != std::string::npos ||
        path_str.find("benchmark") != std::string::npos) {
      continue;
    }

    if (ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
      sources.push_back(fs::relative(entry.path(), dep_path).generic_string());
    } else if (ext == ".h" || ext == ".hpp" || ext == ".hh") {
      std::string rel_dir =
          fs::relative(entry.path().parent_path(), dep_path).generic_string();
      if (std::find(include_dirs.begin(), include_dirs.end(), rel_dir) ==
          include_dirs.end()) {
        include_dirs.push_back(rel_dir);
      }
    }
  }

  std::ofstream cmake_file(dep_path + "/CMakeLists.txt");
  cmake_file << "cmake_minimum_required(VERSION 3.15)\n";
  cmake_file << "project(" << dep_name << "_synthetic C CXX)\n\n";
  if (sources.empty()) {
    cmake_file << "add_library(" << dep_name << " INTERFACE)\n";
    cmake_file << "target_include_directories(" << dep_name << " INTERFACE\n";
    for (const auto& inc : include_dirs) {
      cmake_file << "    \"${CMAKE_CURRENT_SOURCE_DIR}/" << inc << "\"\n";
    }
    cmake_file << ")\n";
  } else {
    cmake_file << "add_library(" << dep_name << " STATIC\n";
    for (const auto& src : sources) {
      cmake_file << "    \"" << src << "\"\n";
    }
    cmake_file << ")\n\n";

    cmake_file << "target_include_directories(" << dep_name << " PUBLIC\n";
    for (const auto& inc : include_dirs) {
      cmake_file << "    \"${CMAKE_CURRENT_SOURCE_DIR}/" << inc << "\"\n";
    }
    cmake_file << ")\n";
  }

  cmake_file.close();
  std::cout << "Generated synthetic CMakeLists.txt for " << dep_name << "\n";
}

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
  Cmake << "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n\n"; // Add this

  std::vector<std::string> link_targets;
  if (j.contains("deps")) {
    for (auto& [name, dep_info] : j["deps"].items()) {
      std::string dep_path       = "deps/" + name;
      std::string dep_cmake_path = dep_path + "/CMakeLists.txt";

      if (!fs::exists(dep_cmake_path)) {
        GenerateSyntheticCMake(name, dep_path);
      }
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
    std::cerr << "CMake configuration failed.\n";
    return;
  }

  int build_res = std::system("cmake --build build");
  if (build_res != 0) {
    std::cerr << "Compilation failed.\n";
    return;
  }

  if (!fs::exists("compile_commands.json")) {
    fs::create_symlink("build/compile_commands.json", "compile_commands.json");
  }
}
