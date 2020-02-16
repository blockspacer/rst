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

#ifndef RST_TASK_RUNNER_THREAD_TASK_RUNNER_H_
#define RST_TASK_RUNNER_THREAD_TASK_RUNNER_H_

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "rst/macros/macros.h"
#include "rst/not_null/not_null.h"
#include "rst/task_runner/item.h"
#include "rst/task_runner/task_runner.h"

namespace rst {

// Task runner that is supposed to run tasks on the dedicated thread.
//
// Example:
//
//   std::function<std::chrono::milliseconds()> time_function = ...;
//   ThreadTaskRunner task_runner(std::move(time_function));
//   task_runner.Detach();
//   ...
//   std::function<void()> task = ...;
//   task_runner.PostTask(std::move(task));
//   ...
//
class ThreadTaskRunner : public TaskRunner {
 public:
  // Takes |time_function| that returns current time.
  explicit ThreadTaskRunner(
      std::function<std::chrono::milliseconds()>&& time_function);
  ~ThreadTaskRunner();

  void PostDelayedTask(std::function<void()>&& task,
                       std::chrono::milliseconds delay) final;
  // Detaches internal thread in order not to block in destructor.
  void Detach();

 private:
  class InternalTaskRunner {
   public:
    explicit InternalTaskRunner(
        std::function<std::chrono::milliseconds()>&& time_function);
    ~InternalTaskRunner();

    // Worker method.
    void WaitAndRunTasks();

   private:
    friend class ThreadTaskRunner;

    // Returns current time.
    const std::function<std::chrono::milliseconds()> time_function_;

    std::mutex thread_mutex_;
    std::condition_variable thread_cv_;
    bool should_exit_ = false;

    // Priority queue of tasks.
    std::vector<internal::Item> queue_;
    // Increasing task counter.
    uint64_t task_id_ = 0;

    // Used to not to allocate memory on every RunPendingTasks() call.
    std::vector<std::function<void()>> pending_tasks_;

    RST_DISALLOW_COPY_AND_ASSIGN(InternalTaskRunner);
  };

  const NotNull<std::shared_ptr<InternalTaskRunner>> task_runner_;
  std::thread thread_;

  RST_DISALLOW_COPY_AND_ASSIGN(ThreadTaskRunner);
};

}  // namespace rst

#endif  // RST_TASK_RUNNER_THREAD_TASK_RUNNER_H_
