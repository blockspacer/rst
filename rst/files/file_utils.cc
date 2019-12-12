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

#include "rst/files/file_utils.h"

#include <cstdio>
#include <memory>
#include <optional>
#include <utility>

#include "rst/format/format.h"
#include "rst/status/status_macros.h"

namespace rst {

char FileError::id_ = '\0';

FileError::FileError(std::string&& message) : message_(std::move(message)) {}

FileError::~FileError() = default;

const std::string& FileError::AsString() const { return message_; }

char FileOpenError::id_ = '\0';

FileOpenError::FileOpenError(std::string&& message)
    : ErrorInfo(std::move(message)) {}

FileOpenError::~FileOpenError() = default;

Status WriteFile(const NotNull<const char*> filename,
                 const std::string_view data) {
  std::unique_ptr<std::FILE, void (*)(std::FILE*)> file(
      std::fopen(filename.get(), "wb"), [](std::FILE* f) {
        if (f != nullptr)
          (void)std::fclose(f);
      });

  if (file == nullptr) {
    return MakeStatus<FileOpenError>(
        Format("Can't open file {}", filename.get()));
  }

  if (std::fwrite(data.data(), 1, data.size(), file.get()) != data.size())
    return MakeStatus<FileError>(Format("Can't write file {}", filename.get()));

  const auto raw_file = file.release();
  if (std::fclose(raw_file) != 0)
    return MakeStatus<FileError>(Format("Can't close file {}", filename.get()));

  return Status::OK();
}

Status WriteImportantFile(const NotNull<const char*> filename,
                          const std::string_view data) {
  const auto temp_filename = Format("{}._tmp_", filename.get());
  RST_TRY(WriteFile(temp_filename.c_str(), data));

  if (std::rename(temp_filename.c_str(), filename.get()) != 0) {
    return MakeStatus<FileError>(
        Format("Can't rename temp file {}", temp_filename));
  }

  return Status::OK();
}

StatusOr<std::string> ReadFile(const NotNull<const char*> filename) {
  std::unique_ptr<std::FILE, void (*)(std::FILE*)> file(
      std::fopen(filename.get(), "rb"), [](std::FILE* f) {
        if (f != nullptr)
          (void)std::fclose(f);
      });

  if (file == nullptr) {
    return MakeStatus<FileOpenError>(
        Format("Can't open file {}", filename.get()));
  }

  const auto get_file_size = [](const NotNull<FILE*> file)
      -> std::optional<long> {  // NOLINT(runtime/int)
    if (std::fseek(file.get(), 0, SEEK_END) != 0)
      return std::nullopt;

    const auto size = std::ftell(file.get());
    if (size == -1)
      return std::nullopt;

    return size;
  };

  static constexpr long kDefaultChunkSize =  // NOLINT(runtime/int)
      128 * 1024 - 1;
  auto chunk_size = get_file_size(file.get()).value_or(kDefaultChunkSize);
  if (chunk_size == 0)  // Some files return 0 bytes (/etc/*).
    chunk_size = kDefaultChunkSize;
  chunk_size++;

  std::rewind(file.get());

  std::string content;
  size_t bytes_read_so_far = 0;
  for (size_t bytes_read_this_pass = 0;
       !std::feof(file.get()) && !std::ferror(file.get());
       bytes_read_so_far += bytes_read_this_pass) {
    RST_DCHECK(content.size() == bytes_read_so_far);
    content.resize(bytes_read_so_far + static_cast<size_t>(chunk_size));
    bytes_read_this_pass =
        std::fread(content.data() + bytes_read_so_far, 1,
                   static_cast<size_t>(chunk_size), file.get());
  }

  if (std::ferror(file.get()))
    return MakeStatus<FileError>(Format("Can't read file {}", filename.get()));

  const auto raw_file = file.release();
  if (std::fclose(raw_file) != 0)
    return MakeStatus<FileError>(Format("Can't close file {}", filename.get()));

  content.resize(bytes_read_so_far);
  return content;
}

}  // namespace rst
