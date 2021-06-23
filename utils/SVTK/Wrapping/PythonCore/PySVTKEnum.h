/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKEnum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef PySVTKEnum_h
#define PySVTKEnum_h

#include "svtkPython.h"
#include "svtkSystemIncludes.h"
#include "svtkWrappingPythonCoreModule.h" // For export macro

extern "C"
{
  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyTypeObject* PySVTKEnum_Add(PyTypeObject* pytype, const char* name);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKEnum_New(PyTypeObject* pytype, int val);
}

#endif
