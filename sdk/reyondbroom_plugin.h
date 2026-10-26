// SPDX-License-Identifier: MIT
#ifndef REYONDBROOM_PLUGIN_H
#define REYONDBROOM_PLUGIN_H
#include <stdint.h>
#include <stddef.h>
#ifdef _WIN32
#define RB_PLUGIN_EXPORT __declspec(dllexport)
#else
#define RB_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
#define RB_PLUGIN_API_VERSION 1u
// All callbacks run on the editor thread. No STL, ImGui or engine structs cross
// this boundary. Buffers and strings remain owned by their caller.
typedef struct RBSettleOptionsV1 {
    double friction, bounce, impulse, seconds;
    uint32_t seed;
} RBSettleOptionsV1;
typedef struct RBHostV1 {
    uint32_t api_version, struct_size;
    double (*number)(const char *name);
    int (*string)(const char *name, char *out, size_t capacity);
    void (*set_number)(const char *name, double value);
    void (*set_string)(const char *name, const char *value);
    void (*command)(const char *text);
    void (*log)(const char *text);
    // Imports a MAP fragment at its original coordinates, with editor Undo.
    int (*import_map)(const char *text, size_t length);
    // 0=start, 1=poll, 2=apply, 3=cancel. Result: -1=error, 0=pending, 1=ready.
    int (*settle)(int operation, const RBSettleOptionsV1 *options);
    void (*text)(const char *text);
    void (*separator)(const char *label);
    int (*input_int)(const char *label, int *value);
    int (*input_float)(const char *label, float *value);
    int (*checkbox)(const char *label, int *value);
    int (*combo)(const char *label, int *value, const char *zero_separated_items);
    int (*button)(const char *label);
    void (*same_line)(void);
    void (*disabled)(int begin);
    void (*progress)(float fraction, const char *label);
} RBHostV1;
typedef struct RBPluginV1 {
    uint32_t api_version, struct_size;
    const char *id;
    void (*draw)(void);
    void (*tick)(void);
    int (*busy)(void);
    void (*shutdown)(void);
} RBPluginV1;
#ifdef __cplusplus
extern "C" {
#endif
// A plugin exports exactly this entry point and returns static metadata.
RB_PLUGIN_EXPORT const RBPluginV1 *ReyondBroomPluginInit(const RBHostV1 *host);
#ifdef __cplusplus
}
#endif
#endif
