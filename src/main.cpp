#include "./Build/Build.h"
#include "./Git/Git.h"
#include "./Init/Init.h"

#include <cstring>
#include <git2.h>
#include <iostream>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: helix <command> [args]\n";
    return 1;
  }

  git_libgit2_init();

  if (std::strcmp(argv[1], "init") == 0) {
    Init();
  } else if (std::strcmp(argv[1], "add") == 0) {
    if (argc < 3) {
      std::cerr << "Usage: helix add <repo_url>\n";
    } else {
      AddDeps(argv[2]);
    }
  } else if (std::strcmp(argv[1], "build") == 0) {
    Build();
  } else {
    std::cerr << "Unknown command: " << argv[1] << "\n";
  }

  git_libgit2_shutdown();
  return 0;
}
