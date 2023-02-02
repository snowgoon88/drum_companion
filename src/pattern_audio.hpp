/* -*- coding: utf-8 -*- */

#ifndef PATTERN_AUDIO_HPP
#define PATTERN_AUDIO_HPP

/** 
 * Class that emit "audio play event" according to a sequence of intervals.
 */

#include <chrono>
#include <thread>
#include <vector>
#include <cctype>    // isdigit
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
#include <io_files.hpp>
#include <sound_engine.hpp>


// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
#define LOG_PA
#ifdef LOG_PA
#  define LOGPA(msg) (LOG_BASE("[PaAu]", msg))
#else
#  define LOGPA(msg)
#endif

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

  // ************************************************** Signature::from_string
  // str = "4x1"
  void from_string( const std::string& str)
  {
    LOGPA( "__Signature BxD from " << str );
    LOGPA( "  with " << *this );
    auto pos_beat = str.find( "x" );
    assert( pos_beat != std::string::npos );

    beats = std::stoi( str.substr(0, pos_beat ));
    subdivisions = std::stoi( str.substr( pos_beat+1, str.size() ));
    
  }
  // ************************************************************Signature::str
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

  // *************************************************************** Note::str
  friend std::ostream& operator<<( std::ostream& os, const Note& n)
  {
    os << "N:( " << n.val << ", " << n.length << ")";
    return os;
  }
};
// **************************************************************** Note - END


// ***************************************************************************
// ************************************************************** PatternAudio
// ***************************************************************************
// TODO: check that _pattern_audio[idx].val is valid (and same as idx
//       from SoundEngine)
class PatternAudio
{
public:
  using Time = std::chrono::time_point<std::chrono::system_clock>;
  using DurationMS = std::chrono::milliseconds;
  enum AudioState { ready, running, paused, ended, empty };
  
public:
  PatternAudio( SoundEngine* engine = nullptr ) :
    _engine(engine),
    _state(empty), _id_beat(0),
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
    case ended:
      status << "ended";
      break;
    case empty:
      status << "empty";
      break;
    }
    return status.str();
  }

  std::string str_dump () const
  {
    std::stringstream dump;
    dump << "PAT [" << _id << "] " << _signature << std::endl;
    dump << "Last = ";
    DurationMS delta_time_start = std::chrono::duration_cast<DurationMS>( _last_update - _start_time );
    dump << delta_time_start.count();
    dump << " Next = " << _time_to_next.count();
    dump << " id_seq = " << _id_seq;
    dump << " Next Beat = " << _time_to_beat.count();
    dump << " id Beat = " << _id_beat;
    dump << std::endl;
    
    dump << " " << str_status();
                          
    return dump.str();
  }
  std::string str_verbose () const
  {
    std::stringstream verbose;
    verbose << _convert_from_timeline();
    verbose << " as " << _signature.beats << "x" << _signature.subdivisions;
    verbose << " at " << _signature.bpm << " BPM." << std::endl;

    return verbose.str();
  }

  // ******************************************************** PatternAudio::io
  void write_to( std::ofstream& os )
  {
    write_token( os, "bpm", _signature.bpm );
    std::string sig_str = std::to_string(_signature.beats)+"x"+std::to_string(_signature.subdivisions); 
    write_token( os, "sig", sig_str );
    write_token( os, "pat", _convert_from_timeline() );
  }
  void read_from( std::ifstream& is )
  {
    auto bpm = read_uint( is, "bpm" );
    auto sig_str = read_string( is, "sig" );
    _signature.bpm = bpm;
    _signature.from_string( sig_str );

    auto pat_str = read_string( is, "pat" );
    init_from_string( pat_str );
  }
  // *************************************************** PatternAudio::set_bpm
  void set_bpm( uint bpm )
  {
    _signature.bpm = bpm;
    _intervale_from_timeline();
  }
  // ****************************************************** PatternAudio::init
  /** from timeline (array of sound {0,1,2} for each subdivisions of Signature)
   *  to Vec of intervals
   */
  void init_from_timeline( Timeline &timeline )
  {
    // ensure pattern is stopped
    stop();
    // check proper size
    assert( timeline.size() == _signature.beats * _signature.subdivisions );
    // store timeline bu copying
    _timeline = Timeline(timeline);

    _intervale_from_timeline();
  }
  //   auto itt = timeline.begin();    

  //   _pattern_intervale.clear();
    
  //   // Count nb of '0' between each non '0' in timeline
  //   // First Note
  //   uint val_note = *itt;
  //   uint count = 1;
  //   while (++itt != timeline.end() ) {
  //     if (*itt == 0) {
  //       count += 1;
  //     }
  //     else {
  //       _pattern_intervale.push_back(
  //           Note{val_note, count * _signature.division_length()});
  //       val_note = *itt;
  //       count = 1;
  //     }
  //   }
  //   _pattern_intervale.push_back(
  //       Note{val_note, count * _signature.division_length()});
  // }
  void _intervale_from_timeline()
  {
    // check proper size
    assert( _timeline.size() == _signature.beats * _signature.subdivisions );
    auto itt = _timeline.begin();    

    _pattern_intervale.clear();
    
    // Count nb of '0' between each non '0' in timeline
    // First Note
    uint val_note = *itt;
    uint count = 1;
    while (++itt != _timeline.end() ) {
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

    _state = ready;
    _id_beat = 0;
  }

  /** 
   * str  ="0x1x1xxx1xx1xx1x"
   */
  void init_from_string( const std::string &str )
  {
    LOGPA( str_dump() );
    LOGPA( "__PA init_from " << str );
    Timeline tl;

    for( const char& c: str) {
      if (c=='x') {
        tl.push_back( 0 );
      }
      else if (std::isdigit(c)) {
        tl.push_back( (int)(c - '0'));
      }
    }
    LOGPA( " TL=" << tl );

    init_from_timeline( tl );
  }
  std::string _convert_from_timeline() const
  {
    std::string target = "x12";
    std::stringstream dump;
    for( auto& val: _timeline) {
      dump << target[val];
    }
    return dump.str();
  }
  // ****************************************************** PatternAudio::next
  bool next()
  {
    if (_state == running) {
      auto time_now = std::chrono::system_clock::now();
      DurationMS delta_time = std::chrono::duration_cast<DurationMS>( time_now - _last_update );
      _time_to_next -= delta_time;
      _time_to_beat -= delta_time;
      //LOGPA( str_dump() );

      // Check for next Sound Event
      if (_time_to_next < DurationMS(5)) {
        
        // TODO return time to next or <0 or bool or None or ...
        //_id_seq = (_id_seq + 1) % _pattern_intervale.size();
        _id_seq += 1;
        if (_id_seq == _pattern_intervale.size()) {
          LOGPA( ">> ended at " << str_dump() << " ***********" ); 
          _state = ready;
          return false;
        }
        if (_pattern_intervale[_id_seq].val != 0 && _engine != nullptr ) {
          // emit event
          LOGPA( "====> *** Emit at " << str_dump() << " ******" );
          _engine->play_sound( _pattern_intervale[_id_seq].val - 1 );
        }
        _time_to_next = std::chrono::milliseconds(_pattern_intervale[_id_seq].length);
      }

      // Check for next Beat
      if (_time_to_beat < DurationMS(5)) {
        _id_beat += 1;
        _time_to_beat = std::chrono::milliseconds(beat_duration());
      }
      _last_update = time_now;
    }
    
    else if (_state == ended) {
     // can and must be restarted
     start();
    }
    else if (_state == paused) {
      _last_update = std::chrono::system_clock::now();
    }
    return true;
  }

  // ******************************************************* PatternAudio::cmd
  void start()
  {
    if (_state == ready) {
      _id_beat = 0;
      _time_to_beat = std::chrono::milliseconds( beat_duration() );
      
      _id_seq = 0;
      _start_time = std::chrono::system_clock::now();
      _time_to_next = std::chrono::milliseconds(_pattern_intervale[_id_seq].length);
      _last_update = std::chrono::system_clock::now();
      LOGPA( "Start at " << str_dump() );
      if (_pattern_intervale[_id_seq].val != 0 && _engine != nullptr ) {
        _engine->play_sound( _pattern_intervale[_id_seq].val - 1 );
      }
      _state = running;
    }
    else if(_state == paused) {
      LOGPA( "UNPAUSE" );
      LOGPA( str_dump() );
      _state = running;
    }
  }
  void pause()
  {
    if (_state == running) {
      _state = paused;
    }
    LOGPA( "PAUSE" );
    LOGPA( str_dump() );
  }
  void stop()
  {
    if (_state != empty) {
      _state = ready;
    }
  }
  // ************************************************ PatternAudio::properties
  uint size()
  {
    return _signature.beats * _signature.subdivisions;
  }
  /** duration of a beat in ms, but as uint.
   * use std::chrono::milliseconds( beat_duration() );
   */
  uint beat_duration()
  {
    return _signature.division_length() * _signature.subdivisions;
  }
  float beat_proportion()
  {
    return static_cast<float>( _time_to_beat.count() ) / static_cast<float>( beat_duration() );
  }
  // ************************************************* PatternAudio::attributs
  Signature _signature;
  SoundEngine *_engine;
  AudioState _state;
  Timeline _timeline;   // sequence of 0,1,2...
  // sequence of (idx_sound x delay in ms)
  std::vector<Note> _pattern_intervale = {{1, 500}, {1, 250},
                                          {1, 500}, {1, 500}, {1,250}};

  uint _id_beat;
  DurationMS _time_to_beat;

  uint _id_seq;
  DurationMS _time_to_next;
  Time _last_update;
  Time _start_time;

  uint _id;            // id of this Pattern
};
// ******************************************************** PatternAudio - END

#endif // PATTERN_AUDIO_HPP
