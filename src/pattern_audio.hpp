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
  PatternAudio() :
    _start_time(std::chrono::system_clock::now())
  {
  }
  virtual ~PatternAudio()
  {
  }
  // ******************************************************* PatternAudio::str
  std::string str_dump () const
  {
    std::stringstream dump;
    dump << "Last = ";
    std::chrono::duration<int, std::milli> delta_time_start = std::chrono::duration_cast<std::chrono::milliseconds>( _last_update - _start_time );
    //auto as_time = std::chrono::system_clock::to_time_t(_last_update);
    //auto millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(_last_update.time_since_epoch()).count();
    //dump << std::ctime(&as_time) << " (" << millisec_since_epoch << ")";
    dump << delta_time_start.count();
    dump << " Next = " << _time_to_next.count();

    dump << " id_seq = " << _id_seq;

    return dump.str();
  }
  std::string str_time () const
  {
    std::stringstream time;
    time << "txt";

    return time.str();
  }
  // ****************************************************** PatternAudio::emit
  void start()
  {
    _id_seq = 0;
    _start_time = std::chrono::system_clock::now();
    _time_to_next = std::chrono::milliseconds(_pattern_intervale[_id_seq]);
    _last_update = std::chrono::system_clock::now();
    std::cout << "Start at " << str_dump() << std::endl;
  }
  void update()
  {
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<int, std::milli> delta_time = std::chrono::duration_cast<std::chrono::milliseconds>( time_now - _last_update );
    _time_to_next -= delta_time;
    std::cout << str_dump() << std::endl;
    
    if (_time_to_next < std::chrono::duration<int, std::milli>(5)) {
      // emit event
      std::cout << "====> *** Emit at " << str_dump() << " ******" << std::endl;
      _id_seq = (_id_seq + 1) % _pattern_intervale.size();
      _time_to_next = std::chrono::milliseconds(_pattern_intervale[_id_seq]);
    }
    _last_update = time_now;
  }
  // ************************************************* PatternAudio::attributs
  // sequence of delay in ms
  std::vector<int> _pattern_intervale = { 500, 250, 500, 500, 250 };
  uint _id_seq;
  std::chrono::duration<int, std::milli> _time_to_next;
  std::chrono::time_point<std::chrono::system_clock> _last_update;
  std::chrono::time_point<std::chrono::system_clock> _start_time;
};
#endif // PATTERN_AUDIO_HPP
