
// SIMD MACROS
//  written by William Nichols; Feb 2020
//  Copyright Diamond Light Source Ltd.
//
// This is a header-only library. There are unit-tests for the functions elsewhere.
//
// put the caps of your processor here to choose what's available.
// use gcc flag -mavx to use these intrinsics; gcc will then define __AVX__ for you.
#define AVX 


#ifdef AVX
#include <immintrin.h>
#endif

#include <cstdint>
#include <string>
#include <iostream>

#ifdef AVX

// avx is missing integer operations on 8i, so we may need 4i registers from SSE.

typedef __m256 SIMD8f;  // 8 floats
typedef __m128i SIMD4i; // 4 integers
typedef __m128i SIMD8i16; // 8x 16 bit integers
typedef __m256i SIMD8i;


#define SIMD_MEM_ALIGN 32

//intrinsic functions
// NB the preprocessor doesn't do overloading, so we need to have a lot of different names
// the ones here probably need extending more.
// warning! be careful to put brackets around pointers, as (SIMD8f*)p+8 is different from (p+8).
#define Store8f(p,x) _mm256_store_ps(p,x)
#define Store4f(p,x) _mm_store_ps(p,x)
#define Store4i(p,x) _mm_store_si128((SIMD4i*)(p),x)
#define Store8i(p,x) _mm256_store_si256((SIMD8i*)(p),x)
// must have 16-byte alignment
#define Load4i(p) _mm_load_si128((SIMD4i*)(p))
// must have 32-byte alignment
#define Load8i(p) _mm256_load_si256((SIMD8i*)(p))
#define Load8f(p) _mm256_load_ps(p)
// #define Set8f(x,y,z,w,a,b,c,d) _mm256_set_ps(x,y,z,w,a,b,c,d)
#define Set4f(a,b,c,d) _mm_set_ps(d,c,b,a)
#define Set4i(a,b,c,d) _mm_set_epi32(d,c,b,a)
// _mm256_set_m128 seems to be missing on linux
#define Set8f(hi,lo) _mm256_insertf128_ps(_mm256_castps128_ps256(hi),lo,0);
// 0 at the end means lo goes into the left
#define Set8i(lo,hi) _mm256_insertf128_si256(_mm256_castsi128_si256(lo),hi,1);
#define Interleave4f4f(x,y) Set8f(_mm_unpacklo_ps(x,y), _mm_unpackhi_ps(x,y))
// I have checked this shifts in zeros
#define ShiftRight4i(x,bits) _mm_srli_epi32(x, bits)
#define ShiftRight8i16(x,bits) _mm_srli_epi16(x, bits)
#define Convert4ito4f(x) _mm_cvtepi32_ps(x)
#define Convert8ito8f(x) _mm256_cvtepi32_ps(x)
#define Cast4ito4f(x) _mm_castsi128_ps(x)
#define Cast4fto4i(x) _mm_castps_si128(x)
#define Cast8fto8i(x) _mm256_castps_si256(x)
#define Cast8ito8f(x) _mm256_castsi256_ps(x)
// use _mm_permute_pd here
#define SwapHalves4i(x) Cast4fto4i(_mm_permute_ps(Cast4ito4f(x),0x4e))
// cvt_epu_ does zero extension!
#define ExtendLoto4i(x) _mm_cvtepu16_epi32(x)
// could use _mm_bsrli_si128 here byte shift right
#define ExtendHito4i(x) _mm_cvtepu16_epi32(SwapHalves4i(x))
#define Extend8i16to8i(x) Set8i(ExtendLoto4i(x), ExtendHito4i(x))
#define And8i(x,y) Cast8fto8i(_mm256_and_ps(Cast8ito8f(x), Cast8ito8f(y)))
#define And4i(x,y) _mm_and_si128(x,y)
#define Multiply8f(x,y) _mm256_mul_ps(x,y)
#define Add8f(x,y) _mm256_add_ps(x,y)
#define Add8i16(x,y) _mm_add_epi16(x,y)
#define Sub8f(x,y) _mm256_sub_ps(x,y)

#define SetAll8f(x) _mm256_set1_ps(x)
#define SetAll4f(x) _mm_set1_ps(x)
#define SetAll8i(x) _mm256_set1_epi32(x)
#define SetAll8i16(x) _mm_set1_epi16(x)

#define Equal8i(x,y) Cast8fto8i(_mm256_cmp_ps(Cast8ito8f(x),Cast8ito8f(y),_CMP_EQ_OQ))
#define SelectXorY8f(x,y,m) _mm256_blendv_ps(x,y,Cast8ito8f(m))

#define SetZero() _mm256_setzero_ps()
#define SetOnei16(x) _mm_set1_epi16(x)

#define SetZeroi(x) _mm256_setzero_epi32(x)


static inline void logRegister8f(std::string comment, const SIMD8f& x)
{
    float  __attribute__ ((aligned (32))) ptr[8];
    Store8f(ptr,x);

    std::cout << comment;
    for(int i=0;i<8;++i)
    {
        std::cout << " " << ptr[i];        
    }
    std::cout << std::endl;
}

static inline void logRegister4i(std::string comment, const SIMD4i& x)
{
    uint32_t  __attribute__ ((aligned (32))) ptr[8];
    Store4i(ptr,x);

    std::cout << comment;
    for(int i=0;i<4;++i)
    {
        std::cout << " " << ptr[i];        
    }
    std::cout << std::endl;
}

static inline void logRegister8i(std::string comment, const SIMD8i& x)
{
    uint32_t  __attribute__ ((aligned (32))) ptr[8];
    Store8i(ptr,x);

    std::cout << comment;
    for(int i=0;i<8;++i)
    {
        std::cout << " " << ptr[i];        
    }
    std::cout << std::endl;
}

#endif

#ifdef AVX2
#define Extend8i16To8i(x) _mm256_cvtepu16_epi32(x)
#endif

