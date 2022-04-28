/* -*- coding: utf-8 -*- */

/** 
 * Test Looper and Sequence
 */

#include <looper.hpp>
#include <analyzer.hpp>
#include <iostream>
#include <chrono>
#include <thread>

void test_concat_pattern()
{
  // Generate two patterns
  Signature _p_sig {90, 4, 2};
  PatternAudio p1;
  p1._signature = _p_sig;
  p1.init_from_string( "2x1x1x1x" );
  std::cout << "__CREATED p1" << std::endl << p1.str_dump() << std::endl;
  PatternAudio p2;
  p2._signature = _p_sig;
  p2.init_from_string( "211x211x" );
  std::cout << "__CREATED p2" << std::endl << p1.str_dump() << std::endl;

  Looper lp;
  std::cout << "__CREATED lp" << std::endl << lp.str_dump() << std::endl;

  std::cout << "  concatenation with empty all_pattern should not work"  << std::endl;
  try {
    lp.concat( 1 );
  }
  catch (std::runtime_error e) {
    std::cout << e.what() << std::endl;
  }
  std::cout << "__AFTER CONCAT " << std::endl << lp.str_dump() << std::endl;

  std::cout << "__ADDING Patterns" << std::endl;
  lp.add( &p1 );
  lp.add( &p2 );
  lp.concat( 1 );
  std::cout << "__AFTER CONCAT 1" << std::endl << lp.str_dump() << std::endl;  
  lp.concat( 0 );
  std::cout << "__AFTER CONCAT 0" << std::endl << lp.str_dump() << std::endl;  
  lp.concat( 0 );
  std::cout << "__AFTER CONCAT 0" << std::endl << lp.str_dump() << std::endl;  

  lp.start();
  for( unsigned int i = 0; i < 2000; ++i) {
    lp.next();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

}

void test_parse()
{
  std::cout << "***** TEST_PARSER *********************************" << std::endl;
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

  Analyzer analyzer(&looper);

  
  std::pair<std::string, std::list<uint>> forms[] =
    {
     {"23", {}},
     {"0  1", {}},
     {"p0", {0}},
     {" p1 ", {1}},
     {"p3", {}},
     {"a0", {}},
     {"pa1", {}},
     {"((p0 )) ", {0}},
     {"p0 + p1", {0,1}},
     {"p0 + p3", {}},
     {" p1 +   p1 ", {1,1}},
     {"(p0 + p0)", {0,0}},
     {" p0 + P1 + p0 ", {0,1,0}},
     {" 2 x p1", {1,1}},
     {" 2 * p1", {1,1}},
     {" 3 x p2", {}},
     {" P1 x 2", {}},
     {" p1 x  p0", {}},
     {" (2 x p1) ", {1,1}},
     {" ((2 x p1)) ", {1,1}},
     {" ((2 x (p1))) ", {1,1}},      
     {"p0 / p1", {}}, 
     {"2 * p1 + p0", {1,1,0}},
     {" p1 + 2 x p0", {1,0,0}},
     {" 2xp1 + 3xp0 ", {1,1,0,0,0}},
     {" 2 x p1 x p0", {}},
     {" 2x3xp1", {1,1,1,1,1,1}},
     {"2xp1 + p0 + 2*p1", {1,1,0,1,1}},
     {" 2 x (p1 + p0)", {1,0,1,0}},
    };

  uint nb_fail = 0;
  uint nb_test = 0;
  for( auto& f: forms) {
    std::cout << "****************************" << std::endl;
    std::cout << "__analyze '" << f.first << "'" << std::endl;
    try {
      nb_test += 1;
      auto res = analyzer.parse( f.first );

      bool success = ( f.second == res );
      if (! success) {
        nb_fail += 1;
      }
      std::cout << "  => " << std::boolalpha << success << " res=" << res << std::endl;
    }
    catch (std::runtime_error& e) {
      std::cout << "**WRONG** " << e.what() << std::endl;

      bool success = (f.second == std::list<uint>{});
      if (! success) {
        nb_fail += 1;
      }
      std::cout << "  => " << std::boolalpha << success << std::endl;
    }
  }

  std::cout << "------------------------------------" << std::endl;
  std::cout << "  " << nb_fail << " failed / " << nb_test << " tests" << std::endl;
}

void test_looper_expression()
{
  std::cout << "***** TEST_PARSER *********************************" << std::endl;
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
}

int main(int argc, char *argv[])
{
  // test_concat_pattern(); // for 10 seconds
  // test_parse();
  test_looper_expression();
  
  return 0;
}
