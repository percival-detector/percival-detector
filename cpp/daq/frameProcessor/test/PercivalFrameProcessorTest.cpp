#define BOOST_TEST_MODULE "PercivalFrameProcessorTests"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>

#include "PercivalProcess2Plugin.h"

class PercivalProcess2PluginTestFixture
{
public:
    PercivalProcess2PluginTestFixture()
    {
        std::cout << "PercivalProcess2PluginTestFixture constructor" << std::endl;
    }

    ~PercivalProcess2PluginTestFixture()
    {
        std::cout << "PercivalProcess2PluginTestFixture destructor" << std::endl;
    }
};

BOOST_FIXTURE_TEST_SUITE(PercivalProcess2PluginUnitTest, PercivalProcess2PluginTestFixture);

BOOST_AUTO_TEST_CASE(PercivalProcess2PluginTestFixture)
{
    std::cout << "PercivalProcess2PluginTestFixture test case" << std::endl;
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END();
