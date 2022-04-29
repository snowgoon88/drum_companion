/* -*- coding: utf-8 -*- */

/** 
 * Test of IO functions
 * - io of token + string
 */
#include <io_files.hpp>
#include <pattern_audio.hpp>
#include <looper.hpp>
#include <analyzer.hpp>

#include <iostream>
// uncomment to disable assert()
// #define NDEBUG
#include <cassert>



void test_string()
{
  std::cout << "****** test_string *************************" << std::endl;
  std::string strarg  = "Salut M'sieur";
  std::string strtok = "greeting";

  unsigned int uintarg  = 28;
  std::string uinttok = "age";

  
  std::cout << "__WRITE to file" << std::endl;
  std::ofstream ofile("try_io.data");
  write_token( ofile, strtok, strarg, "Il faut dire bonjour" );
  write_token( ofile, uinttok, uintarg, "");
  ofile.close();

  std::cout << "__READ from file" << std::endl;
  std::ifstream ifile("try_io.data");
  auto strread = read_string( ifile, strtok );
  std::cout << "  => [" << strread << "]"  << std::endl;
  assert( strread == strarg );
  auto uintread = read_uint( ifile, uinttok );
  std::cout << "  => [" << uintread << "]"  << std::endl;
  assert( uintread == uintarg );
  // should generate error
  try {
    read_uint( ifile, "not in file" );
    assert( false );
  }
  catch (std::runtime_error e) {
    assert( true );
  }
  ifile.close();
}

void test_pattern()
{
  std::cout << "****** test_pattern*************************" << std::endl;  
  std::cout << "__CREATE audio_pattern" << std::endl;
  Signature _p_sig {90, 3, 2};
  PatternAudio po;
  po._signature = _p_sig;
  po.init_from_string( "211x21" );

  std::cout << "__WRITE to file" << std::endl;
  std::ofstream ofile("try_io.data");
  po.write_to( ofile );
  ofile.close();

  std::cout << "__READ from file" << std::endl;
  std::ifstream ifile("try_io.data");
  PatternAudio pr;
  pr.read_from( ifile );
  std::cout << pr.str_dump() << std::endl;
  ifile.close();

  assert( po._timeline == pr._timeline );
}

void test_looper()
{
  std::cout << "****** test_looper *************************" << std::endl;
  Looper looper;
  std::cout << "__generate 2 patterns" << std::endl;
  Signature _p_sig {90, 4, 2};
  PatternAudio p1;
  p1._signature = _p_sig;
  p1.init_from_string( "2x1x1x1x" );
  std::cout << "__CREATED p1" << std::endl << p1.str_dump() << std::endl;
  PatternAudio p2;
  p2._signature = _p_sig;
  p2.init_from_string( "211x211x" );
  std::cout << "__CREATED p2" << std::endl << p1.str_dump() << std::endl;

  uint id_p1 = looper.add( &p1 );
  std::cout << "__Pattern P1 added as " << id_p1 << std::endl;
  uint id_p2 = looper.add( &p2 );
  std::cout << "__Pattern P2 added as " << id_p2 << std::endl;

  std::cout << "__ parsing 2xp1 + p0" << std::endl;
  Analyzer analyzer(&looper);
  auto res = analyzer.parse( "2xp1 + p0" );
  looper.set_sequence( res.begin(), res.end() );
  std::cout << looper.str_dump() << std::endl;

  std::cout << "__WRITE to file" << std::endl;
  std::ofstream ofile("try_io.data");
  looper.write_to( ofile );
  ofile.close();

  std::cout << "__READ from file" << std::endl;
  std::ifstream ifile("try_io.data");
  Looper loop_read;
  loop_read.read_from( ifile );
  ifile.close();
  std::cout << loop_read.str_dump() << std::endl;
}

int main(int argc, char *argv[])
{
  // test_string();
  // test_pattern();
  test_looper();
  
  return 0;
}
