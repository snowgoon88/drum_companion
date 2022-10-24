/* -*- coding: utf-8 -*- */

#ifndef LOOPER_GUI_HPP
#define LOOPER_GUI_HPP

/** 
 * LooperGui( *Looper ) display the Looper, as a text window
 * BPM: can be imposed (or not) on every pattern
 * APPLY : apply the changes
 */

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
    should_apply(false)
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
      
      if (ImGui::Button( "Apply")) {
        should_apply = true;
      }
    }
  }
  // ****************************************************** LooperGUI::private
  void _init_from_looper()
  {
    LOGLG( "_init_from_looper " );
  }
  // **************************************************** LooperGUI::attributs
  Looper* looper;

  // State
  bool should_apply;   // somes changes need to be applied
  
}; // class LooperGUI


#endif // LOOPER_GUI_HPP
