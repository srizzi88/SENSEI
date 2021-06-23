/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPythonAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPythonAlgorithm.h"
#include "svtkObjectFactory.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPythonUtil.h"
#include "svtkSmartPyObject.h"

svtkStandardNewMacro(svtkPythonAlgorithm);

void svtkPythonAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
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

svtkPythonAlgorithm::svtkPythonAlgorithm()
{
  this->Object = nullptr;
}

svtkPythonAlgorithm::~svtkPythonAlgorithm()
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

/// Return value: New reference.
static PyObject* SVTKToPython(svtkObjectBase* obj)
{
  return svtkPythonUtil::GetObjectFromPointer(obj);
}

int svtkPythonAlgorithm::CheckResult(const char* method, const svtkSmartPyObject& res)
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
    return 0;
  }
  if (!PyInt_Check(res))
  {
    return 0;
  }

  int code = PyInt_AsLong(res);

  return code;
}

void svtkPythonAlgorithm::SetPythonObject(PyObject* obj)
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

void svtkPythonAlgorithm::SetNumberOfInputPorts(int n)
{
  this->Superclass::SetNumberOfInputPorts(n);
}

void svtkPythonAlgorithm::SetNumberOfOutputPorts(int n)
{
  this->Superclass::SetNumberOfOutputPorts(n);
}

svtkTypeBool svtkPythonAlgorithm::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo)
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "ProcessRequest";
  SVTK_GET_METHOD(method, this->Object, mname, 0)

  svtkSmartPyObject args(PyTuple_New(4));

  PyObject* svtkself = SVTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, svtkself);

  PyObject* pyrequest = SVTKToPython(request);
  PyTuple_SET_ITEM(args.GetPointer(), 1, pyrequest);

  int nports = this->GetNumberOfInputPorts();
  PyObject* pyininfos = PyTuple_New(nports);
  for (int i = 0; i < nports; ++i)
  {
    PyObject* pyininfo = SVTKToPython(inInfo[i]);
    PyTuple_SET_ITEM(pyininfos, i, pyininfo);
  }
  PyTuple_SET_ITEM(args.GetPointer(), 2, pyininfos);

  PyObject* pyoutinfo = SVTKToPython(outInfo);
  PyTuple_SET_ITEM(args.GetPointer(), 3, pyoutinfo);

  svtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return CheckResult(mname, result);
}

int svtkPythonAlgorithm::FillInputPortInformation(int port, svtkInformation* info)
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "FillInputPortInformation";
  SVTK_GET_METHOD(method, this->Object, mname, 0)

  svtkSmartPyObject args(PyTuple_New(3));

  PyObject* svtkself = SVTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, svtkself);

  PyObject* pyport = PyInt_FromLong(port);
  PyTuple_SET_ITEM(args.GetPointer(), 1, pyport);

  PyObject* pyinfo = SVTKToPython(info);
  PyTuple_SET_ITEM(args.GetPointer(), 2, pyinfo);

  svtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return CheckResult(mname, result);
}

int svtkPythonAlgorithm::FillOutputPortInformation(int port, svtkInformation* info)
{
  svtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "FillOutputPortInformation";
  SVTK_GET_METHOD(method, this->Object, mname, 0)

  svtkSmartPyObject args(PyTuple_New(3));

  PyObject* svtkself = SVTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, svtkself);

  PyObject* pyport = PyInt_FromLong(port);
  PyTuple_SET_ITEM(args.GetPointer(), 1, pyport);

  PyObject* pyinfo = SVTKToPython(info);
  PyTuple_SET_ITEM(args.GetPointer(), 2, pyinfo);

  svtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return CheckResult(mname, result);
}
