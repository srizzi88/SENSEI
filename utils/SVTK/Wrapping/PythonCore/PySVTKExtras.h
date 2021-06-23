/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKExtras.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKExtras was created in Aug 2015 by David Gobbi.

  This file provides extra classes and functions for the svtk module.
  Unlike the contents of svtk.util, the classes and functions provided
  here are ones that must be written in C++ instead of pure python.
-----------------------------------------------------------------------*/

#ifndef PySVTKExtras_h
#define PySVTKExtras_h

#include "svtkPython.h"
#include "svtkWrappingPythonCoreModule.h" // For export macro

//--------------------------------------------------------------------
// This will add extras to the provided dict.  It is called during the
// initialization of the svtkCommonCore python module.
extern "C"
{
  SVTKWRAPPINGPYTHONCORE_EXPORT void PySVTKAddFile_PySVTKExtras(PyObject* dict);
}

#endif
