/* -*- coding: utf-8 -*- */

#ifndef LED_DISPLAY_WIDGET_HPP
#define LED_DISPLAY_WIDGET_HPP

#include <math.h>
#include <algorithm>
#define IMGUI_DEFINE_MATH_OPERATORS // Access to math operators
#include "imgui_internal.h"

#define W 15
#define H 7.5
#define v ImVec2

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
    const int max_nb_digits = 3;
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
      
      _digit(draw_list, _d,
             // half size of digit
             ImVec2(xsize_digit * 0.5, ypos_digit),
             // position of center of digit
            ImVec2( p_max.x - xpos_digit, p_min.y + ypos_digit));
      xpos_digit += xsize_digit + pad;
    }

  }
  
  // ****************************************************** LedDisplay::_digit
private:
  void _digit( ImDrawList *draw_list,
               int n,
               ImVec2 half_size, ImVec2 pos)
  {
    // Try to infer windowbg Color
    const ImU32 col_bg = ImGui::GetColorU32( ImGuiCol_WindowBg );
    // // DEBUG
    // std::cout << "Color of " << name << " is " << col_bg4 << std::endl;
    // std::cout << "   directly col is " << col_bg32 << std::endl;
  
    // TODO points for segment x0,y0 -> x1,y1
    // Transform to the first point of segment: [x,y] * half_size + [x,y]
    float r[7][4]={ {-1,-1,H,H},  
                    {1,-1,-H,H},
                    {1,0,-H,-H},
                    {-1,1,H,-W*1.5},
                    {-1,0,H,-H},
                    {-1,-1,H,H},
                    {-1,0,H,-H},
    };
    // looping for each segment
    for (int i=0;i<7;i++) {
      ImVec2 a,b;
      // horizontal segment ? (i = 0 or 3 or 6)
      if(i%3==0){
        a=ImVec2( pos.x+r[i][0]*half_size.x+r[i][2],
                  pos.y+r[i][1]*half_size.y+r[i][3]-H );
        b=ImVec2( a.x+half_size.x*2-W,
                  a.y+W );
      }
      // vertical segment ?
      else {
        a=ImVec2( pos.x+r[i][0]*half_size.x+r[i][2]-H,
                  pos.y+r[i][1]*half_size.y+r[i][3] );
        b=ImVec2( a.x+W,
                  a.y+half_size.y-W );
      }
    
      ImVec2 q = b - a;
      float s = W * 0.6, u = s - H;
      // // DEBUG
      // if (i==0) {
      //   std::cout << "----------------------" << std::endl;
      //   std::cout << "pos.x=" << pos.x << " pos.y=" << pos.y << std::endl;
      //   std::cout << "hs.x=" << half_size.x << " hs.y=" << half_size.y << std::endl;      
      //   std::cout << "a.x=" << a.x << " a.y=" << a.y << std::endl;
      //   std::cout << "b.x=" << b.x << " b.y=" << b.y << std::endl;
      //   std::cout << "q.x=" << q.x << " q.y=" << q.y << std::endl;
      // }

      // horizontal => 6 points of the polygon (q.y is W)
      if (q.x > q.y) {
        ImVec2 pp[]={{ a.x + u, a.y + q.y * 0.5f},
                     { a.x + s, a.y},
                     { b.x - s, a.y},
                     { b.x - u, a.y + q.y * 0.5f},
                     { b.x - s, b.y},
                     { a.x + s, b.y}};
        draw_list->AddConvexPolyFilled( pp, 6,
                                        // which color ? => on/off
                                        //(kd[n]>>(6-i)) & 1 ? 0xffffffff : 0xff222222);
                                        (kd[n]>>(6-i)) & 1 ? 0xffffffff : col_bg);
      }
      // vertical => 6 points of the polygon, q.x is W
      else {
        ImVec2 pp[]={
                     {a.x+q.x*.5f,a.y+u},
                     {b.x,a.y+s},
                     {b.x,b.y-s},
                     {b.x-q.x*.5f,b.y-u},
                     {a.x,b.y-s},
                     {a.x,a.y+s}};
        draw_list->AddConvexPolyFilled( pp, 6,
                                        // which color ? => on/off
                                        (kd[n]>>(6-i)) & 1 ? 0xffffffff : col_bg);
      }
    }
  }

  // *************************************************** LedDisplay::attributs
private:
  /* 
   Encoding of the segments of each digit.
   order: 0 hor_up right_up right_down hor_low lef_down left_up hor_mid
   0 = 01111110 (7E)
   1 = 00110000 (30)
   2 = 01101101 (6D)
   3 = 01111001 (79)
   4 = 00110011 (33)
   5 = 01011011 (5B)
   6 = 01011111 (5F)
   7 = 01110000 (70)
   8 = 01111111 (7F)
   9 = 01111011 (7B)
  10 = 00000000 (00) // to erase everything
 */
  const char kd[11]={0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B,
                     0x5F, 0x70, 0x7F, 0x7B,
                     0x00};
  
}; // LedDisplay class



#endif // LED_DISPLAY_WIDGET_HPP
