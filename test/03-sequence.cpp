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
  lp.concat( 1 );
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
  while (true) {
    lp.next();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

}

void test_expr()
{
  std::cout << "***** TEST_EXPR ***********************************" << std::endl;
  Expr nul_expr;
  auto res=nul_expr.is_empty();
  std::cout << "nul_expr.empty=" <<  std::boolalpha << res << std::endl;
}

void test_analyze()
{
  std::cout << "***** TEST_ANALYZE*********************************" << std::endl;
  Analyzer analyzer;

  std::pair<std::string, std::list<uint>> forms[] =
    { {"p2 + p1",   {2,1}},
      {" p2  +p1 ", {2,1}},
      {"p3  ", {3}},
      {"2xp1", {1,1}},
      {" 2 x  p3", {3,3}},
      {" p1 + 2 x p2", {1,2,2}},
      {" 2 x p2 + p1", {2, 2, 1}},
  };
  
  for( auto& f: forms) {
    std::cout << "****************************" << std::endl;
    std::cout << "__analyze '" << f.first << "'" << std::endl;
    auto res = analyzer.run( f.first );
    std::cout << "  => " << std::boolalpha << (res == f.second) << " res=" << res << std::endl;
  }
  
}

int main(int argc, char *argv[])
{
  test_expr();
  test_analyze();

  return 0;
}
