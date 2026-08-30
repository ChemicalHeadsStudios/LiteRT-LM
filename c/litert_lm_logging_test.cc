// Copyright 2026 The ODML Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "c/litert_lm_logging.h"

#include <string>

#include <gtest/gtest.h>
#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "litert/c/internal/litert_logging.h"

namespace {

struct CapturedLog {
  int severity = -1;
  std::string message;
};

void CaptureLog(int severity, const char*, int, const char* message,
                void* context) {
  auto* captured = static_cast<CapturedLog*>(context);
  captured->severity = severity;
  captured->message = message;
}

TEST(LiteRtLmLoggingTest, ForwardsNativeLogsToCallback) {
  CapturedLog captured;
  litert_lm_set_log_callback(&CaptureLog, &captured);

  litert_lm_log(1, __FILE__, __LINE__, "native message %d", 7);

  litert_lm_set_log_callback(nullptr, nullptr);
  EXPECT_EQ(captured.severity, 1);
  EXPECT_NE(captured.message.find("native message 7"), std::string::npos);
}

TEST(LiteRtLmLoggingTest, StopsForwardingAfterCallbackIsCleared) {
  CapturedLog captured;
  litert_lm_set_log_callback(&CaptureLog, &captured);
  litert_lm_log(0, __FILE__, __LINE__, "first message");
  litert_lm_set_log_callback(nullptr, nullptr);

  captured.message.clear();
  litert_lm_log(0, __FILE__, __LINE__, "second message");

  EXPECT_TRUE(captured.message.empty());
}

TEST(LiteRtLmLoggingTest, ReplacesAndRestoresStderrSink) {
  const absl::LogSeverityAtLeast original_threshold = absl::StderrThreshold();
  CapturedLog captured;

  litert_lm_set_log_callback(&CaptureLog, &captured);
  EXPECT_EQ(absl::StderrThreshold(), absl::LogSeverityAtLeast::kInfinity);

  litert_lm_set_log_callback(nullptr, nullptr);
  EXPECT_EQ(absl::StderrThreshold(), original_threshold);
}

TEST(LiteRtLmLoggingTest, SilencesLowerLevelLoggerWhileRedirecting) {
  litert_lm_set_min_log_level(0);
  LiteRtLogSeverity original_threshold;
  ASSERT_EQ(LiteRtGetMinLoggerSeverity(LiteRtGetDefaultLogger(),
                                       &original_threshold),
            kLiteRtStatusOk);
  EXPECT_EQ(original_threshold, LITERT_INFO);

  CapturedLog captured;
  litert_lm_set_log_callback(&CaptureLog, &captured);
  litert_lm_set_min_log_level(0);

  LiteRtLogSeverity redirected_threshold;
  ASSERT_EQ(LiteRtGetMinLoggerSeverity(LiteRtGetDefaultLogger(),
                                       &redirected_threshold),
            kLiteRtStatusOk);
  EXPECT_EQ(redirected_threshold, LITERT_SILENT);

  litert_lm_set_log_callback(nullptr, nullptr);
  LiteRtLogSeverity restored_threshold;
  ASSERT_EQ(LiteRtGetMinLoggerSeverity(LiteRtGetDefaultLogger(),
                                       &restored_threshold),
            kLiteRtStatusOk);
  EXPECT_EQ(restored_threshold, original_threshold);
}

}  // namespace
