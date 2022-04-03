# Drum Companion

Tentative to build a custom Metronom for my drumming practice.

## Build

### Dependencies
- Need to clone [ImGUI](https://github.com/ocornut/imgui) in the ```lib``` directory

### Use waf
- to configure (clang): ```./waf configure --out=cbuild --check-cxx-compiler=clang++ --compil_db```
- to build: ```./waf build```
- to run: ```cbuild/test/xxx```
