//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Logging.h>
#include <svtkm/cont/testing/Testing.h>

#include <chrono>
#include <thread>

namespace
{

void DoWork()
{
  SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Info);
  SVTKM_LOG_F(svtkm::cont::LogLevel::Info, "Sleeping for half a second...");
  std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
}

void Scopes(int level = 0)
{
  SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Info, "Called Scope (level=%d)", level);

  DoWork();

  SVTKM_LOG_IF_F(svtkm::cont::LogLevel::Info,
                level % 2 != 0,
                "Printing extra log message because level is odd (%d)",
                level);
  if (level < 5)
  {
    SVTKM_LOG_S(svtkm::cont::LogLevel::Info, "Recursing to level " << level + 1);
    Scopes(level + 1);
  }
  else
  {
    SVTKM_LOG_F(svtkm::cont::LogLevel::Warn, "Reached limit for Scopes test recursion.");
  }
}

void ErrorContext()
{
  // These variables are only logged if a crash occurs.
  // Only supports POD by default, but can be extended (see loguru docs)
  SVTKM_LOG_ERROR_CONTEXT("Some Int", 3);
  SVTKM_LOG_ERROR_CONTEXT("A Double", 236.7521);
  SVTKM_LOG_ERROR_CONTEXT("A C-String", "Hiya!");

  // The error-tracking should work automatically on linux (maybe mac?) but on
  // windows it doesn't trigger automatically (see loguru #74). But we can
  // manually dump the error context log like so:
  std::cerr << svtkm::cont::GetLogErrorContext() << "\n";
}

void UserDefined()
{
  SVTKM_DEFINE_USER_LOG_LEVEL(CustomLevel, 0);
  SVTKM_DEFINE_USER_LOG_LEVEL(CustomLevel2, 2);
  SVTKM_DEFINE_USER_LOG_LEVEL(AnotherCustomLevel2, 2);
  SVTKM_DEFINE_USER_LOG_LEVEL(BigLevel, 300);

  svtkm::cont::SetStderrLogLevel(svtkm::cont::LogLevel::UserLast);
  SVTKM_LOG_S(CustomLevel, "CustomLevel");
  SVTKM_LOG_S(CustomLevel2, "CustomLevel2");
  SVTKM_LOG_S(AnotherCustomLevel2, "AnotherCustomLevel2");

  svtkm::cont::SetStderrLogLevel(svtkm::cont::LogLevel::UserFirst);
  SVTKM_LOG_S(BigLevel, "BigLevel"); // should log nothing

  svtkm::cont::SetStderrLogLevel(svtkm::cont::LogLevel::UserLast);
  SVTKM_LOG_S(BigLevel, "BigLevel");
}

void RunTests()
{
  SVTKM_LOG_F(svtkm::cont::LogLevel::Info, "Running tests.");

  SVTKM_LOG_S(svtkm::cont::LogLevel::Info, "Running Scopes test...");
  Scopes();

  SVTKM_LOG_S(svtkm::cont::LogLevel::Info, "Running ErrorContext test...");
  ErrorContext();

  SVTKM_LOG_S(svtkm::cont::LogLevel::Info, "Running UserDefined test...");
  UserDefined();
}

} // end anon namespace

int UnitTestLogging(int, char* [])
{
  // Test that parameterless init works:
  svtkm::cont::InitLogging();

  RunTests();
  return 0;
}
