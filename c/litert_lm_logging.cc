// Copyright 2025 The ODML Authors.
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

#include <cstdarg>
#include <cstdio>
#include <string>

#include "absl/base/log_severity.h"  // from @com_google_absl
#include "absl/log/absl_log.h"  // from @com_google_absl
#include "absl/log/globals.h"  // from @com_google_absl
#include "absl/log/log_entry.h"  // from @com_google_absl
#include "absl/log/log_sink.h"  // from @com_google_absl
#include "absl/log/log_sink_registry.h"  // from @com_google_absl
#include "absl/synchronization/mutex.h"  // from @com_google_absl
#include "litert/c/internal/litert_logging.h"  // from @litert

namespace {

class CallbackLogSink final : public absl::LogSink {
 public:
  void SetCallback(LiteRtLmLogCallback callback, void* context) {
    absl::MutexLock lock(&mutex_);
    callback_ = callback;
    context_ = context;
  }

  void Send(const absl::LogEntry& entry) override {
    LiteRtLmLogCallback callback;
    void* context;
    {
      absl::MutexLock lock(&mutex_);
      callback = callback_;
      context = context_;
    }

    if (callback == nullptr) {
      return;
    }

    const std::string source_file(entry.source_filename());
    const std::string message(entry.text_message());
    callback(static_cast<int>(entry.log_severity()), source_file.c_str(),
             entry.source_line(), message.c_str(), context);
  }

 private:
  absl::Mutex mutex_;
  LiteRtLmLogCallback callback_ = nullptr;
  void* context_ = nullptr;
};

CallbackLogSink& GetCallbackLogSink() {
  static CallbackLogSink* sink = new CallbackLogSink();
  return *sink;
}

absl::Mutex callback_registration_mutex;
bool callback_registered = false;
absl::LogSeverityAtLeast previous_stderr_threshold =
    absl::LogSeverityAtLeast::kInfo;
LiteRtLogSeverity previous_litert_threshold = LITERT_INFO;

LiteRtLogSeverity ToLiteRtSeverity(int level) {
  switch (level) {
    case 0:
      return LITERT_INFO;
    case 1:
      return LITERT_WARNING;
    default:
      return LITERT_ERROR;
  }
}

}  // namespace

extern "C" {

void litert_lm_log(int severity, const char* file, int line, const char* format,
                   ...) {
  va_list ap;
  va_start(ap, format);
  // A reasonable buffer size for log messages.
  char buf[1024];
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  ABSL_LOG(LEVEL(severity)) << buf;
}

void litert_lm_set_min_log_level(int level) {
  absl::MutexLock lock(&callback_registration_mutex);
  absl::SetMinLogLevel(static_cast<absl::LogSeverityAtLeast>(level));
  LiteRtSetMinLoggerSeverity(LiteRtGetDefaultLogger(),
                             callback_registered ? LITERT_SILENT
                                                 : ToLiteRtSeverity(level));
}

void litert_lm_set_log_callback(LiteRtLmLogCallback callback, void* context) {
  absl::MutexLock lock(&callback_registration_mutex);
  CallbackLogSink& sink = GetCallbackLogSink();

  if (callback != nullptr) {
    sink.SetCallback(callback, context);
    if (!callback_registered) {
      previous_stderr_threshold = absl::StderrThreshold();
      LiteRtGetMinLoggerSeverity(LiteRtGetDefaultLogger(),
                                 &previous_litert_threshold);
      absl::AddLogSink(&sink);
      callback_registered = true;
    }
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfinity);
    LiteRtSetMinLoggerSeverity(LiteRtGetDefaultLogger(), LITERT_SILENT);
    return;
  }

  if (callback_registered) {
    absl::RemoveLogSink(&sink);
    absl::SetStderrThreshold(previous_stderr_threshold);
    LiteRtSetMinLoggerSeverity(LiteRtGetDefaultLogger(),
                               previous_litert_threshold);
    callback_registered = false;
  }
  sink.SetCallback(nullptr, nullptr);
}

}  // extern "C"
