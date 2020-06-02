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

#include "rst/task_runner/thread_pool_task_runner.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "rst/bind/bind_helpers.h"
#include "rst/stl/algorithm.h"

namespace chrono = std::chrono;

namespace rst {

TEST(ThreadPoolTaskRunner, IsTaskRunner) {
  const ThreadPoolTaskRunner task_runner(
      1, []() -> chrono::milliseconds { return chrono::milliseconds(0); });
  const TaskRunner& i_task_runner = task_runner;
  (void)i_task_runner;
}

TEST(ThreadPoolTaskRunner, InvalidPostTaskDelay) {
  ThreadPoolTaskRunner task_runner(
      1, []() -> chrono::milliseconds { return chrono::milliseconds(0); });
  EXPECT_DEATH(
      task_runner.PostDelayedTask(DoNothing(), chrono::milliseconds(-1)), "");
}

TEST(ThreadPoolTaskRunner, PostTaskInOrder) {
  std::mutex mtx;
  ThreadPoolTaskRunner task_runner(
      1, []() -> chrono::milliseconds { return chrono::milliseconds(0); });

  std::string str, expected;
  for (auto i = 0; i < 1000; i++) {
    task_runner.PostTask([i, &mtx, &str]() {
      std::lock_guard lock(mtx);
      str += std::to_string(i);
    });
    expected += std::to_string(i);
  }

  while (true) {
    std::lock_guard lock(mtx);
    if (str == expected)
      break;
  }
}

TEST(ThreadPoolTaskRunner, DestructorRunsPendingTasks) {
  std::string str, expected;

  {
    std::mutex mtx;
    ThreadPoolTaskRunner task_runner(
        1, []() -> chrono::milliseconds { return chrono::milliseconds(0); });

    for (auto i = 0; i < 1000; i++) {
      task_runner.PostTask([i, &mtx, &str]() {
        std::lock_guard lock(mtx);
        str += std::to_string(i);
      });
      expected += std::to_string(i);
    }
  }

  EXPECT_EQ(str, expected);
}

TEST(ThreadPoolTaskRunner, PostDelayedTaskInOrder) {
  std::mutex mtx;
  std::atomic<int> ms = 0;
  ThreadPoolTaskRunner task_runner(
      1, [&ms]() -> chrono::milliseconds { return chrono::milliseconds(ms); });

  std::string str, first_half;
  for (auto i = 0; i < 500; i++) {
    task_runner.PostDelayedTask(
        [i, &mtx, &str]() {
          std::lock_guard lock(mtx);
          str += std::to_string(i);
        },
        chrono::milliseconds(100));
    first_half += std::to_string(i);
  }

  auto expected = first_half;

  for (auto i = 500; i < 1000; i++) {
    task_runner.PostDelayedTask(
        [i, &mtx, &str]() {
          std::lock_guard lock(mtx);
          str += std::to_string(i);
        },
        chrono::milliseconds(200));
    expected += std::to_string(i);
  }

  {
    std::lock_guard lock(mtx);
    EXPECT_EQ(str, std::string());
  }

  ms = 100;
  while (true) {
    std::lock_guard lock(mtx);
    if (str == first_half)
      break;
  }

  ms = 200;
  while (true) {
    std::lock_guard lock(mtx);
    if (str == expected)
      break;
  }
}

TEST(ThreadPoolTaskRunner, PostTaskConcurrently) {
  std::mutex mtx;
  ThreadPoolTaskRunner task_runner(
      1, []() -> chrono::milliseconds { return chrono::milliseconds(0); });

  std::string str, expected;
  std::vector<std::thread> threads;
  static constexpr size_t kMaxThreadNumber = 10;
  threads.reserve(kMaxThreadNumber);
  for (size_t i = 0; i < kMaxThreadNumber; i++) {
    std::thread t([&task_runner, i, &mtx, &str]() {
      task_runner.PostTask([i, &mtx, &str]() {
        std::lock_guard lock(mtx);
        str += std::to_string(i);
      });
    });
    threads.emplace_back(std::move(t));
    expected += std::to_string(i);
  }

  for (auto& t : threads)
    t.join();

  c_sort(expected);
  while (true) {
    std::lock_guard lock(mtx);
    c_sort(str);
    if (str == expected)
      break;
  }
}

TEST(ThreadPoolTaskRunner, MultipleThreads) {
  for (size_t t = 1; t <= 24; t++) {
    std::mutex mtx;
    ThreadPoolTaskRunner task_runner(
        t, []() -> chrono::milliseconds { return chrono::milliseconds(0); });
    EXPECT_EQ(task_runner.threads_num(), t);

    std::string str, expected;
    for (auto i = 0; i < 100; i++) {
      task_runner.PostTask([i, &mtx, &str]() {
        std::lock_guard lock(mtx);
        str += std::to_string(i);
      });
      expected += std::to_string(i);
    }

    {
      std::mutex ending_task_mutex;
      std::condition_variable ending_task_cv;
      auto should_continue = false;

      task_runner.PostTask(
          [&ending_task_mutex, &ending_task_cv, &should_continue]() {
            std::lock_guard lock(ending_task_mutex);
            should_continue = true;
            ending_task_cv.notify_one();
          });

      std::unique_lock lock(ending_task_mutex);
      while (!should_continue)
        ending_task_cv.wait(lock);
    }

    c_sort(expected);
    while (true) {
      std::unique_lock lock(mtx);
      c_sort(str);
      if (str == expected)
        break;
    }
  }
}

}  // namespace rst
