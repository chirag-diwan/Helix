#include "./Init.h"

void Init() {
  if (fs::exists("helix.json")) {
    std::cerr << "Project already initialized.\n";
    return;
  }

  std::string projectname = fs::current_path().filename().string();

  json j;
  j["name"]             = projectname;
  j["version"]          = "0.0.1";
  j["deps"]             = json::object();
  j["build_flags"]      = json::array();
  j["lib_export_flags"] = json::array();

  j["build_flags"].push_back("-O3");
  j["build_flags"].push_back("-Wall");
  j["build_flags"].push_back("-Wextra");

  std::ofstream file("helix.json");
  file << j.dump(4);

  fs::create_directory("src");
  std::ofstream M("src/main.cpp");
  M << "#include <iostream>\n\nint main() {\n    std::cout << \"Hello "
       "World\\n\";\n    return 0;\n}\n";

  fs::create_directory("deps");
  std::cout << "Initialized new CXX project.\n";
}
