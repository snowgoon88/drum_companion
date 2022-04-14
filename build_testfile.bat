@REM Build for Visual Studio compiler
@REM build_testfile.bat filename
@REM 
@REM setup command line compiler
@REM "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
@REM
@IF "%1" == "" (
    @ECHO  ERROR must specify a filename
    @ECHO Usage: build_testfile.bat FILENAME
    @EXIT /B 1
)
@REM read argument
@SET FILE=%1
@SET FILE_CPP=test\%FILE%.cpp
@ECHO Compiling  %FILE_CPP%
@REM Compilationi
@SET OUT_DIR=Debug
@SET OUT_EXE=%FILE%
@SET INCLUDES=/Isrc /Ilibs\docopt.cpp /Ilibs\miniaudio 
@set SOURCES=%FILE_CPP% libs\docopt.cpp\docopt.cpp
@set LIBS=/LIBPATH:libs\imgui\examples\libs\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib
@set DEF=/D LOG_MAIN
mkdir %OUT_DIR%
"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat" && cl /nologo /Zi /MD %INCLUDES% %DEF% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
