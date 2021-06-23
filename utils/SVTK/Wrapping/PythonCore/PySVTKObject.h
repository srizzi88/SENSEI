/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKObject was created in Oct 2000 by David Gobbi for SVTK 3.2.
  It was rewritten in Jul 2015 to wrap SVTK classes as python type objects.
-----------------------------------------------------------------------*/

#ifndef PySVTKObject_h
#define PySVTKObject_h

#include "svtkPython.h"
#include "svtkSystemIncludes.h"
#include "svtkWrappingPythonCoreModule.h" // For export macro

class svtkObjectBase;
typedef svtkObjectBase* (*svtknewfunc)();

// Flags for special properties or features
#define SVTK_PYTHON_IGNORE_UNREGISTER 1 // block Register/UnRegister calls

// This class is used for defining new SVTK wrapped classes.
// It contains information such as the methods and docstring, as well
// as extra info that can't easily be stored in the PyTypeObject.
class SVTKWRAPPINGPYTHONCORE_EXPORT PySVTKClass
{
public:
  PySVTKClass()
    : py_type(nullptr)
    , py_methods(nullptr)
    , svtk_name(nullptr)
    , svtk_new(nullptr)
  {
  }

  PySVTKClass(
    PyTypeObject* typeobj, PyMethodDef* methods, const char* classname, svtknewfunc constructor);

  PyTypeObject* py_type;
  PyMethodDef* py_methods;
  const char* svtk_name; // the name returned by GetClassName()
  svtknewfunc svtk_new;   // creates a C++ instance of classtype
};

// This is the SVTK/Python 'object,' it contains the python object header
// plus a pointer to the associated svtkObjectBase and PySVTKClass.
struct PySVTKObject
{
  PyObject_HEAD PyObject* svtk_dict; // each object has its own dict
  PyObject* svtk_weakreflist;        // list of weak references via python
  PySVTKClass* svtk_class;            // information about the class
  svtkObjectBase* svtk_ptr;           // pointer to the C++ object
  Py_ssize_t* svtk_buffer;           // ndims, shape, strides for Py_buffer
  unsigned long* svtk_observers;     // used to find our observers
  unsigned int svtk_flags;           // flags (see list above)
};

extern SVTKWRAPPINGPYTHONCORE_EXPORT PyGetSetDef PySVTKObject_GetSet[];
extern SVTKWRAPPINGPYTHONCORE_EXPORT PyBufferProcs PySVTKObject_AsBuffer;

extern "C"
{
  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyTypeObject* PySVTKClass_Add(
    PyTypeObject* pytype, PyMethodDef* methods, const char* classname, svtknewfunc constructor);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  int PySVTKObject_Check(PyObject* obj);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKObject_FromPointer(PyTypeObject* svtkclass, PyObject* pydict, svtkObjectBase* ptr);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  svtkObjectBase* PySVTKObject_GetObject(PyObject* obj);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  void PySVTKObject_AddObserver(PyObject* obj, unsigned long id);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  void PySVTKObject_SetFlag(PyObject* obj, unsigned int flag, int val);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  unsigned int PySVTKObject_GetFlags(PyObject* obj);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKObject_Repr(PyObject* op);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKObject_String(PyObject* op);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  int PySVTKObject_Traverse(PyObject* o, visitproc visit, void* arg);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKObject_New(PyTypeObject*, PyObject* args, PyObject* kwds);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  void PySVTKObject_Delete(PyObject* op);
}

#endif
