/* -*- coding: utf-8 -*- */

/** 
 * CLI and Grafik DrumCompagnon
 * - cli arguments: bpm, sig, pattern, loop,
 * - play repeated sound using looper with sound_pattern
 * - grafik On/Off (if GUI, only ONE pattern)
 * - play/stop/pause
 */

// Parsing command line options
#include "docopt.h"

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include <signal.h>          // C std lib (signal, sigaction, etc)

#include <utils.hpp>        // loggers, etc
#include <pattern_audio.hpp>
#include <sound_engine.hpp>
#include <looper.hpp>
#include <analyzer.hpp>
#include <pattern_gui.hpp>

// ***************************************************************************
// ************************************************************* Grafik - INIT
// ***************************************************************************
#include "imgui.h"
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
//#define LOG_MAIN
#ifdef LOG_MAIN
#  define LOGMAIN(msg) (LOG_BASE("[MAIN]", msg))
#else
#  define LOGMAIN(msg)
#endif

// *************************************************************** App GLOBALS

SoundEngine *sound_engine = nullptr;
PatternAudio *pattern_audio = nullptr;  // GUI only
Looper *looper = nullptr;
bool should_exit = false;

// Args
Signature _p_sig {90, 4, 2};
unsigned int _p_bpm = _p_sig.bpm;
std::list<std::string> _p_patternlist;
std::string _p_loop = "p0";
bool _p_gui = false;
bool _p_verb = false;

ImVec4 hoover_color = YELLOW_COL;
ImVec4 NoteButton::colors[3] = {CLEAR_COL, GREEN_COL, RED_COL};
ImVec4 NoteButton::hoover_color = YELLOW_COL;

void clear_globals()
{
  LOGMAIN( "__CLEANING" );
  if (sound_engine != nullptr ) {
    LOGMAIN( "  will clean sound_engine" );
    delete sound_engine;
  }
  LOGMAIN( "  sound_engine OK" );
  
  if (pattern_audio != nullptr ) {
    LOGMAIN( "  will clean pattern_audio" );
    delete pattern_audio;
  }
  LOGMAIN( "  pattern_audio OK" );
  
  if (looper != nullptr) {
    LOGMAIN( "  will clean looper" );
    for( auto& pat_ptr: looper->all_patterns) {
      LOGMAIN( "    will clean pattern " << pat_ptr->_id );
      delete pat_ptr;
      LOGMAIN( "    pattern OK" ); 
    }
    delete looper;
  }
  LOGMAIN( "  looper OK" );
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
      drum_companion [--pattern=<str>... options]

    Options:
      -h --help               Show this screen
      -v --verbose            Display some info
      -g --gui                With GUI (but ONE pattern, NO loop)
      -b, --bpm <uint>        BPM [default: 90]
      -s, --sig <str>         signature [default: 4x2]
      -p, --pattern=<str>...  patterns, can be REPEATED [default: 2x1x1x1x]
      -l, --loop=<str>        sequence of patterns (like 2x(p0+P1)) [default: p0]
)";

void setup_options( int argc, char **argv )
{
  std::map<std::string, docopt::value> args = docopt::docopt(_usage, 
                                                  { argv + 1, argv + argc },
                                                  // show help if requested
                                                  true,
                                                  // version string
                                                  "Drum Companion 1.0");

  // for(auto const& arg : args) {
  //   std::cout << arg.first << ": " << arg.second;
  //   std::cout << " type=" << type_name<decltype(arg.second)>() << std::endl;
  // }
  // std::cout << "Patterns List=" << std::boolalpha << args["--pattern"].isStringList() << std::endl;
  // std::cout << "PATTERN " << args["--pattern"] << std::endl;
  // //std::cout << "STRING " << args["--pattern"].asString() << std::endl;
  // std::cout << "LIST   ";
  // for( auto& elem: args["--pattern"].asStringList()) {
  //   std::cout << elem << ", ";
  // }
  // std::cout << std::endl;
  
  _p_sig.bpm = args["--bpm"].asLong();
  _p_sig.from_string( args["--sig"].asString());
  // Create the various patterns
  for( auto& pat: args["--pattern"].asStringList()) {
    _p_patternlist.push_back( pat );
  }
  _p_loop = args["--loop"].asString();
  if (args["--gui"].asBool()) {
    _p_gui = true;
  }
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
  // ********************************************************** GUI - creation
  // create PatternGUI
  PatternGUI pg( pattern_audio );
  // Other GUI variables
  bool gui_ask_end = false;
  bool should_pause = false;
  bool should_stop = false;
  bool should_run = false;
  
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
      1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
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
      "ressources/unifont.ttf", 12.0f, &config, icons_ranges);
  if (font_unifont == NULL) {
    std::cout << "Error loading ressources/unifont.ttf" << std::endl;
    return 1;
  }

  io.Fonts->Build();

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
      ImGui::Begin("Drum Companion");

      // PatternGUI
      pg.draw();

      // Play/Pause
      if (pattern_audio->_state == PatternAudio::running) {
        ImGui::PushStyleColor(ImGuiCol_Button, GREEN_COL);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, GREEN_COL);
      }
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoover_color);
      if (ImGui::Button(u8"⏵")) { // 0x23F5       
        should_run = true;
      }
      ImGui::PopStyleColor(1);      
      if (pattern_audio->_state == PatternAudio::running) {
        ImGui::PopStyleColor(2);
      }
      
      ImGui::SameLine();
      if (pattern_audio->_state == PatternAudio::paused) {
        ImGui::PushStyleColor(ImGuiCol_Button, YELLOW_COL);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, YELLOW_COL);
      }
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoover_color);
      if (ImGui::Button(u8"⏸")) { // 0x23F8
        should_pause = true;
      }
      ImGui::PopStyleColor(1);      
      if (pattern_audio->_state == PatternAudio::paused) {
        ImGui::PopStyleColor(2);
      }

      
      ImGui::SameLine();
      if (pattern_audio->_state == PatternAudio::ready) {
        ImGui::PushStyleColor(ImGuiCol_Button, RED_COL);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, RED_COL);
      }
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoover_color);
      
      if (ImGui::Button(u8"⏹")) { // 0X23F9
        should_stop = true;
      }
      ImGui::PopStyleColor(1);
      if (pattern_audio->_state == PatternAudio::ready) {
        ImGui::PopStyleColor(2);
      }

      
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
    pg.apply();

    // And deal with Play/Pause
    if (should_run) {
      pattern_audio->start();
      std::cout << "__PLAY__" << std::endl;
    }
    else if (should_stop) {
      pattern_audio->stop();
      std::cout << "__STOP__" << std::endl;
    }
    else if (should_pause) {
      pattern_audio->pause();
      std::cout << "__PAUSE__" << std::endl;
    }
    should_run = should_pause = should_stop = false;

    pattern_audio->next();
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
// *********************************************************************** run
// ***************************************************************************
int run()
{
  looper->start();
  while (! should_exit) {
    looper->next();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

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

  // ****************************************************************** Sounds
  sound_engine = new SoundEngine();
  // variables are not used, but show how could be used
  auto idx_clave = sound_engine->add_sound( "ressources/claves_120ms.wav" );
  UNUSED(idx_clave);
  auto idx_cow = sound_engine->add_sound( "ressources/cowbell.wav" );
  UNUSED(idx_cow);
    
  // ************************************************************* PlayPattern
  LOGMAIN( "__PATTERN_AUDIO with SoundEngine" );
  pattern_audio = new PatternAudio( sound_engine );
  pattern_audio->_signature = _p_sig;
  pattern_audio->init_from_string( _p_patternlist.front() );

  // ****************************************************************** Looper
  LOGMAIN( "__LOOPER with SoundEngine and all PatternAudio" );
  looper = new Looper( sound_engine );
  // add the patterns
  if (_p_verb) {
    std::cout << _p_sig << " at " << _p_bpm << " BPM" << std::endl;
    std::cout << "Using the following patterns" << std::endl;
  }
  for( auto& patstr: _p_patternlist) {
    PatternAudio *pat = new PatternAudio( sound_engine );
    pat->_signature = _p_sig;
    pat->init_from_string( patstr );
    auto id = looper->add( pat );
    LOGMAIN( "  add p" << id << "=" << patstr );
    if (_p_verb) {
      std::cout << "  p" << id << " = " << patstr << std::endl;
    }
  }
  // and the loop
  Analyzer analyzer( looper );
  auto res = analyzer.parse( _p_loop );
  looper->set_sequence( res.begin(), res.end() );
  LOGMAIN( looper->str_dump() );
  if (_p_verb) {
    std::cout << looper->str_dump() << std::endl;
  }
  
  if (_p_gui) {
    run_gui();
  }
  else {
    run();
  }
  
  // Clean up before exit
  clear_globals();
  
  return 0;
}
// ***************************************************************************

