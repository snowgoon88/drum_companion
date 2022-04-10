/* -*- coding: utf-8 -*- */

/** 
 * CLI DrumCompagnon
 * - cli arguments: sig, pattern,
 * - play repeated sound using sound_pattern,
 */

// Parsing command line options
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include<iostream>

#include <chrono>
#include <thread>

#include<pattern_audio.hpp>
#include<sound_engine.hpp>

// *************************************************************** App GLOBALS
Signature _p_sig;
std::string _p_pattern = "";

// ***************************************************************************
// ******************************************************************* options
// ***************************************************************************
void setup_options( int argc, char **argv )
{
  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "produce help message")
    ("sig,s", po::value<std::string>(), "signature as BPM:BeatxDivision")
    ("pattern,p", po::value<std::string>(), "pattern as 1x2x1x2x (according to Signature)")
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
  if (vm.count("pattern")) {
    _p_pattern = vm["sig"].as<std::string>();
  }
}

// ***************************************************************************
// ********************************************************************** MAIN
// ***************************************************************************
int main(int argc, char *argv[])
{
  setup_options( argc, argv );
  
  PatternAudio pa;
  pa._signature = _p_sig;

  if (_p_pattern != "") {
    pa.init_from_string( _p_pattern );
  }
  
  std::cout << "__PATTERN AUDIO" << std::endl;
  std::cout <<  pa.str_dump()  << std::endl;


  // *************************************************************** PlaySound
  SoundEngine sound_engine;
  auto idx_clave = sound_engine.add_sound( "ressources/claves_120ms.wav" );
  auto idx_cow = sound_engine.add_sound( "ressources/cowbell.wav" );

  // Dirty : loop to play sound
  for( unsigned int i = 0; i < 5; ++i) {
    std::cout << "Start play i=" << i << std::endl;
    sound_engine.play_sound( idx_clave );

    std::this_thread::sleep_for(std::chrono::milliseconds(780));

    sound_engine.play_sound( idx_cow );
    std::this_thread::sleep_for(std::chrono::milliseconds(390));
  }
  
  return 0;
}
// ***************************************************************************

