
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSmartPyObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPyObject.h"

#if defined(_MSC_VER) // Visual studio
// Ignore "constant expression" warnings from MSVC due to the "while (0)" in
// the Py_X{IN,DE}CREF macros.
#pragma warning(disable : 4127)
#endif

//--------------------------------------------------------------------
svtkSmartPyObject::svtkSmartPyObject(PyObject* obj)
  : Object(obj)
{
}

//--------------------------------------------------------------------
svtkSmartPyObject::svtkSmartPyObject(const svtkSmartPyObject& other)
  : Object(other.Object)
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  Py_XINCREF(this->Object);
}

//--------------------------------------------------------------------
svtkSmartPyObject::~svtkSmartPyObject()
{
  if (Py_IsInitialized())
  {
    svtkPythonScopeGilEnsurer gilEnsurer;
    Py_XDECREF(this->Object);
  }
}

//--------------------------------------------------------------------
svtkSmartPyObject& svtkSmartPyObject::operator=(const svtkSmartPyObject& other)
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  Py_XDECREF(this->Object);
  this->Object = other.Object;
  Py_XINCREF(this->Object);
  return *this;
}

//--------------------------------------------------------------------
svtkSmartPyObject& svtkSmartPyObject::operator=(PyObject* obj)
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  Py_XDECREF(this->Object);
  this->Object = obj;
  Py_XINCREF(this->Object);
  return *this;
}

//--------------------------------------------------------------------
void svtkSmartPyObject::TakeReference(PyObject* obj)
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  Py_XDECREF(this->Object);
  this->Object = obj;
}

//--------------------------------------------------------------------
PyObject* svtkSmartPyObject::operator->() const
{
  return this->Object;
}

//--------------------------------------------------------------------
svtkSmartPyObject::operator PyObject*() const
{
  return this->Object;
}

//--------------------------------------------------------------------
svtkSmartPyObject::operator bool() const
{
  return this->Object != nullptr;
}

//--------------------------------------------------------------------
PyObject* svtkSmartPyObject::ReleaseReference()
{
  PyObject* tmp = this->Object;
  this->Object = nullptr;
  return tmp;
}

//--------------------------------------------------------------------
PyObject* svtkSmartPyObject::GetPointer() const
{
  return this->Object;
}

//--------------------------------------------------------------------
PyObject* svtkSmartPyObject::GetAndIncreaseReferenceCount()
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  Py_XINCREF(this->Object);
  return this->Object;
}
