// MIT License

// Copyright (c) 2020-2024 Evan Pezent
// Copyright (c) 2025 Breno Cunha Queiroz

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// ***************************************************************************
// ************************************ take from libs/implot/example/main.cpp
// ***************************************************************************
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <GLFW/glfw3.h>
#include <iostream>

#include <filesystem>
#include <math.h>
#include <numeric>     // iota
#include <vector>

// WARN: I force using ALSA, as with PulseAudio, the audio buffer is not
//       purged when I stop the device => some scratch when restarting
//       the device.
#define MINIAUDIO_IMPLEMENTATION
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_ALSA
#include <miniaudio.h>

// taken from libs/common/Fonts/IconsFontAwesome5.h
#define ICON_FA_PLAY u8"\uf04b"
#define ICON_FA_PAUSE u8"\uf04c"
#define ICON_FA_STOP u8"\uf04d"
#define ICON_FA_STEP_BACKWARD u8"\uf048"
#define ICON_FA_STEP_FORWARD u8"\uf051"

const std::string help_msg = "\n\
*********************\n\
** WaveForm PLayer **\n\
*********************\n\n\
** Key Shortcuts ****\n\
 - <SPACE> : play/pause\n\
 - <Ctrl-SPACE>: stop\n\
 - <ENTER>: switch looping\n\n\
 - <F>: restore full_view\n\
\n\
** <Ctrl-Q> to quit\
";

// Callback to handle GLFW errors
void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}
std::string format_str( const ma_format fmt )
{
  switch (fmt) {
    case ma_format_f32:
      return "f32 => [-1.0, 1.0]";
    case ma_format_s16:
      return "s16 => [-32768, 32768]";
    case ma_format_s24:
      return "s24 => [-8388608, 8388608]";
    case ma_format_s32:
      return "s32 => [-2147483648, 2147483648]";
    case ma_format_u8:
      return "u8: [0, 255]";
    case ma_format_unknown:
        return "ma_format_unkown";
    case ma_format_count:
        return "ma_format_count";
  }
  return "unknown";
}
// ******************************************************************* Globals
const ImVec4 CLEAR_COL = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
const ImVec4 RED_COL = ImVec4(1.0f, 0.0f, 0.0f, 1.00f);
const ImVec4 GREEN_COL = ImVec4(0.0f, 1.0f, 0.0f, 1.00f);
const ImVec4 YELLOW_COL = ImVec4(0.7f, 0.7f, 0.0f, 1.00f);

// ********************************************************** miniaudio GLOBAL
static constexpr int FS    = 44100;          // sampling rate
static constexpr int DOWNRATE = 100;         // donwsampling for display

// TODO length of loop > frameCount of data_callback
bool               m_loop_enabled {true};
ma_uint64          m_loop_frame_start {300000};
ma_uint64          m_loop_frame_length {FS * 3};  // 3 second ?
ImPlotRect         g_loop_rect ( (double) (m_loop_frame_start / DOWNRATE),
                                 (double) ((m_loop_frame_start + m_loop_frame_length) / DOWNRATE),
                                 -1.0, 1.0 );

ma_decoder         m_decoder;                // miniaudio decoder for sound
ma_device_config   m_device_config;          // miniaudio device config
ma_device          m_device;                 // miniaudio device for sound

std::string        m_filename;               // filename of audio file provided
double             m_duration;               // length of audio file in seconds
std::vector<float> m_samples;                // local copy of audio file samples
ma_uint64          m_nb_frames;              // nb of frames in audio
double             m_pCursor {0.0};          // actual position of the audio cursor.


double m_time  = 0;                          // current playback time in seconds

enum PlayerState { play, paused, stop };
PlayerState m_playing {stop};                // is current audio playing

std::string g_player_title;                  // title of Player Window
double g_zoom_min {1.0};                 // plot zoom min
double g_zoom_max {10.0};                     // plot zoom max
bool g_demo_win {false};                     // display ImGuiDemoWindow ?
bool g_ask_play {false};                     // ask to play audio ?
bool g_ask_pause {false};                    // ask to play audio ?
bool g_ask_stop {false};                     // ask to play audio ?
bool g_ask_looping {false};                  // ask to switch audio looping
bool g_ask_fullview {false};                 // ask to see full song


// ****************************************************** ElapsedTimeFormatter
int ElapsedTimeFormatter(double value, char* buff, int size, void* user_data)
{
    // WARNING does not look for hours
    auto dts = div( static_cast<int>(value) * DOWNRATE, FS );  // seconds
    auto dtm = div( dts.quot, 60 );                            // minutes
    return snprintf( buff, size, "%d:%.2d", dtm.quot, dtm.rem );
}
// ******************************************************** miniaudio copy_wav
void copy_wav( const std::string& filepath )
{
    std::cout << "__copy_wav:" << std::endl;
    // get filename
    std::filesystem::path p(filepath);
    m_filename = p.filename().string();

    // miniaudio audio file decoder
    ma_decoder         decoder;
    // initialize decoder (force float, mono, 44100 Hz)
    auto decoder_cfg = ma_decoder_config_init(ma_format_f32, 1, FS);
    if (ma_decoder_init_file(filepath.c_str(), &decoder_cfg, &decoder) != MA_SUCCESS) {
        std::runtime_error("Failed to decode audio file: " + filepath);
    }

    // read all samples to local buffer
    ma_decoder_get_length_in_pcm_frames( &decoder, &m_nb_frames);
    m_samples.resize( m_nb_frames);
    // m_samples_x.resize( m_nb_frames );
    std::cout << "  read " << m_nb_frames << " frames." << std::endl;
    // // initialise to 0..m_nb_samples
    // std::iota( m_samples_x.begin(), m_samples_x.end(), 1);

    ma_uint64 nb_frame_read;
    ma_decoder_read_pcm_frames( &decoder, m_samples.data(),
                                m_nb_frames, &nb_frame_read );
    std::cout << "  copied " << nb_frame_read << " frames." << std::endl;
    ma_decoder_seek_to_pcm_frame( &decoder, 0 );
    // compute audio file duration
    m_duration = (double)m_nb_frames / (double)FS;

    ma_decoder_uninit(&decoder);
}

// ************************************************************* data_callback
// data_callback read from data_source (i.e. ma_decoder)
// and copy to pOutput of device
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput,
                   ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }

    // TOOD results
    ma_uint64 pCursor;
    ma_uint64 pFrameRead;
    ma_decoder_get_cursor_in_pcm_frames(pDecoder, &pCursor);
    m_pCursor = (double) (pCursor / DOWNRATE);

    // std::cout << "cursor at " << pCursor << std::endl;

    // TODO length of loop > frameCount of data_callback
    // bool loop_enabled {false};
    // ma_uint64 loop_frame_start {200000};
    // ma_uint64 loop_frame_length {48000 * 2};  // 1 second ?
    if (m_loop_enabled &&
        (pCursor < (m_loop_frame_start + m_loop_frame_length)) &&
        ((pCursor + frameCount) >= (m_loop_frame_start + m_loop_frame_length))) {
        //DEBUG std::cout << "END Loop" << std::endl;

        // feed what is left of loop
        ma_decoder_read_pcm_frames( pDecoder, pOutput,
                                    (m_loop_frame_start+m_loop_frame_length-pCursor),
                                    &pFrameRead );
        //DEBUG std::cout << "feed END " << pFrameRead << " from " << pCursor << std::endl;
        // then set pcm to loop_start
        ma_decoder_seek_to_pcm_frame( pDecoder, m_loop_frame_start );
    }
    else {
        //DEBUG std::cout << "OUT loop" << std::endl;
        ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &pFrameRead);
        // std::cout << "feed NOR " << pFrameRead << " from " << pCursor << std::endl;
    }

    // std::cout << "  " << (pCursor / FS) << " frames" << "\r";
    std::cout << "pCursor=" << pCursor << "\r" << std::flush;
    //DEBUG std::cout << "pCursor=" << pCursor << std::endl;
    // TODO what is this for ????????
    (void)pInput;
}

// TODO use exceptions
bool init_audio( const std::string& filepath )
{
    std::cout << "__init_audio:" << std::endl;
    // get filename
    std::filesystem::path p(filepath);
    m_filename = p.filename().string();
    g_player_title = std::string( "Player: " ) + m_filename;

    ma_result result;
    // open and read file as data_source
    std::cout << "  Opening: " << m_filename << std::endl;
    result = ma_decoder_init_file(filepath.c_str(), NULL, &m_decoder);
    if (result != MA_SUCCESS) {
        std::cerr << "ERROR: could not open file" << std::endl;
        return false;
    }
    std::cout << "  Sound DATA *******************************" << std::endl
            << "  format :" << format_str( m_decoder.outputFormat ) << std::endl
            << "  channels : " << m_decoder.outputChannels << std::endl
            << "  sampleRate : " << m_decoder.outputSampleRate << std::endl;

    // configure and initialize output/sink device with properties similar to
    // the decoded music
    // the device will use the given callbackfunction to be fed.

    m_device_config = ma_device_config_init(ma_device_type_playback);
    m_device_config.playback.format   = m_decoder.outputFormat;
    m_device_config.playback.channels = m_decoder.outputChannels;
    m_device_config.sampleRate        = m_decoder.outputSampleRate;
    m_device_config.dataCallback      = data_callback;
    m_device_config.pUserData         = &m_decoder;

    std::cout << "  Initialize playback device." << std::endl;
    if (ma_device_init(NULL, &m_device_config, &m_device) != MA_SUCCESS) {
        std::cerr << "ERROR: failed to initialize/open device" << std::endl;
        ma_decoder_uninit(&m_decoder);
        return false;
    }

    ma_uint64 nb_frames;
    ma_decoder_get_length_in_pcm_frames( &m_decoder, &nb_frames);
    std::cout << "  size: " << nb_frames << " frames." << std::endl;

    return true;
}

void plot_wav( const ImVec2& size=ImVec2(-1, 0),
               bool set_full_view=false,
               bool verb=false )
{
    // if (ImPlot::BeginPlot( "m_filename x, y" )) {
    //     ImPlot::SetupAxes("x","y");
    //     ImPlot::SetupAxesLimits( 0, m_nb_frames, -1.0, 1.0 );
    //     ImPlot::PlotLine("m_frames_1", m_samples_x.data(), m_samples.data(), m_nb_frames);
    //     ImPlot::EndPlot();
    // }
    if (ImPlot::BeginPlot( "m_filename samples",
                           size,
                           ImPlotFlags_NoTitle |
                           ImPlotFlags_NoMouseText |
                           ImPlotFlags_NoMenus )) {
        // if (verb) {
        //     auto limits_rect_init = ImPlot::GetPlotLimits();
        //     std::cout << "__plot_wav: before from " << limits_rect_init.Min().x
        //               << " to " << limits_rect_init.Max().x << std::endl;
        // }
        // ImPlot::SetupAxes("x","y");
        static auto axis_flags = ImPlotAxisFlags_NoLabel |
                                 ImPlotAxisFlags_NoSideSwitch |
                                 ImPlotAxisFlags_NoMenus;
        ImPlot::SetupAxis( ImAxis_X1, "audio_x", axis_flags );
        ImPlot::SetupAxisZoomConstraints(ImAxis_X1, g_zoom_min, g_zoom_max);
        ImPlot::SetupAxisFormat(ImAxis_X1, ElapsedTimeFormatter, nullptr);
        ImPlot::SetupAxis( ImAxis_Y1, "audio_y",
                           axis_flags | ImPlotAxisFlags_NoHighlight |
                           ImPlotAxisFlags_NoTickLabels );
        // first time, set to full view (set only once)
        ImPlot::SetupAxisLimits( ImAxis_X1,
                                 0.0, static_cast<double>(m_nb_frames / DOWNRATE),
                                 // 0.0, 1000.0,
                                 ImPlotCond_Once );
        // but, if asked, set back to full size
        if (set_full_view) {
            std::cout << "Setting FULLVIEW" << std::endl; //
            ImPlot::SetupAxisLimits( ImAxis_X1,
                                     0.0, static_cast<double>(m_nb_frames / DOWNRATE),
                                     // 0.0, 1000.0,
                                     ImPlotCond_Always );
        }
        ImPlot::SetupAxisLimits( ImAxis_Y1, -1.0, 1.0, ImPlotCond_Always );
        ImPlot::SetupFinish();

        if (verb or set_full_view) {
            auto limits_rect_before = ImPlot::GetPlotLimits();
            std::cout << "__plot_wav: before from " << limits_rect_before.Min().x
                      << " to " << limits_rect_before.Max().x << std::endl;
        }
        ImPlot::PlotLine("m_frames_2 ", m_samples.data(), m_nb_frames / DOWNRATE,
                         1.0, 0.0,                // xscale, xstart
                         ImPlotItemFlags_NoLegend , // flags
                         0, // offset
                         DOWNRATE * sizeof(float) );  // stride
        ImPlot::DragRect( 0, &g_loop_rect.X.Min, &g_loop_rect.Y.Min,
                          &g_loop_rect.X.Max, &g_loop_rect.Y.Max,
                          m_loop_enabled ? GREEN_COL : YELLOW_COL,
                          ImPlotDragToolFlags_NoCursors | ImPlotDragToolFlags_NoFit |
                          ImPlotDragToolFlags_NoInputs );
        ImPlot::DragLineX( 1, &m_pCursor, ImVec4(1,0,0,1), 2 /*thickness */,
                          ImPlotDragToolFlags_NoCursors | ImPlotDragToolFlags_NoFit |
                          ImPlotDragToolFlags_NoInputs );
        // if (verb) {
        //     auto limits_rect_after = ImPlot::GetPlotLimits();
        //     std::cout << "            after from " << limits_rect_after.Min().x
        //               << " to " << limits_rect_after.Max().x << std::endl;
        // }
        ImPlot::EndPlot();
    }
    // get the actual x-limits, keep the same span, but setting the actual pCursor
    // position at 'ratio' from left-limit

    // Returns the current plot axis range.
    //IMPLOT_API ImPlotRect GetPlotLimits(ImAxis x_axis = IMPLOT_AUTO, ImAxis y_axis = IMPLOT_AUTO);
    // see ##Tags, ##Rolling, ##Scrolling in 'implot_demo.cpp'
}

// ************************************************************* Demo_LinePlot
void Demo_LinePlots() {
    static float xs1[1001], ys1[1001];
    for (int i = 0; i < 1001; ++i) {
        xs1[i] = i * 0.001f;
        ys1[i] = 0.5f + 0.5f * sinf(50 * (xs1[i] + (float)ImGui::GetTime() / 10));
    }
    static double xs2[20], ys2[20];
    for (int i = 0; i < 20; ++i) {
        xs2[i] = i * 1/19.0f;
        ys2[i] = xs2[i] * xs2[i];
    }
    if (ImPlot::BeginPlot("Line Plots")) {
        ImPlot::SetupAxes("x","y");
        ImPlot::PlotLine("f(x)", xs1, ys1, 1001);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        ImPlot::PlotLine("g(x)", xs2, ys2, 20,ImPlotLineFlags_Segments);
        ImPlot::EndPlot();
    }
}

// ********************************************************************* utils
std::ostream& operator<<(std::ostream& out, const ImVec2& vec) {
   out << "( " << vec.x << ", " << vec.y << ")";
   return out;
}

/**
 * usage: argv[0]: filepath
 */
int main(int argc, char *argv[])
{
    // load filepath (.wav file) into memory
    std::string filepath = (argc < 2) ? "ressources/Bashung - La Nuit Je Mens.wav" : argv[1];
    if (not init_audio( filepath ))
        return -1;
    copy_wav( filepath );

    // setup graphic options
    // Zoom is min (3 secondes) and max a bit more than the whole song
    g_zoom_max = static_cast<double>(m_nb_frames + 2 * FS) / static_cast<double>(DOWNRATE);
    g_zoom_min = static_cast<double>(1 * FS) / static_cast<double>(DOWNRATE);
    std::cout << "Zoom limits: " << g_zoom_min << " - " << g_zoom_max << std::endl;

    // Setup error callback
    glfwSetErrorCallback(glfw_error_callback);

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Setup OpenGL version
#if defined(__APPLE__)
    // GL 3.2 + GLSL 150 (MacOS)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on MacOS
#else
    // GL 3.0 + GLSL 130 (Windows and Linux)
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    std::cout << help_msg << std::endl;
    // Create window
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Live Looper", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Disable vsync

    // Setup context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Setup style
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;
    config.MergeMode = false;
    io.Fonts->AddFontFromFileTTF("ressources/DejaVuSansMono.ttf", 16.0f, &config);
    // Merge into first font to add Icons
    config.MergeMode = true;
    io.Fonts->AddFontFromFileTTF("ressources/fontawesome-webfont.ttf", 0.0f, &config);

    // Setup backend
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGuiViewport.Size
        auto view_ptr = ImGui::GetMainViewport();
        // std::cout << "ViewPort size " << view_ptr->Size << std::endl;

        // Demo windows
        ImGui::SetNextWindowPos({view_ptr->Size.x - 600, 10}, ImGuiCond_Once);
        ImGui::ShowDemoWindow( &g_demo_win );
        // ImPlot::ShowDemoWindow();

        // Demo_LinePlots();
        ImGui::SetNextWindowPos({10,40}, ImGuiCond_Once);
        // auto ImGUI::ImVec2 w_size
        ImGui::SetNextWindowSize( {view_ptr->Size.x * 0.95f, 400}, ImGuiCond_Once);
        ImGui::PushFont( nullptr, 25.0f );
        if (ImGui::Begin( g_player_title.c_str() )) {

            //if (ImGui::Button( u8"⏵", {30, 30})) {
            ImGui::PushFont(nullptr, 40.0f);    // change fontSize

            ImGui::BeginGroup();
            {
                ImGui::BeginGroup();
                {
                    // if (ImGui::Button( u8"P", {80, 80})) {
                    // Colorize Play/Pause in Green if not stop
                    if (m_playing != stop) {
                        ImGui::PushStyleColor(ImGuiCol_Button, GREEN_COL);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, GREEN_COL);
                    }
                    if (m_playing == play) {
                        if (ImGui::Button( ICON_FA_PAUSE, {80, 80})) {
                            g_ask_pause = true;
                        }
                    }
                    else {
                        if (ImGui::Button( ICON_FA_PLAY, {80, 80})) {
                            g_ask_play = true;
                        }
                    }
                    if (m_playing != stop) {
                        ImGui::PopStyleColor(2);
                    }

                    ImGui::SameLine();
                    if (m_playing == stop) {
                        ImGui::PushStyleColor(ImGuiCol_Button, RED_COL);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, RED_COL);
                    }
                    if (ImGui::Button( ICON_FA_STOP, {80, 80})) {
                        g_ask_stop = true;
                    }
                    if (m_playing == stop) {
                        ImGui::PopStyleColor(2);
                    }
                }
                ImGui::EndGroup();
                // Capture group size to create a Button with same width
                ImVec2 size = ImGui::GetItemRectSize();

                if (m_loop_enabled) {
                    ImGui::PushStyleColor(ImGuiCol_Button, GREEN_COL);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GREEN_COL);
                    if (ImGui::Button( "Looping", {size.x, 80})) {
                        g_ask_looping = true;
                    }
                }
                else {
                    ImGui::PushStyleColor(ImGuiCol_Button, YELLOW_COL);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, YELLOW_COL);
                    if (ImGui::Button( "No loop", {size.x, 80})) {
                        g_ask_looping = true;
                    }
                }
                ImGui::PopStyleColor(2);

                ImGui::PopFont();
            }
            ImGui::EndGroup();
            // Capture group size to create a plot_wave with same height
            ImVec2 btn_size = ImGui::GetItemRectSize();

            ImGui::SameLine();
            plot_wav( ImVec2(-1.0, btn_size.y), g_ask_fullview, false /*verb*/ );
            g_ask_fullview = false;
        }
        ImGui::End();
        ImGui::PopFont();

        // Keyboard logic, like buttons.
        // see also ImGui::Shortcut()
        if (ImGui::IsKeyChordPressed( ImGuiMod_Ctrl | ImGuiKey_Space )) {
                g_ask_stop = true;
        }
        else {
            if (ImGui::IsKeyPressed( ImGuiKey_Space )) {
                if (m_playing == paused or m_playing == stop) {
                    g_ask_play = true;
                }
                if (m_playing == play) {
                    g_ask_pause = true;
                }
            }
        }
        if (ImGui::IsKeyPressed( ImGuiKey_Enter )) {
            g_ask_looping = true;
        }
        // fullview
        if (ImGui::IsKeyPressed( ImGuiKey_F )) {
            std::cout << "Ask for fullview" << std::endl;
            g_ask_fullview = true;
        }

        // Audio logic
        if (g_ask_play) {
            ma_device_start( &m_device);
            m_playing = play;
            g_ask_play = false;
        }
        if (g_ask_pause && m_playing == play) {
            ma_device_stop( &m_device );
            m_playing = paused;
            g_ask_pause = false;
        }
        if (g_ask_stop) {
            m_playing = stop;
            ma_device_stop( &m_device );
            // to start of audio
            ma_decoder_seek_to_pcm_frame( &m_decoder, 0 );
            m_pCursor = 0.0;
            g_ask_stop = false;
        }
        if (g_ask_looping) {
            m_loop_enabled = not m_loop_enabled;
            g_ask_looping = false;
        }

        // Logic
        // ImGuiIO& io = ImGui::GetIO();
        // if (ImGui::IsKeyDown( ImGuiKey_Q )) {
        //     if (io.KeyCtrl) {
        //         std::cout << "__IO: Ctrl-Q" << std::endl;
        //     }
        //     else {
        //         std::cout << "__IO: Q" << std::endl;
        //     }
        // }

        // Exit if Ctrl-Q
        // see also ImGui::Shortcut()
        if (ImGui::IsKeyChordPressed( ImGuiMod_Ctrl | ImGuiKey_Q )) {
            std::cout << "__IO: chord Ctrl-Q" << std::endl;
            glfwSetWindowShouldClose( window, true );
        }

        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    ma_device_uninit( &m_device );
    ma_decoder_uninit( &m_decoder );

    return 0;
}
