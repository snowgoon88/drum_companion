/* -*- coding: utf-8 -*- */

#ifndef LED_DISPLAY_WIDGET_HPP
#define LED_DISPLAY_WIDGET_HPP

#include <math.h>
#include <algorithm>                // std::min
#define IMGUI_DEFINE_MATH_OPERATORS // Access to math operators
#include "imgui_internal.h"
#include "led_digit.hpp"

/**
 * Led Display inspired by 
 * see https://github.com/ocornut/imgui/issues/3606
 */

// ***************************************************************************
// **************************************************************** LedDisplay
// ***************************************************************************
class LedDisplay
{
public:
  LedDisplay( int max_number_of_digit = 3 )
    : max_nb_digits( max_number_of_digit )
  {}
  void draw( int& number )
  {
    ImVec2 size(320.0f, 180.0f);

    // button with Popup
    if (ImGui::InvisibleButton("canvas", size)) {
      ImGui::OpenPopup( "test_popup" );
    }
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(p0, p1);

    draw( draw_list, number,
          p0, p1, size );
    draw_list->PopClipRect();

    if (ImGui::BeginPopup("test_popup")) {
      ImGui::Text( "Enter BPM" );
      ImGui::Separator();
      ImGui::InputInt( "##BPM", &number );
      ImGui::EndPopup();
    }
            
  }
  void draw( ImDrawList *draw_list,
             int number,
             ImVec2 p_min, ImVec2 p_max, ImVec2 size )
  {
    // horizontal padding of digit = 2% of size.x
    float pad = 0.02 * size.x;
    // horizontal size of digit
    float xsize_digit = size.x / max_nb_digits - pad;
    // position of first (rightmost) digit center
    float xpos_digit = xsize_digit * 0.5;
    float ypos_digit = size.y * 0.5;

    // actual nb of digit
    int nb_digits = 0;
    if (number > 0) {
      nb_digits = std::min( max_nb_digits,
                            static_cast<int>(trunc(log10(number)) + 1 ));
    }
    
    for (int i=0; i<max_nb_digits; i++) {
      // get digit one by one
      int _d = number % 10;
      number /= 10;

      // to erase non-existant digits at the left
      if (i >= nb_digits) {
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

  // *************************************************** LedDisplay::attributs
private:
 int max_nb_digits;

}; // LedDisplay class



#endif // LED_DISPLAY_WIDGET_HPP
