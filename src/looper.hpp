/* -*- coding: utf-8 -*- */

#ifndef LOOPER_HPP
#define LOOPER_HPP

/** 
 * Chain PatternAudio till the end of time...
 */

#include <utils.hpp>
#include <io_files.hpp>
#include <pattern_audio.hpp>
#include <sound_engine.hpp>
#include <list>
#include <vector>
#include <string>
#include <cctype>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
//#define LOG_LO
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
  using UintIt = std::iterator<std::input_iterator_tag, unsigned int>;
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
  // ************************************************************** Looper::io
  void write_to( std::ofstream& os )
  {
    write_comment( os, "-- Looper --" );
    write_token( os, "nb_pat", all_patterns.size() );
    for( auto& pat: all_patterns) {
      write_comment( os, "pattern p"+std::to_string( pat->_id ) );
      pat->write_to( os );
    }

    write_comment( os, "sequence" );
    write_token( os, "nb_seq", sequence.size() );
    for( auto& pat: sequence) {
      write_token( os, "id", pat->_id );
    }
  }
  void read_from( std::ifstream& is )
  {
    // ensure stop
    stop();
    
    auto nb_pattern = read_uint( is, "nb_pat" );
    all_patterns.clear(); // TODO what if some undeleted patterns ?
    for( unsigned int idp = 0; idp < nb_pattern; ++idp) {
      PatternAudio *pat = new PatternAudio( _engine );
      pat->read_from( is );
      add( pat );
    }
    
    sequence.clear(); // TODO what if some undeleted patterns ?
    auto nb_seq = read_uint( is, "nb_seq" );
    for( unsigned int ids = 0; ids < nb_seq; ++ids) {
      auto id_pat = read_uint( is, "id" );
      concat( id_pat );
    }
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
  // void clear_sequence()
  // {
  //   sequence.clear();
  // }
  // void parse_string( const std::string& expression )
  // {
  //   Analyzer analyzer(this);
  //   auto res = analyser.parse( expression );
  //   set_sequence( res.begin(), res.end() );
  // }
  // void set_sequence( std::list<uint> uint_list)
  // {
  //   sequence.clear();
  //   for( auto& var: uint_list) {
  //     concat( var );
  //   }
  // }
  template<typename Iterator>
  void set_sequence( Iterator start, Iterator end )
  {
    sequence.clear();
    for( auto it = start; it != end; it++ ) {
      concat( (*it) );
    }
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
  /** Check Valid Pattern Id */
  bool is_valid_id( const uint id_pattern )
  {
    return (id_pattern < all_patterns.size());
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
  void start()
  {
    if (_state == ready && sequence.size() > 0) {
      LOGLO( "START" );
      _its = sequence.begin();
      (*_its)->start();
      _state = running;
    }
    else if (_state == paused) {
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
  void pause()
  {
    if (_state == running) {
      _state = paused;
      (*_its)->pause();
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

#endif // LOOPER_HPP
