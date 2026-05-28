#pragma once

#ifdef CPU_ONLY
struct float4 { float x, y, z, w; };
struct uchar4 { unsigned char x, y, z, w; };
#else
#include <cuda_runtime.h>
#endif
