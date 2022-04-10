/* -*- coding: utf-8 -*- */

/** 
 * CLI DrumCompagnon
 * - cli arguments: sig, pattern,
 * - play repeated sound using sound_pattern,
 */

// Parsing command line options
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include <signal.h>          // C std lib (signal, sigaction, etc)

#include <pattern_audio.hpp>
#include <sound_engine.hpp>
#include <utils.hpp>        // loggers

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
uint _p_bpm = _p_sig.bpm;
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
void setup_options( int argc, char **argv )
{
  po::options_description desc("Options");
  std::string desc_pattern = std::string( "<string> pattern as " ) + _p_pattern + std::string(" (according to Signature divisions)");
  
  desc.add_options()
    ("help,h", "produce help message")
    ("bpm,b", po::value<uint>(&_p_bpm)->default_value(_p_bpm), "<uint> BPM ")
    ("sig,s", po::value<std::string>()->default_value("4x2"), "<string> signature as BeatxDivision")
    ("pattern,p", po::value<std::string>()->default_value(_p_pattern), desc_pattern.c_str() )
    ;

  // Options on command line 
  po::options_description cmdline_options;
  cmdline_options.add(desc);
  
  // Options that are 'after'
  po::positional_options_description pod;
  //pod.add( "data_file", 1);

  // Parse
  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).
	      options(desc).positional(pod).run(), vm);
    
    if (vm.count("help")) {
      std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
      std::cout << desc << std::endl;
      exit(1);
    }
    
    po::notify(vm);
  }
  catch(po::error& e)  { 
    std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
    std::cerr << desc << std::endl; 
    exit(2);
  }

  if (vm.count("sig")) {
    _p_sig.from_string( vm["sig"].as<std::string>() );
  }
  if (vm.count("bpm")) {
    _p_sig.bpm = vm["bpm"].as<uint>();
  }
  if (vm.count("pattern")) {
    _p_pattern = vm["pattern"].as<std::string>();
  }
}

// ***************************************************************************
// ********************************************************************** MAIN
// ***************************************************************************
int main(int argc, char *argv[])
{
  // To deal with Ctrl-C
  struct sigaction sig_int_handler;

  sig_int_handler.sa_handler = ctrlc_cbk;
  sigemptyset(&sig_int_handler.sa_mask);
  sig_int_handler.sa_flags = 0;

  sigaction(SIGINT, &sig_int_handler, NULL);

  // Args
  setup_options( argc, argv );
  
  // pattern_audio = new PatternAudio();
  // pattern_audio->_signature = _p_sig;

  // if (_p_pattern != "") {
  //   pattern_audio->init_from_string( _p_pattern );
  // }
  
  // std::cout << "__PATTERN AUDIO" << std::endl;
  // std::cout <<  pattern_audio->str_dump()  << std::endl;
  // delete pattern_audio;
  // pattern_audio = nullptr;
  // std::cout << "  delete pattern_audio" << std::endl;

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
  
  // pattern_audio->_signature = Signature { 94, 8, 2 };
  // auto clave_32 = std::string( "2xx1xx1xxx1x1xxx" );
  // pattern_audio->init_from_string( clave_32 );
  // std::cout << "Playing clave 3/2 pattern (" << clave_32 << ")";
  // std::cout << " at " << pattern_audio->_signature.bpm  << " bpm..." << std::endl;
  // LOGMAIN( "__PATTERN clave 3/2" );

  std::cout << "Playing " << _p_pattern << " at " << pattern_audio->_signature.bpm  << " bpm..." << std::endl;
  LOGMAIN( pattern_audio->str_dump() );

  
  pattern_audio->start();
  while (not should_exit) {
    pattern_audio->update();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Clean up before exit
  clear_globals();
  
  return 0;
}
// ***************************************************************************

