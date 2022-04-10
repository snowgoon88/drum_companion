/* -*- coding: utf-8 -*- */

/** 
 * Try to reapeat "claves_120ms.wav" every second
 * using 'sleep_for()'
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <stdio.h>  // getchar();
#include <iostream>

#include <chrono>
#include <thread>

int main(int argc, char *argv[])
{

  // create engine
  ma_result result;
  ma_engine engine;
  result = ma_engine_init( NULL, &engine );
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to initialize MiniAudio engine" << std::endl;
    return -1;
  }
  auto channels = ma_engine_get_channels( &engine );
  std::cout << "  channel: " << channels << std::endl;
  auto sample_rate = ma_engine_get_sample_rate( &engine );
  std::cout << "  s.rate: " << sample_rate << std::endl;
  
  //ma_engine_play_sound( &engine, argv[1], NULL );

  // preload sound
  //char clave_path[] = "ressources/claves_120ms.wav";
  //char clave_path[] = "ressources/Cowbell-3.wav";
  char clave_path[] = "ressources/cowbell.wav";
  
  std::cout << "__Loading " << std::endl;
  ma_sound clave_sound;
  result = ma_sound_init_from_file( &engine, clave_path, MA_SOUND_FLAG_DECODE,
                                    NULL, /* no group -> engine end point*/
                                    NULL, /* no loading fence */
                                    &clave_sound );
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to load " << clave_path << std::endl;
    return result;
  }

  // Dirty : loop to play sound
  for( unsigned int i = 0; i < 5; ++i) {
    std::cout << "Start play i=" << i << std::endl;
    result = ma_sound_start( &clave_sound);
    if (result != MA_SUCCESS) {
      std::cerr << "Failed to play i=" << i << std::endl;
      ma_engine_uninit( &engine );
      return result;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(780));
  }

  std::cout << "Press ENTER to quit..." << std::endl;
  getchar();

  ma_engine_uninit( &engine );
  
  return 0;
}

