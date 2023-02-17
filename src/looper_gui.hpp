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
#include <analyzer.hpp>
#include <string>
#include <exception>
#include <memory>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
#define LOG_LOOPERGUI
#ifdef LOG_LOOPERGUI
#  define LOGLG(msg) (LOG_BASE("[LoGUI]", msg))
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
  LooperGUI( std::shared_ptr<Analyzer> analyzer ) :
    analyzer( analyzer ),
    looper( analyzer->_looper ),
    should_apply(false),
    input_flags(ImGuiInputTextFlags_None),
    error_flags(ImGuiInputTextFlags_ReadOnly)
  {
    _init_from_looper();
  }
  virtual ~LooperGUI()
  {
    LOGLG( "__DESTROY LooperGUI" );
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

      ImGui::InputText( "###LooperInput", &input_buffer, input_flags,
                        NULL /*cbk*/, NULL /*data*/);

      if (analyzer->has_error()) {
        // Red
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::InputTextMultiline("###LooperError", &error_buffer,
                                  // take all width, 5 lines
                                  ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 5),
                                  error_flags, NULL /*cbk*/, NULL /*data*/);
        ImGui::PopStyleColor();

      }
      if (ImGui::Button( "Apply")) {
        should_apply = true;
      }
    }
  }

  /** Apply the changes asked during draw */
  void apply()
  {
    if (should_apply) {
      LOGLG( "__Analyze : " << input_buffer );
      try {
        LOGLG( "  parsing ok" );
        auto res = analyzer->parse( input_buffer );
        looper->_formula.clear();
        looper->_formula.insert( looper->_formula.begin(),
                                 analyzer->_formula.begin(),
                                 analyzer->_formula.end() );
        looper->set_sequence( res.begin(), res.end() );
      }
      catch( std::runtime_error& e) {
        LOGLG( "  error in parsing" );
        LOGLG( analyzer->str_error() );
        error_buffer = analyzer->str_error();
      }
    }
    should_apply = false;
  }
  // ****************************************************** LooperGUI::private
  void _init_from_looper()
  {
    LOGLG( "_init_from_looper " );
    input_buffer.clear();
    input_buffer.insert( input_buffer.begin(),
                         looper->_formula.begin(), looper->_formula.end() );
    
  }
  // **************************************************** LooperGUI::attributs
  std::shared_ptr<Analyzer> analyzer;
  std::shared_ptr<Looper> looper;

  // State
  bool should_apply;   // somes changes need to be applied
  std::string input_buffer;
  std::string error_buffer;
  ImGuiInputTextFlags input_flags;
  ImGuiInputTextFlags error_flags;
  
}; // class LooperGUI


#endif // LOOPER_GUI_HPP
