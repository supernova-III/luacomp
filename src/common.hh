#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

using u8 = uint8_t;
using u32 = uint32_t;
using i32 = int32_t;
using i64 = int64_t;
using u64 = uint64_t;
using usize = size_t;
using f32 = float;
using f64 = double;
static_assert(sizeof(f32) == 4 && sizeof(f64) == 8);

#define Panic(fmt, ...)     \
  printf(fmt, __VA_ARGS__); \
  exit(EXIT_FAILURE);
