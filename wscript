#!/usr/bin/env python
# encoding: utf-8
# waf configuration file .waf --help pour commande par défaut
# Utilisé par CMD dist

## Configured to use:
## - ImGUI on Linux, Windows, MacOS (but only tested on Debian 11.0)

## Par défaut ./waf configure va utiliser buildir=wbuild et CXX=g++
## MAIS avec ./waf configure --out=cbuild --check-cxx-compiler=clang++
##      on peut utilise clang :o)
## l'option --color permet de coloriser la sortie du compilo

APPNAME = 'DrumCompanion'
VERSION = '0.1'

# root, build dir
top = '.'
out = 'wbuild'

opt_flags = '-O3'
debug_flags = '-O0 -g'

# ************************************************************************* help
def help(ctx):
    print( "**** WAF for DrumCompanion, usual commands ************" )
    print( "configuration :  ./waf configure --out=cbuild --check-cxx-compiler=clang++" )
    print( "build :          ./waf build ")
    print( "build specific : ./waf build --targets=test/001-curve" )
    print( "clean :          ./waf clean" )
    print( "detailed help :  ./waf --help or see https://waf.io/book" )
    print( "  options :      --compil_db --debug" )

# ********************************************************************** options
def options( opt ):
    opt.load( 'compiler_cxx' )

    # option debug
    opt.add_option('--debug', dest='debug', action="store_true", default=False,
                   help='compile with debugging symbols' )

    # clang compilation database
    opt.add_option('--compil_db', dest='compil_db', action="store_true", default=False,
                   help='use clang compilation database' )

    # define some macro for C++
    # (equivalent to #define LABEL or -DLABEL to compilator
    opt.add_option('-D', '--define', action="append", dest="defined_macro",
                   help='define preprocessing macro' )

    # option debug
    opt.add_option('--test', dest='test', action="store_true", default=False,
                   help='compile all test programs (name begins with digits)' )
    
# **************************************************************** CMD configure
def configure( conf ):
    # print( "__START__" )
    # print( conf.env)
    # print( "******************************************************************")
    conf.load( 'compiler_cxx' )
    # print( "__CXX__" )
    # print( conf.env)
    # print( "******************************************************************")

    # ------------------------------------------------------------------ WARNING
    # WARNING, for ImGUI, I removed some linker flags that prevented proper
    #          linking
    print( "**************************************************************************" )
    print( "__WARNING__ : changing conf.env.STLIB_MARKER and conf.env.SHLIB_MARKER" )
    conf.env.STLIB_MARKER = '' # instead of '-Wl,-Bstatic'
    conf.env.SHLIB_MARKER = '' # instead of '-Wl,-Bdynamic'
    print( "**************************************************************************" )

    
    if conf.options.compil_db:
        ## To generate 'compile_commands.json' at the root of buildpath
        # to be linked/copied in order to user 'cquery' in emacs through lsp-mode
        # see https://github.com/cquery-project/cquery/wiki/Emacs
        conf.load('clang_compilation_database', tooldir="ressources")
        print( "CXX=",conf.env.CXX)
    
    #conf.env['CXXFLAGS'] = ['-D_REENTRANT','-Wall','-fPIC','-std=c++11']
    conf.env['CXXFLAGS'] = ['-Wall','-std=c++11']
    ##DELconf.env.INCLUDES_JSON = conf.path.abspath()+'/include'

    # ## Check ImGUI is present and configured and ready to be used ...
    # print( "Checking SUBLIBRARY 'ImGUI' present and configured" )
    # conf.recurse( 'visugl' )
    # visuglnode = conf.path.find_node( 'libs/imgui' )
    # conf.env.INCLUDES_VISUGL  = [visuglnode.abspath()+'/src']

    ## Check MiniAudio
    print( "Looking for MiniAudio" )
    ma_node = conf.path.find_node( 'libs/miniaudio' )
    if not ma_node :
        from waflib.Errors import ConfigurationError
        raise ConfigurationError( msg='MiniAudio '+conf.path.parent.abspath() )
    conf.env.LIB_MINIAUDIO = [ "dl", "m", "pthread" ]
    conf.env.INCLUDES_MINIAUDIO = [ma_node.abspath()]
    print( "** MiniAudio *************************************************************" )
    print( conf.env )
    
    # ## Require VisuGL
    # # print( "Looking for VisuGL" )
    # # #print( "path="+conf.path.name )
    # # visuglnode = conf.path.find_node( '../VisuGL' );
    # # if not visuglnode :
    # #     from waflib.Errors import ConfigurationError
    # #     raise ConfigurationError( msg='VisuGL not fount in '+conf.path.parent.abspath() )
    # # print( "  VisuGL path="+visuglnode.abspath() )
    # # # conf.env.LIB_VISUGL = ['VisuGL']
    # # conf.env.INCLUDES_VISUGL  = [visuglnode.abspath()+'/src']
    # # # conf.env.LIBPATH_VISUGL = [visuglnode.abspath()+'/lib']
    # visuglnode = conf.path.find_node( 'visugl' );
    # conf.check( msg="Checking for 'visugl'",
    #             errmsg="'visugl' not present, try git ....",
    #             compile_mode='cxx',
    #             cxxflags='-I '+visuglnode.abspath()+'/src',
    #             fragment='''
    #             #include <visugl.hpp>
    #             int main() { return 0; }
    #             ''',
    #             uselib_store='VISUGL' )

    ## Require/Check libboost
    conf.env.LIB_BOOST = ['boost_program_options']
    conf.env.LIBPATH_BOOST = ['/usr/lib/x86_64-linux-gnu','/usr/lib/i386-linux-gnu']
    print "Checking for 'BOOST::program_options'"
    conf.find_file( 'lib'+conf.env.LIB_BOOST[0]+'.so', conf.env.LIBPATH_BOOST )

    
    ## Require OpenGL, using wraper around pkg-onfig
    conf.check_cfg(package='gl',
                   uselib_store='GL',
                   args=['--cflags', '--libs']
    )
    # ## Require OpenGL >1.1 (glew), using wraper around pkg-onfig
    # conf.check_cfg(package='glew',
    #                uselib_store='GLEW',
    #                args=['--cflags', '--libs']
    # )
    ## Require GLFW3, using wraper around pkg-config
    ## and option --static (for ImGUI)
    # print( "__BEFORE__" )
    # print( conf.env )
    # print( "******************************************************************")
    conf.check_cfg(package='glfw3',
                   uselib_store='GLFW3',
                   args=['--cflags', '--static', '--libs']
    )
    # print( "__AFTER__" )
    # print( conf.env )
    # print( "******************************************************************")
    

    
    
# ******************************************************************** CMD build
def build( bld ):
    print('→ build from ' + bld.path.abspath())

    # check debug option
    if bld.options.debug:
        bld.env['CXXFLAGS'] += debug_flags.split(' ')
    else:
        bld.env['CXXFLAGS'] += opt_flags.split(' ')
    print( bld.env['CXXFLAGS'] )
        
    # add macro defines as -D options
    # print( "MACRO=", bld.options.defined_macro )
    if bld.options.defined_macro:
        # print( "ENV=", bld.env.DEFINES )
        bld.env.DEFINES = bld.env.DEFINES + bld.options.defined_macro
        # print( "new ENV=", bld.env.DEFINES )
    
    bld.recurse( 'test' )
    
# g++ -std=c++11 -I../libs/imgui -I../libs/imgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3` -c -o imgui_draw.o ../libs/imgui/imgui_draw.cpp
# g++ -o 00-button 00-button.o imgui.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui_impl_glfw.o imgui_impl_opengl3.o -std=c++11 -I../libs/imgui -I../libs/imgui/backends -g -Wall -Wformat `pkg-config --cflags glfw3`  -lGL `pkg-config --static --libs glfw3`
# Build complete for Linux
