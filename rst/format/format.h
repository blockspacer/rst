// Copyright (c) 2016, Sergey Abbakumov
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef RST_FORMAT_FORMAT_H_
#define RST_FORMAT_FORMAT_H_

#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "rst/macros/macros.h"
#include "rst/not_null/not_null.h"

// This component is for efficiently performing string formatting.
//
// Unlike printf-style format specifiers, Format() functions do not need to
// specify the type of the arguments. Supported arguments following the format
// string, such as strings, string_views, ints, floats, and bools, are
// automatically converted to strings during the formatting process. See below
// for a full list of supported types.
//
// Format() does not allow you to specify how to format a value, beyond the
// default conversion to string. For example, you cannot format an integer in
// hex.
//
// The format string uses identifiers indicated by a {} like in Python.
//
// A '{{' or '}}' sequence in the format string causes a literal '{' or '}' to
// be output.
//
// Example:
//   std::string s = Format("{} purchased {} {}", "Bob", 5, "Apples");
//   assert(s == "Bob purchased 5 Apples");
//
// Supported types:
//   * std::string_view, std::string, const char*
//   * short, unsigned short, int, unsigned int, long, unsigned long, long long,
//     unsigned long long
//   * float, double, long double (printed as if %g is specified for printf())
//   * bool (printed as "true" or "false")
//   * char
//   * enums (printed as underlying integer type)
//
// If an invalid format string is provided, Format() asserts in a debug build.
namespace rst {
namespace internal {

class Arg {
 public:
  explicit Arg(bool value);
  explicit Arg(char value);
  explicit Arg(short value);
  explicit Arg(unsigned short value);
  explicit Arg(int value);
  explicit Arg(unsigned int value);
  explicit Arg(long value);
  explicit Arg(unsigned long value);
  explicit Arg(long long value);
  explicit Arg(unsigned long long value);
  explicit Arg(float value);
  explicit Arg(double value);
  explicit Arg(long double value);
  explicit Arg(std::string_view value);
  // Provides const char* overload since otherwise it will be implicitly
  // converted to bool.
  explicit Arg(const char* value);

  template <class T, class = typename std::enable_if<std::is_enum<T>{}>::type>
  explicit Arg(const T e)
      : Arg(static_cast<typename std::underlying_type<T>::type>(e)) {}

  // Prevents Arg(pointer) from accidentally producing a bool.
  Arg(void*) = delete;

  ~Arg();

  std::string_view view() const { return view_; }

 private:
  char buffer_[21];  // Can store 2^64 - 1 that is 18,446,744,073,709,551,615
                     // with '\0'.
  const std::string_view view_;

  RST_DISALLOW_COPY_AND_ASSIGN(Arg);
};

std::string FormatAndReturnString(NotNull<const char*> format,
                                  size_t format_size,
                                  Nullable<const Arg*> values, size_t size);

}  // namespace internal

template <size_t N>
inline std::string Format(const char (&format)[N]) {
  static_assert(N > 0);
  return internal::FormatAndReturnString(format, N - 1, nullptr, 0);
}

template <size_t N, class... Args>
inline std::string Format(const char (&format)[N], Args&&... args) {
  static_assert(N > 0);
  const internal::Arg values[] = {internal::Arg(std::forward<Args>(args))...};
  return internal::FormatAndReturnString(format, N - 1, values,
                                         std::size(values));
}

}  // namespace rst

#endif  // RST_FORMAT_FORMAT_H_