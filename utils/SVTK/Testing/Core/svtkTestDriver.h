/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTestDriver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This header is included by all the C++ test drivers in SVTK.
#ifndef svtkTestDriver_h
#define svtkTestDriver_h

#include "svtkFloatingPointExceptions.h"
#include <exception> // for std::exception

#include <clocale> // C setlocale()
#include <locale>  // C++ locale

#include <svtksys/SystemInformation.hxx> // for stacktrace

#include <svtkLogger.h> // for logging

#include "svtkWindowsTestUtilities.h" // for windows stack trace

#endif
// SVTK-HeaderTest-Exclude: svtkTestDriver.h
