/* -*- coding: utf-8 -*- */

#ifndef PATTERN_AUDIO_HPP
#define PATTERN_AUDIO_HPP

/** 
 * Class that emit "audio play event" according to a sequence of intervals.
 */

#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include <sstream>
#include <math.h>   // round
#include <cmath>
#include <string>

// uncomment to disable assert()
// #define NDEBUG
#include <cassert>
// Use (void) to silence unused warnings.
#define assertm(exp, msg) assert(((void)msg, exp))

#include <utils.hpp>

// ***************************************************************************
// ****************************************************************** Timeline
// ***************************************************************************
using Timeline = std::vector<uint>;

// ***************************************************************************
// ***************************************************************** Signature
// ***************************************************************************
struct Signature
{
  uint bpm           = 120;   // beat per min
  uint beats         = 4;     // nb of beat in a pattern
  uint subdivisions  = 1;     // nb of sub division in a beat

  Signature() {};
  Signature(uint bpm, uint beats, uint subdivisions)
      : bpm(bpm), beats(beats), subdivisions(subdivisions) {}

  // Signature& operator=(const Signature& c) // copie
// { 
//   if(this != &c) { // protect against invalid self-assignment
//     bpm      = c.bpm;
//     beats      = c.beats;
//     subdivisions = c.subdivisions;
//   }
//   return *this;
// };

  // ************************************************** Signature::from_string
  // str = "90:4x1"
  void from_string( const std::string& str)
  {
    // extract bpm
    auto pos_bpm = str.find( ":" );
    assert( pos_bpm != std::string::npos );

    bpm = std::stoi( str.substr(0, pos_bpm ));

    auto pos_beat = str.find( "x" );
    assert( pos_bpm != std::string::npos );

    beats = std::stoi( str.substr(pos_bpm+1, pos_beat ));
    subdivisions = std::stoi( str.substr( pos_beat+1, str.size() ));
    
  }
  // ************************************************************Signature::str
  //DEL std::string str_dump () const
  // {
  //   std::stringstream dump;
  //   dump << "Signature: " << bpm << " bpm ";
  //   dump << beats << "x" << subdivisions << " beats";

  //   return dump.str();
  // }
  friend std::ostream &operator<<(std::ostream &os, const Signature &s)
  {
    os << "Signature: " << s.bpm << " bpm ";
    os << s.beats << "x" << s.subdivisions << " beats";
      return os;
  };
  // *********************************************** Signature::division_length
  // duration in ms of a subdivisions
  uint division_length() {
    double min_in_ms = 1000.0 * 60.0;
    double length = min_in_ms / static_cast<double>(bpm)
      / static_cast<double>(subdivisions);
    return static_cast<uint>( round( length ));
  }
};
// *********************************************************** Signature - END


// ***************************************************************************
// ********************************************************************** Note
// ***************************************************************************
struct Note
{
  uint val              = 1;
  uint length           = 10;

  Note() {};
  Note(uint val, uint length) : val(val), length(length) {};

  //DEL std::string str_dump () const
  // {
  //   std::stringstream dump;
  //   dump << "N:( " << val << ", " << length << ")";

  //   return dump.str();
  // }
  friend std::ostream& operator<<( std::ostream& os, const Note& n)
  {
    os << "N:( " << n.val << ", " << n.length << ")";
    return os;
  }
};



// ***************************************************************************
// ************************************************************** PatternAudio
// ***************************************************************************
class PatternAudio
{
public:
  using Time = std::chrono::time_point<std::chrono::system_clock>;
  using DurationMS = std::chrono::milliseconds;
  enum AudioState { ready, running, paused };
  
public:
  PatternAudio() :
    _state(ready),
    _start_time(std::chrono::system_clock::now())
  {
  }
  virtual ~PatternAudio()
  {
  }
  // ******************************************************* PatternAudio::str
  std::string str_status () const
  {
    std::stringstream status;
    status << "Status: ";
    switch (_state) {
    case ready: 
      status << "ready";
      break;
    case running:
      status << "running";
      break;
    case paused:
      status << "paused";
      break;
    }
    return status.str();
  }

  std::string str_dump () const
  {
    std::stringstream dump;
    dump << _signature << std::endl;
    dump << "Last = ";
    DurationMS delta_time_start = std::chrono::duration_cast<DurationMS>( _last_update - _start_time );
    dump << delta_time_start.count();
    dump << " Next = " << _time_to_next.count();

    dump << " id_seq = " << _id_seq;

    dump << " " << str_status();
                          
    return dump.str();
  }


  // ****************************************************** PatternAudio::init
  /** from timeline (array of sound for each subdivisions of Signature)
   *  to Vec of intervals
   */
  void init_from_timeline( std::vector<uint> &timeline )
  {
    // check proper size
    assert( timeline.size() == _signature.beats * _signature.subdivisions );

    auto itt = timeline.begin();    

    _pattern_intervale.clear();
    
    // Count nb of '0' between each non '0' in timeline
    // First Note
    uint val_note = *itt;
    uint count = 1;
    while (++itt != timeline.end() ) {
      if (*itt == 0) {
        count += 1;
      }
      else {
        _pattern_intervale.push_back(
            Note{val_note, count * _signature.division_length()});
        val_note = *itt;
        count = 1;
      }
    }
    _pattern_intervale.push_back(
        Note{val_note, count * _signature.division_length()});
  }

  /** 
   * str  ="0x1x1xxx1xx1xx1x"
   */
  void init_from_string( std::string &str )
  {
    Timeline tl;

    for( char& c: str) {
      if (c=='x') {
        tl.push_back( 0 );
      }
      else if (std::isdigit(c)) {
        tl.push_back( (int)(c - '0'));
      }
    }
    std::cout << " TL=" << tl << std::endl;

    init_from_timeline( tl );
  }

 
  // ****************************************************** PatternAudio::emit
  void update()
  {
    if (_state == running) {
      auto time_now = std::chrono::system_clock::now();
      DurationMS delta_time = std::chrono::duration_cast<DurationMS>( time_now - _last_update );
      _time_to_next -= delta_time;
      std::cout << str_dump() << std::endl;
    
      if (_time_to_next < DurationMS(5)) {
        // emit event
        std::cout << "====> *** Emit at " << str_dump() << " ******" << std::endl;
        _id_seq = (_id_seq + 1) % _pattern_intervale.size();
        _time_to_next = std::chrono::milliseconds(_pattern_intervale[_id_seq].length);
      }
      _last_update = time_now;
    }
    else if (_state == paused) {
      _last_update = std::chrono::system_clock::now();
    }
  }

  // ******************************************************* PatternAudio::cmd
  void start()
  {
    if (_state == ready) {
      _id_seq = 0;
      _start_time = std::chrono::system_clock::now();
      _time_to_next = std::chrono::milliseconds(_pattern_intervale[_id_seq].length);
      _last_update = std::chrono::system_clock::now();
      std::cout << "Start at " << str_dump() << std::endl;
      _state = running;
    }
    else if(_state == paused) {
      _state = running;
    }
  }
  void pause()
  {
    if (_state == running) {
      _state = paused;
    }
  }
  void stop()
  {
    _state = ready;
  }
  // ************************************************* PatternAudio::attributs
  Signature _signature;
  AudioState _state;
  // sequence of delay in ms
  std::vector<Note> _pattern_intervale = {{1, 500}, {1, 250},
                                          {1, 500}, {1, 500}, {1,250}};
  uint _id_seq;
  DurationMS _time_to_next;
  Time _last_update;
  Time _start_time;
};
#endif // PATTERN_AUDIO_HPP
