#pragma once

#define POR_WINDOWS
#define POR_WITH_CUDA
#define POR_WITH_CUDA_DLL

#include <vector>

#pragma comment(lib, "visionworks.lib")

#if defined(POR_WINDOWS)
using u8		= unsigned __int8;
using u16		= unsigned __int16;
using u32		= unsigned __int32;
using u64		= unsigned __int64;

using i8		= __int8;
using i16		= __int16;
using i32		= __int32;
using i64		= __int64;

using f32		= float;
using f64		= double;

#elif defined(POR_LINUX)
using u8		= uint8_t;
using u16		= uint16_t;
using u32		= uint32_t;
using u64		= uint64_t;

using i8		= int8_t;
using i16		= int16_t;
using i32		= int32_t;
using i64		= int64_t;

using f32		= float;
using f64		= double;
#endif
