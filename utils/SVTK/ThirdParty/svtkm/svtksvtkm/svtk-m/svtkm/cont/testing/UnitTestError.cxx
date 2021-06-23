//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Error.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Logging.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

void RecursiveFunction(int recurse)
{
  if (recurse < 5)
  {
    RecursiveFunction(++recurse);
  }
  else
  {
    throw svtkm::cont::ErrorBadValue("Too much recursion");
  }
}

void ValidateError(const svtkm::cont::Error& error)
{
  std::string message = "Too much recursion";
  std::string stackTrace = error.GetStackTrace();
  std::stringstream stackTraceStream(stackTrace);
  std::string tmp;
  size_t count = 0;
  while (std::getline(stackTraceStream, tmp))
  {
    count++;
  }

  // StackTrace may be unavailable on certain Devices
  if (stackTrace == "(Stack trace unavailable)")
  {
    SVTKM_TEST_ASSERT(count == 1, "Logging disabled, stack trace shouldn't be available");
  }
  else
  {
#if defined(NDEBUG)
    // The compiler can optimize out the recursion and other function calls in release
    // mode, but the backtrace should contain atleast one entry.
    std::string assert_msg = "No entries in the stack frame\n" + stackTrace;
    SVTKM_TEST_ASSERT(count >= 1, assert_msg);
#else
    std::string assert_msg = "Expected more entries in the stack frame\n" + stackTrace;
    SVTKM_TEST_ASSERT(count > 5, assert_msg);
#endif
  }
  SVTKM_TEST_ASSERT(test_equal(message, error.GetMessage()), "Message was incorrect");
  SVTKM_TEST_ASSERT(test_equal(message + "\n" + stackTrace, std::string(error.what())),
                   "what() was incorrect");
}

void DoErrorTest()
{
  SVTKM_LOG_S(svtkm::cont::LogLevel::Info, "Check base error messages");
  try
  {
    RecursiveFunction(0);
  }
  catch (const svtkm::cont::Error& e)
  {
    ValidateError(e);
  }
}

} // anonymous namespace

int UnitTestError(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(DoErrorTest, argc, argv);
}
