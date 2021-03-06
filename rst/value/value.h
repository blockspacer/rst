// Copyright (c) 2019, Sergey Abbakumov
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

#ifndef RST_VALUE_VALUE_H_
#define RST_VALUE_VALUE_H_

#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rst/check/check.h"
#include "rst/macros/macros.h"
#include "rst/not_null/not_null.h"

namespace rst {

// A Chromium-like JSON Value class.

// This is a recursive data storage class intended for storing settings and
// other persistable data.
//
// A Value represents something that can be stored in JSON or passed to/from
// JavaScript. As such, it is not a generalized variant type, since only the
// types supported by JavaScript/JSON are supported.
//
// In particular this means that there is no support for int64_t or unsigned
// numbers. Writing JSON with such types would violate the spec. If you need
// something like this, either use a double or make a string value containing
// the number you want.
class Value {
 public:
  using String = std::string;
  using Array = std::vector<Value>;
  using Object = std::map<std::string, Value, std::less<>>;

  // Types supported by JSON.
  enum class Type : int8_t {
    kNull,
    kBool,
    kNumber,
    kString,
    kArray,
    kObject,
  };

  // Constructs the default value of a given type.
  explicit Value(Type type);

  Value() : type_(Type::kNull) {}
  explicit Value(bool value) : type_(Type::kBool), bool_(value) {}
  explicit Value(int32_t value) : Value(static_cast<int64_t>(value)) {}
  // Can store |2^53 - 1| at maximum since it's a max safe integer that can be
  // stored in JavaScript.
  explicit Value(int64_t value) : Value(static_cast<double>(value)) {
    RST_DCHECK(std::abs(value) <= kMaxSafeInteger);
  }

  explicit Value(double value) : type_(Type::kNumber), number_(value) {
    RST_DCHECK(std::isfinite(value) &&
               "Non-finite (i.e. NaN or positive/negative infinity) values "
               "cannot be represented in JSON");
  }

  // Provides const char* overload since otherwise it will be implicitly
  // converted to bool.
  explicit Value(const char* value) : Value(String(value)) {
    RST_DCHECK(value != nullptr);
  }

  explicit Value(String&& value)
      : type_(Type::kString), string_(std::move(value)) {}
  explicit Value(Array&& value)
      : type_(Type::kArray), array_(std::move(value)) {}
  explicit Value(Object&& value)
      : type_(Type::kObject), object_(std::move(value)) {}

  // Prevents Value(pointer) from accidentally producing a bool.
  explicit Value(void*) = delete;

  Value(Value&& other) noexcept;

  ~Value();

  Value& operator=(Value&& rhs) noexcept;

  // Creates an explicit copy.
  Value Clone() const;
  static Array Clone(const Array& array);
  static Object Clone(const Object& object);

  // Returns the type of the stored value.
  Type type() const { return type_; }

  // Returns true if the current value represents a given type.
  bool IsNull() const { return type() == Type::kNull; }
  bool IsBool() const { return type() == Type::kBool; }
  bool IsNumber() const { return type() == Type::kNumber; }
  bool IsInt64() const {
    return IsNumber() && (std::abs(number_) <= kMaxSafeInteger);
  }
  bool IsInt() const {
    return IsNumber() && (number_ >= std::numeric_limits<int>::min()) &&
           (number_ <= std::numeric_limits<int>::max());
  }
  bool IsString() const { return type() == Type::kString; }
  bool IsArray() const { return type() == Type::kArray; }
  bool IsObject() const { return type() == Type::kObject; }

  // These will all assert that the type matches.
  bool GetBool() const {
    RST_DCHECK(IsBool());
    return bool_;
  }
  int64_t GetInt64() const {
    RST_DCHECK(IsInt64());
    return static_cast<int64_t>(number_);
  }
  int GetInt() const {
    RST_DCHECK(IsInt());
    return static_cast<int>(number_);
  }
  double GetDouble() const {
    RST_DCHECK(IsNumber());
    return number_;
  }
  const String& GetString() const {
    RST_DCHECK(IsString());
    return string_;
  }
  String& GetString() {
    return const_cast<String&>(std::as_const(*this).GetString());
  }
  const Array& GetArray() const {
    RST_DCHECK(IsArray());
    return array_;
  }
  Array& GetArray() {
    return const_cast<Array&>(std::as_const(*this).GetArray());
  }
  const Object& GetObject() const {
    RST_DCHECK(IsObject());
    return object_;
  }
  Object& GetObject() {
    return const_cast<Object&>(std::as_const(*this).GetObject());
  }

  // Looks up |key| in the underlying dictionary. Asserts that the value is
  // object.
  Nullable<const Value*> FindKey(std::string_view key) const;
  Nullable<Value*> FindKey(std::string_view key) {
    return const_cast<Value*>(std::as_const(*this).FindKey(key).get());
  }

  // Similar to FindKey(), but it also requires the found value to have type
  // |type|. Asserts that the value is object.
  Nullable<const Value*> FindKeyOfType(std::string_view key, Type type) const;
  Nullable<Value*> FindKeyOfType(std::string_view key, Type type) {
    return const_cast<Value*>(
        std::as_const(*this).FindKeyOfType(key, type).get());
  }

  // These are convenience forms of FindKeyOfType(). Asserts that the value is
  // object.
  std::optional<bool> FindBoolKey(std::string_view key) const;
  std::optional<int64_t> FindInt64Key(std::string_view key) const;
  std::optional<int> FindIntKey(std::string_view key) const;
  std::optional<double> FindDoubleKey(std::string_view key) const;
  Nullable<const String*> FindStringKey(std::string_view key) const;
  Nullable<const Value*> FindArrayKey(std::string_view key) const {
    return FindKeyOfType(key, Type::kArray);
  }
  Nullable<const Value*> FindObjectKey(std::string_view key) const {
    return FindKeyOfType(key, Type::kObject);
  }

  // Looks up |key| in the underlying dictionary and sets the mapped value to
  // |value|. If |key| could not be found, a new element is inserted. A pointer
  // to the modified item is returned. Asserts that the value is object.
  NotNull<Value*> SetKey(std::string&& key, Value&& value);

  // Attempts to remove the value associated with |key|. In case of failure,
  // e.g. the |key| does not exist, false is returned and the underlying
  // dictionary is not changed. In case of success, |key| is deleted from the
  // dictionary and the method returns true. Asserts that the value is object.
  bool RemoveKey(std::string_view key);

  // Sets the |value| associated with the given |path| starting from this
  // object. A |path| has the form "<key>" or "<key>.<key>.[...]", where "."
  // indexes into the next Value down. Obviously, "." can't be used within a
  // key, but there are no other restrictions on keys. If the key at any step
  // of the way doesn't exist, or exists but isn't an Object, a new Value will
  // be created and attached to the path in that location. A pointer to the
  // modified item is returned. Asserts that the value is object.
  NotNull<Value*> SetPath(std::string_view path, Value&& value);

  // Finds the value associated with the given |path| starting from this
  // object. A |path| has the form "<key>" or "<key>.<key>.[...]", where "."
  // indexes into the next Value down. Asserts that the value is object.
  Nullable<const Value*> FindPath(std::string_view path) const;
  Nullable<Value*> FindPath(std::string_view path) {
    return const_cast<Value*>(std::as_const(*this).FindPath(path).get());
  }

 private:
  static constexpr int64_t kMaxSafeInteger =
      (int64_t{1} << std::numeric_limits<double>::digits) - 1;
  static_assert(kMaxSafeInteger == (int64_t{1} << 53) - 1);

  friend bool operator==(const Value& lhs, const Value& rhs);
  friend bool operator<(const Value& lhs, const Value& rhs);

  void MoveConstruct(Value&& other);
  void MoveAssign(Value&& rhs);
  void Cleanup();

  Type type_;
  union {
    bool bool_;
    double number_;
    String string_;
    Array array_;
    Object object_;
  };

  RST_DISALLOW_COPY_AND_ASSIGN(Value);
};

bool operator==(const Value& lhs, const Value& rhs);
inline bool operator!=(const Value& lhs, const Value& rhs) {
  return !(lhs == rhs);
}
bool operator<(const Value& lhs, const Value& rhs);
inline bool operator>(const Value& lhs, const Value& rhs) { return rhs < lhs; }
inline bool operator<=(const Value& lhs, const Value& rhs) {
  return !(rhs < lhs);
}
inline bool operator>=(const Value& lhs, const Value& rhs) {
  return !(lhs < rhs);
}

}  // namespace rst

#endif  // RST_VALUE_VALUE_H_
