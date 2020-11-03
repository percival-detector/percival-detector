
#include "CalibratorReset.h"
#include "SIMDMacros.h"

#include <boost/bind.hpp>
#include <iostream>


// it's probably a bad idea to put this here!
static const int max_threads = 16;
static tbb::task_scheduler_init init(max_threads);

CalibratorReset::CalibratorReset(int rows, int cols)
{
    m_logger = log4cxx::Logger::getLogger("FP.RCalibrator");
    m_rows = rows;
    m_cols = cols;
    allocGainMem();
}

CalibratorReset::~CalibratorReset()
{

}

void CalibratorReset::processFrame(MemBlockI16& input, MemBlockF& output)
{
    for(int i=0;i<m_rows;++i)
    {
        processFrameRow(input, output, i);
    }
}

void CalibratorReset::processFrameP(MemBlockI16& input, MemBlockF& output)
{
    auto fn = boost::bind(&CalibratorReset::processFrameRowsTBB, this, &input, &output, _1);
    tbb::parallel_for( tbb::blocked_range<int>(0,m_rows,50), fn, tbb::simple_partitioner());
}

void CalibratorReset::processFrameRowsTBB(MemBlockI16* pInput, MemBlockF* pOutput, tbb::blocked_range<int> rows)
{
    for(int r = rows.begin(); r<rows.end(); ++r)
    {
        processFrameRowSIMD(*pInput, *pOutput, r);
    }
}


inline void CalibratorReset::processFrameRow(MemBlockI16& input, MemBlockF& output, int row)
{
    // seems that we have 7 rows in the calib arrays
    float idealOf = 128.0f * 32.0f;
    int row_start_idx = row * m_cols;
    for(int col=0;col<m_cols;++col)
    {
          // Decode
          size_t pixel_index = row_start_idx + col;
          uint16_t pixel = input.at(pixel_index);
          // ADC decoding
          // Note for reset frames, the gain bits are invalid; should be 0 always.
          register uint16_t fine = (pixel & 0x1fe0) >> 5;
          register uint16_t coarse = (pixel & 0x001f);
          // ADC Correction
          register float decoded_input = idealOf + ( m_Gc.at(pixel_index) * (coarse - m_Oc.at(pixel_index)) )
                                      + ( m_Gf.at(pixel_index) * (fine - m_Of.at(pixel_index)) );

          output.at(pixel_index) = decoded_input;
    }
}


inline void CalibratorReset::processFrameRowSIMD(MemBlockI16& input, MemBlockF& output, int row)
{
    float idealOf = 128.0f * 32.0f;
    int row_start_idx = row * m_cols;
    for(int col=0;col<m_cols;col+=8)
    {
        int curElt = row_start_idx + col;
        uint16_t* pIn = input.data() + curElt;
        SIMD8i16 in = Load4i(pIn);

        // could move these outside the loop?
        SIMD8i16 maskCoarse = SetAll8i16(0x1f);
        SIMD8i16 maskFine = SetAll8i16(0xff);
        SIMD8f idealOffset8f = SetAll8f(128.0f * 32.0f);
        SIMD8f zero8f = SetAll8f(0.0f);

        SIMD8i16 coarseStep1 = And4i(ShiftRight8i16(in, 0), maskCoarse);
        SIMD8i16 fineStep1 = And4i(ShiftRight8i16(in, 5), maskFine);

        SIMD4i loCoarseStep2 = ExtendLoto4i(coarseStep1);
        SIMD4i hiCoarseStep2 = ExtendHito4i(coarseStep1);
        SIMD4i loFineStep2 = ExtendLoto4i(fineStep1);
        SIMD4i hiFineStep2 = ExtendHito4i(fineStep1);
        SIMD8i coarseStep3 = Set8i(loCoarseStep2,hiCoarseStep2);
        SIMD8i fineStep3 = Set8i(loFineStep2,hiFineStep2);

        SIMD8f coarsef = Convert8ito8f(coarseStep3);
        SIMD8f finef = Convert8ito8f(fineStep3);

        SIMD8f Gc8f = Load8f(m_Gc.data()+curElt);
        SIMD8f Oc8f = Load8f(m_Oc.data()+curElt);
        SIMD8f Gf8f = Load8f(m_Gf.data()+curElt);
        SIMD8f Of8f = Load8f(m_Of.data()+curElt);

        SIMD8f coarseStep4 = Sub8f(coarsef, Oc8f);
        SIMD8f fineStep4 = Sub8f(finef, Of8f);

        SIMD8f coarseStep5 = Multiply8f(coarseStep4, Gc8f);
        SIMD8f fineStep5 = Multiply8f(fineStep4, Gf8f);

        SIMD8f result8f = Add8f(coarseStep5, idealOffset8f);
        result8f = Add8f(result8f, fineStep5);

        Store8f(&output.at(curElt), result8f);
    }
}

void CalibratorReset::allocGainMem()
{
    m_Gc.init(m_logger, m_rows, m_cols);
    m_Oc.init(m_logger, m_rows, m_cols);
    m_Gf.init(m_logger, m_rows, m_cols);
    m_Of.init(m_logger, m_rows, m_cols);
}


int64_t CalibratorReset::loadADCGain(std::string filename)
{
    int64_t rc = 0;

    std::string group = "/reset";
    // group should look like "/sample" or "/reset"

    m_Gc.loadFromH5(filename, group + "/coarse/slope", 0);
    m_Oc.loadFromH5(filename, group + "/coarse/offset", 0);
    m_Gf.loadFromH5(filename, group + "/fine/slope", 0);
    m_Of.loadFromH5(filename, group + "/fine/offset", 0);

   // listen up folks! The equation that they want is this:
   // idOf = 128.0*32;
   // idSl = idOf / 1.5;
   // result = idOf + (idSl / Sc[r,c]) * (coarse_adc[r,c] + 1.0 - Oc[r,c]) - (idSl / Sf[r,c]) * (fine_adc[r,c] - Of[r,c]);
   //
   // so we are going to alter the values in the Gc, Gf to make this easier
   // the equation we apply is:
   // res = idOf + Gc * (coarse - Oc) + Gf * (fine - Of)

   float k = 128.0f * 32.0f / 1.5f;
   for(int r=0;r<m_rows;++r)
   {
       for(int c=0;c<m_cols;++c)
       {
           m_Gc.at(r,c) = k / m_Gc.at(r,c);
           m_Gf.at(r,c) = -1.0f * k / m_Gf.at(r,c);
           m_Oc.at(r,c) -= 1.0f;
       }
   }

   return rc;
}

