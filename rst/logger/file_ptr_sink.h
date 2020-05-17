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

#ifndef RST_LOGGER_FILE_PTR_SINK_H_
#define RST_LOGGER_FILE_PTR_SINK_H_

#include <cstdio>
#include <memory>
#include <mutex>

#include "rst/logger/sink.h"
#include "rst/macros/macros.h"
#include "rst/not_null/not_null.h"
#include "rst/type/type.h"

namespace rst {

// The class for sinking to a FILE* (can be stdout or stderr).
class FilePtrSink final : public Sink {
 public:
  using ShouldClose = Type<class ShouldCloseTag, bool>;

  // Saves the |file| pointer. If |should_close| is not set, doesn't close the
  // |file| pointer (e.g. stdout, stderr).
  explicit FilePtrSink(NotNull<std::FILE*> file,
                       ShouldClose should_close = ShouldClose(true));
  ~FilePtrSink() override;

  // Thread safe logging function.
  void Log(std::string_view message) override;

 private:
  // A RAII-wrapper around std::FILE.
  std::unique_ptr<std::FILE, void (*)(std::FILE*)> log_file_{
      nullptr, [](std::FILE* f) {
        if (f != nullptr)
          (void)std::fclose(f);
      }};

  const NotNull<std::FILE*> file_;

  // Mutex for thread-safe Log function.
  std::mutex mutex_;

  RST_DISALLOW_COPY_AND_ASSIGN(FilePtrSink);
};

}  // namespace rst

#endif  // RST_LOGGER_FILE_PTR_SINK_H_
