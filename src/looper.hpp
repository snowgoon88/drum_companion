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
using StrIt = std::string::iterator;
using StrPos = std::string::size_type;
using Expr = struct expr_T
{
  StrIt start;
  StrIt end;

  /** constructor */
  expr_T() {}
  expr_T( StrIt start_, StrIt end_ ) {start=start_; end=end_;}

  /** should work even with default init as start=end=0; */
  bool is_empty() {
    return start == end; 
  }
};

// ***************************************************************************
// ******************************************************************** Looper
// ***************************************************************************
class Looper
{
  using PatternList = std::list<PatternAudio*>;
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
  void add( PatternAudio* pattern )
  {
    if (pattern->_state == PatternAudio::ready ) {
      pattern->_id = all_patterns.size();
      all_patterns.push_back( pattern );
    }
  }
  // ******************************************************** Looper::sequence
  void analyze( const std::string& formula )
  {
    _formula = formula;
    Expr le = _find_expr( _formula.begin(), _formula.end() );
    if (le.is_empty()) {
      std::cerr << "ERROR: no l_expr found in '" << _formula << "'" << std::endl;
      exit(1);
    }
    std::cout << "L_expr=" << _subformula(le) << std::endl;
    
    Expr op = _find_operator(le.end, _formula.end());
    if (op.is_empty()) {
      std::cerr << "ERROR: no operator found in '" << _subformula(le.end, _formula.end()) << "'" << std::endl;
      exit(1);
    }
    std::cout << "OPE=" << _subformula(op) << std::endl;
    
    Expr re = _find_expr(op.end, _formula.end());
    if (re.is_empty()) {
      std::cerr << "ERROR: no l_expr found in '" << _subformula(op.end, _formula.end()) << "'" << std::endl;
      exit(1);
    }
    std::cout << "R_expr=" << _subformula(re) << std::endl;
    
  }
  /** look for left or end expression in expr */

  // void _find_expr( const std::string& formula,
  //                  StrPos& pos_start, StrPos& pos_end )

  Expr _find_expr( StrIt it_start, StrIt it_end)
  {
    LOGLO( "_find_expr in '" <<  _subformula(it_start, it_end) << "'" );
    // do not care for SPACE
    it_start = _trim_space( it_start, it_end );

    StrIt expr_start = it_start;
    LOGLO( "  start (" << _pos(expr_start) << "):" << _at(expr_start) );
    
    // advance until no digit
    while (_is_digit( it_start ) && it_start != it_end) {
      it_start++;
    }
    StrIt expr_end = it_start;
    LOGLO( "  end (" << _pos(expr_end) << "):" << _at(expr_end) );
    LOGLO( "  => found " << _subformula( expr_start, expr_end ));

    return Expr{ expr_start, expr_end };
  }
  Expr _find_operator( StrIt it_start, StrIt it_end)
  {
    LOGLO( "_find_operator in '" <<  _subformula(it_start, it_end) << "'" );
    it_start = _trim_space( it_start, it_end );

    // Concat
    if (_at( it_start ) == "+") {
      LOGLO( "  start (" << _pos(it_start) << "):" << _at(it_start) );
      return Expr(it_start, it_start+1);
    }
    return Expr(it_start, it_start);
  }
  void _lookfor_concat( const std::string& expr )
  {
      
  }

  StrIt _trim_space( StrIt it_start, StrIt it_end )
  {
    while (*it_start == ' ' && it_start != it_end) {
      it_start++;
    }
    return it_start;
  }
  bool _is_digit( StrIt it )
  {
    // need to cast as unsigned char for it to work properly
    return std::isdigit(static_cast<unsigned char>(*it));
  }
  StrPos _pos( StrIt it )
  {
    return std::distance( _formula.begin(), it);
  }
  std::string _subformula( StrIt it_start, StrIt it_end)
  {
    return _formula.substr( _pos(it_start), _pos(it_end)-_pos(it_start) );
  }
  std::string _subformula( const Expr& expr )
  {
    return _subformula( expr.start, expr.end );
  }
  std::string _at( StrIt it )
  {
    return _subformula( it, it+1 );
  }
  // ****************************************************** Looper::operations
  void concat( uint id_pattern )
  {
    if (id_pattern < all_patterns.size()) {
      sequence.push_back( all_patterns[id_pattern] );
    }
    if (sequence.size() > 0) {
      _state = ready;
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

#endif // LOOPER_HPP
