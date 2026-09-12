#pragma once
#include <stdint.h>
#if defined(_WIN32)
#define DF_EXPORT __declspec(dllexport)
#else
#define DF_EXPORT __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct df_voice df_voice;
typedef struct df_strike {
  float strength, location, hardness, implement, contact_spread, constraint;
  uint32_t seed;
} df_strike;
// Creation/configuration/JSON functions are non-realtime. Returned strings
// remain owned by the library; copy them before the next string-returning call.
DF_EXPORT const char *df_last_error(void);
DF_EXPORT df_voice *df_create(float sample_rate, const char *document);
DF_EXPORT void df_destroy(df_voice *);
DF_EXPORT int df_configure(df_voice *, const char *document);
DF_EXPORT const char *df_document(df_voice *);
DF_EXPORT const char *df_descriptors(df_voice *);
DF_EXPORT const char *df_default_patch(const char *recipe);
DF_EXPORT int df_reset(df_voice *);
DF_EXPORT int df_default_strike(df_voice *, df_strike *);
DF_EXPORT int df_trigger(df_voice *, const df_strike *);
DF_EXPORT int df_set_mute(df_voice *, float amount);
DF_EXPORT int df_process(df_voice *, float *output, uint32_t frames);
#ifdef __cplusplus
}
#endif
