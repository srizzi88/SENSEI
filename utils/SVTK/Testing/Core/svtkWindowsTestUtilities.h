/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWindowsTestUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// on msvc add in stack trace info as systeminformation
// does not seem to include it.
//
#ifndef SVTK_WINDOWS_TEST_UTILITIES
#define SVTK_WINDOWS_TEST_UTILITIES

#if defined(SVTK_COMPILER_MSVC) && defined(WIN32)
#include <sstream>
#include <windows.h>

inline LONG WINAPI svtkWindowsTestUlititiesExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
  switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
  {
    case EXCEPTION_ACCESS_VIOLATION:
      svtkLog(ERROR, << "Error: EXCEPTION_ACCESS_VIOLATION\n");
      break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      svtkLog(ERROR, << "Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n");
      break;
    case EXCEPTION_BREAKPOINT:
      svtkLog(ERROR, << "Error: EXCEPTION_BREAKPOINT\n");
      break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      svtkLog(ERROR, << "Error: EXCEPTION_DATATYPE_MISALIGNMENT\n");
      break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
      svtkLog(ERROR, << "Error: EXCEPTION_FLT_DENORMAL_OPERAND\n");
      break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      svtkLog(ERROR, << "Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n");
      break;
    case EXCEPTION_FLT_INEXACT_RESULT:
      svtkLog(ERROR, << "Error: EXCEPTION_FLT_INEXACT_RESULT\n");
      break;
    case EXCEPTION_FLT_INVALID_OPERATION:
      svtkLog(ERROR, << "Error: EXCEPTION_FLT_INVALID_OPERATION\n");
      break;
    case EXCEPTION_FLT_OVERFLOW:
      svtkLog(ERROR, << "Error: EXCEPTION_FLT_OVERFLOW\n");
      break;
    case EXCEPTION_FLT_STACK_CHECK:
      svtkLog(ERROR, << "Error: EXCEPTION_FLT_STACK_CHECK\n");
      break;
    case EXCEPTION_FLT_UNDERFLOW:
      svtkLog(ERROR, << "Error: EXCEPTION_FLT_UNDERFLOW\n");
      break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      svtkLog(ERROR, << "Error: EXCEPTION_ILLEGAL_INSTRUCTION\n");
      break;
    case EXCEPTION_IN_PAGE_ERROR:
      svtkLog(ERROR, << "Error: EXCEPTION_IN_PAGE_ERROR\n");
      break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      svtkLog(ERROR, << "Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n");
      break;
    case EXCEPTION_INT_OVERFLOW:
      svtkLog(ERROR, << "Error: EXCEPTION_INT_OVERFLOW\n");
      break;
    case EXCEPTION_INVALID_DISPOSITION:
      svtkLog(ERROR, << "Error: EXCEPTION_INVALID_DISPOSITION\n");
      break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      svtkLog(ERROR, << "Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n");
      break;
    case EXCEPTION_PRIV_INSTRUCTION:
      svtkLog(ERROR, << "Error: EXCEPTION_PRIV_INSTRUCTION\n");
      break;
    case EXCEPTION_SINGLE_STEP:
      svtkLog(ERROR, << "Error: EXCEPTION_SINGLE_STEP\n");
      break;
    case EXCEPTION_STACK_OVERFLOW:
      svtkLog(ERROR, << "Error: EXCEPTION_STACK_OVERFLOW\n");
      break;
    default:
      svtkLog(ERROR, << "Error: Unrecognized Exception\n");
      break;
  }

  std::string stack = svtksys::SystemInformation::GetProgramStack(0, 0);

  svtkLog(ERROR, << stack);

  return EXCEPTION_CONTINUE_SEARCH;
}

inline void svtkWindowsTestUtilitiesSetupForTesting(void)
{
  SetUnhandledExceptionFilter(svtkWindowsTestUlititiesExceptionHandler);
}
#else
inline void svtkWindowsTestUtilitiesSetupForTesting(void)
{
  return;
}
#endif
#endif
// SVTK-HeaderTest-Exclude: svtkWindowsTestUtilities.h
