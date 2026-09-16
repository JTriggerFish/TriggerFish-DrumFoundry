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
// Non-realtime modal editing. Output capacity is always 32 modes. The caller
// supplies all fields; generation uses the same C++ implementation as the UI.
typedef struct df_series {
  double fundamental, stretch, level, rolloff, turbulence, minimum, maximum;
  uint32_t count, harmonic_core, first, family, truncate_to_range;
} df_series;
typedef struct df_mode {
  double frequency, level, turbulence, allocation;
} df_mode;
DF_EXPORT int df_generate_series(const df_series *, df_mode *output,
                                 uint32_t *count);
// Input/output each hold count doubles (0..32); output may alias input.
// Returns 1 on success, 2 for an infeasible transformed range, 0 for an error.
// Output is untouched on either rejection or error.
DF_EXPORT int df_transform_series(const double *frequencies, uint32_t count,
                                  double pitch, double stretch, uint32_t core,
                                  double minimum, double maximum,
                                  double *output);
typedef struct df_strike {
  float strength, location, hardness, implement, contact_spread, constraint;
  uint32_t seed;
} df_strike;
// Creation/configuration/JSON functions are non-realtime. Returned strings
// remain owned by the library; copy them before the next string-returning call.
// Errors are thread-local, truncated to 511 bytes, and cleared by a successful
// operation (except df_last_error itself). Valid audio operations do not
// allocate.
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
