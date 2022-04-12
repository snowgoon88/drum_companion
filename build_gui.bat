@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@REM Important: to build on 32-bit systems, the DX12 backends needs '#define ImTextureID ImU64', so we pass it here.
@set OUT_DIR=Debug
@set OUT_EXE=00-button
@set INCLUDES=/Ilibs\imgui /Ilibs\imgui\backends /Ilibs\imgui\examples\libs\glfw\include
@set SOURCES=test\00-button.cpp libs\imgui\backends\imgui_impl_glfw.cpp libs\imgui\backends\imgui_impl_opengl3.cpp libs\imgui\imgui*.cpp
@set LIBS=/LIBPATH:libs\imgui\examples\libs\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib
mkdir %OUT_DIR%
cl /nologo /Zi /MD %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
