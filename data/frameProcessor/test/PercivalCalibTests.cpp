#define BOOST_TEST_MODULE "root_name"
// #define BOOST_TEST_MAIN

// note we only test CalibratorSample.
#include "CalibratorSample.h"

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

#include <random>
#include <iostream>


// this bitpacker is only used in the unittests, so doesn't need to be fast.
struct BitPacker
{
    void clear()
    {
        m_val = 0;
    }

    void setCoarse(uint16_t c)
    {
        uint16_t mask = 0x001f;
        c <<= 0;
        c &= mask;
        m_val &= ~mask;
        m_val |= c;
    }
    void setFine(uint16_t f)
    {
        uint16_t mask = 0x1fe0;
        f <<= 5;
        f &= mask;
        m_val &= ~mask;
        m_val |= f;
    }
    void setGain(uint16_t g)
    {
        uint16_t mask = 0x6000;
        g <<= 13;
        g &= mask;
        m_val &= ~mask;
        m_val |= g;
    }
    uint16_t getBits()
    {
        return m_val;
    }
    void setBits(uint16_t val)
    {
        m_val = val;
    }
    uint16_t m_val = 0;
};


// note there are two versions of the same function: processFrameRowSIMD and processFrameRow
// These unit-tests should pass on BOTH, but our strategy is to test one of them, and then
// check they both produce the same results. Although we can also do a search-replace.
// recall the equation we are testing is:
// res = IdOf + Gc * (c-Oc) + Gf * (f-Of)


// these k values should be non-zero
static const float k1 = 1.1f;
static const float k2 = 2.2f;
static const float k3 = 3.4f;
static const float k4 = 4.8f;
static const float k5 = 5.2f;
static const float k6 = 6.5f;
static const float k7 = 7.8f;
static const float k8 = 8.3f;
static const float smallPercent = 0.001f;
static const float idealOffset = 128.0 * 32.0f;

BOOST_AUTO_TEST_CASE(CalibratorCreateAndDestroy)
{
    CalibratorSample calibrator;
    
    BOOST_CHECK_CLOSE(1000.0F, 1000.001f, smallPercent);
    // alignment check
    BOOST_CHECK((uint64_t)calibrator.m_Gc.data()%32 == 0);
}

// this one checks that IdOf is ok.
BOOST_AUTO_TEST_CASE(CalibratorIdealOffset)
{
    int rows=1, cols=8;
    CalibratorSample calibrator(rows,cols);

    calibrator.m_Gc.at(0,0) = 0.0;
    calibrator.m_Oc.at(0,0) = k2;
    calibrator.m_Gf.at(0,0) = 0.0;
    calibrator.m_Of.at(0,0) = k4;
    calibrator.m_Gain0.at(0,0) = 1.0;

    BitPacker bp;
    bp.clear();
    bp.setCoarse(4); // random value
    bp.setFine(4);
    bp.setGain(0);

    MemBlockI16 input;
    MemBlockF output;

    input.init(rows,cols);
    output.init(rows,cols);

    input.at(0,0) = bp.getBits();

    calibrator.processFrameRow(input, output, 0);
    BOOST_CHECK_CLOSE(output.at(0,0), idealOffset, smallPercent);
}

// this one checks that the coarse value is processed ok
// then it checks that the fine value is processed ok
// then it checks the complete sum works out ok
// also known as ADC stage
BOOST_AUTO_TEST_CASE(CalibratorCoarseAndFine)
{
    int rows=1, cols=8;
    CalibratorSample calibrator(rows,cols);

    calibrator.m_Gc.at(0,0) = k1;
    calibrator.m_Oc.at(0,0) = k2;
    calibrator.m_Gf.at(0,0) = k3;
    calibrator.m_Of.at(0,0) = k4;
    calibrator.m_Gain0.at(0,0) = 1.0;

    BitPacker bp;
    bp.clear();
    bp.setCoarse(8);
    bp.setFine(150);
    bp.setGain(0);

    MemBlockI16 input;
    MemBlockF output;

    input.init(rows,cols);
    output.init(rows,cols);

    input.at(0,0) = bp.getBits();

    // check coarse alone
    calibrator.m_Gf.at(0,0) = 0.0;
    calibrator.processFrameRow(input, output, 0);
    BOOST_CHECK_CLOSE(output.at(0,0), idealOffset + k1 * (8.0f - k2), smallPercent);

    // check fine alone
    input.at(0,0) = bp.getBits();
    calibrator.m_Gc.at(0,0) = 0.0;
    calibrator.m_Gf.at(0,0) = k3;
    calibrator.processFrameRow(input, output, 0);
    BOOST_CHECK_CLOSE(output.at(0,0), idealOffset + k3 * (150.0f - k4), smallPercent);

    // now do both together:
    input.at(0,0) = bp.getBits();
    calibrator.m_Gc.at(0,0) = k1;
    calibrator.processFrameRow(input, output, 0);
    BOOST_CHECK_CLOSE(output.at(0,0), idealOffset + k1 * (8.0f - k2) + k3 * (150.0f - k4), smallPercent);
}

// this one checks that the cds-stage = subtract reset frame, is ok
BOOST_AUTO_TEST_CASE(CalibratorReset)
{
    int rows=1, cols=8;
    CalibratorSample calibrator(rows,cols);

    calibrator.m_Gc.at(0,0) = 0.0;
    calibrator.m_Gf.at(0,0) = 0.0;
    calibrator.m_Gc.at(0,1) = 0.0;
    calibrator.m_Gf.at(0,1) = 0.0;
    calibrator.m_resetFrame.at(0,0) = k8;

    BitPacker bp;
    bp.clear();
    bp.setGain(0);

    MemBlockI16 input;
    MemBlockF output;

    input.init(rows,cols);
    output.init(rows,cols);

    input.at(0,0) = bp.getBits();
    bp.setGain(1);
    input.at(0,1) = bp.getBits();

    float idOf = 128.0f * 32.0f;
    // check reset frame is subtracted
    calibrator.processFrameRow(input, output, 0);
    BOOST_CHECK_CLOSE(output.at(0,0), idOf - k8, smallPercent);
    // but not when gain = 1
    BOOST_CHECK_CLOSE(output.at(0,1), idOf, smallPercent);
}

// this one checks the Lat stage: subtract Ped and multiply
BOOST_AUTO_TEST_CASE(CalibratorLatPedGain)
{
    int rows=1, cols=8;
    CalibratorSample calibrator(rows,cols);

    calibrator.m_Gc.at(0,0) = 0.0;
    calibrator.m_Gf.at(0,0) = 0.0;
    calibrator.m_Gc.at(0,1) = 0.0;
    calibrator.m_Gf.at(0,1) = 0.0;
    calibrator.m_Ped0.at(0,0) = k1;
    calibrator.m_Ped1.at(0,1) = k2;
    calibrator.m_Ped2.at(0,2) = k3;
    calibrator.m_Gain0.at(0,0) = k4;
    calibrator.m_Gain1.at(0,1) = k5;
    calibrator.m_Gain2.at(0,2) = k6;

    BitPacker bp;
    bp.clear();
    bp.setGain(0);

    MemBlockI16 input;
    MemBlockF output;

    input.init(rows,cols);
    output.init(rows,cols);

    input.at(0,0) = bp.getBits();
    bp.setGain(1);
    input.at(0,1) = bp.getBits();
    bp.setGain(2);
    input.at(0,2) = bp.getBits();
    bp.setGain(3);
    input.at(0,3) = bp.getBits();

    float idOf = 128.0f * 32.0f;
    // check right things subtracted in each case
    calibrator.processFrameRow(input, output, 0);
    BOOST_CHECK_CLOSE(output.at(0,0), k4 * (idOf - k1), smallPercent);
    BOOST_CHECK_CLOSE(output.at(0,1), k5 * (idOf - k2), smallPercent);
    BOOST_CHECK_CLOSE(output.at(0,2), k6 * (idOf - k3), smallPercent);
    BOOST_CHECK(isnan(output.at(0,3)));

}

BOOST_AUTO_TEST_CASE(CalibratorAlgNanOk)
{
    int rows=1, cols=24;
    CalibratorSample calibrator(rows,cols);

    MemBlockI16 input;
    input.init(rows,cols);

    for(int c=0;c<cols;++c)
    {
        calibrator.m_Gc.at(0,c) = k1;
        calibrator.m_Oc.at(0,c) = k2;
        calibrator.m_Gf.at(0,c) = k3;
        calibrator.m_Of.at(0,c) = k4;
        calibrator.m_Gain0.at(0,c) = k5;
        calibrator.m_Gain1.at(0,c) = k6;
        calibrator.m_Gain2.at(0,c) = k7;
        calibrator.m_Gain3 = k8;

        input.at(0,c) = rand();
    }

    BitPacker bp;
    bp.clear();
    bp.setCoarse(1);
    bp.setFine(1);
    bp.setGain(0);

    input.at(0,0) = bp.getBits();
    calibrator.m_Gain0.at(0,0) = std::numeric_limits<float>::quiet_NaN();

    calibrator.m_Gc.at(0,1) = std::numeric_limits<float>::quiet_NaN();

    MemBlockF output1;
    output1.init(rows,cols);
    calibrator.processFrameRow(input, output1, 0);

    float lastOne = 0.0f;
    for(int c=0;c<cols;++c)
    {
        if( c==0 || c==1 )
        {
            BOOST_CHECK( isnan(output1.at(0,c)) );
        }
        else
        {
            BOOST_CHECK( !isnan(output1.at(0,c)) );
        }
    }
}

// this tests it when the cma value is not nan (all g0 on cma cols)
BOOST_AUTO_TEST_CASE(CalibratorCMA_real)
{
    int rows=1, cols=64;
    int col1 = 40;
    CalibratorSample calibrator(rows,cols);
    calibrator.setCMA(true, 0);

    MemBlockF pic;
    MemBlockI16 gain;
    pic.init(rows,cols);
    gain.init(rows,cols);
    gain.setAll(0);
    gain.at(0, col1) = 1;
    
    for(int c=0;c<cols;++c)
    {
        pic.at(0,c) = static_cast<float>(c);
    }

    calibrator.applyCMA(gain, pic, 0);

    for(int c=0;c<cols;++c)
    {
        if(c==col1)
        {
            // cma value not subtracted here cos G1
            BOOST_CHECK_CLOSE(pic.at(0,c), c, smallPercent);           
        }
        else
        {
            BOOST_CHECK_CLOSE(pic.at(0,c), c - (31.0f/2.0f), smallPercent);
        }
    }
}

// this tests it when the cma value is nan (not G0 on cma cols)
BOOST_AUTO_TEST_CASE(CalibratorCMA_nan)
{
    int rows=1, cols=64;
    int col1 = 40, col2 = 0;
    CalibratorSample calibrator(rows,cols);
    calibrator.setCMA(true, 0);

    MemBlockF pic;
    MemBlockI16 gain;
    pic.init(rows,cols);
    gain.init(rows,cols);
    gain.setAll(0);
    gain.at(0, col2) = 2;
    gain.at(0, col1) = 1;
    
    for(int c=0;c<cols;++c)
    {
        pic.at(0,c) = static_cast<float>(c);
    }

    calibrator.applyCMA(gain, pic, 0);

    for(int c=0;c<cols;++c)
    {
        if(c==col1 || c==col2)
        {
            // cma value not subtracted here cos G1
            BOOST_CHECK_CLOSE(pic.at(0,c), c, smallPercent);           
        }
        else
        {
            BOOST_CHECK(isnan(pic.at(0,c)));
        }
    }
}

// this one tests processFrameRowSIMD produces the same as processFrameRow for
// some random data. If they agree, they are both right because they are on
// different code paths. We can't test CMA because it requires gain 0 on its row.
BOOST_AUTO_TEST_CASE(CalibratorAlgSIMDSameAsNormal)
{
    const float percentDiff = 0.2;
    int rows=1, cols=24;
    CalibratorSample calibrator(rows,cols);

    MemBlockI16 input, input2;
    input.init(rows,cols);

    calibrator.m_resetFrame.setAll(k4);

    for(int c=0;c<cols;++c)
    {
        calibrator.m_Gc.at(0,c) = k1;
        calibrator.m_Oc.at(0,c) = k2;
        calibrator.m_Gf.at(0,c) = k3;
        calibrator.m_Of.at(0,c) = k4;

        calibrator.m_Ped0.at(0,c) = k3;
        calibrator.m_Ped1.at(0,c) = k6;
        calibrator.m_Ped2.at(0,c) = k7;

        calibrator.m_Gain0.at(0,c) = k5;
        calibrator.m_Gain1.at(0,c) = k6;
        calibrator.m_Gain2.at(0,c) = k7;
        calibrator.m_Gain3 = k8;

        input.at(0,c) = rand();
    }

    input2.clone(input);

    MemBlockF output1, output2;
    output1.init(rows,cols);
    output2.init(rows,cols);

    calibrator.processFrameRowSIMD(input, output1, 0);
    calibrator.processFrameRow(input2, output2, 0);

    float lastOne = 0.0f;
    for(int c=0;c<cols;++c)
    {
        {
            BOOST_CHECK_CLOSE(output1.at(0,c), output2.at(0,c), percentDiff);
            BOOST_CHECK(input.at(0,c) == input2.at(0,c));
            // just check it's not all the same garbage data
            BOOST_CHECK(lastOne != output1.at(0,c));
            lastOne = output1.at(0,c);
        }
    }
}


