/* -*- coding: utf-8 -*- */

#ifndef UTILS_HPP
#define UTILS_HPP

/** 
 * TODO
 */

#include <iostream>      // cout, endl
#include <imgui.h>       // ImVec4 for colors

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

// ********************************************************************* Types
using uint = unsigned int;

// ******************************************************************* Globals
const ImVec4 CLEAR_COL = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
const ImVec4 RED_COL = ImVec4(1.0f, 0.0f, 0.0f, 1.00f);
const ImVec4 GREEN_COL = ImVec4(0.0f, 1.0f, 0.0f, 1.00f);
const ImVec4 YELLOW_COL = ImVec4(0.7f, 0.7f, 0.0f, 1.00f);

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
