/* -*- coding: utf-8 -*- */


/**
 * Test LedDisplay widgets
 *
 * Based on Skeleton base using ImGui, miniaudio, docopt, 
 * my custom LOGGER
 *
 * Exit via Ctrl-C and ESC
 */

// Parsing command line options
#include "docopt.h"

#include <ctime>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include <signal.h>          // C std lib (signal, sigaction, etc)

#include <utils.hpp>        // loggers, etc

#include <date_widget.hpp>

// ***************************************************************************
// ************************************************************* Grafik - INIT
// ***************************************************************************
#include "imgui.h"
//#define IMGUI_DISABLE_OBSOLETE_KEYIO
#define IMGUI_DEFINE_MATH_OPERATORS // Access to math operators
#include "imgui_internal.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to
// maximize ease of testing and compatibility with old VS compilers. To link
// with VS2010-era libraries, VS2015+ requires linking with
// legacy_stdio_definitions.lib, which we do using this pragma. Your own project
// should not be affected, as you are likely to link with a newer binary of GLFW
// that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) &&                                 \
    !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void
glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
#define LOG_MAIN
#ifdef LOG_MAIN
#  define LOGMAIN(msg) (LOG_BASE("[MAIN]", msg))
#else
#  define LOGMAIN(msg)
#endif

// *************************************************************** App GLOBALS
DateWidget *date_widget;
bool should_exit = false;

// Args
bool _p_verb = false;

// ImGui Colors
ImVec4 hoover_color = YELLOW_COL;

// ************************************************************* Clean Globals
void clear_globals()
{
  LOGMAIN( "__CLEANING" );
  if (date_widget) delete date_widget;
}

// *********************************************************** Ctrl-C Callback
void ctrlc_cbk( int s )
{
  LOGMAIN( "__EXIT through Ctrl-C" );
  should_exit = true;
}


// ***************************************************************************
// ******************************************************************* options
// ***************************************************************************
static const char _usage[] =
R"(Drum Companion.

    Usage:
      drum_companion [-h -v]

    Options:
      -h --help              Show this screen
      -v --verbose           Display some info
)";

void setup_options( int argc, char **argv )
{
  std::map<std::string, docopt::value> args = docopt::docopt(_usage, 
                                                  { argv + 1, argv + argc },
                                                  // show help if requested
                                                  true,
                                                  // version string
                                                  "Time Widget");

  std::cout << "******* DocOpt arguments" << std::endl;
  for(auto const& arg : args) {
    std::cout << arg.first << ": " << arg.second;
    std::cout << " type=" << type_name<decltype(arg.second)>() << std::endl;
  }
  std::cout << std::endl;
  // //exit(23);

  if (args["--verbose"].asBool()) {
    _p_verb = true;
  }
  //exit(22);

}

// ***************************************************************************
// ******************************************************************* run_gui
// ***************************************************************************
int run_gui()
{
  // ******************************************************************* State

  // ********************************************************** GUI - creation
  date_widget = new DateWidget();

  // Other GUI variables
  bool gui_ask_end = false;
  
  // ******************************************************* Grafik - creation
  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())  
    return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char *glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  // Transparency only works with a Compositing WindowManager
  // as WindowMaker does not allows that, use "picom"
  // but slower ??
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
  // Create window with graphics context
  GLFWwindow *window = glfwCreateWindow(
      600, 500, "Time Widget v1.0", NULL, NULL);
  if (window == NULL)
    return 1;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad
  // Controls

  // Setup Dear ImGui style
  // ImGui::StyleColorsDark();
  ImGui::StyleColorsClassic();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Load Fonts
  io.Fonts->AddFontDefault();
  // Load font and merge to default
  static const ImWchar icons_ranges[] = {0x23f0, 0x23ff,
                                         0}; // static as not copied by AddFont
  ImFontConfig config;
  config.MergeMode = true;
  ImFont *font_unifont = io.Fonts->AddFontFromFileTTF(
      "ressources/unifont.ttf", 24.0f, &config, icons_ranges);
  if (font_unifont == NULL) {
    std::cout << "Error loading ressources/unifont.ttf" << std::endl;
    return 1;
  }
  ImFont *font_unifont36 = io.Fonts->AddFontFromFileTTF(
      "ressources/unifont.ttf", 50.0f, &config, icons_ranges);
  if (font_unifont36 == NULL) {
    std::cout << "Error loading ressources/unifont.ttf for size 36" << std::endl;
    return 1;
  }

  
  io.Fonts->Build();
  //ImFontAtlas::Build();

  // Try Scaling up
  //ImGui::GetStyle().ScaleAllSizes(2.0f);

  // ************************************************************** Gui - Loop
  while (!glfwWindowShouldClose(window) && !gui_ask_end) {
    // Poll and handle events (inputs, window resize, etc.)
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGUI Frame
    {
      // Create a window with title and append into it.
      ImGui::Begin("Time Widget");

      date_widget->draw();

      // ImGui::SameLine();

      ImGui::End();
    }
    
    // Rendering
    ImGui::Render();     int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    glClearColor(CLEAR_COL.x * CLEAR_COL.w, CLEAR_COL.y * CLEAR_COL.w,
                 CLEAR_COL.z * CLEAR_COL.w, CLEAR_COL.w);
    glClear(GL_COLOR_BUFFER_BIT);
    // // When using a transparent Window
    // glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);

    // test Keyboard
    if (ImGui::IsKeyDown(526)) { // Escape
      gui_ask_end = true;
    }

    // Now apply logic
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

// ***************************************************************************
// ********************************************************************** MAIN
// ***************************************************************************
int main(int argc, char *argv[])
{
  // // To deal with Ctrl-C (Win32 and Linux)
  signal(SIGINT, ctrlc_cbk);

  // Args
  setup_options( argc, argv );

  // ************************************************************ init engines
  LOGMAIN( "__Main Init engines");

  if (_p_verb) {
  }
  
  run_gui();

  // Clean up before exit
  clear_globals();
  
  return 0;
}
// ***************************************************************************

