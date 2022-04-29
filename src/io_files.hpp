/* -*- coding: utf-8 -*- */

#ifndef IO_FILES_HPP
#define IO_FILES_HPP

/** 
 * Utilities to write/read token+arg to/from file.
 */

#include <string>        // getline, 
#include <fstream>

// ********************************************************************* write
void write_comment( std::ostream& os, const std::string& comment )
{
  os << "# " << comment << std::endl;
}
template<typename T>
void write_token( std::ostream& os, const std::string& token,
                   const T& arg,
                   const std::string& comment = "" )
{
  if (comment != "") {
    os << "# " << comment << std::endl;
  }
  os << token << ": " << arg << std::endl;
}
// *************************************************************** read_string
std::string read_string( std::istream& is, const std::string& token )
{
  std::string line;

  while (std::getline(is, line)) {
    // Ignore ligne which start with '#' ou ' '
    if (line.front() != '#' && line.front() != ' ') {
      // begin with our token ?
      auto pos = line.find( token+": " );
      if (pos != std::string::npos) {
        return line.substr( pos+token.size()+2 );
      }
    }
  }
  throw std::runtime_error( "read_string: token '"+token+"' not found" );
}
// ***************************************************************** read_uint
unsigned int read_uint( std::istream& is, const std::string& token )
{
  std::string line;

  while (std::getline(is, line)) {
    // Ignore ligne which start with '#' ou ' '
    if (line.front() != '#' && line.front() != ' ') {
      // begin with our token ?
      auto pos = line.find( token+": " );
      if (pos != std::string::npos) {
        auto result = std::stoi(line.substr( pos+token.size()+2 ));
        return static_cast<unsigned int>(result);
      }
    }
  }
  throw std::runtime_error( "read_uint: token '"+token+"' not found" );
}

#endif // IO_FILES_HPP
