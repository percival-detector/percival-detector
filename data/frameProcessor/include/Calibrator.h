
#pragma once

#include "FrameMem.h"

// this is common code to the CalibratorReset & CalibratorSample, which have different algorithms.
// I think we could move this actually.
class Calibrator
{
public:

    MemBlockF m_Gc;
    MemBlockF m_Oc;
    MemBlockF m_Gf;
    MemBlockF m_Of;

    // the other member we could have here is numCMACols, but it's always 32.
    int m_rows, m_cols;

    // if you set these, you get some info on std::cout about the calculations at that point.
    int m_debugRow=-1, m_debugCol;
};


