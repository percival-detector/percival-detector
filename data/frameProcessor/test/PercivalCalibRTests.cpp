#define BOOST_TEST_MODULE "root_name"
// #define BOOST_TEST_MAIN

#include "CalibratorSample.h"
#include "CalibratorReset.h"

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

#include <random>
#include <string>
#include <iostream>
#include <chrono>


//* The test files for this come from Alessandro Marras at Desy 16/1/20. They are available at:
/* 
/* DLS: see pathToTestFiles below
**/

static const float smallPercent = 1.0f;
static const int rows = 1484;
static const int cols = 1440;


BOOST_AUTO_TEST_CASE(LoadFromH5uint16Frame1)
{
    std::string pathToTestFiles = "/dls/detectors/Percival/test_data/LATcorrectionExample/";
    MemBlockI16 block;
    block.init(rows, cols);
    block.loadFromH5(pathToTestFiles + "dataElaboration_example2_step1_2Img_DLSraw.h5", "/reset", 0);

    BOOST_CHECK_EQUAL(block.at(0,36), 3602);

    block.loadFromH5(pathToTestFiles + "dataElaboration_example2_step1_2Img_DLSraw.h5", "/reset", 1);

    BOOST_CHECK_EQUAL(block.at(0,36), 3538);
}

BOOST_AUTO_TEST_CASE(LoadFromH5float)
{
    std::string pathToTestFiles("/dls/detectors/Percival/test_data/CalibRTests/");
    MemBlockF block;
    block.init(rows, cols);
    block.loadFromH5(pathToTestFiles + "BSI04_Tminus20_dmuxSELsw_2019.11.20_ADCcor.h5", "/sample/coarse/offset", 0);

    BOOST_CHECK_CLOSE(block.at(0,36), 51.089, smallPercent);
    BOOST_CHECK_CLOSE(block.at(1483,1435), 52.1033, smallPercent);

    block.loadFromH5(pathToTestFiles + "example_1_1Img_ADU.h5", "/sample", 0);

    BOOST_CHECK_CLOSE(block.at(0,34), -1535.9, smallPercent);
    BOOST_CHECK_CLOSE(block.at(1483,1432), -287.8, smallPercent);
}

BOOST_AUTO_TEST_CASE(LoadFromH5uint16)
{
    std::string pathToTestFiles("/dls/detectors/Percival/test_data/CalibRTests/");
    MemBlockI16 block;
    block.init(rows, cols);
    block.loadFromH5(pathToTestFiles + "example_1_1Img_DLSraw.h5", "/sample", 0);

    BOOST_CHECK_EQUAL(block.at(1313,1313), 3057);
    BOOST_CHECK_EQUAL(block.at(0,50), 12201);
    BOOST_CHECK_EQUAL(block.at(1313,1426), 4339);

}

BOOST_AUTO_TEST_CASE(calibrationADC)
{
    std::string pathToTestFiles("/dls/detectors/Percival/test_data/CalibRTests/");
    CalibratorSample calibratorS(rows, cols);

    calibratorS.loadADCGain(pathToTestFiles + "BSI04_Tminus20_dmuxSELsw_2019.11.20_ADCcor.h5");

    // we're not doing the cda stage at this point, so by setting 1.0f we make it nilpotent
    calibratorS.m_Gain3 = 1.0f;

    MemBlockI16 input;
    MemBlockF output, correct;
    input.init(rows,cols);
    correct.init(rows,cols);
    output.init(rows,cols);
    input.loadFromH5(pathToTestFiles + "example_1_1Img_DLSraw.h5", "/sample", 0);
    correct.loadFromH5(pathToTestFiles + "example_1_1Img_ADU.h5", "/sample", 0);

    calibratorS.processFrame(input, output);
    for(int r=0;r<rows;++r)
    {
       for(int c=0;c<cols;++c)
       {
           if(isnan(output.at(r,c)))
           {
              BOOST_CHECK(isnan(correct.at(r,c)));
           }
           else if( 0.01f < fabs( output.at(r,c)-correct.at(r,c) ) )
           {
                // I found that using boost_check_close failed for small values of output, so I did this instead.                
                std::cout << "at r,c: " << r << " " << c;
                std::cout << " calc: " << output.at(r,c) << " correct: " << correct.at(r,c) << std::endl;
                BOOST_CHECK(false);
           }
       }
    }
}

BOOST_AUTO_TEST_CASE(sampleCMACDA)
{
    std::string pathToTestFiles("/dls/detectors/Percival/test_data/CalibRTests/");
    CalibratorReset calibratorR(rows, cols);
    CalibratorSample calibratorS(rows,cols);

    calibratorR.loadADCGain(pathToTestFiles + "BSI04_Tminus20_dmuxSELsw_2019.11.20_ADCcor.h5");
    calibratorS.loadADCGain(pathToTestFiles + "BSI04_Tminus20_dmuxSELsw_2019.11.20_ADCcor.h5");

    MemBlockI16 input;
    MemBlockF reset, output, correct;
    input.init(rows,cols);
    correct.init(rows,cols);
    reset.init(rows,cols);
    output.init(rows,cols);
    input.loadFromH5(pathToTestFiles + "example_1_1Img_DLSraw.h5", "/reset", 0);
    calibratorR.processFrame(input, reset);

    input.loadFromH5(pathToTestFiles + "example_1_1Img_DLSraw.h5", "/sample", 0);
    correct.loadFromH5(pathToTestFiles + "example_1_1Img_CMACDS.h5", "/sample", 0);
    calibratorS.setCMA(true, 1024);
    calibratorS.m_Gain3 = 1.0f;
    calibratorS.setResetFrame(reset);
    calibratorS.processFrame(input, output);

    for(int r=0;r<rows;++r)
    {
       for(int c=0;c<cols;++c)
       {
           if(isnan(output.at(r,c)))
           {
              if(!isnan(correct.at(r,c)))
              {
                 std::cout << "nan mismatch at " << r << "," << c << " output nan, correct is " << correct.at(r,c) << std::endl;
                 BOOST_CHECK(false);
              }
           }
           else if( 0.01f < fabs( output.at(r,c)-correct.at(r,c) ) )
           {
                // I found that using boost_check_close failed for small values of output, so I did this instead.                
                std::cout << "at r,c: " << r << " " << c;
                std::cout << " calc: " << output.at(r,c) << " correct: " << correct.at(r,c) << std::endl;
                BOOST_CHECK(false);
           }
       }
   }
}


BOOST_AUTO_TEST_CASE(sampleCDA_LAT)
{
    std::string pathToTestFiles = "/dls/detectors/Percival/test_data/LATcorrectionExample/";
    CalibratorReset calibratorR(rows, cols);
    CalibratorSample calibratorS(rows,cols);

    calibratorR.loadADCGain(pathToTestFiles + "BSI04_Tminus20_dmuxSELsw_2019.11.20_ADCcor.h5");
    calibratorS.loadADCGain(pathToTestFiles + "BSI04_Tminus20_dmuxSELsw_2019.11.20_ADCcor.h5");
    calibratorS.loadLatGain(pathToTestFiles + "latgainfile.h5");

    MemBlockI16 input;
    MemBlockF reset, output, correct;
    input.init(rows,cols);
    correct.init(rows,cols);
    reset.init(rows,cols);
    output.init(rows,cols);
    input.loadFromH5(pathToTestFiles + "dataElaboration_example2_step1_2Img_DLSraw.h5", "/reset", 0);
    calibratorR.processFrame(input, reset);

    input.loadFromH5(pathToTestFiles + "dataElaboration_example2_step1_2Img_DLSraw.h5", "/data", 1);
    correct.loadFromH5(pathToTestFiles + "dataElaboration_example2_step4_1Img_e.h5", "/data/data", 0);

    calibratorS.setResetFrame(reset);
    calibratorS.processFrame(input, output);

    bool oneRowHasAllG0 = false;
    for(int r=0;r<rows;++r)
    {
       int countNumPixelsG0 = 0;
       for(int c=0;c<cols;++c)
       {
           if(isnan(output.at(r,c)))
           {
              if(!isnan(correct.at(r,c)))
              {
                 std::cout << "nan mismatch at " << r << "," << c << " output nan, correct is " << correct.at(r,c) << std::endl;
                 BOOST_CHECK(false);
              }
           }
           else
           {
               float diff = fabs( output.at(r,c)-correct.at(r,c) );
               if(input.at(r,c) == 0)
                  ++countNumPixelsG0;

               if( 1.0f < diff && 0.001 < fabs(diff / output.at(r,c))  )
               {
                    // I found that using boost_check_close failed for small values of output, so I did this instead.                
                    std::cout << "at r,c: " << r << " " << c;
                    std::cout << " calc: " << output.at(r,c) << " correct: " << correct.at(r,c) << std::endl;
                    BOOST_CHECK(false);
               }
           }
       }

       if(countNumPixelsG0 == 1408)
           oneRowHasAllG0 = true;
    }

    // this little check verifies that the cma vals are not all NAN on one row, ie that our cma value is correct and real.
    BOOST_CHECK(oneRowHasAllG0);
}


BOOST_AUTO_TEST_CASE(sampleCMA_CDA_LAT)
{
    std::string pathToTestFiles = "/dls/detectors/Percival/test_data/LATcorrectionExample/";
    CalibratorReset calibratorR(rows, cols);
    CalibratorSample calibratorS(rows,cols);
    calibratorS.setCMA(true, 1152);

    calibratorR.loadADCGain(pathToTestFiles + "BSI04_Tminus20_dmuxSELsw_2019.11.20_ADCcor.h5");
    calibratorS.loadADCGain(pathToTestFiles + "BSI04_Tminus20_dmuxSELsw_2019.11.20_ADCcor.h5");
    calibratorS.loadLatGain(pathToTestFiles + "latgainfile.h5");

    MemBlockI16 input;
    MemBlockF reset, output, correct;
    input.init(rows,cols);
    correct.init(rows,cols);
    reset.init(rows,cols);
    output.init(rows,cols);
    input.loadFromH5(pathToTestFiles + "dataElaboration_example2_step1_2Img_DLSraw.h5", "/reset", 0);
    calibratorR.processFrame(input, reset);

    input.loadFromH5(pathToTestFiles + "dataElaboration_example2_step1_2Img_DLSraw.h5", "/data", 1);
    correct.loadFromH5(pathToTestFiles + "dataElaboration_example2_step4_1Img_cma_e.h5", "/data/data", 0);

    calibratorS.setResetFrame(reset);
    calibratorS.processFrame(input, output);

    bool oneRowHasAllG0 = false;
    for(int r=0;r<rows;++r)
    {
       int countNumPixelsG0 = 0;
       for(int c=0;c<cols;++c)
       {
           if(isnan(output.at(r,c)))
           {
              if(!isnan(correct.at(r,c)))
              {
                 std::cout << "nan mismatch at " << r << "," << c << " output nan, correct is " << correct.at(r,c) << std::endl;
                 BOOST_CHECK(false);
              }
           }
           else
           {
               float diff = fabs( output.at(r,c)-correct.at(r,c) );
               if(input.at(r,c) == 0)
                  ++countNumPixelsG0;

               if( 1.0f < diff && 0.001 < fabs(diff / output.at(r,c))  )
               {
                    // I found that using boost_check_close failed for small values of output, so I did this instead.                
                    std::cout << "at r,c: " << r << " " << c;
                    std::cout << " calc: " << output.at(r,c) << " correct: " << correct.at(r,c) << std::endl;
                    BOOST_CHECK(false);
               }
           }
       }

       if(countNumPixelsG0 == 1408)
           oneRowHasAllG0 = true;
    }

    // this little check verifies that the cma vals are not all NAN on one row, ie that our cma value is correct and real.
    BOOST_CHECK(oneRowHasAllG0);

    // now do the same with a parallel computer and see how long it takes.
    input.loadFromH5(pathToTestFiles + "dataElaboration_example2_step1_2Img_DLSraw.h5", "/data", 1);
    output.setAll(0.0f);
    BOOST_CHECK_EQUAL(output.at(0,0),0.0f);

    auto t1 = std::chrono::high_resolution_clock::now();
    calibratorS.processFrameP(input, output);
    auto t2 = std::chrono::high_resolution_clock::now();

    auto d = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1);
    std::cout << "S parallel process took " << d.count() << "us" << std::endl;

    for(int r=0;r<rows;++r)
    {
       for(int c=0;c<cols;++c)
       {
           if(isnan(output.at(r,c)))
           {
              if(!isnan(correct.at(r,c)))
              {
                 std::cout << "nan mismatch at " << r << "," << c << " output nan, correct is " << correct.at(r,c) << std::endl;
                 BOOST_CHECK(false);
              }
           }
           else
           {
               float diff = fabs( output.at(r,c)-correct.at(r,c) );

               if( 1.0f < diff && 0.001 < fabs(diff / output.at(r,c))  )
               {
                    // I found that using boost_check_close failed for small values of output, so I did this instead.                
                    std::cout << "at r,c: " << r << " " << c;
                    std::cout << " calc: " << output.at(r,c) << " correct: " << correct.at(r,c) << std::endl;
                    BOOST_CHECK(false);
               }
           }
       }
   }
}

