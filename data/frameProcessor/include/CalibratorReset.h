
#pragma once

#include "Calibrator.h"
#include "FrameMem.h"

#include <tbb/tbb.h>

#include <string>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <cassert>

/** This class is separate from the CalibratorSample because the algorithm is different.
* We prefer to have a different class to avoid branching like if(reset) ... else ...
* although I accept this means there is some code duplication.
*/

class CalibratorReset : public Calibrator
{
public:
    CalibratorReset(int rows, int cols);
    ~CalibratorReset();
    void processFrame(MemBlockI16& input, MemBlockF& output);
    void processFrameP(MemBlockI16& input, MemBlockF& output);
    int64_t loadADCGain(std::string filename);

// this are private really:
    void processFrameRow(MemBlockI16& input, MemBlockF& output, int row);
    void processFrameRow_SIMD(MemBlockI16& input, MemBlockF& output, int row);

    void processFrameRowSIMD(MemBlockI16& input, MemBlockF& output, int row);

    // had to use MemBlock* here because boost:bind didn't like references
    void processFrameRowsTBB(MemBlockI16* input, MemBlockF* output, tbb::blocked_range<int> rows);
    // rename allocFrameMem later
    void allocGainMem();

};


