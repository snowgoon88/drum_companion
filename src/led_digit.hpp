/* -*- coding: utf-8 -*- */

#ifndef LED_DIGIT_HPP
#define LED_DIGIT_HPP

#include <math.h>
#include <algorithm>
#define IMGUI_DEFINE_MATH_OPERATORS // Access to math operators
#include "imgui_internal.h"

// Width and Height of segments for digit
#define W 15
#define H 7.5

/** draw_digit
  * - draw_list: ImDrawList where to actually draw
  * - n: digit to draw, in {0,..,9}
  * - half_size: horizontal half size of digit
  * - pos: position of digit center
  *
  * Draw a digit as a collection of segment.
  * Each segment is either colored in white (0xffffffff) if present
  * or in col_bg if absent in the digit 'n'.
  * */
void draw_digit( ImDrawList *draw_list,
                 int n,
                 ImVec2 half_size, ImVec2 pos)
{
  // Try to infer windowbg Color
  const ImU32 col_bg = ImGui::GetColorU32( ImGuiCol_WindowBg );
  // // DEBUG
  // std::cout << "Color of " << name << " is " << col_bg4 << std::endl;
  // std::cout << "   directly col is " << col_bg32 << std::endl;

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

#endif // LED_DIGIT_HPP
