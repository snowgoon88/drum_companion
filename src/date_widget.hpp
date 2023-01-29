/* -*- coding: utf-8 -*- */

#ifndef DATE_WIDGET_HPP
#define DATE_WIDGET_HPP

/** 
 * Date with large LED dispay,
 * can be changed by button, click (InputInt) and keyboard +/- (+CTRL)
 */

#include <ctime>
#include <led_display_widget.hpp>

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
  bool verb = true;
  
public:
  // ***************************************************** DateWidget::creation
  DateWidget() : hour_leds(2), min_leds(2)
  {
  }
  virtual ~DateWidget() {}

  // ********************************************************* DateWidget::draw
  void draw()
  {
    ImVec2 size(160.0f, 180.0f);

    // get local time
    std::time_t time_now = std::time(nullptr);
    auto time_st = std::localtime( &time_now );
    ImGui::Text("Time: %2d:%2d", time_st->tm_hour, time_st->tm_min);

    // button with Popup
    ImGui::InvisibleButton("date", size);

    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(p0, p1);

    hour_leds.draw( draw_list, time_st->tm_hour,
                       p0, p1, size );
    draw_list->PopClipRect();

  }
private:
  // **************************************************** DateWidget::attributs
  // Widget to draw large LED display
  LedDisplay hour_leds, min_leds;
  
}; // DateWidget

#endif // Date_WIDGET_HPP
