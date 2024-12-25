/* -*- coding: utf-8 -*- */

#ifndef BPM_WIDGET_HPP
#define BPM_WIDGET_HPP

/** 
 * BPM with large LED dispay,
 * can be changed by button, click (InputInt) and keyboard +/- (+CTRL)
 */

#include <string>
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
    int effect;
    int nb_repeat;
  };
  using KeyMem = key_mem_S;
  KeyMem keys[2] = { {ImGuiKey_KeypadSubtract, -1, 0},
                     {ImGuiKey_KeypadAdd, +1, 0} };
  float repeat_duration = 0.1f;

public:
  // ***************************************************** BPMWidget::creation
  BPMWidget()
  {
  }
  virtual ~BPMWidget() {}

  // ********************************************************* BPMWidget::draw
  void draw( unsigned int bpm, bool verb=false)
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

    // Pre-defined tempo buttton
    ImGui::SameLine(0, 30); // spacing
    ImGui::BeginGroup();
    static int tempo[] = {60, 72, 84, 96, 110, 120};
    for( auto t: tempo) {
      if (ImGui::Button( std::to_string(t).c_str() )) {
        _bpm = t;
      }
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

        // when key has just been pressed, make sure added is a least 1
        int added_repeat = (key.nb_repeat==0) ? 1 : 0;
        added_repeat += static_cast<int>(trunc((new_duration - repeat_duration * key.nb_repeat) / repeat_duration));
        // Ctrl multiply speed
        int mult_effect = 1;
        if (ImGui::GetIO().KeyCtrl) {//ImGui::ImGuiKey_ModCtrl()) {
           mult_effect = 10;
        }

        if (verb) {
          ImGui::Text( "Down for %f s, rep=%d, nb=%d, mul=%d",
                       new_duration, key.nb_repeat, added_repeat, mult_effect );
        }
        _bpm += key.effect * added_repeat * mult_effect;

        key.nb_repeat += added_repeat;
      }
      else if (ImGui::IsKeyReleased( key.id )) {
        key.nb_repeat = 0;
      }
    }
    
  }
  // **************************************************** BPMWidget::attributs
public:
    unsigned int get_new_bpm() { return _bpm; }
    void set_new_bpm( unsigned int bpm ) { _bpm = bpm; }
private:
    int _bpm;


  // Widget to draw large LED display
  LedDisplay led_display;
  
}; // BPMWidget

#endif // BPM_WIDGET_HPP
