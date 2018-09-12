/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include <grpcpp/alarm.h>
#include <grpcpp/completion_queue.h>

#include <gtest/gtest.h>

#include "test/core/util/test_config.h"

namespace grpc {
namespace {

TEST(AlarmTest, RegularExpiry) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  Alarm alarm;
  alarm.Set(&cq, grpc_timeout_seconds_to_deadline(1), junk);

  void* output_tag;
  bool ok;
  const CompletionQueue::NextStatus status =
      cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(2));

  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_TRUE(ok);
  EXPECT_EQ(junk, output_tag);
}

struct Completion {
  bool completed = false;
  std::mutex mu;
  std::condition_variable cv;
};

TEST(AlarmTest, CallbackRegularExpiry) {
  Alarm alarm;

  auto c = std::make_shared<Completion>();
  alarm.experimental().Set(
      std::chrono::system_clock::now() + std::chrono::seconds(1), [c](bool ok) {
        EXPECT_TRUE(ok);
        std::lock_guard<std::mutex> l(c->mu);
        c->completed = true;
        c->cv.notify_one();
      });

  std::unique_lock<std::mutex> l(c->mu);
  EXPECT_TRUE(c->cv.wait_until(
      l, std::chrono::system_clock::now() + std::chrono::seconds(10),
      [c] { return c->completed; }));
}

TEST(AlarmTest, CallbackZeroExpiry) {
  Alarm alarm;

  auto c = std::make_shared<Completion>();
  alarm.experimental().Set(grpc_timeout_seconds_to_deadline(0), [c](bool ok) {
    EXPECT_TRUE(ok);
    std::lock_guard<std::mutex> l(c->mu);
    c->completed = true;
    c->cv.notify_one();
  });

  std::unique_lock<std::mutex> l(c->mu);
  EXPECT_TRUE(c->cv.wait_until(
      l, std::chrono::system_clock::now() + std::chrono::seconds(10),
      [c] { return c->completed; }));
}

TEST(AlarmTest, CallbackNegativeExpiry) {
  Alarm alarm;

  auto c = std::make_shared<Completion>();
  alarm.experimental().Set(
      std::chrono::system_clock::now() + std::chrono::seconds(-1),
      [c](bool ok) {
        EXPECT_TRUE(ok);
        std::lock_guard<std::mutex> l(c->mu);
        c->completed = true;
        c->cv.notify_one();
      });

  std::unique_lock<std::mutex> l(c->mu);
  EXPECT_TRUE(c->cv.wait_until(
      l, std::chrono::system_clock::now() + std::chrono::seconds(10),
      [c] { return c->completed; }));
}

TEST(AlarmTest, MultithreadedRegularExpiry) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  void* output_tag;
  bool ok;
  CompletionQueue::NextStatus status;
  Alarm alarm;

  std::thread t1([&alarm, &cq, &junk] {
    alarm.Set(&cq, grpc_timeout_seconds_to_deadline(1), junk);
  });

  std::thread t2([&cq, &ok, &output_tag, &status] {
    status =
        cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(2));
  });

  t1.join();
  t2.join();
  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_TRUE(ok);
  EXPECT_EQ(junk, output_tag);
}

TEST(AlarmTest, DeprecatedRegularExpiry) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  Alarm alarm(&cq, grpc_timeout_seconds_to_deadline(1), junk);

  void* output_tag;
  bool ok;
  const CompletionQueue::NextStatus status =
      cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(2));

  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_TRUE(ok);
  EXPECT_EQ(junk, output_tag);
}

TEST(AlarmTest, MoveConstructor) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  Alarm first;
  first.Set(&cq, grpc_timeout_seconds_to_deadline(1), junk);
  Alarm second(std::move(first));
  void* output_tag;
  bool ok;
  const CompletionQueue::NextStatus status =
      cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(2));
  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_TRUE(ok);
  EXPECT_EQ(junk, output_tag);
}

TEST(AlarmTest, MoveAssignment) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  Alarm first;
  first.Set(&cq, grpc_timeout_seconds_to_deadline(1), junk);
  Alarm second(std::move(first));
  first = std::move(second);

  void* output_tag;
  bool ok;
  const CompletionQueue::NextStatus status =
      cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(2));

  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_TRUE(ok);
  EXPECT_EQ(junk, output_tag);
}

TEST(AlarmTest, RegularExpiryChrono) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  std::chrono::system_clock::time_point one_sec_deadline =
      std::chrono::system_clock::now() + std::chrono::seconds(1);
  Alarm alarm;
  alarm.Set(&cq, one_sec_deadline, junk);

  void* output_tag;
  bool ok;
  const CompletionQueue::NextStatus status =
      cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(2));

  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_TRUE(ok);
  EXPECT_EQ(junk, output_tag);
}

TEST(AlarmTest, ZeroExpiry) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  Alarm alarm;
  alarm.Set(&cq, grpc_timeout_seconds_to_deadline(0), junk);

  void* output_tag;
  bool ok;
  const CompletionQueue::NextStatus status =
      cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(1));

  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_TRUE(ok);
  EXPECT_EQ(junk, output_tag);
}

TEST(AlarmTest, NegativeExpiry) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  Alarm alarm;
  alarm.Set(&cq, grpc_timeout_seconds_to_deadline(-1), junk);

  void* output_tag;
  bool ok;
  const CompletionQueue::NextStatus status =
      cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(1));

  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_TRUE(ok);
  EXPECT_EQ(junk, output_tag);
}

TEST(AlarmTest, Cancellation) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  Alarm alarm;
  alarm.Set(&cq, grpc_timeout_seconds_to_deadline(2), junk);
  alarm.Cancel();

  void* output_tag;
  bool ok;
  const CompletionQueue::NextStatus status =
      cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(1));

  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_FALSE(ok);
  EXPECT_EQ(junk, output_tag);
}

TEST(AlarmTest, CallbackCancellation) {
  Alarm alarm;

  auto c = std::make_shared<Completion>();
  alarm.experimental().Set(
      std::chrono::system_clock::now() + std::chrono::seconds(10),
      [c](bool ok) {
        EXPECT_FALSE(ok);
        std::lock_guard<std::mutex> l(c->mu);
        c->completed = true;
        c->cv.notify_one();
      });
  alarm.Cancel();

  std::unique_lock<std::mutex> l(c->mu);
  EXPECT_TRUE(c->cv.wait_until(
      l, std::chrono::system_clock::now() + std::chrono::seconds(1),
      [c] { return c->completed; }));
}

TEST(AlarmTest, SetDestruction) {
  CompletionQueue cq;
  void* junk = reinterpret_cast<void*>(1618033);
  {
    Alarm alarm;
    alarm.Set(&cq, grpc_timeout_seconds_to_deadline(2), junk);
  }

  void* output_tag;
  bool ok;
  const CompletionQueue::NextStatus status =
      cq.AsyncNext(&output_tag, &ok, grpc_timeout_seconds_to_deadline(1));

  EXPECT_EQ(status, CompletionQueue::GOT_EVENT);
  EXPECT_FALSE(ok);
  EXPECT_EQ(junk, output_tag);
}

TEST(AlarmTest, CallbackSetDestruction) {
  auto c = std::make_shared<Completion>();
  {
    Alarm alarm;
    alarm.experimental().Set(
        std::chrono::system_clock::now() + std::chrono::seconds(10),
        [c](bool ok) {
          EXPECT_FALSE(ok);
          std::lock_guard<std::mutex> l(c->mu);
          c->completed = true;
          c->cv.notify_one();
        });
  }

  std::unique_lock<std::mutex> l(c->mu);
  EXPECT_TRUE(c->cv.wait_until(
      l, std::chrono::system_clock::now() + std::chrono::seconds(1),
      [c] { return c->completed; }));
}

TEST(AlarmTest, UnsetDestruction) {
  CompletionQueue cq;
  Alarm alarm;
}

}  // namespace
}  // namespace grpc

int main(int argc, char** argv) {
  grpc_test_init(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
