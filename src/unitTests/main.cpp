/*
 * main.cpp
 *
 *  Created on: 12.01.2011
 *      Author: christian
 */

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/Portability.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cstdint>
#include <iostream>

#include "../misc/defs.h"
#include "../vm/Universe.h"
#include "BasicInterpreterTests.h"
#include "BytecodeGenerationTest.h"
#include "CloneObjectsTest.h"
#include "HashingTest.h"
#include "TrivialMethodTest.h"
#include "WalkObjectsTest.h"

#if GC_TYPE == GENERATIONAL
  #include "WriteBarrierTest.h"
#endif

CPPUNIT_TEST_SUITE_REGISTRATION(WalkObjectsTest);
CPPUNIT_TEST_SUITE_REGISTRATION(CloneObjectsTest);
#if GC_TYPE == GENERATIONAL
CPPUNIT_TEST_SUITE_REGISTRATION(WriteBarrierTest);
#endif
CPPUNIT_TEST_SUITE_REGISTRATION(BytecodeGenerationTest);
CPPUNIT_TEST_SUITE_REGISTRATION(TrivialMethodTest);
CPPUNIT_TEST_SUITE_REGISTRATION(BasicInterpreterTests);
CPPUNIT_TEST_SUITE_REGISTRATION(HashingTest);

int32_t main(int32_t ac, char** av) {
    Universe::Start(ac, av);

    //--- Create the event manager and test controller
    CPPUNIT_NS::TestResult controller;

    //--- Add a listener that collects test result
    CPPUNIT_NS::TestResultCollector result;
    controller.addListener(&result);

    //--- Add a listener that print dots as test run.
    CPPUNIT_NS::BriefTestProgressListener progress;
    controller.addListener(&progress);

    //--- Add the top suite to the test runner
    CPPUNIT_NS::TestRunner runner;
    runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
    runner.run(controller);

    // output results in compiler-format
    CPPUNIT_NS::CompilerOutputter compileroutputter(&result, std::cerr);
    compileroutputter.write();

    return result.wasSuccessful() ? 0 : 1;
}
