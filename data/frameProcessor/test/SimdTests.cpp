#define BOOST_TEST_MODULE "root_name"

#include "SIMDMacros.h"

#include <boost/test/unit_test.hpp>

#include <iostream>


#if 0
BOOST_AUTO_TEST_CASE(BoostTestWorks)
{
    BOOST_CHECK_CLOSE(100.0,101.0,0.1);
    BOOST_CHECK(false);
    BOOST_CHECK_EQUAL(0,1);
}
#endif

BOOST_AUTO_TEST_CASE(OpLoad4i)
{
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) data[4] = {1,2,3,4};
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) result[4] = {};
    SIMD4i x;
    x = Load4i(data);

    Store4i(result, x);
    for(int i=0;i<4;++i)
    {
        BOOST_CHECK_EQUAL(result[i], i+1);
    }
}


BOOST_AUTO_TEST_CASE(OpLoad8i)
{
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) data[8];
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) result[8] = {};

    for(int i=0;i<8;++i)
    {
        data[i] = i;
    }
    SIMD8i x;
    x = Load8i(data);

    Store8i(result, x);
    for(int i=0;i<8;++i)
    {
        BOOST_CHECK_EQUAL(result[i], i);
    }
}

BOOST_AUTO_TEST_CASE(OpExtendto4i)
{
    uint16_t data[8];
    for(int i=0;i<8;++i)
        data[i] = i;
    data[0] = 0xffff;

    uint32_t result[4] = {};
    SIMD4i x;

    x = Load4i(data);
    x = ExtendLoto4i(x);
    Store4i(result, x);

    for(int i=0;i<4;++i)
    {
        BOOST_CHECK_EQUAL(result[i], data[i]);
    }

    x = Load4i(data);
    x = ExtendHito4i(x);
    Store4i(result, x);

    for(int i=0;i<4;++i)
    {
        BOOST_CHECK_EQUAL(result[i], data[i+4]);
    }
}

BOOST_AUTO_TEST_CASE(OpSet8i)
{
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) data[8];
    for(int i=0;i<8;++i)
        data[i] = i;

    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) result[8] = {};
    SIMD4i x,y;
    SIMD8i z;

    x = Load4i(data);
    y = Load4i(data+4);
    z = Set8i(x,y);

    Store8i(result, z);

    for(int i=0;i<8;++i)
    {
        BOOST_CHECK_EQUAL(result[i], data[i]);
    }  
}

BOOST_AUTO_TEST_CASE(SetAll8i16)
{
    uint16_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) result[8] = {};

    SIMD8i16 x;
    const uint16_t val = 5;

    x = SetAll8i16(val);

    Store4i(result, x);

    for(int i=0;i<8;++i)
    {
        BOOST_CHECK_EQUAL(result[i], val);
    }  
}

BOOST_AUTO_TEST_CASE(OpCast8ito8f)
{
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) data[8];
    for(int i=0;i<8;++i)
        data[i] = i;

    float  __attribute__ ((aligned (SIMD_MEM_ALIGN))) result[8];
    SIMD8i a;
    SIMD8f x;
    a = Load8i(data);
    x = Convert8ito8f(a);
    Store8f(result, x);

    for(int i=0;i<8;++i)
    {
        BOOST_CHECK_EQUAL(result[i],(float)i);
    } 
}

BOOST_AUTO_TEST_CASE(OpMultiply8f)
{
    float  __attribute__ ((aligned (SIMD_MEM_ALIGN))) data[8];
    float  __attribute__ ((aligned (SIMD_MEM_ALIGN))) result[8]={};
    for(int i=0;i<8;++i)
        data[i] = i;

    SIMD8f x,y;
    x = Load8f(data);

    y = Multiply8f(x,x);
    Store8f(result, y);
    for(int i=0;i<8;++i)
    {
        BOOST_CHECK_EQUAL(result[i],(float)i*i);
    } 
}


BOOST_AUTO_TEST_CASE(OpAdd8f)
{
    float  __attribute__ ((aligned (SIMD_MEM_ALIGN))) data[8];
    float  __attribute__ ((aligned (SIMD_MEM_ALIGN))) result[8]={};
    for(int i=0;i<8;++i)
        data[i] = i;

    SIMD8f x,y;
    x = Load8f(data);

    y = Add8f(x,x);
    Store8f(result, y);
    for(int i=0;i<8;++i)
    {
        BOOST_CHECK_EQUAL(result[i],(float)i+i);
    } 
}

BOOST_AUTO_TEST_CASE(OpEqual8i)
{
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) data[8];
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) data2[8];
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) result[8]={};

    for(int i=0;i<8;++i)
    {
      data[i] = i;
      data2[i] = i+1;
    }
    data2[0] = 0;

    SIMD8i a,b,c;
    a = Load8i(data);
    b = Load8i(data2);
    c = Equal8i(a,b);

    Store8i(result, c);

    for(int i=0;i<8;++i)
    {
        if(i==0)
        {
           BOOST_CHECK_EQUAL(result[i], 0xffffffffu);
        }
        else
        {
           BOOST_CHECK_EQUAL(result[i], 0);
        }
    }   
}

BOOST_AUTO_TEST_CASE(OpSelectXorY8f)
{
    uint32_t  __attribute__ ((aligned (SIMD_MEM_ALIGN))) mask[8];
    float  __attribute__ ((aligned (SIMD_MEM_ALIGN))) result[8]={};

    for(int i=0;i<8;++i)
    {
        mask[i] = i%2 ? 0xffffffffu:0;
    }
    BOOST_CHECK(mask[1]);
    BOOST_CHECK(false == mask[0]);

    SIMD8f x,y,z;
    SIMD8i m;
    const float yval(5.0), xval(-4.0);
    x = SetAll8f(xval);
    y = SetAll8f(yval);
    m = Load8i(mask);

    z = SelectXorY8f(x,y,m);

    Store8f(result, z);
    for(int i=0;i<8;++i)
    {
        BOOST_CHECK_EQUAL(result[i], mask[i]?yval:xval);
    }  
}



