/* -*- coding: utf-8 -*- */

/** 
 * Test of AudioPattern.
 */

#include <iostream>
#include <pattern_audio.hpp>
#include <random>


int main(int argc, char *argv[])
{
  // Random Generator
  // generate seed
  std::random_device random_seeder;
  std::default_random_engine rnd_engine( random_seeder() );
  auto rnd_gen = std::uniform_int_distribution<int>(0,20);
  
  PatternAudio pa;

  pa.start();
  for( unsigned int i = 0; i < 300; ++i) {
    auto rnd_int = rnd_gen( rnd_engine );
    std::this_thread::sleep_for(std::chrono::milliseconds(rnd_int));
    pa.update();
  }
  
  return 0;
}
