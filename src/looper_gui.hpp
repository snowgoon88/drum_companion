/* -*- coding: utf-8 -*- */

#ifndef LOOPER_GUI_HPP
#define LOOPER_GUI_HPP

/** 
 * LooperGui( *Looper ) display the Looper, as a text window
 * BPM: can be imposed (or not) on every pattern
 * APPLY : apply the changes
 */
#include <utils.hpp>
#include "imgui.h"
#include <looper.hpp>
#include <string>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
//#define LOG_LOOPERGUI
#ifdef LOG_LOOPERGUI
#  define LOGLG(msg) (LOG_BASE("[LOGU]", msg))
#else
#  define LOGLG(msg)
#endif

// ***************************************************************************
// ***************************************************************** LooperGUI
// ***************************************************************************
class LooperGUI
{
public:
  // ***************************************************** LooperGUI::creation
  LooperGUI( Looper* looper ) :
    looper( looper ),
    should_apply(false),
    flags(ImGuiInputTextFlags_None)
  {
    _init_from_looper();
  }
  virtual ~LooperGUI()
  {
  }
  // **************************************************** LooperGUI::str_header
  std::string str_header () const
  {
    std::stringstream header;
    header << "Loop: " << looper->_formula;
    // to be sure the ImGui will not be computed from label, gives an ID
    header << "###LOOPER";

    return header.str();
  }
  
  // ********************************************************* LooperGUI::draw
  void draw()
  {
    if (ImGui::CollapsingHeader( str_header().c_str(),
                                 ImGuiTreeNodeFlags_None)) {

      ImGui::InputTextMultiline("###LooperInput", &buffer,
                                // take all width, 4 lines
                                ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 4),
                                flags, NULL /*cbk*/, NULL /*data*/);
      
      if (ImGui::Button( "Apply")) {
        should_apply = true;
      }
    }
  }

  /** Apply the changes asked during draw */
  void apply()
  {
    if (should_apply) {
      std::cout << "__LGUI = " << buffer << "-END BUFFER" << std::endl;
    }
    should_apply = false;
  }
  // ****************************************************** LooperGUI::private
  void _init_from_looper()
  {
    LOGLG( "_init_from_looper " );
    buffer.clear();
    buffer.insert( buffer.begin(),
                   looper->_formula.begin(), looper->_formula.end() );
  }
  // **************************************************** LooperGUI::attributs
  Looper* looper;

  // State
  bool should_apply;   // somes changes need to be applied
  std::string buffer;
  ImGuiInputTextFlags flags;
  
}; // class LooperGUI


#endif // LOOPER_GUI_HPP
