/* -*- coding: utf-8 -*- */

#ifndef ANALYZER_HPP
#define ANALYZER_HPP

/** 
 * Allowed operations:
 * repeat: nb x expr
 * concat: expr + expr
 * parenthesis : (expr)
 *
 * We use the Shunting Yard algorithm
 * https://en.wikipedia.org/wiki/Shunting_yard_algorithm
 * but as our operand may be different (number, pattern and expression)
 * we "cheat" by using here UintList for all operand,
 * as a substitute to PatternList.
 * After evaluation, we will map UintList to Looper::PatternList.
 *
 * TODO parse_error as subclass of runtime_error
 * TODO add last error (msg + bool) ?
 */

#include <utils.hpp>
#include <vector>
#include <iostream>
#include <looper.hpp>
#include <exception>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
#define LOG_AN
#ifdef LOG_AN
#  define LOGAN(msg) (LOG_BASE("[Alyz]", msg))
#else
#  define LOGAN(msg)
#endif

//namespace analyzer
//{
// ********************************************************************* Types
using StrIt = std::string::iterator;
using StrPos = std::string::size_type;
using UintList = std::list<uint>;
enum OType {concat, repeat, paren_in, paren_out};
// ***************************************************************************
// ********************************************************************* Token
// ***************************************************************************
class Token
{
public:
  Token(StrIt start_, StrIt end_) : start(start_), end(end_) {}
  virtual ~Token() {}
  virtual std::string str_dump () const
  {
    std::stringstream dump;
    dump << "Tok[]";

    return dump.str();
  }
  // ******************************************************** Token::attributs
  StrIt start;
  StrIt end;
};
class NbToken : public Token
{
public:
  NbToken(StrIt start_, StrIt end_, uint nb_) : Token(start_, end_), nb(nb_) {}
  virtual ~NbToken() {}
  virtual std::string str_dump () const
  {
    std::stringstream dump;
    dump << "Tok[nb=" << nb << "]";

    return dump.str();
  }
  // ****************************************************** NbToken::attributs
  uint nb;
};
class ExpToken : public Token
{
public:
  ExpToken(StrIt start_, StrIt end_, const UintList &patlist)
      : Token(start_, end_), exp(patlist) {}
  virtual ~ExpToken() {}
  virtual std::string str_dump () const
  {
    std::stringstream dump;
    dump << "Tok[exp=" << exp << "]";

    return dump.str();
  }
  // ***************************************************** ExpToken::attributs
  UintList exp;
};
class OpToken : public Token
{
public:
  OpToken(StrIt start_, StrIt end_, OType op_) :
    Token(start_, end_), op(op_) {}
  virtual ~OpToken() {}
  virtual std::string str_dump () const
  {
    std::stringstream dump;
    dump << "Tok[op=";
    switch (op) {
    case concat:
      dump << "+";
      break;
    case repeat:
      dump << *start;
      break;
    case paren_in:
      dump << "(";
      break;
    case paren_out:
      dump << ")";
      break;
    }
    dump << "]";
    return dump.str();
  }
  // ****************************************************** OpToken::attributs
  OType op;
};

using TokenStack = std::vector<Token*>;
using OpTokenStack = std::vector<OpToken*>;
// *************************************************************** Token - End

// ***************************************************************************
// ****************************************************************** Analyzer
// ***************************************************************************
class Analyzer
{
public:
  // ****************************************************** Analyzer::creation
  Analyzer( Looper* looper = nullptr ) :
    _looper(looper),
    _error(false),
    _str_error("")
  {
  }
  // *********************************************************** Analyser::str
  std::string str_dump () const
  {
    std::stringstream dump;
    dump << "_formula=" << _formula << std::endl;
    dump << "Output: ";
    for( auto& tok: output) {
      dump <<  tok->str_dump() << ", ";
    }
    dump << std::endl;
    dump << "Auxiliary: ";
    for( auto& tok: auxiliary) {
      dump << tok->str_dump() << ", ";
    }
    dump << std::endl;
    
    return dump.str();
  }
   
  // ********************************************************* Analyzer::parse
  //void parse( StrIt it_start, StrIt it_end )
  UintList parse( const std::string& formula )
  {
    _error = false;
   
    _formula = formula;
    output = TokenStack{}; // create new empty stack
    auxiliary = OpTokenStack{}; 
    StrIt it_start = _formula.begin();
    StrIt it_end = _formula.end();

    LOGAN( "__PARSE Init\n" << str_dump() );
    while (it_start != it_end) {
      it_start = _trim_space(it_start, it_end);
      if (it_start != it_end) {

        if (_is_digit( it_start )) {
          LOGAN( "  is_number ? " );
          StrIt begin = it_start;
          while (_is_digit( it_start )) {
            it_start++;
          }

          NbToken *tok_ptr = new NbToken(begin, it_start,
                                         _find_uint(begin, it_start));
          output.push_back( tok_ptr );
        }

        else if (*it_start == 'p' || *it_start == 'P') {
          LOGAN( "  is_pattern ? " );
          StrIt begin = it_start;
          it_start++;
          while (_is_digit( it_start )) {
            it_start++;
          }

          uint id_pattern = _find_pattern( begin, it_start );
          ExpToken *tok_ptr = new ExpToken(begin, it_start, {id_pattern});
          output.push_back( tok_ptr );          
        }
        else if (*it_start == 'x' || *it_start == '*' ) {
          LOGAN( "  is_repeat ? " );
          StrIt begin = it_start;
          it_start++;

          OpToken* tok_ptr = new OpToken( begin, it_start, OType::repeat );
          push_auxiliary( tok_ptr );
        }
        else if (*it_start == '+') {
          LOGAN( "  is_concat ? " );
          StrIt begin = it_start;
          it_start++;

          OpToken* tok_ptr = new OpToken( begin, it_start, OType::concat );
          push_auxiliary( tok_ptr );
        }
        else if (*it_start == '(') {
          LOGAN( "  is paren_in ? " );
          StrIt begin = it_start;
          it_start++;

          OpToken* tok_ptr = new OpToken( begin, it_start, OType::paren_in );
          push_auxiliary( tok_ptr );
        }
        else if (*it_start == ')') {
          LOGAN( "  is paren_out ? " );
          StrIt begin = it_start;
          it_start++;

          OpToken* tok_ptr = new OpToken( begin, it_start, OType::paren_out );
          push_auxiliary( tok_ptr );
        }
        else {
          display_error( Token(it_start,it_end), "Expression pas reconnue" );
        }

        LOGAN( "=> to parse '" << _subformula( it_start, it_end) << "'\n" << str_dump());
      }
      else {
        LOGAN( "nothing else to parse" );
      }
    }

    // Now, pop back auxiliary back to output
    LOGAN( "__POP BACK auxiliary to output" );
    while (! auxiliary.empty()) {
      OpToken* tokop = auxiliary.back();
      switch (tokop->op) {
      case OType::repeat:
      case OType::concat:
        break;
      case OType::paren_in:
        display_error( *tokop, "Parenthèse ouvrante qui n'est pas fermée." );
        break;
      case OType::paren_out:
        display_error( *tokop, "Parenthèse fermante qui n'a rien à faire là." );
        break;
      }

      auxiliary.pop_back();
      // Should be possible to evaluate aux_op
      eval( tokop );
      delete tokop;
    }

    // If one pattern left in output...
    // if (output.size() == 1 && output.back().type == TType::pattern) {
    //   _check_pattern();
    // }

    // now, should be empty except one expression
    if (output.size() > 1 ) {
      throw std::runtime_error( "parse: output not empty at end" );
    }
    if (output.size() == 1 ) {
      Token* tok = output.back();
      ExpToken* tokexp = dynamic_cast<ExpToken*>(tok);
      if (tokexp == nullptr) {
        display_error( *tok, "Cette Expression n'est pas valide" );
      }
      LOGAN( "at end\n" << str_dump());
      return tokexp->exp;
    }
    return UintList{};
  }
  void push_auxiliary( OpToken* tok )
  {
    LOGAN( "push_auxiliary: " << tok->str_dump() );
    // before pushing to auxiliary, pop back opertor of higher precedence
    if (tok->op == OType::concat) {
      bool keep_checking = (!auxiliary.empty());
      while( keep_checking ) {
        OpToken* aux_op = auxiliary.back();
        if (aux_op->op == OType::repeat) {
          // pop back
          auxiliary.pop_back();
          eval( aux_op );
          delete aux_op;

          keep_checking = (!auxiliary.empty());
        }
        else if (aux_op->op == OType::concat) {
          // pop back
          auxiliary.pop_back();
          eval( aux_op );
          delete aux_op;

          keep_checking = (!auxiliary.empty());
        }
        else {
          keep_checking = false;
        }
      }
      auxiliary.push_back( tok );
    }

    else if (tok->op == OType::repeat) {
      LOGAN( "  => PUSHED to auxiliary" );
      auxiliary.push_back( tok );
    }

    else if (tok->op == OType::paren_in) {
      LOGAN( "  => PUSHED to auxiliary" );
      auxiliary.push_back( tok );
    }

    else if (tok->op == OType::paren_out) {
      LOGAN( "  => empty auxiliary to paren_in" );
      while (auxiliary.back()->op != OType::paren_in) {
        OpToken* aux_op = auxiliary.back();
        LOGAN( "  pop back " << aux_op->str_dump() );
        if (aux_op->op == OType::repeat) {
          // pop back
          auxiliary.pop_back();
          eval( aux_op );
          delete aux_op;
        }
        else if (aux_op->op == OType::concat) {
          // pop back
          auxiliary.pop_back();
          eval( aux_op );
          delete aux_op;
        }
        if (auxiliary.empty()) {
          display_error( *tok, "Parenthèse fermée qui n'est pas ouverte" );
        }
      }
      // remove paren_in
      auxiliary.pop_back();
    }
  }
  /** Apply top operator from output */
  void eval( OpToken* tokop)
  {
    LOGAN( " eval " << tokop->str_dump() );

    switch (tokop->op) {
    case OType::concat:
      _check_concat( _subformula( tokop->start, tokop->end ));
      break;
    case OType::repeat:
      _check_repeat( _subformula( tokop->start, tokop->end ));
      break;
    default:
      throw std::runtime_error( "eval: op="+_subformula( tokop->start, tokop->end )+" NOT IMPLEMENTED" );
    }
  }
  // ********************************************************** Analyzer::find
  uint _find_pattern( StrIt it_start, StrIt it_end)
  {
    LOGAN( "_find_pattern in '" << _subformula(it_start, it_end )+"'" );
    if (*it_start != 'p' && *it_start != 'P') {
      display_error( Token{it_start, it_end}, "N'est pas un nom de PATTERN" ); 
    }

    try {
      uint id_pattern = static_cast<uint>(std::stoi(_subformula(it_start+1, it_end)));
      if (_looper) {
        if (!_looper->is_valid_id( id_pattern )) {
          display_error(Token{it_start, it_end}, "N'est pas un PATTERN valide");
        }
      }
      return id_pattern;
    }
    catch (std::invalid_argument e) {
      display_error( Token{it_start, it_end}, "N'est pas un nom de PATTERN" );
    }
    return 0;
  }
  uint _find_uint( StrIt it_start, StrIt it_end)
  {
    LOGAN( "_find_uint in '" << _subformula( it_start, it_end )+"'" );
    return static_cast<uint>(std::stoi(_subformula(it_start, it_end))); 
  }
  // ********************************************************* Analyzer::check
  void _check_repeat( const std::string& op )
  {
    // check nb, pattern are the last in output
    Token* tokright = output.at(output.size()-1);
    ExpToken* exptok = dynamic_cast<ExpToken*>(tokright);
    if (exptok == nullptr) {
      display_error( *tokright, " n'est pas un PATTERN ou une EXPRESSION, demandé par REPEAT (" + op + ")" );
    }
    
    Token* tokleft = output.at(output.size()-2);
    NbToken *nbtok = dynamic_cast<NbToken*>(tokleft);
    if (nbtok == nullptr) {
      display_error( *tokleft, " n'est pas un NOMBRE, demandé par REPEAT (" + op + ")" );
    }

    UintList result;
    for( unsigned int i = 0; i < nbtok->nb; ++i) {
      std::copy(exptok->exp.begin(), exptok->exp.end(),
                std::back_inserter(result));
    }

    ExpToken* tokres = new ExpToken( std::min( tokright->start, tokleft->start),
                                     std::max( tokright->end, tokleft->end),
                                     result );
    // popout from output and clean
    output.pop_back();
    output.pop_back();
    delete exptok;
    delete nbtok;

    output.push_back( tokres );
  }
  void _check_concat( const std::string& op )
  {
    // check nb, pattern are the last in output
    Token* tokright = output.back();
    ExpToken* expright = dynamic_cast<ExpToken*>(tokright);
    if (expright == nullptr) {
      display_error( *tokright, " n'est pas un PATTERN ou une EXPRESSION, demandé par REPEAT (" + op + ")" );
    }
    output.pop_back();
    
    Token* tokleft = output.back();
    ExpToken* expleft = dynamic_cast<ExpToken*>(tokleft);
    if (expleft == nullptr) {
      display_error( *tokleft, " n'est pas un PATTERN ou une EXPRESSION, demandé par REPEAT (" + op + ")" );
    }
    output.pop_back();
    
    UintList result = expleft->exp;
    std::copy( expright->exp.begin(), expright->exp.end(),
               std::back_inserter(result));
    ExpToken* tokres = new ExpToken( std::min( tokright->start, tokleft->start),
                                     std::max( tokright->end, tokleft->end),
                                     result );
    // popout from output and clean
    LOGAN( "  ready to clean\n" << str_dump() );
    delete expleft;
    delete expright;

    output.push_back( tokres );
  }
  // ********************************************************* Analyzer::utils
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
  // *********************************************************** Analyzer::error
  void display_error( const Token& tok, const std::string& msg )
  {
    // Display formula
    const std::string header = "Erreur dans '";
    std::cerr << header << _formula << "'" << std::endl;

    std::string::size_type offset = _pos(tok.start)+header.size();
    std::string::size_type patlen = _pos(tok.end) - _pos(tok.start);
    std::stringstream err;
    err << header << _formula << "'" << std::endl;
    err << std::string( offset, ' ' );
    err << std::string( patlen, '^');
    err << std::endl;
    err << std::string( offset+patlen/2, ' ' );
    err << std::string("|");
    err << std::endl;
    // TODO check msg is long enough
    err << msg;
    std::cerr <<  err.str() << std::endl;
    
    // _str_error.clear();
    // _str_error.insert( _str_error.begin(),
    //                    err.str().begin(), err.str().end());
    _str_error = err.str();
    _error = true;


    throw std::runtime_error( msg );
  }

  // *********************************************************** Analyzer::error
  bool has_error() { return _error; }
  std::string str_error() { return _str_error; }
  // ***************************************************** Analyzer::attributs
  std::string _formula;
  Looper* _looper;
  
  TokenStack output;
  OpTokenStack auxiliary;

  bool _error;
  std::string _str_error;
};
// ************************************************************ Analyzer - End



//}; // namespace

#endif // ANALYZER_HPP
