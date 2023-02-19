/* -*- coding: utf-8 -*- */

/**
 * Test usage of Smart Pointers
 *
 * Semantics of "owning" vs "using".
 */
#include <string>
#include <iostream>
#include <memory>   // unique_ptr, make_unique<...>()

// Class for object
class Object {
public:
  std::string _name;

public:
  Object( const std::string& name ) : _name( name )
  {}
  void who() const { std::cout << "I'm " << _name << std::endl;}

};
// Class that will store the object
class Owner {
  public:
  std::unique_ptr<Object> _ptr;

  public:
  Owner( std::unique_ptr<Object> obj ) : _ptr(std::move(obj))
  {}
};

// It's better to use ref than shared/unique ptr in func arguments
// except IF ownership must be transfered/shared
void user_of_object( const Object& obj )
{
  std::cout << "__user_of_object" << std::endl;
  std::cout << "  Hello ";
  obj.who();
}

int main(int argc, char *argv[]) {

  Object car = Object( "car" );

  std::cout << "** func call ************************* " << std::endl;
  user_of_object( car );

  std::cout << "** using Owner *********************** " << std::endl;
  Owner o_bus = Owner( std::make_unique<Object>( "bus" ) );
  o_bus._ptr->who();

  std::cout << "** func of Owner ********************* " << std::endl;
  user_of_object( *(o_bus._ptr) );

  return 0;
}
