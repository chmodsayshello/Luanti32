#pragma once

#include <stdint.h>
#define BIGE __attribute__((scalar_storage_order("big-endian")))

typedef struct v3s32 {
    int32_t x;
    int32_t y;
    int32_t z;
} v3s32;

typedef struct BIGE v3f32 {
    float x;
    float y;
    float z;
} v3f32;