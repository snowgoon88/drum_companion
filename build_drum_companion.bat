@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@REM Important: to build on 32-bit systems, the DX12 backends needs '#define ImTextureID ImU64', so we pass it here.
@REM C:\"Program Files (x86)"\"Microsoft Visual Studio"\2017\Community\VC\Auxiliary\Build\vcvars32.bat
@set OUT_DIR=Debug
@set OUT_EXE=drum_companion
@set INCLUDES=/Isrc /Ilibs\docopt.cpp /Ilibs\miniaudio 
@REM/Ilibs\imgui /Ilibs\imgui\backends /Ilibs\imgui\examples\libs\glfw\include
@set SOURCES=test\drum_compagnon.cpp libs\docopt.cpp\docopt.cpp
@REM libs\imgui\backends\imgui_impl_glfw.cpp libs\imgui\backends\imgui_impl_opengl3.cpp libs\imgui\imgui*.cpp
@set LIBS=/LIBPATH:libs\imgui\examples\libs\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib
@set DEF=/D LOG_MAIN
mkdir %OUT_DIR%
cl /nologo /Zi /MD %INCLUDES% %DEF% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
