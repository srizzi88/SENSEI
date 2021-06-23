/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPythonItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPythonItem.h"
#include "svtkObjectFactory.h"

#include "svtkContext2D.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPythonUtil.h"
#include "svtkSmartPyObject.h"

svtkStandardNewMacro(svtkPythonItem);

//------------------------------------------------------------------------------
void svtkPythonItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  svtkPythonScopeGilEnsurer gilEnsurer;
  svtkSmartPyObject str;
  if (this->Object)
  {
    str.TakeReference(PyObject_Str(this->Object));
  }

  os << indent << "Object: " << Object << std::endl;
  if (str)
  {
    os << indent << "Object (string): ";
#ifndef SVTK_PY3K
    os << PyString_AsString(str);
#else
    PyObject* bytes = PyUnicode_EncodeLocale(str, SVTK_PYUNICODE_ENC);
    if (bytes)
    {
      os << PyBytes_AsString(bytes);
      Py_DECREF(bytes);
    }
#endif
    os << std::endl;
  }
}

//------------------------------------------------------------------------------
svtkPythonItem::svtkPythonItem()
{
  this->Object = nullptr;
}

//------------------------------------------------------------------------------
svtkPythonItem::~svtkPythonItem()
{
  // we check if Python is still initialized since the Python interpreter may
  // have been finalized before the SVTK object is released.
  if (Py_IsInitialized())
  {
    svtkPythonScopeGilEnsurer gilEnsurer;
    Py_XDECREF(this->Object);
  }
}

// This macro gets the method passed in as the parameter method
// from the PyObject passed in as the parameter obj and creates a
// svtkSmartPyObject variable with the name passed in as the parameter
// var containing that method's PyObject.  If obj is nullptr, obj.method
// does not exist or obj.method is not a callable method, this macro
// causes the function using it to return with the return value
// passed in as the parameter failValue
//    var - the name of the resulting svtkSmartPyObject with the
//          method object in it.  Can be used in the code following
//          the macro's use as the variable name
//    obj - the PyObject to get the method from
//    method - the name of the method to look for.  Should be a
//          C string.
//    failValue - the value to return if the lookup fails and the
//          function using the macro should return.  Pass in a
//          block comment /**/ for void functions using this macro
#define SVTK_GET_METHOD(var, obj, method, failValue)                                                \
  if (!(obj))                                                                                      \
  {                                                                                                \
    return failValue;                                                                              \
  }                                                                                                \
  svtkSmartPyObject var(PyObject_GetAttrString(obj, method));                                       \
  if (!(var))                                                                                      \
  {                                                                                                \
    return failValue;                                                                              \
  }                                                                                                \
  if (!PyCallable_Check(var))                                                                      \
  {                                                                                                \
    return failValue;                                                                              \
  }

//------------------------------------------------------------------------------
static PyObject* SVTKToPython(svtkObjectBase* obj)
{
  // Return value: New reference.
  return svtkPythonUtil::GetObjectFromPointer(obj);
}

//------------------------------------------------------------------------------
bool svtkPythonItem::CheckResult(const char* method, const svtkSmartPyObject& res)
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  if (!res)
  {
    svtkErrorMacro("Failure when calling method: \"" << method << "\":");
    if (PyErr_Occurred() != nullptr)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    return false;
  }
  if (!PyBool_Check(res))
  {
    svtkWarningMacro("The method \"" << method << "\" should have returned boolean but did not");
    return false;
  }

  if (res == Py_False)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
void svtkPythonItem::SetPythonObject(PyObject* obj)
{
  svtkPythonScopeGilEnsurer gilEnsurer;

  if (!obj)
  {
    return;
  }

  Py_XDECREF(this->Object);

  this->Object = obj;
  Py_INCREF(this->Object);

  char mname[] = "Initialize";
  SVTK_GET_METHOD(method, this->Object, mname, /* no return */)

  svtkSmartPyObject args(PyTuple_New(1));

  PyObject* svtkself = SVTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, svtkself);

  svtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  CheckResult(mname, result);
}

//------------------------------------------------------------------------------
bool svtkPythonItem::Paint(svtkContext2D* painter)
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "Paint";
  SVTK_GET_METHOD(method, this->Object, mname, 0)

  svtkSmartPyObject args(PyTuple_New(2));

  PyObject* svtkself = SVTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, svtkself);

  PyObject* pypainter = SVTKToPython(painter);
  PyTuple_SET_ITEM(args.GetPointer(), 1, pypainter);

  svtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return CheckResult(mname, result);
}
