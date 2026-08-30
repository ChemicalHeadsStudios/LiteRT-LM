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

#ifndef THIRD_PARTY_ODML_LITERT_LM_C_LITERT_LM_LOGGING_H_
#define THIRD_PARTY_ODML_LITERT_LM_C_LITERT_LM_LOGGING_H_

#ifndef LITERT_LM_C_API_EXPORT
#if defined(_WIN32)
#define LITERT_LM_C_API_EXPORT __declspec(dllexport)
#else
#define LITERT_LM_C_API_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LiteRtLmLogCallback)(int severity, const char* source_file,
                                    int source_line, const char* message,
                                    void* context);

LITERT_LM_C_API_EXPORT
void litert_lm_log(int severity, const char* file, int line, const char* format,
                   ...);

LITERT_LM_C_API_EXPORT
void litert_lm_set_min_log_level(int level);

// Routes native logs through callback and disables the default stderr sink.
// The callback may run from any LiteRT-LM thread. Pass NULL to restore stderr.
LITERT_LM_C_API_EXPORT
void litert_lm_set_log_callback(LiteRtLmLogCallback callback, void* context);

#ifdef __cplusplus
}
#endif

#endif  // THIRD_PARTY_ODML_LITERT_LM_C_LITERT_LM_LOGGING_H_
