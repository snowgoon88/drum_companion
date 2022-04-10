/* -*- coding: utf-8 -*- */

#ifndef UTILS_HPP
#define UTILS_HPP

/** 
 * TODO
 */

#include <iostream>      // cout, endl

// ******************************************************************* loggers
// in order to define other loggers using
// #define LOG_TRUC
// #ifdef LOG_TRUC
// #  define LOGTRUC(msg) (LOG_BASE("[TRUC]", msg)
// #else
// #  define LOGTRUC(msg)
// #endif
#define LOG_BASE(head,msg) (std::cout << head << " " << msg << std::endl)


// ********************************************************** unused variables
// These macro can prevent "warning: unused variable" from compilator
#define UNUSED(expr) ((void)(expr))
#define USED_IN_MACRO(expr) ((void)(expr))

// **************************************************************** str_vector
std::ostream &operator<<(std::ostream &os,
                                const std::vector<uint> &t)
{
  os << "{ ";
  for( const uint& val: t) {
    os << val << ", ";
  }
  os << "}";
  
  return os;
}


// std::copy( pa._pattern_intervale.begin(),
//              pa._pattern_intervale.end(),
//              std::ostream_iterator<Note>(std::cout, " "));



#endif // UTILS_HPP
