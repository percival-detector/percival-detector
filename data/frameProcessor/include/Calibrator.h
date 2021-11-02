
#pragma once

#include "FrameMem.h"
#include <log4cxx/logger.h>

// this is common code to the CalibratorReset & CalibratorSample, which have different algorithms.
// I think we could move this actually.
class Calibrator
{
public:

    MemBlockF m_Gc;
    MemBlockF m_Oc;
    MemBlockF m_Gf;
    MemBlockF m_Of;

    int m_rows, m_cols;

    // if you set these, you get some info about the calculations at that point.
    int m_debugRow=-1, m_debugCol;

protected:
    log4cxx::LoggerPtr m_logger;

};


