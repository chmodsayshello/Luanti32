/**
 * @file common.h
 * @author chmodsayshello (chmodsayshello@hotmail.com)
 * @brief common.h defines common datatypes.
 * @date 2024-12-30
 */
#pragma once

#include <stdint.h>
/**
 * @brief Struct attribute to make struct members big endian
 */
#define BIGE __attribute__((scalar_storage_order("big-endian")))

/**
 * @struct v3s32
 * @brief The v3s32 struct represents a three dimensional vector of signed 32-bit
 * integers in memory
 *
 * @var v3s32::x
 * int x
 *
 * @var v3s32::y
 * int y
 *
 * @var v3s32::z
 * int z
 */
typedef struct v3s32 {
    int32_t x;
    int32_t y;
    int32_t z;
} v3s32;

/**
 * @struct v3f32
 * @brief The v3f32 struct represents a three dimensional vector of floats in memory.
 * 
 * @var v3f32::x
 * Float x
 *
 * @var v3f32::y
 * Float y
 *
 * @var v3f32::z
 * Float z
 */
typedef struct BIGE v3f32 {
    float x;
    float y;
    float z;
} v3f32;