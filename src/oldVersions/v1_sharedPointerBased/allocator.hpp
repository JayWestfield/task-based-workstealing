#pragma once
#include <oneapi/tbb/scalable_allocator.h>

template <typename T> using DefaultAllocator = tbb::scalable_allocator<T>;