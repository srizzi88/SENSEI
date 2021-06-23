/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKTemplate was created in May 2011 by David Gobbi.

  This object is a container for instantiations of templated types.
  Essentially, it is a "dict" that accepts template args as keys,
  and provides the corresponding python type.
-----------------------------------------------------------------------*/

#ifndef PySVTKTemplate_h
#define PySVTKTemplate_h

#include "svtkPython.h"
#include "svtkSystemIncludes.h"
#include "svtkWrappingPythonCoreModule.h" // For export macro

extern SVTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PySVTKTemplate_Type;

#define PySVTKTemplate_Check(obj) (Py_TYPE(obj) == &PySVTKTemplate_Type)

extern "C"
{
  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKTemplate_New(const char* name, const char* docstring);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  int PySVTKTemplate_AddItem(PyObject* self, PyObject* val);
}

#endif
