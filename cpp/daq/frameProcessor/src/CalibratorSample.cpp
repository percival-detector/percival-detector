
#include "CalibratorSample.h"
#include "SIMDMacros.h"

#include <boost/version.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <sstream>

#if 106000 <= BOOST_VERSION
using namespace boost::placeholders;
#endif

// it's probably a bad idea to put this here!
static const int max_threads = 6;
static tbb::task_scheduler_init init(max_threads);

CalibratorSample::CalibratorSample(int rows, int cols)
{
    m_logger = log4cxx::Logger::getLogger("FP.SCalibrator");
    m_rows = rows;
    m_cols = cols;

    allocGainMem();
}

CalibratorSample::~CalibratorSample()
{

}

void CalibratorSample::getCMA(bool& on, int& firstCol)
{
  on = m_cmaFlag;
  firstCol = on ? m_cmaFirstCol : -1;
}

void CalibratorSample::setCMA(bool on, int firstCol)
{
    m_cmaFlag = false;
    if(on)
    {
      int endCol = firstCol + numCMACols;
      if(0 <= firstCol && endCol <= m_cols)
      {
          m_cmaFlag = on;
          m_cmaFirstCol = firstCol;
      }
      else
      {
          LOG4CXX_ERROR(m_logger, "CMA invalid column, frame has " << m_cols << "cols and you set cma start " << firstCol);
      }
    }
    LOG4CXX_INFO(m_logger, "Setting cma " << (m_cmaFlag?"on":"off"));
}

void CalibratorSample::processFrame(MemBlockI16& input, MemBlockF& output)
{
    for(int r=0;r<m_rows;++r)
    {
        processFrameRow(input, output, r);
    }
}

void CalibratorSample::processFrameP(MemBlockI16& input, MemBlockF& output)
{
    auto fn = boost::bind(&CalibratorSample::processFrameRowsTBB, this, &input, &output, _1);
    // blocked_range(begin, end, grain G)
    // Using simple_partitioner guarantees that ⌈G/2⌉ ≤ chunksize ≤ G. 
    tbb::parallel_for( tbb::blocked_range<int>(0,m_rows,150), fn, tbb::simple_partitioner());
}

void CalibratorSample::processFrameRowsTBB(MemBlockI16* pInput, MemBlockF* pOutput, tbb::blocked_range<int> rows)
{
    for(int r = rows.begin(); r<rows.end(); ++r)
    {
         processFrameRow(*pInput, *pOutput, r);
    }
}

float CalibratorSample::getCMAVal(MemBlockI16& gainBlock, MemBlockF& output, int row)
{
    // we calculate an average value across this range subject to the constraint that they are all G0
    const int row_start_idx = row * m_cols;
    float total = 0.0f;
    for(int col=m_cmaFirstCol; col < m_cmaFirstCol + numCMACols; ++col)
    {
        size_t pixel_index = row_start_idx + col;
        uint16_t gain = gainBlock.at(pixel_index);
        if(gain == 0)
        {
            total += output.at(pixel_index);
        }
        else
        {
            total = std::numeric_limits<float>::quiet_NaN();
        }
    }
    float cmaVal = total / numCMACols;
    if(m_debugRow == row)
    {
        LOG4CXX_INFO(m_logger, "CMA value for row " << row << ": " << cmaVal);
    }
    return cmaVal;
}

void CalibratorSample::applyCMA(MemBlockI16& gainBlock, MemBlockF& output, int row)
{
    // this does not need to be a member function
    const int row_start_idx = row * m_cols;
    float cmaVal = getCMAVal(gainBlock, output, row);

    for(int col=0;col<m_cols;++col)
    {
        size_t pixel_index = row_start_idx + col;
        // we only apply the cmaVal if the gain is zero
        uint16_t gain = gainBlock.at(pixel_index);
        if(gain == 0)
        {
            output.at(row,col) -= cmaVal;
        }
    }
}

void CalibratorSample::processFrameRow(MemBlockI16& input, MemBlockF& output, int row)
{
    float idealOf = 128.0f * 32.0f;
    const int row_start_idx = row * m_cols;
    for(int col=0;col<m_cols;++col)
    {
          // Decode
          size_t pixel_index = row_start_idx + col;
          uint16_t pixel = input.at(pixel_index);
          // ADC decoding
          register uint16_t gain = (pixel & 0x6000) >> 13;
          register uint16_t fine = (pixel & 0x1fe0) >> 5;
          register uint16_t coarse = (pixel & 0x001f);
          input.at(pixel_index) = gain;

          // ADC Combination; rename CADC
          register float valueADC = idealOf + ( m_Gc.at(pixel_index) * (coarse - m_Oc.at(pixel_index)) )
                                      + ( m_Gf.at(pixel_index) * (fine - m_Of.at(pixel_index)) );

          // this is unlikely
          if(m_debugRow == row && m_debugCol == col)
          {
                std::stringstream sstr;
                size_t pixel_index = row * m_cols + col;
                sstr << " at " << row << "," << col;
                sstr << " ADC value:" << valueADC;
                sstr << " 2-bit Gain value:" << gain;
                sstr << " 5-bit Crs value:" << coarse;
              //  sstr << " Reset pixel:" << m_resetFrame.at(pixel_index);
                if(gain==0)
                {
                    sstr << " Ped Value:" << m_Ped0.at(pixel_index);
                    sstr << " Gain Value:" << m_Gain0.at(pixel_index);
                }
                else if(gain==1)
                {
                    sstr << " Ped Value:" << m_Ped1.at(pixel_index) << std::endl;
                    sstr << " Gain Value:" << m_Gain1.at(pixel_index) << std::endl;
                }
                else if(gain==2)
                {
                    sstr << " Ped Value:" << m_Ped2.at(pixel_index) << std::endl;
                    sstr << " Gain Value:" << m_Gain2.at(pixel_index) << std::endl;
                }
                LOG4CXX_INFO(m_logger, sstr.str());
          }

          switch (gain) {
            case 0b00:
              // subtracting the reset is the CDS stage
              valueADC -= m_resetFrame.at(pixel_index);
              valueADC -= m_Ped0.at(pixel_index);
              valueADC *= m_Gain0.at(pixel_index);
              break;
            case 0b01:
              valueADC -= m_Ped1.at(pixel_index);
              valueADC *= m_Gain1.at(pixel_index);
              break;
            case 0b10:
              valueADC -= m_Ped2.at(pixel_index);
              valueADC *= m_Gain2.at(pixel_index);
              break;
            case 0b11:
              // if we get a lot of these, then we should review the efficiency of this
              // 3of7 mode sets 0b11 on the unused rows.
              valueADC *= m_Gain3;
              break;
            default:
              valueADC = 0;
              assert(false);
              break;
          }

          output.at(pixel_index) = valueADC;
    }

    if(m_cmaFlag)
        applyCMA(input, output, row);
}

#ifdef __AVX__

void CalibratorSample::processFrameRowSIMD(MemBlockI16& input, MemBlockF& output, int row)
{
    for(int c=0;c<input.cols();c+=8)
    {
        int curElt = row * input.cols() + c;
        uint16_t* pIn = input.data() + curElt;
        float* pOut = output.data() + curElt;
        SIMD8i16 in = Load4i(pIn);

        // could move these outside the loop?
        SIMD8i16 maskCoarse = SetAll8i16(0x1f);
        SIMD8i16 maskFine = SetAll8i16(0xff);
        SIMD8i16 maskK = SetAll8i16(0x03);
        SIMD8f idealOffset8f = SetAll8f(128.0f * 32.0f);
        SIMD8f zero8f = SetAll8f(0.0f);
        SIMD8f K3_8f = SetAll8f(m_Gain3);

        SIMD8i16 coarseStep1 = And4i(ShiftRight8i16(in, 0), maskCoarse);
        SIMD8i16 fineStep1 = And4i(ShiftRight8i16(in, 5), maskFine);
        SIMD8i16 inK = And4i(ShiftRight8i16(in, 13), maskK);

        Store4i(pIn, inK);
#ifdef __AVX2__
        SIMD8i coarseStep3 = Extend8i16To8i(coarseStep1);
        SIMD8i fineStep3 = Extend8i16To8i(fineStep1);
        SIMD8i inKStep2 = Extend8i16To8i(inK);       
#else
        SIMD4i loCoarseStep2 = ExtendLoto4i(coarseStep1);
        SIMD4i hiCoarseStep2 = ExtendHito4i(coarseStep1);
        SIMD4i loFineStep2 = ExtendLoto4i(fineStep1);
        SIMD4i hiFineStep2 = ExtendHito4i(fineStep1);
        SIMD8i coarseStep3 = Set8i(loCoarseStep2,hiCoarseStep2);
        SIMD8i fineStep3 = Set8i(loFineStep2,hiFineStep2);
        SIMD8i inKStep2 = Set8i(ExtendLoto4i(inK),ExtendHito4i(inK));
#endif

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

        SIMD8f resultADC8f = Add8f(coarseStep5, idealOffset8f);
        resultADC8f = Add8f(resultADC8f, fineStep5);

        SIMD8f P0_8f = Load8f(m_Ped0.data()+curElt);
        SIMD8f P1_8f = Load8f(m_Ped1.data()+curElt);
        SIMD8f P2_8f = Load8f(m_Ped2.data()+curElt);
        
        SIMD8f K0_8f = Load8f(m_Gain0.data()+curElt);
        SIMD8f K1_8f = Load8f(m_Gain1.data()+curElt);
        SIMD8f K2_8f = Load8f(m_Gain2.data()+curElt);
        SIMD8f reset = Load8f(m_resetFrame.data()+curElt);
        // we should rename a few things, but I'm just going to add
        // the reset to P0
        P0_8f = Add8f(P0_8f, reset);

        SIMD8i isK0 = Equal8i(inKStep2, SetAll8i(0x00));
        SIMD8i isK1 = Equal8i(inKStep2, SetAll8i(0x01));
        SIMD8i isK2 = Equal8i(inKStep2, SetAll8i(0x02));
        SIMD8i isK3 = Equal8i(inKStep2, SetAll8i(0x03));

        // default to set1 values for some good reason?
        SIMD8f K_8f = K1_8f;
        K_8f = SelectXorY8f(K_8f, K0_8f, isK0);
        K_8f = SelectXorY8f(K_8f, K2_8f, isK2);
        K_8f = SelectXorY8f(K_8f, K3_8f, isK3);

        SIMD8f P_8f = P1_8f;
        P_8f = SelectXorY8f(P_8f, P0_8f, isK0);
        P_8f = SelectXorY8f(P_8f, P2_8f, isK2);
        // P3 doesn't exist

        SIMD8f resultStep3 = Sub8f(resultADC8f, P_8f);

        SIMD8f resultStep4 = Multiply8f(resultStep3, K_8f);
        Store8f(pOut, resultStep4);
    }
}
#endif

void CalibratorSample::allocGainMem()
{
    m_Gc.init(m_logger, m_rows, m_cols);
    m_Oc.init(m_logger, m_rows, m_cols);
    m_Gf.init(m_logger, m_rows, m_cols);
    m_Of.init(m_logger, m_rows, m_cols);

    m_Gain0.init(m_logger, m_rows, m_cols);
    m_Gain1.init(m_logger, m_rows, m_cols);
    m_Gain2.init(m_logger, m_rows, m_cols);

    m_Ped0.init(m_logger, m_rows, m_cols);
    m_Ped1.init(m_logger, m_rows, m_cols);
    m_Ped2.init(m_logger, m_rows, m_cols);

    m_resetFrame.init(m_logger, m_rows, m_cols);
    // set the values to something nilpotent mainly for the benefit
    // of testing.
    m_resetFrame.setAll(0.0f);
    m_Ped0.setAll(0.0f);
    m_Ped1.setAll(0.0f);
    m_Ped2.setAll(0.0f);

    m_Gain0.setAll(1.0f);
    m_Gain1.setAll(1.0f);
    m_Gain2.setAll(1.0f);
}

int64_t CalibratorSample::loadADCGain(std::string filename)
{
    int64_t rc = 0;
    MemBlockD Gc, Oc, Gf, Of;

    std::string group = "/sample";
    // group should look like "/sample" or "/reset"

    // unfortunate mismatch: the calib data has the ref cols included, but
    // Percival drops them so the image is actually 32 cols smaller
    Gc.init(m_logger, FRAME_ROWS, FRAMER_COLS);
    Oc.init(m_logger, FRAME_ROWS, FRAMER_COLS);
    Gf.init(m_logger, FRAME_ROWS, FRAMER_COLS);
    Of.init(m_logger, FRAME_ROWS, FRAMER_COLS);

    rc |= Gc.loadFromH5(filename, group + "/coarse/slope", 0);
    rc |= Oc.loadFromH5(filename, group + "/coarse/offset", 0);
    rc |= Gf.loadFromH5(filename, group + "/fine/slope", 0);
    rc |= Of.loadFromH5(filename, group + "/fine/offset", 0);

    int coffset = FRAMER_COLS - m_cols;
    if(coffset==0 || coffset==32)
    {
        for(int r=0;r<m_rows;++r)
        {
            for(int c=0;c<m_cols;++c)
            {
                m_Gc.at(r,c) = Gc.at(r,c+coffset);
                m_Oc.at(r,c) = Oc.at(r,c+coffset);
                m_Gf.at(r,c) = Gf.at(r,c+coffset);
                m_Of.at(r,c) = Of.at(r,c+coffset);
            }
        }
    }
    else
    {
        LOG4CXX_ERROR(m_logger, "calibrator dimensions wrong: " << m_rows << "," << m_cols);
    }

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

int64_t CalibratorSample::loadLatGain(std::string filename)
{
    int64_t rc = 0;
    MemBlockD Ped0, Ped1, Ped2, Gain0, Gain1, Gain2;
    Ped0.init(m_logger, FRAME_ROWS, FRAMER_COLS);
    Ped1.init(m_logger, FRAME_ROWS, FRAMER_COLS);
    Ped2.init(m_logger, FRAME_ROWS, FRAMER_COLS);

    Gain0.init(m_logger, FRAME_ROWS, FRAMER_COLS);
    Gain1.init(m_logger, FRAME_ROWS, FRAMER_COLS);
    Gain2.init(m_logger, FRAME_ROWS, FRAMER_COLS);

    rc |= Ped0.loadFromH5(filename, "Pedestal_ADU", 0);
    rc |= Ped1.loadFromH5(filename, "Pedestal_ADU", 1);
    rc |= Ped2.loadFromH5(filename, "Pedestal_ADU", 2);

    rc |= Gain0.loadFromH5(filename, "e_per_ADU", 0);
    rc |= Gain1.loadFromH5(filename, "e_per_ADU", 1);
    rc |= Gain2.loadFromH5(filename, "e_per_ADU", 2);

    int coffset = FRAMER_COLS - m_cols;
    if(coffset==0 || coffset==32)
    {
        for(int r=0;r<m_rows;++r)
        {
            for(int c=0;c<m_cols;++c)
            {
                m_Ped0.at(r,c) = Ped0.at(r,c+coffset);
                m_Ped1.at(r,c) = Ped1.at(r,c+coffset);
                m_Ped2.at(r,c) = Ped2.at(r,c+coffset);

                m_Gain0.at(r,c) = Gain0.at(r,c+coffset);
                m_Gain1.at(r,c) = Gain1.at(r,c+coffset);
                m_Gain2.at(r,c) = Gain2.at(r,c+coffset);
            }
        }
    }
    else
    {
        LOG4CXX_ERROR(m_logger, "calibrator dimensions wrong: " << m_rows << "," << m_cols);
    }

    return rc;
}

void CalibratorSample::setResetFrame(MemBlockF& reset)
{
    m_resetFrame.init(m_logger, reset.rows(),reset.cols(),reset.data());
}



#ifdef __AVX__


void CalibratorSample::applyCMA_SIMD(MemBlockI16& gainBlock, MemBlockF& output, int row)
{
    const int row_start_idx = row * m_cols;
    float cmaVal = getCMAVal(gainBlock, output, row);

    SIMD8i zerosI = SetAll8i(0);
    SIMD8f zerosF = SetAll8f(0);
    SIMD8f cmaVals = SetAll8f(cmaVal);
    for(int col=0;col<m_cols;col+=8)
    {
        size_t pixel_index = row_start_idx + col;
        float* pixelP = &output.at(pixel_index);
        // we only apply the cmaVal if the gain is zero
        SIMD8i gain = Extend8i16to8i(Load4i(&gainBlock.at(pixel_index)));
        SIMD8i maskG0 = Equal8i(gain, zerosI);
        SIMD8f cmaIfG0 = SelectXorY8f(zerosF, cmaVals, maskG0);
        SIMD8f pixels = Load8f(pixelP);
        pixels = Sub8f(pixels, cmaIfG0);
        Store8f(pixelP, pixels);        
    }
}

#endif
