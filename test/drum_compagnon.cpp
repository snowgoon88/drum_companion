/* -*- coding: utf-8 -*- */

/** 
 * CLI DrumCompagnon
 * - cli arguments: bpm, sig, pattern,
 * - play repeated sound using sound_pattern,
 */

// Parsing command line options
#include "docopt.h"

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include <signal.h>          // C std lib (signal, sigaction, etc)

#include <pattern_audio.hpp>
#include <sound_engine.hpp>
#include <utils.hpp>        // loggers, etc

// ***************************************************************************
// ******************************************************************* Loggers
// ***************************************************************************
//#define LOG_MAIN
#ifdef LOG_MAIN
#  define LOGMAIN(msg) (LOG_BASE("[MAIN]", msg))
#else
#  define LOGMAIN(msg)
#endif

// *************************************************************** App GLOBALS
SoundEngine *sound_engine = nullptr;
PatternAudio *pattern_audio = nullptr;
bool should_exit = false;

// Args
Signature _p_sig {90, 4, 2};
unsigned int _p_bpm = _p_sig.bpm;
std::string _p_pattern = "2x1x1x1x";

void clear_globals()
{
  if (sound_engine != nullptr ) {
    delete sound_engine;
  }
  if (pattern_audio != nullptr) {
    delete pattern_audio;
  }
}

// *********************************************************** Ctrl-C Callback
void ctrlc_cbk( int s )
{
  LOGMAIN( "__EXIT through Ctrl-C" );
  should_exit = true;
}


// ***************************************************************************
// ******************************************************************* options
// ***************************************************************************
static const char _usage[] =
R"(Drum Companion.

    Usage:
      drum_companion [-b/--bpm=<uint>] [-s/--sig=<str>] [-p/--pattern=<str>]
      drum_companion (-h | --help)

    Options:
      -h --help       Show this screen
      -b <uint>, --bpm <uint>    BPM [default: 90]
      -s <str>, --sig <str>      signature [default: 4x2]
      -p <str>, --pattern <str>  pattern as 2x1x1x1x [default: 2x1x1x1x]
)";

void setup_options( int argc, char **argv )
{
  std::map<std::string, docopt::value> args = docopt::docopt(_usage, 
                                                  { argv + 1, argv + argc },
                                                  true,               // show help if requested
                                                  "Drum Companion 1.0");  // version string
  // for(auto const& arg : args) {
  //   std::cout << arg.first << ": " << arg.second << std::endl;
  // }

  _p_sig.bpm = args["--bpm"].asLong();
  _p_sig.from_string( args["--sig"].asString());
  _p_pattern = args["--pattern"].asString();
}
    
// ***************************************************************************
// ********************************************************************** MAIN
// ***************************************************************************
int main(int argc, char *argv[])
{
  // // To deal with Ctrl-C (Win32 and Linux)
  signal(SIGINT, ctrlc_cbk);

  // Args
  setup_options( argc, argv );

  // ****************************************************************** Sounds
  sound_engine = new SoundEngine();
  // variables are not used, but show how could be used
  auto idx_clave = sound_engine->add_sound( "ressources/claves_120ms.wav" );
  UNUSED(idx_clave);
  auto idx_cow = sound_engine->add_sound( "ressources/cowbell.wav" );
  UNUSED(idx_cow);
    
  // ************************************************************* PlayPattern
  LOGMAIN( "__PATTERN_AUDIO with SoundEngine" );
  pattern_audio = new PatternAudio( sound_engine );
  pattern_audio->_signature = _p_sig;
  pattern_audio->init_from_string( _p_pattern );
  
  std::cout << "Playing " << _p_pattern << " at " << pattern_audio->_signature.bpm  << " bpm..." << std::endl;
  LOGMAIN( pattern_audio->str_dump() );
  
  pattern_audio->start();
  while (! should_exit) {
    pattern_audio->update();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Clean up before exit
  clear_globals();
  
  return 0;
}
// ***************************************************************************

