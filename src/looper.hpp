/* -*- coding: utf-8 -*- */

#ifndef LOOPER_HPP
#define LOOPER_HPP

/** 
 * Chain PatternAudio till the end of time...
 */

#include <utils.hpp>
#include <pattern_audio.hpp>
#include <sound_engine.hpp>
#include <list>
#include <vector>
#include <string>
#include <cctype>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
#define LOG_LO
#ifdef LOG_LO
#  define LOGLO(msg) (LOG_BASE("[Loop]", msg))
#else
#  define LOGLO(msg)
#endif

// ***************************************************************************

// ***************************************************************************
// ******************************************************************** Looper
// ***************************************************************************
class Looper
{
public:
  using PatternList = std::list<PatternAudio*>;
private:
  using PatternVec = std::vector<PatternAudio*>;
  enum LooperState { ready, running, paused, empty };
  
public:
  // ******************************************************** Looper::creation
  Looper( SoundEngine* engine = nullptr ) :
    _engine(engine),
    _state(empty)
  {
  }
  virtual ~Looper()
  {
  }
  // ************************************************************* Looper::str
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
    case empty:
      status << "empty";
      break;
    }
    return status.str();
  }
  std::string str_dump () const
  {
    std::stringstream dump;
    dump << "Looper: " << str_status() << std::endl;
    dump << "  sequence.size=" << sequence.size();

    dump << " : [";
    for( auto& pat: sequence) {
      dump << pat->_id <<", ";
    }
    dump << "]";

    return dump.str();
  }
  // ******************************************************** Looper::patterns
  uint add( PatternAudio* pattern )
  {
    if (pattern->_state == PatternAudio::ready ) {
      pattern->_id = all_patterns.size();
      all_patterns.push_back( pattern );
      return pattern->_id;
    }
    throw std::runtime_error( "add: pattern is not ready" );
    return 0;
  }
  // ******************************************************** Looper::sequence
  // ****************************************************** Looper::operations
  void clear_sequence()
  {
    sequence.clear();
  }
  void concat( uint id_pattern )
  {
    if (id_pattern < all_patterns.size()) {
      sequence.push_back( all_patterns[id_pattern] );
    }
    else {
      throw std::runtime_error( "concat: id_pattern not valid" );
    }
    if (sequence.size() > 0) {
      _state = ready;
    }
  }
  /** Check Valid Pattern Id and create a PatterList with it */
  PatternList into_list( const uint id_pattern )
  {
    if (id_pattern >= all_patterns.size()) {
      throw std::runtime_error( "into_list: id_pattern="+std::to_string(id_pattern)+" not valid as size="+std::to_string(all_patterns.size()) );
    }
    PatternList res{ all_patterns[id_pattern] };
    return res;
  }
  // ************************************************************ Looper::next
  bool next()
  {
    if (_state == running) {
      // try to advance current pattern
      bool advanced = (*_its)->next();
      // if not, ask next in sequence
      if (! advanced) {
        LOGLO( " next: advance to next Pattern" );
        _its++;
        if (_its == sequence.end()) {
          LOGLO( " next: it was the last pattern, begin again" );
          _its = sequence.begin();
        }
        (*_its)->start();
      }
    }
    return true;
  }
  // ************************************************************* Looper::cmd
  // TODO pause with state
  void start()
  {
    if (_state == ready && sequence.size() > 0) {
      LOGLO( "START" );
      _its = sequence.begin();
      (*_its)->start();
      _state = running;
    }
  }
  void stop()
  {
    if (_state != empty) {
      (*_its)->stop();
      _state = ready;
    }
  }
  // ******************************************************* Looper::attributs
  SoundEngine *_engine;
  PatternList sequence;
  PatternVec  all_patterns;
  PatternList::iterator _its;
  LooperState _state;
  std::string _formula;
};
// ************************************************************** Looper - End

//DEL std::ostream &operator<<(std::ostream &os, const Looper::PatternList &pl)
// {
//   os << "PatList: {";
//   for( auto& pat: pl) {
//     os << pat->_id << ", ";
//   }
//   os << "}";
//   return os;
// }


#endif // LOOPER_HPP
