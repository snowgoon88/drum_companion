/* -*- coding: utf-8 -*- */

#ifndef PATTERN_GUI_HPP
#define PATTERN_GUI_HPP

/** 
 * PatternGui( *PatternAudio ) display the current pattern
 * pattern can be changed with APPLY
 * resynchronize with pattern using RESET
 * 
 * TODO better compute when the set of NoteButtons must be on SameLine
 */

#include <utils.hpp>
#include <pattern_audio.hpp>
#include <iostream>     // std::cout
#include "imgui.h"
#include <algorithm>    // std::max, std::transform
#include <iterator>     // std::back_inserter, 
#include <string>       // std::stoi

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
//#define LOG_PATTERNGUI
#ifdef LOG_PATTERNGUI
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
              const ImVec4 *border_color=nullptr ) :
    id(id),
    val(0),
    label(label),
    bordercol(border_color),
    val_changed(false)
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
      val_changed = true;
    }
    ImGui::PopStyleColor(3);
    
    if (bordercol != nullptr) {
      ImGui::PopStyleVar(1);
      ImGui::PopStyleColor(1);
    }
  }
  // *************************************************** NoteButton::attributs
public:
  uint get_id() const { return id; }
  uint get_val() const { return val; }
  void set_val(uint val) { this->val = (val % max_val); }

  void set_label( const std::string& label )
  {
    this->label = label;
  }
  void set_color( const ImVec4* border_color )
  {
    this->bordercol = border_color;
  }
  
private:
  uint id;
  uint val;
  std::string label;
  const ImVec4* bordercol;
public:
  bool val_changed;
};
// ********************************************************** NoteButton - END


// ***************************************************************************
// **************************************************************** PatternGUI
// ***************************************************************************
/**
 * At creation, init node_btns from pattern
 * Running : update node_btns according to displayed beats and subdivisions
 * Apply => change underlying pattern
 * Reset => revert back to pattern values
 */
class PatternGUI
{
public:
  // **************************************************** PatternGUI::creation
  PatternGUI( std::shared_ptr<PatternAudio> pattern_audio ) :
    pattern( pattern_audio ),
    should_apply(false), should_reset(false),
    should_dump(false),
    val_changed(false), need_notes(false), 
    bpm_val(0), beat_val(0), subdiv_val(0)
  {
    _init_from_pattern();
  }
  virtual ~PatternGUI()
  {
    LOGPG( "__DESTROy PatternGUI for p" << pattern->_id );
  }
  // ************************************************** PatternGUI::str_header
  std::string str_header () const
  {
    std::stringstream header;
    header << "P" << pattern->_id << ": ";
    header << "[" << pattern->_signature.beats;
    header << "x" << pattern->_signature.subdivisions << "]";
    header << " " << pattern->_convert_from_timeline();
    // to be sure the ImGui will not be computed from label, gives an ID
    header << "###PAT" << pattern->_id;

    return header.str();
  }

  // ******************************************************** PatternGUI::draw
  void draw( bool ask_bpm=true )
  {
    static bool show_open = true;
    if (ImGui::CollapsingHeader( str_header().c_str(),
                                 // &show_open, could display closing cross
                                 ImGuiTreeNodeFlags_None)) {

      // To ensure unique ID for ImGui
      std::string pat_id = "P"+std::to_string( pattern->_id);
      ImGui::PushID( pat_id.c_str() );

      if (ask_bpm) {
        // read info from PatternAudio
        bpm_val = pattern->get_bpm();
        // BPM, Signature
        ImGui::InputInt( "BPM: ", &bpm_val );
      }
      ImGui::InputInt( "Nb Beats: ", &beat_val );
      ImGui::InputInt( "Nb SubDiv: ", &subdiv_val );

      // draw Buttons
      for( auto it = note_btns.begin(); it != note_btns.end(); it++ ) {
        ImGui::PushID( it->get_id() );
        it->draw();
        if (std::next(it) != note_btns.end()) {
          ImGui::SameLine();
        }
        ImGui::PopID();
      }

      // Button RESET (+DUMP)
      if (! val_changed) {
        ImGui::BeginDisabled(true);
      }

      if (ImGui::Button( "Reset Change")) {
        should_dump = true;
        should_reset = true;
      }

      // Button APPLY (+DUMP)
      ImGui::SameLine();
      if (val_changed) {
        ImGui::PushStyleColor(ImGuiCol_Button, RED_COL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, YELLOW_COL);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, RED_COL);
      }
      if (ImGui::Button( "Apply")) {
        should_dump = true;
        should_apply = true;
      }
      if (val_changed) {
        ImGui::PopStyleColor(3);
      }
      if (! val_changed) {
        ImGui::EndDisabled();
      }

      ImGui::PopID();
    }
  }

  /** Apply the changes asked during draw */
  void apply()
  {
    bpm_val = std::max(1, bpm_val);
    beat_val = std::max(1, beat_val);
    subdiv_val = std::max(1, subdiv_val);
    LOGPG( "__APPLY bpm=" << bpm_val << ", beats=" << beat_val << ", sub=" << subdiv_val ); 
    
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
      else { // check in btns
        for( auto& b: note_btns) {
          if (b.val_changed) {
            val_changed = true;
            break;
          }
        }
      }
    }
    need_notes = ((beat_val * subdiv_val) != note_btns.size());
    LOGPG( "  need_notes=" << need_notes );
    
    if (should_apply && !need_notes) {
      // updade pattern signature
      // TODO msg to tell it failed
      pattern->_signature.bpm = static_cast<uint>(bpm_val);
      pattern->_signature.beats = static_cast<uint>(beat_val);
      pattern->_signature.subdivisions = static_cast<uint>(subdiv_val);
      // update pattern intervales
      Timeline vals;
      // create a Timeline for the note_btns
      std::transform( note_btns.begin(), note_btns.end(),
                      std::back_inserter( vals ),
                      [](const NoteButton& b) -> uint {
                        return b.get_val(); });
      pattern->init_from_timeline( vals );
      val_changed = false;
      for( auto& b: note_btns) {
        b.val_changed = false;
      }
    }
    if (need_notes) {
      _update_note_buttons( beat_val * subdiv_val, subdiv_val);
    }

    if (should_reset) {
      _init_from_pattern();
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
    should_reset = false;
  }
  void _update_note_buttons( uint size, uint subdiv )
  {
    LOGPG( "_update need " << size << ", have " << note_btns.size());
    // walk NoteButtons and change or add
    for( unsigned int id = 0; id < size; ++id) {
      // compute label and color
      std::string label = ".";
      const ImVec4* col_ref = nullptr;
      // For 'beats' note, # of beat and yellow_border
      if (id % subdiv == 0) {
        label = std::to_string( (id / subdiv) + 1 );
        col_ref = &YELLOW_COL;
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
    while (note_btns.size() > size) {
      note_btns.pop_back();
    }

    need_notes = false;
  }
  void _init_from_pattern()
  {
    LOGPG( "_init_from_pattern " << pattern->size());
    bpm_val = static_cast<int>(pattern->_signature.bpm);
    beat_val = static_cast<int>(pattern->_signature.beats);
    subdiv_val = static_cast<int>(pattern->_signature.subdivisions);


    note_btns.clear();
    // add 1 button for each item in pattern->_timeline
    uint id = 0;
    for( auto& valnote: pattern->_timeline) {
      // compute label and color
      std::string label = ".";
      const ImVec4* col_ref = nullptr;
      // For 'beats' note, # of beat and yellow_border
      if (id % pattern->_signature.subdivisions == 0) {
        label = std::to_string( (id / pattern->_signature.subdivisions) + 1 );
        col_ref = &YELLOW_COL;
      }
      
      LOGPG( "create btn " << id << " with [" << label << "]" );
      auto newbtn = NoteButton( id, label, col_ref );
      newbtn.set_val( valnote );
      note_btns.push_back( newbtn );

      id += 1;
    }
    need_notes = false;
    val_changed = false;
    for( auto& b: note_btns) {
      b.val_changed = false;
    }
  }

  // *************************************************** PatternGUI::attributs
  std::shared_ptr<PatternAudio> pattern;

  // State
  bool should_apply; // some changes need to be applied ?
  bool should_reset; // synchronize with current pattern
  bool should_dump;
  bool val_changed;
  bool need_notes;
  int bpm_val;
  int beat_val;
  int subdiv_val;
  std::vector<NoteButton> note_btns;

};


#endif // PATTERN_GUI_HPP
