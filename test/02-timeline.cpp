/* -*- coding: utf-8 -*- */

/** 
 * Test timeline and signature
 */

#include <iostream>
#include <algorithm> // for copy
#include <iterator> // for ostream_iterator

#include <pattern_audio.hpp>

#include <vector>

void test()
{
  Signature sig;
  std::cout << "__DEFAULT Signature" << std::endl;
  std::cout << "  " << sig << std::endl;
  std::cout << "  " << "sub_length=" << sig.division_length() << std::endl;
  
  sig = { 90, 2, 5 };
  std::cout << "__90/5x2 Signature" << std::endl;
  std::cout << "  " << sig  << std::endl;
  std::cout << "  " << "sub_length=" << sig.division_length() << std::endl;

  PatternAudio pa;
  std::cout << "__DEFAULT PatternAudio" << std::endl;
  std::cout << "  " << pa.str_dump()  << std::endl;
  std::cout << "  " << "sub_length=" << pa._signature.division_length() << std::endl;

  std::vector<uint> v1 {1, 0, 1, 0};
  pa.init_from_timeline( v1 );
  std::cout << "__timeline 1,0,1,0" << std::endl;
  std::copy( pa._pattern_intervale.begin(),
             pa._pattern_intervale.end(),
             std::ostream_iterator<Note>(std::cout, " "));
  std::cout << std::endl;

  std::vector<uint> v2 {1, 0, 0, 1};
  pa.init_from_timeline( v2 );
  std::cout << "__timeline 1,0,0,1" << std::endl;
  std::copy( pa._pattern_intervale.begin(),
             pa._pattern_intervale.end(),
             std::ostream_iterator<Note>(std::cout, " "));
  std::cout << std::endl;

  std::vector<uint> v3 {0, 0, 1, 1};
  pa.init_from_timeline( v3 );
  std::cout << "__timeline 0,0,1,1" << std::endl;
  std::copy( pa._pattern_intervale.begin(),
             pa._pattern_intervale.end(),
             std::ostream_iterator<Note>(std::cout, " "));
  std::cout << std::endl;

  pa._signature = sig;
  std::vector<uint> v4 {0, 1, 0, 0, 2, 0, 0, 1, 0, 1};
  pa.init_from_timeline( v4 );
  std::cout << "__timeline 0, 1, 0, 0, 2, 0, 0, 1, 0, 1" << std::endl;
  std::copy( pa._pattern_intervale.begin(),
             pa._pattern_intervale.end(),
             std::ostream_iterator<Note>(std::cout, " "));
  std::cout << std::endl;


}

int main(int argc, char *argv[])
{
  test();

  return 0;
}

