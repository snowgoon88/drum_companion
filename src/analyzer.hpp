/* -*- coding: utf-8 -*- */

#ifndef ANALYZER_HPP
#define ANALYZER_HPP

/** 
 * Allowed operations:
 * repeat: nb x expr
 * concat: expr + expr
 *
 * Given our operations, we can scan from left to right for 'thing OP thing'
 * and evaluate first the RIGHT thing and then the LEFT thing.
 * TODO : change this
 */

#include <utils.hpp>
//#include <stack>
#include <vector>
#include <iostream>
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
using PatternList = std::list<uint>;
using StrIt = std::string::iterator;
using StrPos = std::string::size_type;
using Expr = struct expr_T {
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
// ********************************************************************* Types
// ***************************************************************************
/** We will be using 2 stacks
 * - output stack with number (for x) and pattern (for x and +)
 */
enum TType {number, pattern, operation};
enum OType {concat, repeat, paren_in, paren_out};
using Token = struct token_T {
  TType type;
  StrIt start;     // iterator to the token string
  StrIt end;
  union val_T {
    uint nb;
    OType op;
  } val;
};
using TokenStack = std::vector<Token>;
std::ostream &operator<<(std::ostream &os, const Token &t)
{
  os << "Tok[";
  switch ( t.type ) {
  case number:
    os << "nb=" << t.val.nb;
    break;
  case pattern:
    os << "pat=" << t.val.nb;
    break;
  case operation:
    os << "op=";
    switch (t.val.op) {
    case concat:
      os << "+";
      break;
    case repeat:
      os << *t.start;
      break;
    case paren_in:
      os << "(";
      break;
    case paren_out:
      os << ")";
      break;
    }
  }
  os << "]";

  return os;
}

// ***************************************************************************
// ****************************************************************** Analyzer
// ***************************************************************************
class Analyzer
{
public:
  // *********************************************************** Analyser::str
  std::string str_dump () const
  {
    std::stringstream dump;
    dump << "_formula=" << _formula << std::endl;
    dump << "Output: ";
    for( auto& tok: output) {
      dump << tok << ", ";
    }
    dump << std::endl;
    dump << "Auxiliary: ";
    for( auto& tok: auxiliary) {
      dump << tok << ", ";
    }
    dump << std::endl;
    
    return dump.str();
  }
   
  // ********************************************************* Analyzer::parse
  //void parse( StrIt it_start, StrIt it_end )
  void parse( const std::string& formula )
  {
    _formula = formula;
    output = TokenStack{}; // create new empty stack
    auxiliary = TokenStack{}; 
    StrIt it_start = _formula.begin();
    StrIt it_end = _formula.end();

    LOGAN( "__PARSE Init\n" << str_dump() );
    while (it_start != it_end) {
      it_start = _trim_space(it_start, it_end);
      if (it_start != it_end) {
        // TType::number
        if (_is_digit( it_start )) {
          LOGAN( "  is_number ? " );
          StrIt begin = it_start;
          while (_is_digit( it_start )) {
            it_start++;
          }
        
          Token tok_nb{ TType::number, begin, it_start,
              {_find_uint( begin, it_start )} };
          output.push_back( tok_nb );
        }
        else if (*it_start == 'p' || *it_start == 'P') {
          LOGAN( "  is_pattern ? " );
          StrIt begin = it_start;
          it_start++;
          while (_is_digit( it_start )) {
            it_start++;
          }
          // TODO check valid pattern 
          Token tok_nb{ TType::pattern, begin, it_start,
              {_find_pattern( begin, it_start )} };
          output.push_back( tok_nb );
        }
        else if (*it_start == 'x' || *it_start == '*' ) {
          LOGAN( "  is_repeat ? " );
          StrIt begin = it_start;
          it_start++;

          Token tok{ TType::operation, begin, it_start,
                     {OType::repeat} };
          //auxiliary.push_back( tok );
          push_auxiliary( tok );
        }
        else if (*it_start == '+') {
          LOGAN( "  is_concat ? " );
          StrIt begin = it_start;
          it_start++;

          Token tok{ TType::operation, begin, it_start,
                     {OType::concat} };
          push_auxiliary( tok );
          //auxiliary.push_back( tok );
        }
        else if (*it_start == '(') {
          LOGAN( "  is paren_in ? " );
          StrIt begin = it_start;
          it_start++;

          Token tok{ TType::operation, begin, it_start,
                     {OType::paren_in} };
          push_auxiliary( tok );
        }
        else if (*it_start == ')') {
          LOGAN( "  is paren_out ? " );
          StrIt begin = it_start;
          it_start++;

          Token tok{ TType::operation, begin, it_start,
                     {OType::paren_out} };
          push_auxiliary( tok );
        }
        else {
          throw std::runtime_error( "parse: unrecognized token '"+_subformula(it_start, it_end));
        }

        LOGAN( "=> to parse '" << _subformula( it_start, it_end) << "'\n" << str_dump());
      }
      else {
        LOGAN( "nothing else to parse" );
      }
    }

    // Now, pop back auxiliary back to output
    while (! auxiliary.empty()) {
      Token& tokop = auxiliary.back();
      switch (tokop.val.op) {
      case OType::repeat:
        //TODO check when eval _check_repeat( _subformula(tokop.start, tokop.end) );
        // TODO : maybe pop back many operators
        break;
      case OType::concat:
        //TODO check when eval _check_concat( _subformula(tokop.start, tokop.end) );
        // TODO : beware of precedence
        break;
        //throw std::runtime_error( "parse: check pop back CONCAT NOT IMPLEMENTED" );
      case OType::paren_in:
        throw std::runtime_error( "parse: Parenthèse ouvrante qui n'est pas fermée." );
      case OType::paren_out:
        throw std::runtime_error( "parse: Parenthèse fermante qui n'a rien à faire là." );
      }

      output.push_back( tokop );
      auxiliary.pop_back();
    }
    LOGAN( "at end\n" << str_dump());
  }
  void push_auxiliary( Token& tok )
  {
    LOGAN( "push_auxiliary: " << tok );
    // before pushing to auxiliary, pop back opertor of higher precedence
    if (tok.val.op == OType::concat) {
      bool keep_checking = (!auxiliary.empty());
      while( keep_checking ) {
        Token& aux_op = auxiliary.back();
        if (aux_op.val.op == OType::repeat) {
          // pop back
          // TODO check when eval _check_repeat( _subformula( aux_op.start, aux_op.end ));
          output.push_back( aux_op );
          auxiliary.pop_back();
          
          keep_checking = (!auxiliary.empty());
        }
        else if (aux_op.val.op == OType::concat) {
          // pop back
          // TODO check when eval _check_concat( _subformula( aux_op.start, aux_op.end ));
          output.push_back( aux_op );
          auxiliary.pop_back();
          
          keep_checking = (!auxiliary.empty());
        }
        else {
          keep_checking = false;
        }
      }
      auxiliary.push_back( tok );
    }
    else if (tok.val.op == OType::repeat) {
      LOGAN( "  => PUSHED to auxiliary" );
      auxiliary.push_back( tok );
    }
    else if (tok.val.op == OType::paren_in) {
      LOGAN( "  => PUSHED to auxiliary" );
      auxiliary.push_back( tok );
    }
    else if (tok.val.op == OType::paren_out) {
      LOGAN( "  => empty auxiliary to paren_in" );
      while (auxiliary.back().val.op != OType::paren_in) {
        Token& aux_op = auxiliary.back();
        LOGAN( "  pop back " << aux_op );
        if (aux_op.val.op == OType::repeat) {
          // pop back
          _check_repeat( _subformula( aux_op.start, aux_op.end ));
          output.push_back( aux_op );
          auxiliary.pop_back();
        }
        else if (aux_op.val.op == OType::concat) {
          // pop back
          _check_concat( _subformula( aux_op.start, aux_op.end ));
          output.push_back( aux_op );
          auxiliary.pop_back();
        }
        if (auxiliary.empty()) {
          throw std::runtime_error( "auxiliary push: Parenthèse fermée qui n'est pas ouverte" );
        }
      }
      // remove paren_in
      auxiliary.pop_back();
    }
  }
  // ********************************************************** Analyzer::find
  /** look for left or end expression in expr */ 
  Expr _find_expr( StrIt it_start, StrIt it_end)
  {
    LOGAN( "_find_expr in '" <<  _subformula(it_start, it_end) << "'" );
    // do not care for SPACE
    it_start = _trim_space( it_start, it_end );

    StrIt expr_start = it_start;
    LOGAN( "  start (" << _pos(expr_start) << "):" << _at(expr_start) );

    // advance until no digit or no p/P
    while ((_is_digit( it_start ) || (*it_start == 'p') || (*it_start == 'P')) && it_start != it_end) {
      it_start++;
    }
    StrIt expr_end = it_start;
    LOGAN( "  end (" << _pos(expr_end) << "):" << _at(expr_end) );
    LOGAN( "  => found " << _subformula( expr_start, expr_end ));

    return Expr{ expr_start, expr_end };
  }
  Expr _find_operator( StrIt it_start, StrIt it_end)
  {
    LOGAN( "_find_operator in '" <<  _subformula(it_start, it_end) << "'" );
    it_start = _trim_space( it_start, it_end );

    // Concat
    if (_at( it_start ) == "+") {
      LOGAN( "  start (" << _pos(it_start) << "):" << _at(it_start) );
      return Expr(it_start, it_start+1);
    }
    // Repeat
    if (_at( it_start ) == "x" || _at(it_start) == "*") {
      LOGAN( "  start (" << _pos(it_start) << "):" << _at(it_start) );
      return Expr(it_start, it_start+1);
    }
    return Expr(it_start, it_start);
  }
  uint _find_pattern( Expr& expr )
  {
    LOGAN( "_find_pattern in '" << _subformula( expr )+"'" );
    if (*expr.start != 'p' && *expr.start != 'P') {
      throw std::runtime_error( "'"+_subformula( expr )+"' n'est PAS un nom de Pattern.");
    }
    return static_cast<uint>(std::stoi(_subformula(expr.start+1, expr.end))); 
  }
  uint _find_pattern( StrIt it_start, StrIt it_end)
  {
    LOGAN( "_find_pattern in '" << _subformula(it_start, it_end )+"'" );
    if (*it_start != 'p' && *it_start != 'P') {
      throw std::runtime_error( "'"+_subformula(it_start, it_end)+"' n'est PAS un nom de Pattern.");
    }
    return static_cast<uint>(std::stoi(_subformula(it_start+1, it_end))); 
  }
  uint _find_uint( Expr& expr )
  {
    LOGAN( "_find_uint in '" << _subformula( expr )+"'" );
    // if (*expr.start != 'p' && *expr.start != 'P') {
    //   throw std::runtime_error( "'"+_subformula( expr )+"' n'est PAS un nom de Pattern.");
    // }
    return static_cast<uint>(std::stoi(_subformula(expr.start, expr.end))); 
  }
  uint _find_uint( StrIt it_start, StrIt it_end)
  {
    LOGAN( "_find_uint in '" << _subformula( it_start, it_end )+"'" );
    // if (*expr.start != 'p' && *expr.start != 'P') {
    //   throw std::runtime_error( "'"+_subformula( expr )+"' n'est PAS un nom de Pattern.");
    // }
    return static_cast<uint>(std::stoi(_subformula(it_start, it_end))); 
  }
  // ********************************************************* Analyzer::check
  void _check_repeat( const std::string& op )
  {
    // check nb, pattern are the last in output
    Token& tok_last = output.at(output.size()-1);
    if (tok_last.type != TType::pattern) {
      throw std::runtime_error( _subformula( tok_last.start, tok_last.end ) + " n'est pas un PATTERN, demandé par REPEAT (" + op + ")" );
    }
    Token& tok_nb = output.at(output.size()-2);
    if (tok_nb.type != TType::number) {
      throw std::runtime_error( _subformula( tok_nb.start, tok_nb.end ) + " n'est pas un NOMBRE, demandé par REPEAT (" + op + ")" );
    }
  }
  void _check_concat( const std::string& op )
  {
    // check nb, pattern are the last in output
    Token& tok_last = output.at(output.size()-1);
    if (tok_last.type != TType::pattern && tok_last.type != TType::operation) {
      throw std::runtime_error( _subformula( tok_last.start, tok_last.end ) + " n'est pas un PATTERN, demandé par CONCAT (" + op + ")" );
    }
    Token& tok_before = output.at(output.size()-2);
    if (tok_before.type != TType::pattern && tok_before.type != TType::operation) {
      throw std::runtime_error( _subformula( tok_before.start, tok_before.end ) + " n'est pas un PATTERN, demandé par CONCAT (" + op + ")" );
    }
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
  std::string _subformula( const Expr& expr )
  {
    return _subformula( expr.start, expr.end );
  }
  std::string _at( StrIt it )
  {
    return _subformula( it, it+1 );
  }

  // ***************************************************** Analyzer::attributs
  std::string _formula;

  TokenStack output;
  TokenStack auxiliary;
};
// ************************************************************ Analyzer - End



//}; // namespace

#endif // ANALYZER_HPP
