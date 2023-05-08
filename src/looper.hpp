/* -*- coding: utf-8 -*- */

#ifndef LOOPER_HPP
#define LOOPER_HPP

/** 
 * Chain PatternAudio till the end of time...
 *
 * Has a _main_bpm.
 * TODO _main_bpm is applied to all PatternAudio
 * @see ENSURE in code
 *
 * Maintains also a "Beat count" (relative timing, start of pattern, switching)
 * so as to help BeatSlider widget.
 *
 * At creation: _state=empty.
 * Becomes ready WHEN set_sequence() or concat().
 * Can only "plays" (through next()) if has been ready before.
 * TODO a way to become empty when unvalidated (by analyzer, for example, after
 * some GUI action).
 *
 * - add_pattern(): can only add 'ready' Pattern, set its id. Update 'all_patterns'
 * if _sync_bpm, then also sets all Patterns bpm.
 *
 * - set_sequence() or concat(): _state <- ready (if sequence.size() > 0)
 * if _sync_bpm, then sets all Patterns bpm.
 *
 * - start():
 *   _state == ready AND sequence not empty THEN _its->start(), _state <- running.
 *   _state == paused THEN _its->start(), _state <- running
 *
 * - stop():
 *   _state NOT empty THEN _its->stop, _state <- ready, _its to begin
 *
 * - pause():
 *   _state == running THEN _its->pause(), _state <- paused,
 *
 * - next():
 *   _state == running THEN
 *          advance Pattern (*_its)->next(),
 *          and sequence if needed
 *   _state == paused THEN
 *          pass on to pattern (*_its)->next();
 */

#include <utils.hpp>
#include <io_files.hpp>
#include <pattern_audio.hpp>
#include <sound_engine.hpp>
#include <list>
#include <memory>
#include <algorithm>
//#include <vector>
#include <string>
#include <cctype>
#include <ios> //boolalpha

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
  using PatternList = std::list<std::shared_ptr<PatternAudio>>;
private:
  using UintIt = std::iterator<std::input_iterator_tag, unsigned int>;
  using PatternVec = std::list<std::shared_ptr<PatternAudio>>;
public:
  enum LooperState { ready, running, paused, empty };
  
public:
  // ******************************************************** Looper::creation
  Looper( std::shared_ptr<SoundEngine> engine = nullptr, unsigned int main_bpm=90 ) :
    _engine(engine), max_pattern_id(0),
    _state(empty),
    _main_bpm(main_bpm), _sync_bpm(true),
    to_first_beat(false), from_first_beat(true), to_next_beat(1.0f),
    odd_beat(true)
  {
  }
  virtual ~Looper()
  {
    LOGLO( "__DESTROY Looper" );
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
    dump << "  bpm=" << _main_bpm << " sync: " << std::boolalpha << _sync_bpm;
    dump << " sequence.size=" << sequence.size();

    dump << " : [";
    for( auto& pat: sequence) {
      dump << pat->_id <<", ";
    }
    dump << "]";
    dump << std::endl;

    dump << "  beat: pos=" << std::boolalpha << to_next_beat;
    dump << " odd=" << odd_beat;
    dump << " to_first=" << std::boolalpha << to_first_beat;
    dump << " from_first=" << std::boolalpha << from_first_beat;
    dump << std::endl;
    
    return dump.str();
  }
  std::string str_verbose () const
  {
    std::stringstream verbose;
    verbose << "__Looper at " << _main_bpm << " BPM ";
    if (_sync_bpm) verbose << "(sync) ";
    verbose << all_patterns.size() << " patterns" << std::endl;
    for( auto& pat: all_patterns) {
      verbose << "  p"<<std::to_string( pat->_id );
      verbose << "=" << pat->str_verbose() << std::endl;
    }

    verbose << "Sequence= [";
    for( auto& pat: sequence) {
      verbose << pat->_id <<", ";
    }
    verbose << "]" << std::endl;

    verbose << "Beat: pos=" << to_next_beat;
    verbose << " odd=" << odd_beat;
    verbose << " to_first=" << to_first_beat;
    verbose << " from_first=" << from_first_beat;
    verbose << std::endl;
    
    return verbose.str();
  }
  // ************************************************************** Looper::io
  void write_to( std::ofstream& os )
  {
    write_comment( os, "-- Looper --" );
    write_token( os, "main_bpm", _main_bpm );
    write_token( os, "sync_bpm", _sync_bpm );
    write_token( os, "formula", _formula );
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

    _main_bpm = read_uint( is, "main_bpm" );
    _sync_bpm = read_bool( is, "sync_bpm" );
    _formula = read_string( is, "formula" );
    auto nb_pattern = read_uint( is, "nb_pat" );
    all_patterns.clear(); // TODO what if some undeleted patterns ?
    max_pattern_id = 0;
    for( unsigned int idp = 0; idp < nb_pattern; ++idp) {
      auto pat = std::make_shared<PatternAudio>( _engine );
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
  uint add( std::shared_ptr<PatternAudio> pattern )
  {
    if (pattern->_state == PatternAudio::ready ) {
      pattern->_id = max_pattern_id;
      max_pattern_id += 1;
      all_patterns.push_back( pattern );

      // ENSURE same BPM
      if (_sync_bpm) {
        set_all_bpm( _main_bpm );
      }

      return pattern->_id;
    }
    throw std::runtime_error( "add: pattern is not ready" );
    return 0;
  }
  /** Returns true if pat_id is removed */
  bool remove( uint pat_id )
  {
    // erase if we find pat_id
    auto res = std::find_if( all_patterns.begin(), all_patterns.end(),
                             [pat_id](std::shared_ptr<PatternAudio> p){ return p->_id == pat_id;} );
    if (res != all_patterns.end()) {
      all_patterns.erase( res );
      return true;
    }
    return false;
  }
  std::shared_ptr<PatternAudio> get_pattern( uint id )
  {
    auto res = std::find_if( all_patterns.begin(), all_patterns.end(),
                             [id](std::shared_ptr<PatternAudio> p){ return p->_id == id;} );
    if (res != all_patterns.end()) {
      return *res;
    }
    else {
      return nullptr;
    }
  }
  /** Check Valid Pattern Id */
  bool is_valid_id( const uint id_pattern )
  {
    return (get_pattern( id_pattern ) != nullptr);
  }
  // ******************************************************** Looper::sequence
  /** Given Iterator to a sequence of uint (index to Patterns)
   *  builds 'sequence' by concatenating Patterns
   */
  template<typename Iterator>
  void set_sequence( Iterator start, Iterator end )
  {
    sequence.clear();
    for( auto it = start; it != end; it++ ) {
      concat( (*it) );
    }
    if (sequence.size() > 0) {
      _state = ready;
      _its = sequence.begin();

      // ENSURE same BPM
      if (_sync_bpm) {
        set_all_bpm( _main_bpm );
      }
    }
  }
  void concat( uint id_pattern )
  {
    auto pat = get_pattern( id_pattern );
    if (pat != nullptr) {
      sequence.push_back( pat );
    }
    else {
      throw std::runtime_error( "concat: id_pattern not valid" );
    }
    if (sequence.size() > 0) {
      _state = ready;
      _its = sequence.begin();

      // ENSURE same BPM
      if (_sync_bpm) {
        set_all_bpm( _main_bpm );
      }
    }
  }
  void set_all_bpm( unsigned int bpm )
  {
    if ( bpm != _main_bpm ) {
      _main_bpm = bpm;
      for( auto& pat: all_patterns) {
        pat->set_bpm( bpm );
      }
    }
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
          from_first_beat = true;
        }
        (*_its)->start();
      }

      // update Beat related values
      // if relative time left has increased, then switch
      float new_to_beat = (*_its)->beat_proportion();
      if (new_to_beat > to_next_beat) {
        odd_beat = !odd_beat;
        // also check if playing last beat, i.e. goint to first of next
        to_first_beat = ((*_its)->_id_beat == ((*_its)->_signature.beats-1));
        from_first_beat = (*_its)->_id_beat == 0;

      }
      to_next_beat = new_to_beat;
    }
    else if (_state == paused) {
      (*_its)->next();
      // no need to check if sequence must advanced, as paused
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

      to_first_beat = false;
      from_first_beat = true;
      to_next_beat = 1.0f;
      odd_beat = true;
    }
    else if (_state == paused) {
      LOGLO( "UNPAUSE" );
      LOGLO( str_dump() );
      (*_its)->start();
      _state = running;
    }
  }
  void stop()
  {
    if (_state != empty) {
      (*_its)->stop();
      _state = ready;

      _its = sequence.begin();
      from_first_beat = true;
      to_first_beat = false;
      to_next_beat = 1.0f;
      odd_beat = true;
    }
  }
  void pause()
  {
    if (_state == running) {
      _state = paused;
      (*_its)->pause();
    }
    LOGLO( "PAUSE" );
    LOGLO( str_dump() );

  }
  // ******************************************************* Looper::attributs
  std::shared_ptr<SoundEngine>  _engine;
  PatternList sequence;
  PatternVec  all_patterns;
  uint max_pattern_id;
  PatternList::iterator _its;
  LooperState _state;
  std::string _formula;

  unsigned int _main_bpm;
  bool _sync_bpm;
  bool to_first_beat;
  bool from_first_beat;
  float to_next_beat;
  bool odd_beat;
  inline int beat_number()
  {
    int bn = 0;
    if (_state != empty) {
      bn = (*_its)->_id_beat;
    }
    return bn;
  }
};
// ************************************************************** Looper - End

#endif // LOOPER_HPP
