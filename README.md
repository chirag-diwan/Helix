# Helix
***CXX package manager from hell***

### The Problem
Cpp though being a old and versetaile language dosent has a package manager. Installing dependencies and ensuring correct build process is a mess. 

### The Solution
A package manager , thats it , thats all was needed for a much nicer experience 


# Installing Helix
Clone the repo and build the helix binary using cmake .
```bash
    
git clone https://github.com/chirag-diwan/Helix.git
cd Helix
mkdir build && cd build
cmake ..
mv Helix ~/.local/bin/helix

```

# Using Helix
Helix offers three commands as of now `helix init` for CXX project init , `helix add <github_url>` for adding dependencies and `helix build` for building the project

# Helix.json config file
```json
{
    "build_flags": [],          //like -O3 or -Wall
    "link_libraries": ["git2"], //like X11 , m , pthread etc.
    "deps": {},                 // the dependencies you install 
    "lib_export_flags": [],     // If your binary exports some flags then add them here
    "name": "Helix",            //Project name
    "version": "0.0.1"          //Project version
}

```
