/* -*- coding: utf-8 -*- */

#ifndef BPM_WIDGET_HPP
#define BPM_WIDGET_HPP

/** 
 * BPM with large LED dispay,
 * can be changed by button, click (InputInt) and keyboard +/- (+CTRL)
 */

#include <led_display_widget.hpp>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
#define LOG_BPMW
#ifdef LOG_BPMW
#  define LOGBPMW(msg) (LOG_BASE("[BPMW]", msg))
#else
#  define LOGBPMW(msg)
#endif

// ***************************************************************************
// ***************************************************************** BPMWidget
// ***************************************************************************
class BPMWidget
{
  struct key_mem_S {
    ImGuiKey id;
    float duration;
    int effect;
  };
  using KeyMem = key_mem_S;
  KeyMem keys[2] = { {ImGuiKey_KeypadSubtract, 0.f, -1},
                     {ImGuiKey_KeypadAdd, 0.f, +1} };

  bool verb = true;
  
public:
  // ***************************************************** BPMWidget::creation
  BPMWidget()
  {
  }
  virtual ~BPMWidget() {}

  // ********************************************************* BPMWidget::draw
  void draw( unsigned int bpm )
  {
    _bpm = bpm;
    ImVec2 size(320.0f, 180.0f);

    // button with Popup
    if (ImGui::InvisibleButton("bpm_canvas", size)) {
      ImGui::OpenPopup( "bpm_popup" );
    }
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(p0, p1);

    led_display.draw( draw_list, _bpm,
                       p0, p1, size );
    draw_list->PopClipRect();

    if (ImGui::BeginPopup("bpm_popup")) {
      ImGui::Text( "Enter BPM" );
      ImGui::Separator();
      ImGui::InputInt( "##BPM", &_bpm );
      ImGui::EndPopup();
    }

    // Buttons
    ImGui::SameLine();
    ImGui::BeginGroup();
    if (ImGui::Button( "+ 1" )) {
      _bpm += 1;
    }
    if (ImGui::Button( "- 1" )) {
      _bpm -= 1;
    }
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    if (ImGui::Button( "+10" )) {
      _bpm += 10;
    }
    if (ImGui::Button( "-10" )) {
      _bpm -= 10;
    }
    ImGui::EndGroup();

    // Check status and effect of keys
    for( auto& key: keys) {
      if (verb) {
        ImGui::Text( "Checking key %s (%d)", ImGui::GetKeyName( key.id ),
                     key.id);
      }
      if (ImGui::IsKeyDown( key.id )) {
        float new_duration = ImGui::GetKeyData( key.id )->DownDuration;
        int nb_repeat = 0;
        if (verb) {
          ImGui::Text( "Down for %f s, old=%f", new_duration, key.duration );
        }
        
            float delta = new_duration - key.duration;
        if (delta > 0.1) {
          nb_repeat = static_cast<int>( trunc(delta / 0.1));
          if (ImGui::GetIO().KeyCtrl) {//ImGui::ImGuiKey_ModCtrl()) {
            nb_repeat *= 10;
          }
          _bpm += key.effect * nb_repeat;
          
          key.duration = new_duration;
        }
      }
      else if (ImGui::IsKeyReleased( key.id )) {
        key.duration = 0.f;
      }
    }
    
  }
  // **************************************************** BPMWidget::attributs
public:
  unsigned int get_new_bpm() { return _bpm; }
private:
  int _bpm;


  // Widget to draw large LED display
  LedDisplay led_display;
  
}; // BPMWidget

#endif // BPM_WIDGET_HPP
