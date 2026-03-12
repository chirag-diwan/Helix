#include "./Build/Build.h"
#include "./Git/Git.h"
#include "./Init/Init.h"

#include <cstring>
#include <git2.h>
#include <iostream>

void PrintUsage() {
  std::clog << "helix <command> [args]"
               "\n\thelix init #To init a new projects"
               "\n\thelix add <git_url> #To add a dependency"
               "\n\thelix remove <git_url> #To remove a dependency"
               "\n\thelix build #To build the binary";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    PrintUsage();
    return 1;
  }

  git_libgit2_init();

  if (std::strcmp(argv[1], "init") == 0) {
    Init();
  } else if ((std::strcmp(argv[1], "add") == 0) ||
             (std::strcmp(argv[1], "a") == 0)) {
    if (argc < 3) {
      PrintUsage();
    } else {
      AddDeps(argv[2]);
    }
  } else if ((std::strcmp(argv[1], "remove") == 0) ||
             std::strcmp(argv[1], "r") == 0) {
    if (argc < 3) {
      PrintUsage();
    } else {
      RemoveDeps(argv[2]);
    }
  } else if (std::strcmp(argv[1], "build") == 0) {
    Build();
  } else {
    std::cerr << "Unknown command: " << argv[1] << "\n";
  }

  git_libgit2_shutdown();
  return 0;
}
