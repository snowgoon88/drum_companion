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

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

// Callback to handle GLFW errors
void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// ********************************************************** miniaudio GLOBAL
static constexpr int FS    = 44100;          // sampling rate
static constexpr int DOWNRATE = 100;         // danw sampling for display

ma_decoder         m_decoder;                // miniaudio audio file decoder
ma_device          m_device;                 // miniaudio playback device

std::string        m_filename;               // filename of audio file provided
double             m_duration;               // length of audio file in seconds
std::vector<float> m_samples;                // local copy of audio file samples
ma_uint64          m_nb_frames;                       // nb of frames in audio

double m_time  = 0;                          // current playback time in seconds

// ******************************************************** miniaudio copy_wav
void copy_wav( const std::string& filepath )
{
    std::cout << "__copy_wav:" << std::endl;
    // get filename
    std::filesystem::path p(filepath);
    m_filename = p.filename().string();

    // initialize decoder (force float, mono, 44100 Hz)
    auto decoder_cfg = ma_decoder_config_init(ma_format_f32, 1, FS);
    if (ma_decoder_init_file(filepath.c_str(), &decoder_cfg, &m_decoder) != MA_SUCCESS) {
        std::runtime_error("Failed to decode audio file: " + filepath);
    }

    // read all samples to local buffer
    ma_decoder_get_length_in_pcm_frames( &m_decoder, &m_nb_frames);
    m_samples.resize( m_nb_frames);
    // m_samples_x.resize( m_nb_frames );
    std::cout << "  read " << m_nb_frames << " frames." << std::endl;
    // // initialise to 0..m_nb_samples
    // std::iota( m_samples_x.begin(), m_samples_x.end(), 1);

    ma_uint64 nb_frame_read;
    ma_decoder_read_pcm_frames( &m_decoder, m_samples.data(),
                                m_nb_frames, &nb_frame_read );
    std::cout << "  copied " << nb_frame_read << " frames." << std::endl;
    ma_decoder_seek_to_pcm_frame( &m_decoder, 0 );
    // compute audio file duration
    m_duration = (double)m_nb_frames / (double)FS;
}

void plot_wav()
{
    // if (ImPlot::BeginPlot( "m_filename x, y" )) {
    //     ImPlot::SetupAxes("x","y");
    //     ImPlot::SetupAxesLimits( 0, m_nb_frames, -1.0, 1.0 );
    //     ImPlot::PlotLine("m_frames_1", m_samples_x.data(), m_samples.data(), m_nb_frames);
    //     ImPlot::EndPlot();
    // }
    if (ImPlot::BeginPlot( "m_filename samples" )) {
        ImPlot::SetupAxes("x","y");
        ImPlot::PlotLine("m_frames_2 ", m_samples.data(), m_nb_frames / DOWNRATE,
                         1.0, 0.0,                // xscale, xstart
                         ImPlotLineFlags_None, 0, // flags, offset
                         DOWNRATE * sizeof(float) );  // stride

        ImPlot::EndPlot();
    }
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

/**
 * usage: argv[0]: filepath
 */
int main(int argc, char *argv[])
{
    // load filepath (.wav file) into memory
    std::string filepath = (argc < 2) ? "ressources/Bashung - La Nuit Je Mens.wav" : argv[1];
    copy_wav( filepath );

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

        // // Demo windows
        // ImGui::ShowDemoWindow();
        // ImPlot::ShowDemoWindow();

        // Demo_LinePlots();
        plot_wav();

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

    return 0;
}
