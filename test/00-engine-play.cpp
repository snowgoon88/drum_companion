/* -*- coding: utf-8 -*- */

/** 
 * Basic example of using "high-level" miniaudio to play a file
 * using the engine.
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <stdio.h>  // getchar();
#include <iostream>

int main(int argc, char *argv[])
{

  // test input file in arguments
  if (argc < 2) {
    std::cerr <<  "No input file." << std::endl;
    return -1;
  }

  // create engine
  ma_engine engine;
  ma_result result = ma_engine_init( NULL, &engine );
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to initialize MiniAudio engine" << std::endl;
    return -1;
  }

  ma_engine_play_sound( &engine, argv[1], NULL );

  std::cout << "Press ENTER to quit..." << std::endl;
  getchar();

  ma_engine_uninit( &engine );
  
  return 0;
}

