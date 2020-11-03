#define BOOST_TEST_MODULE "PercivalFrameProcessorTests"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>

#include "PercivalProcessPlugin.h"

class PercivalProcessPluginTestFixture
{
public:
    PercivalProcessPluginTestFixture()
    {
        std::cout << "PercivalProcessPluginTestFixture constructor" << std::endl;
    }

    ~PercivalProcessPluginTestFixture()
    {
        std::cout << "PercivalProcessPluginTestFixture destructor" << std::endl;
    }
};

BOOST_FIXTURE_TEST_SUITE(PercivalProcessPluginUnitTest, PercivalProcessPluginTestFixture);

BOOST_AUTO_TEST_CASE(PercivalProcessPluginTestFixture)
{
    std::cout << "PercivalProcessPluginTestFixture test case" << std::endl;
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END();
