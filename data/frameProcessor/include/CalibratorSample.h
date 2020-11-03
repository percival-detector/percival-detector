
#pragma once


#include "Calibrator.h"

#include <tbb/tbb.h>

#include <string>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <cassert>

/** This class does all the work in our calibration algorithm. We need to use a class
* instead of a function here because there are several arrays of calibration data that
* are permanent, so we need to store them as member variables. It also gives us a chance
* to have a log4cxx which is standard.
*/

class CalibratorSample : public Calibrator
{
public:
    CalibratorSample() {};
    CalibratorSample(int rows, int cols);
    ~CalibratorSample();
    // these functions will destroy the data in input
    void processFrame(MemBlockI16& input, MemBlockF& output);
    void processFrameP(MemBlockI16& input, MemBlockF& output);
    int64_t loadADCGain(std::string filename);
    int64_t loadLatGain(std::string filename);
    // this does no copying, so keep reset alive yourself.
    void setResetFrame(MemBlockF& reset);
    // use this to turn on cma; firstCol is only used when it's ON. cma is off by default.
    void setCMA(bool on, int firstCol)
    {
        m_cmaFlag = on;
        m_cmaFirstCol = firstCol;
    }

// this are private really, but the testing needs to get hold of them!
    // this can return NaN.
    float getCMAVal(MemBlockI16& input, MemBlockF& output, int row);
    void processFrameRow(MemBlockI16& input, MemBlockF& output, int row);
    void applyCMA(MemBlockI16& gain, MemBlockF& inout, int row);

    void processFrameRowSIMD(MemBlockI16& input, MemBlockF& output, int row);
    void applyCMA_SIMD(MemBlockI16& gain, MemBlockF& inout, int row);

    // had to use MemBlock* here because boost:bind didn't like references
    void processFrameRowsTBB(MemBlockI16* input, MemBlockF* output, tbb::blocked_range<int> rows);
    // rename allocFrameMem later
    void allocGainMem();

    MemBlockF m_Ped0;
    MemBlockF m_Ped1;
    MemBlockF m_Ped2;

    MemBlockF m_Gain0;
    MemBlockF m_Gain1;
    MemBlockF m_Gain2;

    float m_Gain3 = std::numeric_limits<float>::quiet_NaN();
    MemBlockF m_resetFrame;

    bool m_cmaFlag = false;
    int m_cmaFirstCol = 0;
};


