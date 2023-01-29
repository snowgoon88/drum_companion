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

#include <sound_engine.hpp>
#include <pattern_audio.hpp>
#include <looper.hpp>
#include <analyzer.hpp>
#include <bpm_widget.hpp>
#include <beat_slider_widget.hpp>

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
BPMWidget *bpm_widget;
BeatSlider *beat_widget;
bool should_exit = false;

// Audio
int _tempo = 30;
Signature _p_sig { 30, 4, 2};
SoundEngine *sound_engine = nullptr;
PatternAudio *pattern_audio = nullptr;  // GUI only
Looper *looper = nullptr;
Analyzer *analyzer = nullptr;

// Args
bool _p_verb = false;

// ImGui Colors
ImVec4 hoover_color = YELLOW_COL;

// ************************************************************* Clean Globals
void clear_globals()
{
  LOGMAIN( "__CLEANING" );
  if (bpm_widget) delete bpm_widget;
  if (beat_widget) delete beat_widget;

  if (sound_engine != nullptr ) {
    LOGMAIN( "  will clean sound_engine" );
    delete sound_engine;
  }
  LOGMAIN( "  sound_engine OK" );
  
  if (looper != nullptr) {
    LOGMAIN( "  will clean looper" );
    // Needed if looper patterns are created at start
    // for( auto& pat_ptr: looper->all_patterns) {
    //   LOGMAIN( "    will clean pattern " << pat_ptr->_id );
    //   delete pat_ptr;
    //   LOGMAIN( "    pattern OK" ); 
    // }
    delete looper;
  }
  LOGMAIN( "  looper OK" );

  if (pattern_audio != nullptr ) {
    LOGMAIN( "  will clean pattern_audio" );
    delete pattern_audio;
  }
  LOGMAIN( "  pattern_audio OK" );

  if (analyzer != nullptr) {
    LOGMAIN( "  will clean analyzer" );
    delete analyzer;
  }
  LOGMAIN( "  analyzer OK" );

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
                                                  "Skeleton");

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
  bool should_pause = false;
  bool should_stop = false;
  bool should_run = false;

  // ********************************************************** GUI - creation
  bpm_widget = new BPMWidget( _tempo );
  beat_widget = new BeatSlider();
  
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

  // Create window with graphics context
  GLFWwindow *window = glfwCreateWindow(
      600, 500, "Skeleton v1.0", NULL, NULL);
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
      ImGui::Begin("Skeleton");

      bpm_widget->draw();
      // update status of BeatSlider
      beat_widget->set_dir_forward( looper->odd_beat );
      beat_widget->draw( looper->to_next_beat, looper->to_first_beat );
      
      if (ImGui::Button(u8"⏵")) { // 0x23F5       
        should_run = true;
      }
      
      ImGui::SameLine();
      if (ImGui::Button(u8"⏸")) { // 0x23F8
        should_pause = true;
      }
      
      ImGui::SameLine();
      if (ImGui::Button(u8"⏹")) { // 0X23F9
        should_stop = true;
      }

// We iterate both legacy native range and named ImGuiKey ranges, which is a little odd but this allow displaying the data for old/new backends.
            // User code should never have to go through such hoops: old code may use native keycodes, new code may use ImGuiKey codes.
#ifdef IMGUI_DISABLE_OBSOLETE_KEYIO
            struct funcs { static bool IsLegacyNativeDupe(ImGuiKey) { return false; } };
            const ImGuiKey key_first = ImGuiKey_NamedKey_BEGIN;
#else
            struct funcs { static bool IsLegacyNativeDupe(ImGuiKey key) { return key < 512 && ImGui::GetIO().KeyMap[key] != -1; } }; // Hide Native<>ImGuiKey duplicates when both exists in the array
            const ImGuiKey key_first = 0;
            //ImGui::Text("Legacy raw:");       for (ImGuiKey key = key_first; key < ImGuiKey_COUNT; key++) { if (io.KeysDown[key]) { ImGui::SameLine(); ImGui::Text("\"%s\" %d", ImGui::GetKeyName(key), key); } }
#endif
            ImGui::Text("Keys down:");          for (ImGuiKey key = key_first; key < ImGuiKey_COUNT; key++) { if (funcs::IsLegacyNativeDupe(key)) continue; if (ImGui::IsKeyDown(key)) { ImGui::SameLine(); ImGui::Text("\"%s\" %d (%.02f secs)", ImGui::GetKeyName(key), key, ImGui::GetKeyData(key)->DownDuration); } }

      // local time
      std::time_t time_now = std::time(nullptr);
      auto time_st = std::localtime( &time_now );
      ImGui::Text("Time: %2d:%2d", time_st->tm_hour, time_st->tm_min);
      ImGui::End();
    }
    
    // Rendering
    ImGui::Render();     int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(CLEAR_COL.x * CLEAR_COL.w, CLEAR_COL.y * CLEAR_COL.w,
                 CLEAR_COL.z * CLEAR_COL.w, CLEAR_COL.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);

    // test Keyboard
    if (ImGui::IsKeyDown(526)) { // Escape
      gui_ask_end = true;
    }

    // Now apply logic

    // And deal with Play/Pause
    // MiniAudio
    if (should_run) {
      looper->start();
      std::cout << "__PLAY__" << std::endl;
    }
    else if (should_stop) {
      looper->stop();
      std::cout << "__STOP__" << std::endl;
    }
    else if (should_pause) {
      looper->pause();
      std::cout << "__PAUSE__" << std::endl;
    }
    should_run = should_pause = should_stop = false;

    looper->next();

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
  sound_engine = new SoundEngine();
  // variables are not used, but show how could be used
  auto idx_clave = sound_engine->add_sound( "ressources/claves_120ms.wav" );
  UNUSED(idx_clave);
  auto idx_cow = sound_engine->add_sound( "ressources/cowbell.wav" );
  UNUSED(idx_cow);

  // ************************************************************* PlayPattern
  LOGMAIN( "__PATTERN_AUDIO with SoundEngine" );
  pattern_audio = new PatternAudio( sound_engine );
  pattern_audio->_id = 0;
  pattern_audio->_signature = _p_sig;
  pattern_audio->init_from_string( "2x1x1x1x" );

    // ****************************************************************** Looper
  LOGMAIN( "__LOOPER with SoundEngine and all PatternAudio" );
  looper = new Looper( sound_engine );
  analyzer = new Analyzer( looper );

  auto id = looper->add( pattern_audio );
  LOGMAIN( "  add p" << id << "=" << pattern_audio->str_verbose() );

  auto res = analyzer->parse( "p0" );
  looper->_formula = "p0";
  looper->set_sequence( res.begin(), res.end() );
  LOGMAIN( looper->str_dump() );
    
  if (_p_verb) {
  }
  
  run_gui();

  
  // Clean up before exit
  clear_globals();
  
  return 0;
}
// ***************************************************************************

