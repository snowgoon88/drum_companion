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
    dump << "Last = ";
    DurationMS delta_time_start = std::chrono::duration_cast<DurationMS>( _last_update - _start_time );
    dump << delta_time_start.count();
    dump << " Next = " << _time_to_next.count();

    dump << " id_seq = " << _id_seq;

    dump << " " << str_status();
                          
    return dump.str();
  }
  // ****************************************************** PatternAudio::emit
  void start()
  {
    if (_state == ready) {
      _id_seq = 0;
      _start_time = std::chrono::system_clock::now();
      _time_to_next = std::chrono::milliseconds(_pattern_intervale[_id_seq]);
      _last_update = std::chrono::system_clock::now();
      std::cout << "Start at " << str_dump() << std::endl;
      _state = running;
    }
    else if(_state == paused) {
      _state = running;
    }
  }
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
        _time_to_next = std::chrono::milliseconds(_pattern_intervale[_id_seq]);
      }
      _last_update = time_now;
    }
    else if (_state == paused) {
      _last_update = std::chrono::system_clock::now();
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
  AudioState _state;
  // sequence of delay in ms
  std::vector<int> _pattern_intervale = { 500, 250, 500, 500, 250 };
  uint _id_seq;
  DurationMS _time_to_next;
  Time _last_update;
  Time _start_time;
};
#endif // PATTERN_AUDIO_HPP
