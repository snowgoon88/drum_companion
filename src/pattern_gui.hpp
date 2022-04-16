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
#include <string>       // std::stoi

#include <utils.hpp>
// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
//#define LOG_PATTERNGUI
#ifdef LOG_PATTTERNGUI
#  define LOGPG(msg) (LOG_BASE("[PAGU]", msg))
#else
#  define LOGPG(msg)
#endif
//#define LOG_NOTEGUI
#ifdef LOG_NOTEGUI
#  define LOGNG(msg) (LOG_BASE("[NOGU]", msg))
#else
#  define LOGNG(msg)
#endif

// ***************************************************************************
// **************************************************************** NoteButton
// ***************************************************************************
/** It is Button with/out bordern that can take 3 colored states
 *
 */
class NoteButton
{
  // ********************************************* NoteButton::class_variables
public:
  static const uint max_val = 3;
  static ImVec4 colors[max_val];
  static ImVec4 hoover_color;
  
public:
  // **************************************************** NoteButton::creation
  NoteButton( const uint id, const std::string& label,
              ImVec4 *border_color=nullptr ) :
    id(id),
    val(0),
    label(label),
    bordercol(border_color)
  {
    
  }
  virtual ~NoteButton()
  {
  }
  // ******************************************************** NoteButton::draw
  void draw()
  {
    if (bordercol != nullptr) {
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f );
      ImGui::PushStyleColor(ImGuiCol_Border, *bordercol );
    }
    // Button color depends on state
    ImGui::PushStyleColor(ImGuiCol_Button, colors[val]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors[(val+1)%max_val]);
    if (ImGui::Button( label.c_str() )) {
      LOGNG( "Trying to change val=" << val << " of " << label);
      val = (val+1) % max_val;
    }
    ImGui::PopStyleColor(3);
    
    if (bordercol != nullptr) {
      ImGui::PopStyleVar(1);
      ImGui::PopStyleColor(1);
    }
  }
  // *************************************************** NoteButton::attributs
public:
  uint get_id() { return id; }
  uint get_val() { return val; }
  void set_val(uint val) { this->val = (val % max_val); }

  void set_label( const std::string& label )
  {
    this->label = label;
  }
  void set_color( ImVec4* border_color )
  {
    this->bordercol = border_color;
  }
  
private:
  uint id;
  uint val;
  std::string label;
  ImVec4* bordercol;
};
// ********************************************************** NoteButton - END


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
    _update_note_buttons();
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

    // // Example : 4 buttons
    // // Build one button for every item of pattern
    // // Beat item have a yellow color
    // for( unsigned int i = 0; i < pattern->size(); ++i) {
    //   if ( i % pattern->_signature.subdivisions == 0) {
    //     ImGui::PushStyleColor(ImGuiCol_Border, yellow_color );
    //     ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f );
    //   }

    //   ImGui::Button( std::to_string(i+1).c_str() );

    //   if ( i % pattern->_signature.subdivisions == 0 ) {
    //     ImGui::PopStyleVar(1);
    //     ImGui::PopStyleColor(1);
    //   }

    //   if (i < (pattern->size()-1)) {
    //     ImGui::SameLine();
    //   }
    // }
    for( auto it = note_btns.begin(); it != note_btns.end(); it++ ) {
      ImGui::PushID( it->get_id() );
      it->draw();
      if (std::next(it) != note_btns.end()) {
        ImGui::SameLine();
      }
      ImGui::PopID();
    }


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
      _update_note_buttons();
    }
    
    if (should_dump) {
      std::cout << pattern->_signature << std::endl;
      std::cout << "Notes ";
      for( auto& n: note_btns) {
        std::cout << std::to_string(n.get_val());
      }
      std::cout << std::endl;
    }
    should_dump = false;
    should_apply = false;
  }
  void _update_note_buttons()
  {
    LOGPG( "_update need " << pattern->size() << ", have " << note_btns.size());
    // walk NoteButtons and change or add
    for( unsigned int id = 0; id < pattern->size(); ++id) {
      // compute label and color
      std::string label = ".";
      //DELstd::string label = std::to_string( id );
      ImVec4* col_ref = nullptr;
      // For 'beats' note, # of beat and yellow_border
      if (id % pattern->_signature.subdivisions == 0) {
        label = std::to_string( (id / pattern->_signature.subdivisions) + 1 );
        col_ref = &yellow_color;
      }
      // need to add ?
      if (id >= note_btns.size()) {
        LOGPG( "create btn [" << label << "]" );
        auto newbtn = NoteButton( id, label, col_ref );
        note_btns.push_back( newbtn );
      }
      else {
        LOGPG( "change btn id=" << id << " to [" << label << "]" );        
        note_btns[id].set_label(label);
        note_btns[id].set_color(col_ref);
      }
    }
    // remove unneeded NoteButtons
    while (note_btns.size() > pattern->size()) {
      note_btns.pop_back();
    }
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
  std::vector<NoteButton> note_btns;

  // Usefull
  ImVec4 green_color;
  ImVec4 yellow_color;
};


#endif // PATTERN_GUI_HPP
