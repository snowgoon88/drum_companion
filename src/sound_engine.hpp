/* -*- coding: utf-8 -*- */

#ifndef SOUND_ENGINE_HPP
#define SOUND_ENGINE_HPP

/** 
 * SoundEngind regroup all miniaudio methods and variables
 *
 * TODO: how to bin a callback ?
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <cstdlib>  // exit
#include <iostream>
#include <string>
#include <vector>

#include <utils.hpp>

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
//#define LOG_SE
#ifdef LOG_SE
#  define LOGSE(msg) (LOG_BASE("[SoEn]", msg))
#else
#  define LOGSE(msg)
#endif


// ***************************************************************************
// *************************************************************** SoundEngine
// ***************************************************************************
class SoundEngine
{
public:
  using SoundVec = std::vector<ma_sound*>;
  
public:
  // ***************************************************** SoundEngine::creation

  SoundEngine()
  {
    LOGSE( "__START SoundEngine" );
    result = ma_engine_init( NULL, &engine );
    check_error( "Failed to initialize MiniAudio engine" );
    //ma_engine_start( &engine );

    auto channels = ma_engine_get_channels(&engine);
    LOGSE( "  channel: " << channels );
    USED_IN_MACRO(channels);
    auto sample_rate = ma_engine_get_sample_rate(&engine);
    LOGSE( "  s.rate: " << sample_rate );
    USED_IN_MACRO(sample_rate);
    
  }
  virtual ~SoundEngine()
  {
    LOGSE( "__DESTROY SoundEngine" );
    for( auto& s: sounds) {
      ma_sound_uninit( s );
      delete s;
    }
    ma_engine_uninit( &engine );
  }

  // ***************************************************** SoundEngine::sounds
  size_t add_sound( const std::string& filepath )
  {
    ma_sound* tmp_sound = new ma_sound;
    result = ma_sound_init_from_file( &engine,
                                      filepath.c_str(),
                                      MA_SOUND_FLAG_DECODE,
                                      NULL, /* no group -> engine end point*/
                                      NULL, /* no loading fence */
                                      tmp_sound );
    check_error( "Failed to load "+filepath );
    sounds.push_back( tmp_sound );
    return (sounds.size() - 1);
  }
  // ******************************************************** SoundEngine::cmd
  void play_sound( const size_t& idx )
  {
    LOGSE( "Playing " << idx  );
    result = ma_sound_start( sounds[idx] );
    check_error( "Failed to play sound ("+std::to_string(idx)+")" );
    LOGSE( "  (running)" );
  }
  void pause_sound( const size_t& idx )
  {
    
    result = ma_sound_stop( sounds[idx] );
    check_error( "Failed to pause sound ("+std::to_string(idx)+")" );
  }
  void stop_sound( const size_t& idx )
  {
    
    result = ma_sound_stop( sounds[idx] );
    check_error( "Failed to stop sound ("+std::to_string(idx)+")" );
    result = ma_sound_seek_to_pcm_frame( sounds[idx], 0 ); // "rewind" sound
    check_error( "Failed to rewind sound ("+std::to_string(idx)+")" );
  }
  // ************************************************** SoundEngine::attributs
  ma_result result;
  ma_engine engine;

  SoundVec sounds;

private:
  // ************************************************ SoundEngine::check_error
  void check_error( const std::string& msg )
  {
    if (result != MA_SUCCESS) {
      std::cerr << msg << std::endl;
      std::exit(-1);
    }
  }
};
// ********************************************************* SoundEngine - END


#endif // SOUND_ENGINE_HPP
