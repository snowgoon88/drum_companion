/* -*- coding: utf-8 -*- */

#ifndef DATE_WIDGET_HPP
#define DATE_WIDGET_HPP

/** 
 * Date with large LED dispay,
 * can be changed by button, click (InputInt) and keyboard +/- (+CTRL)
 */

#include <ctime>
#include "led_digit.hpp"
#include <iostream>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
#define LOG_DATEW
#ifdef LOG_DATEW
#  define LOGDATEW(msg) (LOG_BASE("[DATEW]", msg))
#else
#  define LOGDATEW(msg)
#endif

// ***************************************************************************
// ***************************************************************** DateWidget
// ***************************************************************************
class DateWidget
{

public:
  // ***************************************************** DateWidget::creation
  DateWidget()
  {
  }
  virtual ~DateWidget() {}

  // ********************************************************* DateWidget::draw
  void draw()
  {
    ImVec2 size(350.0f, 180.0f);

    // get local time
    std::time_t time_now = std::time(nullptr);
    auto time_st = std::localtime( &time_now );
    // ImGui::Text("Time: %2d:%2d", time_st->tm_hour, time_st->tm_min);

    // button with Popup
    ImGui::InvisibleButton("date", size);

    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(p0, p1);

    draw( draw_list, time_st->tm_hour, time_st->tm_min, time_st->tm_sec,
          p0, p1, size );
    draw_list->PopClipRect();

  }
  void draw( ImDrawList *draw_list,
             int hour, int min, int sec,
             ImVec2 p_min, ImVec2 p_max, ImVec2 size )
  {
    // Try to infer windowbg Color
    const ImU32 col_bg = ImGui::GetColorU32( ImGuiCol_WindowBg );

    const int nb_digit = 2;
    const float semicolumn_hprop = 0.5;
    const float semicolumn_vprop = 0.3;
    // horizontal padding of digit = 2% of size.x
    float pad = 0.02 * size.x;
    // horizontal size of digit, taking into account size of ':'
    float xsize_digit = size.x / (2 * nb_digit + semicolumn_hprop) - pad;
    // horizontal spacing for the : between hour and minutes
    float inter_space = xsize_digit * semicolumn_hprop;
    // position of first (rightmost) digit center
    float xpos_digit = xsize_digit * 0.5;
    float ypos_digit = size.y * 0.5;

    // actual nb of digit for min
    int nb_mins = 1;
    if (min > 9) {
      nb_mins = 2;
    }

    // First minutes
    int _d = 0;
    for (int i=0; i<nb_digit; i++) {
      // get digit one by one
      _d = min % 10;
      min /= 10;

      draw_digit(draw_list, _d,
                 // half size of digit
                 ImVec2(xsize_digit * 0.5, ypos_digit),
                 // position of center of digit
                 ImVec2( p_max.x - xpos_digit, p_min.y + ypos_digit));
      xpos_digit += xsize_digit + pad;
    }

    // now the ':'
    // replace xpos_digit to end of previous digit and add half inter_space
    xpos_digit -= (xsize_digit + pad);
    xpos_digit += (xsize_digit)/2 + inter_space / 2;
    ImVec2 sc_min = ImVec2( p_max.x - xpos_digit - (H),
                            p_min.y + ypos_digit * (1.0-semicolumn_vprop) - (H) );
    ImVec2 sc_max = sc_min + ImVec2( 2*H, 2*H );

    draw_list->AddRectFilled( sc_min, sc_max,
                              sec % 2 == 0 ? 0xffffffff : col_bg );

    sc_min = ImVec2( p_max.x - xpos_digit - (H),
                     p_min.y + ypos_digit * (1.0+semicolumn_vprop) - (H) );
    sc_max = sc_min + ImVec2( 2*H, 2*H );

    draw_list->AddRectFilled( sc_min, sc_max,
                              sec % 2 == 0 ? 0xffffffff : col_bg );

    // add leftover of inter_space and position at center of next digit
    xpos_digit += inter_space / 2;
    xpos_digit += (xsize_digit)/2 + pad;

    // and hours
    // actual nb of digit
    int nb_hours = 1;
    if (hour> 9) {
      nb_hours = 2;
    }

    _d = 0;
    for (int i=0; i<nb_digit; i++) {
      // get digit one by one
      _d = hour % 10;
      hour /= 10;

      // to erase non-existant digits at the left
      if (i >= nb_hours) {
        _d = 10;
      }

      draw_digit(draw_list, _d,
                 // half size of digit
                 ImVec2(xsize_digit * 0.5, ypos_digit),
                 // position of center of digit
                 ImVec2( p_max.x - xpos_digit, p_min.y + ypos_digit));
      xpos_digit += xsize_digit + pad;
    }
  }

}; // DateWidget

#endif // Date_WIDGET_HPP
