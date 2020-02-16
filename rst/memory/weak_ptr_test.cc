// Copyright (c) 2018, Sergey Abbakumov
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

#include "rst/memory/weak_ptr.h"

#include <memory>
#include <string>
#include <utility>

#include <gtest/gtest.h>

namespace rst {
namespace {

struct Base {
  std::string member;
};

struct Derived : public Base {};

}  // namespace

TEST(WeakPtr, Basic) {
  auto data = 0;
  const WeakPtrFactory<int> factory(&data);
  const auto ptr = factory.GetWeakPtr();
  EXPECT_EQ(ptr.GetNullable(), &data);
}

TEST(WeakPtr, Comparison) {
  auto data = 0;
  const WeakPtrFactory<int> factory(&data);
  const auto ptr = factory.GetWeakPtr();
  const auto ptr2 = ptr;
  EXPECT_EQ(ptr.GetNullable(), ptr2.GetNullable());
}

TEST(WeakPtr, Move) {
  auto data = 0;
  const WeakPtrFactory<int> factory(&data);
  const auto ptr = factory.GetWeakPtr();
  auto ptr2 = factory.GetWeakPtr();
  const auto ptr3 = std::move(ptr2);
  EXPECT_EQ(ptr.GetNullable(), ptr3.GetNullable());
}

TEST(WeakPtr, OutOfScope) {
  WeakPtr<int> ptr;
  EXPECT_EQ(ptr.GetNullable(), nullptr);
  {
    auto data = 0;
    const WeakPtrFactory<int> factory(&data);
    ptr = factory.GetWeakPtr();
  }
  EXPECT_EQ(ptr.GetNullable(), nullptr);
}

TEST(WeakPtr, Multiple) {
  WeakPtr<int> a, b;
  {
    auto data = 0;
    const WeakPtrFactory<int> factory(&data);
    a = factory.GetWeakPtr();
    b = factory.GetWeakPtr();
    EXPECT_EQ(a.GetNullable(), &data);
    EXPECT_EQ(b.GetNullable(), &data);
  }
  EXPECT_EQ(a.GetNullable(), nullptr);
  EXPECT_EQ(b.GetNullable(), nullptr);
}

TEST(WeakPtr, MultipleStaged) {
  WeakPtr<int> a;
  {
    auto data = 0;
    const WeakPtrFactory<int> factory(&data);
    a = factory.GetWeakPtr();
    { const auto b = factory.GetWeakPtr(); }
    EXPECT_NE(a.GetNullable(), nullptr);
  }
  EXPECT_EQ(a.GetNullable(), nullptr);
}

TEST(WeakPtr, UpCast) {
  Derived data;
  const WeakPtrFactory<Derived> factory(&data);
  WeakPtr<Base> ptr = factory.GetWeakPtr();
  EXPECT_EQ(ptr.GetNullable(), &data);
}

TEST(WeakPtr, ConstructFromNullptr) {
  const WeakPtr<int> ptr(nullptr);
  EXPECT_EQ(ptr.GetNullable(), nullptr);
}

TEST(WeakPtr, AssignNullptr) {
  Derived data;
  const WeakPtrFactory<Derived> factory(&data);
  WeakPtr<Base> ptr = factory.GetWeakPtr();
  ptr = nullptr;
  EXPECT_EQ(ptr.GetNullable(), nullptr);
}

}  // namespace rst
