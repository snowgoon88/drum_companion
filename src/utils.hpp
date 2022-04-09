/* -*- coding: utf-8 -*- */

#ifndef UTILS_HPP
#define UTILS_HPP

/** 
 * TODO
 */

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
