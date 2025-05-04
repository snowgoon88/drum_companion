/* -*- coding: utf-8 -*- */

/** 
 * CLI and Grafik DrumCompagnon
 * - cli arguments: bpm, sig, pattern, loop,
 * - play repeated sound using looper with sound_pattern
 * - grafik On/Off (if GUI, only ONE pattern)
 * - play/stop/pause
 *
 * Has a GLOBAL bpm (_main_bpm) which is superimposed on every Pattern
 * => disabling BPM setting in PatternGUI
 * => _default_bpm if not passed as parameter.
 * TODO looper should have a _main_bpm, it can inpose to its PatternAudio
 * TODO allow PatternAudio with separate BPM
 */

// Parsing command line options
#include "docopt.h"

#include <filesystem>
namespace stdfs = std::filesystem;
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <memory>            // unique_ptr, shared_ptr, etc
#include <optional>

#include <signal.h>          // C std lib (signal, sigaction, etc)

#include "utils.hpp"        // loggers, etc
#include "pattern_audio.hpp"
#include "sound_engine.hpp"
#include "looper.hpp"
#include "analyzer.hpp"
#include "pattern_gui.hpp"
#include "looper_gui.hpp"
#include "date_widget.hpp"
#include "beat_slider_widget.hpp"
#include "bpm_widget.hpp"
#include "blank_screen.hpp"
// File Dialog for Load/Save
#include <ImGuiFileDialog.h>

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
#define LOG_MAIN
#ifdef LOG_MAIN
#  define LOGMAIN(msg) (LOG_BASE("[MAIN]", msg))
#else
#  define LOGMAIN(msg)
#endif

//#define LOG_MAINLOOP
#ifdef LOG_MAINLOOP
#  define LOGMAINLOOP(msg) (LOG_BASE("[LOOP]", msg))
#else
#  define LOGMAINLOOP(msg)
#endif
// *************************************************************** App GLOBALS
std::shared_ptr<SoundEngine> sound_engine = nullptr;
std::shared_ptr<PatternAudio> pattern_audio = nullptr;  // GUI only
std::shared_ptr<Analyzer> analyzer = nullptr;
std::shared_ptr<Looper> looper = nullptr;
bool should_exit = false;
BlankScreen blank_screen;



// *********************************************************************** GUI

std::list<PatternGUI> pg_list;
std::unique_ptr<LooperGUI> looper_widget = nullptr;
std::unique_ptr<DateWidget> date_widget;
std::unique_ptr<BeatSlider> beat_widget;
std::unique_ptr<BPMWidget> bpm_widget;

// Args
Signature _p_sig {90, 4, 2};
// unsigned int _main_bpm = _p_sig.bpm;
// unsigned int _p_bpm_default = _p_sig.bpm;
std::optional<unsigned int> _p_bpm;
std::list<std::string> _p_patternlist;
std::string _p_loop = "p0";
std::optional<std::string> _p_infile;
std::optional<std::string> _p_outfile;
bool _p_gui = false;
bool _p_time = false;
bool _p_verb = false;
bool _p_debug = false;

ImVec4 hoover_color = YELLOW_COL;
ImVec4 NoteButton::colors[3] = {CLEAR_COL, GREEN_COL, RED_COL};
ImVec4 NoteButton::hoover_color = YELLOW_COL;

// Popup file dialogs
bool is_popup_dialog = false;
bool should_open_load_dialog = false;
bool should_open_save_dialog = false;
const char *filter_dialog = "Loop *.loop{.loop},All{(.*)}}";
std::optional<std::string> ask_load_file = std::nullopt;
std::optional<std::string> ask_save_file = std::nullopt;

void clear_globals()
{
  LOGMAIN( "__CLEANING" );
  // if (sound_engine)
  //   LOGMAIN( "  will clean sound_engine" );
  //   delete sound_engine;
  // }
  // LOGMAIN( "  sound_engine OK" );

  // if (pattern_audio != nullptr ) {
  //   LOGMAIN( "  will clean pattern_audio" );
  //   delete pattern_audio;
  // }
  // LOGMAIN( "  pattern_audio OK" );

  // if (looper != nullptr) {
  //   LOGMAIN( "  will clean looper" );
  //   for( auto& pat_ptr: looper->all_patterns) {
  //     LOGMAIN( "    will clean pattern " << pat_ptr->_id );
  //     delete pat_ptr;
  //     LOGMAIN( "    pattern OK" );
  //   }
  //   delete looper;
  // }
  // LOGMAIN( "  looper OK" );

  // if (analyzer != nullptr) {
  //   LOGMAIN( "  will clean analyzer" );
  //   delete analyzer;
  // }
  // LOGMAIN( "  analyzer OK" );

  // if (lg) delete lg;

  //DEL if (date_widget) delete date_widget;
  //DEL if (beat_widget) delete beat_widget;
  //DEL if (bpm_widget-> delete bpm_widget;
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
      drum_companion [-h -v -d -g -t -b <int> -s <str>] [-p <str>]... [-l <str> -o <str>]
      drum_companion [-h -v -d -g -t -b <uint>] <infile>


    Options:
      -h --help              Show this screen
      -v --verbose           Display some info
      -d --debug             Debug Mode (ImGui::ShowStackToolWindow())
      -g --gui               With GUI
      -b --bpm=<uint>        BPM (default is 90)
      -s --sig=<str>         signature [default: 4x2]
      -p --pattern=<str>     patterns, can be REPEATED [default: 2x1x1x1x]
      -l --loop=<str>        sequence of patterns (like 2x(p0+P1)) [default: p0]
      -t --time              Display time as HH:MM (only in GUI mode)
      -o --outfile=<str>     file to save looper (or pattern)
)";

void setup_options( int argc, char **argv )
{
  std::map<std::string, docopt::value> args = docopt::docopt(_usage,
                                                  { argv + 1, argv + argc },
                                                  // show help if requested
                                                  true,
                                                  // version string
                                                  "Drum Companion 1.0");

  std::cout << "******* DocOpt arguments" << std::endl;
  for(auto const& arg : args) {
    std::cout << arg.first << ": " << arg.second;
    std::cout << " type=" << type_name<decltype(arg.second)>() << std::endl;
  }
  std::cout << "Patterns List=" << std::boolalpha << args["--pattern"].isStringList() << std::endl;
  std::cout << "PATTERN " << args["--pattern"] << std::endl;
  // //std::cout << "STRING " << args["--pattern"].asString() << std::endl;
  std::cout << "LIST   ";
  for( auto& elem: args["--pattern"].asStringList()) {
     std::cout << elem << ", ";
  }
  std::cout << std::endl;
  // //exit(23);

  if (args["--bpm"]) {
    _p_bpm = args["--bpm"].asLong();
  }
  // TODO _p_sig.bpm not set !!
  _p_sig.from_string( args["--sig"].asString());
  // Create the various patterns
  for( auto& pat: args["--pattern"].asStringList()) {
    _p_patternlist.push_back( pat );
  }
  _p_loop = args["--loop"].asString();
  if (args["--outfile"]) {
    _p_outfile = args["--outfile"].asString();
  }
  if (args["<infile>"]) {
    _p_infile = args["<infile>"].asString();
    // std::cout << "INFILE=" << *_p_infile << std::endl;
  }
  if (args["--gui"].asBool()) {
    _p_gui = true;
  }
  if (args["--time"].asBool()) {
    _p_time = true;
  }
  if (args["--verbose"].asBool()) {
    _p_verb = true;
  }
  if (args["--debug"].asBool()) {
    _p_debug = true;
  }

  //exit(22);

}
// ***************************************************************************
// ********************************************************** ADD/DEL Patterns
// ***************************************************************************
void add_pattern()
{
  auto pat = std::make_shared<PatternAudio>( sound_engine );
  pat->_signature = _p_sig;
  pat->init_as_empty();
  auto id = looper->add( pat );
  UNUSED( id );
  LOGMAIN( "  add new empty pat" << id );

 if (_p_gui) {
   pg_list.push_back( PatternGUI(pat) );
 }
}
void del_pattern( int id )
{
  if (_p_gui) {
    // need the PatternAudio to remove it from pg_list, if needed
    auto pat_gui = std::find_if( pg_list.begin(), pg_list.end(),
                                 [id] (PatternGUI pg) { return pg.pattern->_id == id; } );
    if (pat_gui != pg_list.end()) {
      pg_list.erase( pat_gui );
    }
  }
  // remove patterns from Looper
  auto removed = looper->remove( id );
  // if removed, must revalidate Looper formula
  if (removed) {
    LOGMAIN( "  pat " << id << " removed" );
    try {
      LOGMAIN( "  analyze " << looper->_formula );
      auto res = analyzer->parse(looper->_formula);

      looper->_formula.clear();
      looper->_formula.insert(looper->_formula.begin(),
                              analyzer->_formula.begin(),
                              analyzer->_formula.end());
      looper->set_sequence(res.begin(), res.end());
    } catch (std::runtime_error &e) {
      LOGMAIN( "  ERROR in parsing " << e.what() );
      LOGMAIN( analyzer->str_error() );
      if (analyzer->has_error()) LOGMAIN( "  analyzer HAS ERROR" );
      // formula is invalid, make sure Looper cannot run.
      looper->_state = Looper::LooperState::empty;
      // TODO accessing lg->error_buffer is bad practice
      looper_widget->error_buffer = analyzer->str_error();
    }
  }
}
// ***************************************************************************
// ***************************************************************** Save/Load
// ***************************************************************************
void save_looper( const std::string& filename )
{
  LOGMAIN( "__Writing to " << filename );
  std::ofstream ofile( filename );
  looper->write_to( ofile );
  ofile.close();
}
void load_looper(const std::string &filename)
{
  LOGMAIN( "__Loading " << filename );
  std::ifstream ifile(filename);
  looper->read_from(ifile);
  ifile.close();
}
// ***************************************************************************
// ********************************************************** build/update_gui
// ***************************************************************************
void build_pattern_gui()
{
  pg_list.clear();
  for( auto pat: looper->all_patterns) {
    pg_list.push_back( PatternGUI(pat) );
  }
}

// ***************************************************************************
// ******************************************************************* run_gui
// ***************************************************************************
int run_gui()
{
  // ************************************************************* GUI - state

  // ********************************************************** GUI - creation
  build_pattern_gui();

  looper_widget = std::make_unique<LooperGUI>( analyzer );

  beat_widget = std::make_unique<BeatSlider>();
  bpm_widget = std::make_unique<BPMWidget>();
  date_widget = std::make_unique<DateWidget>();

  // Other GUI variables
  bool gui_ask_end = false;
  bool should_pause = false;
  bool should_stop = false;
  bool should_run = false;

  bool ask_add_pa = false;
  int ask_del_pa = -1;      // if >= 0, ask to delete indicated PatternAudio

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
      600, 500, "Drum Compagnon v0", NULL, NULL);
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

  // Try Scaling up
  ImGui::GetStyle().ScaleAllSizes(2.0f);

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
      if (_p_time) {
        // Create a window with title and append into it.
        ImGui::Begin("Time");

        date_widget->draw();

        // ImGui::SameLine();

        ImGui::End();
      }
      LOGMAINLOOP("__guiloop Drum Companion");
      // Create a window with title and append into it.
      std::stringstream title_str;
      title_str << "Drum Companion - ";
      if (_p_outfile) {
        // path relative to current_dir()
        stdfs::path rel_outfile = stdfs::proximate( stdfs::path( _p_outfile.value() ));
        title_str << rel_outfile;
        //title_str << _p_outfile.value();
      }
      else {
        title_str << "NOFILE";
      }
      ImGui::Begin( title_str.str().c_str(), nullptr /*no close btn*/, ImGuiWindowFlags_MenuBar);
      // MenuBar
      if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
          if (ImGui::MenuItem("Open", "Ctrl+O")) {
            should_open_load_dialog = true;
          }
          if (ImGui::MenuItem("Save", "Ctrl+S")) {
            if (_p_outfile) {
              ask_save_file = _p_outfile.value();
            }
            else {
              should_open_save_dialog = true;
            }
          }
          if (ImGui::MenuItem("Save As..")) {
            should_open_save_dialog = true;
          }
          ImGui::Separator();
          if (ImGui::MenuItem("Quit", "Ctrl+Q")) {}
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
          if (ImGui::MenuItem("About", nullptr)) {}
          ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
      }

      // FileDialog
      if (should_open_load_dialog) {
        should_open_load_dialog = false;
        is_popup_dialog = true;
        ImGuiFileDialog::Instance()->OpenDialog("ChooseLoadFileK", // dialog key
                                                "Choose file to load",     // dialog title
                                                filter_dialog,
                                                ".",  // basedir for scan
                                                "",   // base filename
                                                0,    // func for display right
                                                0.0f, // base width pane
                                                1,    // count selection
                                                nullptr, // user data
                                                0 // flags
                                                );
      }
      else if (should_open_save_dialog) {
        should_open_save_dialog = false;
        is_popup_dialog = true;
        ImGuiFileDialog::Instance()->OpenDialog("ChooseSaveFileK", // dialog key
                                                "Choose file to save",     // dialog title
                                                filter_dialog,
                                                ".",  // basedir for scan
                                                "",   // base filename
                                                0,    // func for display right
                                                0.0f, // base width pane
                                                1,    // count selection
                                                nullptr, // user data
                                                ImGuiFileDialogFlags_ConfirmOverwrite  // flags
                                                );
      }
      // Then deal with FileDialog
      if (ImGuiFileDialog::Instance()->Display("ChooseLoadFileK")) {
        // Action OK
        if (ImGuiFileDialog::Instance()->IsOk()) {
          std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
          std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
          // action
          std::cout << "__LOADING " << std::endl;
          std::cout << "PathName = " << filePathName << std::endl;
          std::cout << "Path = " << filePath << std::endl;
          ask_load_file = std::make_optional<std::string>(filePathName);
        }
        // Close
        ImGuiFileDialog::Instance()->Close();
        is_popup_dialog = false;
      }
      else if (ImGuiFileDialog::Instance()->Display("ChooseSaveFileK")) {
        // Action OK
        if (ImGuiFileDialog::Instance()->IsOk()) {
          std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
          std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
          // action
          std::cout << "__SAVING" << std::endl;
          std::cout << "PathName = " << filePathName << std::endl;
          std::cout << "Path = " << filePath << std::endl;
          //save_looper(filePathName);
          // add ".loop" if not already the case
          std::filesystem::path pathName {filePathName};
          if (not pathName.has_extension()) {
            pathName.replace_extension( std::filesystem::path {"loop"});
          }
          ask_save_file = pathName.c_str();
          //ask_load_file = std::make_optional<std::string>(filePathName);
        }
        // Close
        ImGuiFileDialog::Instance()->Close();
        is_popup_dialog = false;
      }


      // BeatSlider
      beat_widget->set_dir_forward( looper->odd_beat );
      beat_widget->draw( looper->to_next_beat,
                         looper->beat_number(),
                         looper->from_first_beat );
      // BPM
      bpm_widget->draw( looper->_main_bpm );

      LOGMAINLOOP( "  buttons" );
      // Play/Pause
      if (looper->_state == Looper::LooperState::running) {
        ImGui::PushStyleColor(ImGuiCol_Button, GREEN_COL);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, GREEN_COL);
      }
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoover_color);
      if (ImGui::Button(u8"⏵")) { // 0x23F5
        should_run = true;
      }
      ImGui::PopStyleColor(1);
      if (looper->_state == Looper::LooperState::running) {
        ImGui::PopStyleColor(2);
      }

      ImGui::SameLine();
      if (looper->_state == Looper::LooperState::paused) {
        ImGui::PushStyleColor(ImGuiCol_Button, YELLOW_COL);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, YELLOW_COL);
      }
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoover_color);
      if (ImGui::Button(u8"⏸")) { // 0x23F8
        should_pause = true;
      }
      ImGui::PopStyleColor(1);
      if (looper->_state == Looper::LooperState::paused) {
        ImGui::PopStyleColor(2);
      }
      ImGui::SameLine();
      if (looper->_state == Looper::LooperState::ready) {
        ImGui::PushStyleColor(ImGuiCol_Button, RED_COL);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, RED_COL);
      }
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoover_color);

      if (ImGui::Button(u8"⏹")) { // 0X23F9
        should_stop = true;
      }
      ImGui::PopStyleColor(1);
      if (looper->_state == Looper::LooperState::ready) {
        ImGui::PopStyleColor(2);
      }

      // LooperGUI
      looper_widget->draw();

      // PatternGUIs
      for( auto& pg: pg_list) {
        pg.draw( !looper->_sync_bpm );
      }

      ImGui::Separator();
      LOGMAINLOOP( "ADD/DEL PatternAudio" );

      if (ImGui::Button("ADD pattern")) {
        ask_add_pa = true;
      }
      for( auto& pg: pg_list ) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, RED_COL);
        std::string del_str = "Del P"+std::to_string( pg.pattern->_id );
        if (ImGui::Button( del_str.c_str() )) {
            ask_del_pa = pg.pattern->_id;
        }
        ImGui::PopStyleColor(1);
      }

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();

      if (_p_debug) {
        ImGui::ShowStackToolWindow(&_p_debug);
      }
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
    if (ImGui::IsKeyReleased(526)) { // Escape
      LOGMAIN( "__ESC");
      // if a popup_menu is open, close it
      if (is_popup_dialog) {
        ImGuiFileDialog::Instance()->Close();
        is_popup_dialog = false;
        LOGMAIN( "  closing popup");
      }
      else {
        gui_ask_end = true;
        LOGMAIN( "  asking to end");
      }
    }
    if (ImGui::IsKeyReleased(564)) { // S
      if (ImGui::GetIO().KeyCtrl) {  // Ctrl+S
        if (_p_outfile) {
          ask_save_file = _p_outfile.value();
        }
        else {
          should_open_save_dialog = true;
        }
      }
    }
    if (ImGui::IsKeyReleased(564)) { // S
      if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift) {  // Ctrl+Shift+S
        should_open_save_dialog = true;
      }
    }
    if (ImGui::IsKeyReleased(560)) { // O
      if (ImGui::GetIO().KeyCtrl) {  // Ctrl+O
        should_open_load_dialog = true;
      }
    }
    if (ImGui::IsKeyReleased(546)) { // Q (key, not char)
      if (ImGui::GetIO().KeyCtrl) {  // Ctrl+Q
        gui_ask_end = true;
      }
    }

    // Now apply logic
    LOGMAINLOOP( "__apply logic" );
    // TODO some logic are not mutually possible ?

    // saving file
    if (ask_save_file) {
      save_looper( ask_save_file.value() );
      // update _p_outfile
      _p_outfile = ask_save_file;
      ask_save_file.reset();
    }

    // loading file
    if (ask_load_file) {
      // TODO reset, not running
      load_looper(ask_load_file.value());
      looper_widget->_init_from_looper();

      bpm_widget->set_new_bpm( looper->_main_bpm );
      build_pattern_gui();

      _p_outfile = ask_load_file.value();
      ask_load_file.reset();
    }

    // TODO toujours vrai et ok ce qui suit ?
    //      NON !!!, car BPM overule new looper read from file
    // update BPM of looper only when NOT runnine
    // if (looper->_state != Looper::LooperState::running) {
      looper->set_all_bpm( bpm_widget->get_new_bpm() );
    // }

    // ADD/DEL PatternAudio
    if (ask_add_pa) {
      add_pattern();
      ask_add_pa = false;
    }
    if (ask_del_pa >= 0) {
      del_pattern( ask_del_pa );
      ask_del_pa = -1;
    }

    looper_widget->apply();
    for( auto& pg: pg_list) {
      pg.apply();
    }

    // And deal with Play/Pause
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

  // path to working directory
  LOGMAIN( "__WorkingDir " << stdfs::current_path() );

  // Args
  setup_options( argc, argv );

  // ****************************************************************** Sounds
  LOGMAIN( "__SoundEngine" );
  sound_engine = std::make_shared<SoundEngine>();
  // variables are not used, but show how could be used
  auto idx_clave = sound_engine->add_sound( "ressources/claves_120ms.wav" );
  UNUSED(idx_clave);
  auto idx_cow = sound_engine->add_sound( "ressources/cowbell.wav" );
  UNUSED(idx_cow);
    
  // ************************************************************* PlayPattern
  // DEL not needed anymore
  // LOGMAIN( "__PATTERN_AUDIO with SoundEngine" );
  // pattern_audio = new PatternAudio( sound_engine );
  // pattern_audio->_id = 0;
  // if (_p_bpm) {
  //   _p_sig.bpm = *_p_bpm;
  // }
  // pattern_audio->_signature = _p_sig;
  // // docopt default value ensure that _p_patternlist has at least ONE element
  // pattern_audio->init_from_string( _p_patternlist.front() );

  // ****************************************************************** Looper
  LOGMAIN( "__LOOPER with SoundEngine and all PatternAudio" );
  looper = std::make_shared<Looper>( sound_engine );
  analyzer = std::make_shared<Analyzer>( looper );
  // if there is an infile, it has priority
  if (_p_infile) {
    stdfs::path infile = stdfs::relative( stdfs::path( _p_infile.value() ));
    LOGMAIN( "__Input file: " << infile  );
    if (infile.has_relative_path()) {
      LOGMAIN( "  Rel. Path: " << infile.relative_path() );
    }
    load_looper( _p_infile.value() );
    _p_outfile = _p_infile.value();

    // if param bpm; takes precedence
    if (_p_bpm) {
      looper->set_all_bpm( _p_bpm.value() );
    }
  }
  else {
    // bpm passed as param takes precedence
    if (_p_bpm) {
      _p_sig.bpm = _p_bpm.value();
    }
    for( auto& patstr: _p_patternlist) {
      auto pat = std::make_shared<PatternAudio>( sound_engine );
      pat->_signature = _p_sig;
      pat->init_from_string( patstr );
      auto id = looper->add( pat );
      UNUSED (id);
      LOGMAIN( "  add p" << id << "=" << patstr );
    }
    // and the loop
    auto res = analyzer->parse( _p_loop );
    looper->_formula = _p_loop;
    looper->set_sequence( res.begin(), res.end() );
    LOGMAIN( looper->str_dump() );

    if (_p_outfile) {
      save_looper( _p_outfile.value() );
    }
  }

  if (_p_verb) {
    std::cout << looper->str_verbose() << std::endl;
  }

  // prevent blank_screen
  blank_screen.disable();
  //LOGMAIN( "__afterDisable"+blank_screen.str_info() );
  if (_p_gui) {
    run_gui();
  }
  else {
    run();
  }
  // restore blank_screen behavior
  blank_screen.restore();
  // LOGMAIN( "__afterRestore"+blank_screen.str_info() );
  
  // Clean up before exit
  clear_globals();

  LOGMAIN( "__END" );
  return 0;
}
// ***************************************************************************

