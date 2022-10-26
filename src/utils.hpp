/* -*- coding: utf-8 -*- */

#ifndef UTILS_HPP
#define UTILS_HPP

/** 
 * TODO
 */

#include <vector>
#include <list>
#include <iostream>      // cout, endl
#include <imgui.h>       // ImVec4 for colors

// --- for type_name ---
#include <type_traits>
#include <typeinfo>
#ifndef _MSC_VER
#   include <cxxabi.h>
#endif
#include <memory>
#include <string>
#include <cstdlib>

// ******************************************************************* loggers
// in order to define other loggers using
// #define LOG_TRUC
// #ifdef LOG_TRUC
// #  define LOGTRUC(msg) (LOG_BASE("[TRUC]", msg)
// #else
// #  define LOGTRUC(msg)
// #endif
#define LOG_BASE(head,msg) (std::cout << head << " " << msg << std::endl)


// ********************************************************** unused variables
// These macro can prevent "warning: unused variable" from compilator
#define UNUSED(expr) ((void)(expr))
#define USED_IN_MACRO(expr) ((void)(expr))

// ********************************************************************* Types
using uint = unsigned int;

// ******************************************************************* Globals
const ImVec4 CLEAR_COL = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
const ImVec4 RED_COL = ImVec4(1.0f, 0.0f, 0.0f, 1.00f);
const ImVec4 GREEN_COL = ImVec4(0.0f, 1.0f, 0.0f, 1.00f);
const ImVec4 YELLOW_COL = ImVec4(0.7f, 0.7f, 0.0f, 1.00f);

// **************************************************************** str_vector
std::ostream &operator<<(std::ostream &os,
                                const std::vector<uint> &t)
{
  os << "{ ";
  for( const uint& val: t) {
    os << val << ", ";
  }
  os << "}";
  
  return os;
}
// ****************************************************************** str_list
std::ostream &operator<<(std::ostream &os,
                                const std::list<uint> &t)
{
  os << "{ ";
  for( const uint& val: t) {
    os << val << ", ";
  }
  os << "}";
  
  return os;
}

// std::copy( pa._pattern_intervale.begin(),
//              pa._pattern_intervale.end(),
//              std::ostream_iterator<Note>(std::cout, " "));

// ***************************************************************************
// ******************************************************************** typeof
// ********************************************************************
/** see https://stackoverflow.com/questions/81870/is-it-possible-to-print-a-variables-type-in-standard-c
 *
 * print the TYPE of a variable
 * usage cout << type_name<TYPE>() 
 * more often cout << type_name<decltype(var)>()
 */
template <class T>
std::string
type_name()
{
    typedef typename std::remove_reference<T>::type TR;
    std::unique_ptr<char, void(*)(void*)> own
      (
#ifndef _MSC_VER
                abi::__cxa_demangle(typeid(TR).name(), nullptr,
                                           nullptr, nullptr),
#else
                nullptr,
#endif
                std::free
       );
    std::string r = own != nullptr ? own.get() : typeid(TR).name();
    if (std::is_const<TR>::value)
        r += " const";
    if (std::is_volatile<TR>::value)
        r += " volatile";
    if (std::is_lvalue_reference<T>::value)
        r += "&";
    else if (std::is_rvalue_reference<T>::value)
        r += "&&";
    return r;
}

// ***************************************************************************
// ******************************** ImGui::InputTextMultiline with std::string
// ***************************************************************************
// taken from imgui/misc/imgui_stdlib.h/cpp
namespace ImGui
{
  // ImGui::InputText() with std::string
  // Because text input needs dynamic resizing, we need to setup a callback to grow the capacity

  struct InputTextCallback_UserData
  {
    std::string*            Str;
    ImGuiInputTextCallback  ChainCallback;
    void*                   ChainCallbackUserData;
  };

  static int InputTextCallback(ImGuiInputTextCallbackData* data)
  {
    InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
      {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        std::string* str = user_data->Str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = (char*)str->c_str();
      }
    else if (user_data->ChainCallback)
      {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
      }
    return 0;
  }

  bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
  {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputText(label, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
  }
  
  bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
  {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;
    
    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputTextMultiline(label, (char*)str->c_str(), str->capacity() + 1, size, flags, InputTextCallback, &cb_user_data);
  }
} // namespace ImGui

#endif // UTILS_HPP
