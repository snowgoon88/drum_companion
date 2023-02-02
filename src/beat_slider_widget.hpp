/* -*- coding: utf-8 -*- */

#ifndef BEAT_SLIDER_HPP
#define BEAT_SLIDER_HPP

/** 
 * TODO fix padding,size
 * TODO better detection of direction/color in looper
 */

#define IMGUI_DEFINE_MATH_OPERATORS // Access to math operators
#include "imgui_internal.h"

#include <string>
#include <utils.hpp>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
// #define LOG_BS
#ifdef LOG_BS
#  define LOGBS(msg) (LOG_BASE("[BeatW]", msg))
#else
#  define LOGBS(msg)
#endif

// ***************************************************************************
// **************************************************************** BeatSlider
// ***************************************************************************
class BeatSlider
{
  const float H_slider = 40.0f;
  float W_slider = 40.0f;
  float PAD_UB_slider = 5.0f;
public:
  // **************************************************** BeatSlider::creation
  BeatSlider()
  {
    reset();
  }
  virtual ~BeatSlider() {};
  void reset()
  {
    dir = 1.f;
  }
  void set_dir_forward( bool value )
  {
    if (value) {
      dir = 1.f;
    }
    else {
      dir = -1.0f;
    }
  }
  // ******************************************************** BeatSlider::draw
  void draw( float relative_pos_to_goal, int beat_nb, bool highlight_color=false )
  {
    ImVec2 size(320.0f, 50.0f);

    // button
    ImGui::InvisibleButton("canvas_beat_slider", size);

    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(p0, p1);

    draw( draw_list, relative_pos_to_goal,
          highlight_color ? ImGui::ColorConvertFloat4ToU32(YELLOW_COL) : 0xffffffff,
          beat_nb + 1,
          p0, p1, size );
    draw_list->PopClipRect();

  }

  /** Redraw the slider (padded) and the Marker */
  void draw( ImDrawList *draw_list,
             float relative_pos_to_goal,
             ImU32 color,
             int beat_number,
             ImVec2 p_min, ImVec2 p_max, ImVec2 size )
  {
    LOGBS( "draw with " << std::to_string(beat_number) );

    // horizontal padding = 2% of size.x
    float pad = 0.02 * size.x;
    // horizontal size of digit
    float xsize_slider = size.x - pad;
    // UpperLeft and BottomRight limits
    ImVec2 slider_ul = ImVec2( p_min.x + pad, p_min.y + PAD_UB_slider );
    ImVec2 slider_br = ImVec2( p_max.x - pad, slider_ul.y+H_slider );
    // Size of Marker
    ImVec2 marker_size = ImVec2( W_slider, H_slider );
        
    // Position of goal
    ImVec2 goal_pos;
    if (dir > 0.f) {
      goal_pos = ImVec2( slider_br.x - marker_size.x,
                         slider_ul.y );
    }
    else {
      goal_pos = slider_ul;
    }
    // Compute position of upper-left for marker
    ImVec2 pos = ImVec2( goal_pos.x - dir * (xsize_slider-marker_size.x) * relative_pos_to_goal,
                         goal_pos.y );

    // Erase Slider
    // Try to infer windowbg Color
    // const ImU32 col_bg = ImGui::GetColorU32( ImGuiCol_WindowBg );
    draw_list->AddRectFilled( slider_ul, slider_br, 0x55555555 /*grey*/ );

    // Draw Marker (rounded Rect)
    draw_list->AddRectFilled( pos, pos + marker_size, color /*color*/,
                        2.f /*rounding*/ );

    // Number of beat in black, center of rounded rect
    std::string beat_str = std::to_string( beat_number );
    LOGBS( "__BS fontsize " << ImGui::GetFontSize() );
    //draw_list->AddText( pos + marker_size * 0.25, IM_COL32_BLACK, beat_str.c_str() );
    draw_list->AddText( NULL /*default font*/, 30 /*font siz*/,
                        pos + ImVec2( W_slider/4 ,0 ), IM_COL32_BLACK,
                        beat_str.c_str() );
  }

private:
  float dir; // direction to goal
}; // BeatSlider

#endif // BEAT_SLIDER_HPP


