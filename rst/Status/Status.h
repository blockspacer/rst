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

#ifndef RST_STATUS_STATUS_H_
#define RST_STATUS_STATUS_H_

#include <memory>
#include <string>
#include <utility>

#include "rst/Check/Check.h"
#include "rst/Macros/Macros.h"
#include "rst/Status/Status.h"

namespace rst {

class StatusAsOutParameter;

class ErrorInfoBase {
 public:
  ErrorInfoBase();
  virtual ~ErrorInfoBase();

  static const void* GetClassID() { return &id_; }

  virtual const std::string& AsString() const = 0;
  virtual const void* GetDynamicClassID() const = 0;

  virtual bool IsA(const void* class_id) const;

  template <class ErrorInfoT>
  bool IsA() const {
    return IsA(ErrorInfoT::GetClassID());
  }

 private:
  static char id_;

  RST_DISALLOW_COPY_AND_ASSIGN(ErrorInfoBase);
};

template <class T>
class ErrorInfo : public ErrorInfoBase {
 public:
  using ErrorInfoBase::ErrorInfoBase;

  static const void* GetClassID() { return &T::id_; }

  const void* GetDynamicClassID() const override { return &T::id_; }

  bool IsA(const void* class_id) const override {
    RST_DCHECK(class_id != nullptr);
    return class_id == GetClassID() || ErrorInfoBase::IsA(class_id);
  }

 private:
  RST_DISALLOW_COPY_AND_ASSIGN(ErrorInfo);
};

// A Google-like Status class for error handling.
class [[nodiscard]] Status {
 public:
  static Status OK() { return Status(); }

  // Sets the object not checked by default and to be the error object.
  Status(std::unique_ptr<ErrorInfoBase> error);

  // Sets the object not checked by default and moves rhs content.
  Status(Status&& rhs);

  ~Status();

  // Sets the object not checked by default and moves rhs content.
  Status& operator=(Status&& rhs);

  // Sets the object to be checked and returns whether the object is OK object.
  bool ok() { return !err(); }

  // Sets the object to be checked and returns whether the object is error
  // object.
  bool err() {
    set_was_checked(true);
    return error_ != nullptr;
  }

  const ErrorInfoBase& GetError() const;

  // Sets the object to be checked.
  void Ignore() { set_was_checked(true); }

 private:
  friend class StatusAsOutParameter;

  template <class T>
  friend class StatusOr;

  // Sets the object not checked by default and to be OK.
  Status();

#ifndef NDEBUG
  void set_was_checked(bool was_checked) { was_checked_ = was_checked; }
#else   // NDEBUG
  void set_was_checked(bool) {}
#endif  // NDEBUG

  // Information about the error. nullptr if the object is OK.
  std::unique_ptr<ErrorInfoBase> error_;

#ifndef NDEBUG
  // Whether the object was checked.
  bool was_checked_ = false;
#endif  // NDEBUG

  RST_DISALLOW_COPY_AND_ASSIGN(Status);
};

template <class Err, class... Args>
Status MakeStatus(Args&&... args) {
  return Status(std::make_unique<Err>(std::forward<Args>(args)...));
}

// A helper for Status used as out-parameters.
class StatusAsOutParameter {
 public:
  explicit StatusAsOutParameter(Status* status);
  ~StatusAsOutParameter();

 private:
  Status* status_ = nullptr;

  RST_DISALLOW_COPY_AND_ASSIGN(StatusAsOutParameter);
};

}  // namespace rst

#endif  // RST_STATUS_STATUS_H_
