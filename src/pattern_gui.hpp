/* -*- coding: utf-8 -*- */

#ifndef PATTERN_GUI_HPP
#define PATTERN_GUI_HPP

/** 
 * TODO
 */

#include <pattern_audio.hpp>
#include <iostream>                                                     // DEL
#include "imgui.h"
#include <algorithm>    // std::max
// ***************************************************************************
// **************************************************************** PatternGUI
// ***************************************************************************
class PatternGUI
{
public:
  // **************************************************** PatternGUI::creation
  PatternGUI() :
    pattern( new PatternAudio() ),
    should_apply(false),
    should_dump(false),
    val_changed(false),
    bpm_val(0), beat_val(0), subdiv_val(0),
    green_color(ImVec4(0.0f, 1.0f, 0.0f, 1.00f)),
    yellow_color(ImVec4(0.7f, 0.7f, 0.0f, 1.00f))
  {
    bpm_val = static_cast<int>(pattern->_signature.bpm);
    beat_val = static_cast<int>(pattern->_signature.beats);
    subdiv_val = static_cast<int>(pattern->_signature.subdivisions);
    val_changed = false;
  }
  virtual ~PatternGUI()
  {
    delete pattern;
  }

  // ******************************************************** PatternGUI::draw
  void draw()
  {
    // BPM, Signature
    ImGui::InputInt( "BPM: ", &bpm_val );
    ImGui::InputInt( "Nb Beats: ", &beat_val );
    ImGui::InputInt( "Nb SubDiv: ", &subdiv_val );

    // Button DUMP
    if (val_changed) {
      ImGui::PushStyleColor(ImGuiCol_Button, green_color);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, yellow_color);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, green_color);
    }
    if (ImGui::Button( "Dump")) {
      should_dump = true;
      should_apply = true;
    }
    if (val_changed) {
      ImGui::PopStyleColor(3);
    }
  }

  /** Apply the changes asked during draw */
  void apply()
  {
    bpm_val = std::max(1, bpm_val);
    beat_val = std::max(1, beat_val);
    subdiv_val = std::max(1, subdiv_val);
    
    if (val_changed == false ) {
      if (pattern->_signature.bpm != static_cast<uint>(bpm_val)) {
        val_changed = true;
      }
      else if (pattern->_signature.beats != static_cast<uint>(beat_val)) {
        val_changed = true;
      }
      else if (pattern->_signature.subdivisions != static_cast<uint>(subdiv_val)) {
        val_changed = true;
      }
    }
    
    if (should_apply) {
      pattern->_signature.bpm = static_cast<uint>(bpm_val);
      pattern->_signature.beats = static_cast<uint>(beat_val);
      pattern->_signature.subdivisions = static_cast<uint>(subdiv_val);
      val_changed = false;
    }
    
    if (should_dump) {
      std::cout << pattern->_signature << std::endl;
    }
    should_dump = false;
    should_apply = false;
  }

  // *************************************************** PatternGUI::attributs
  PatternAudio *pattern;

  // State
  bool should_apply; // some changes need to be applied ?
  bool should_dump;
  bool val_changed;
  int bpm_val;
  int beat_val;
  int subdiv_val;

  // Usefull
  ImVec4 green_color;
  ImVec4 yellow_color;
};


#endif // PATTERN_GUI_HPP
