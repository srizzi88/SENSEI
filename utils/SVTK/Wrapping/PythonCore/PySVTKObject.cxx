/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKObject was created in Oct 2000 by David Gobbi for SVTK 3.2.
  Support for weakref added in July 2005 by Prabhu Ramachandran.
  Buffer interface for svtk arrays added in Feb 2008 by Berk Geveci.

  A PySVTKObject is a python object that represents a SVTK object.
  The methods are stored in the __dict__ of the associated type objects.
  Each PySVTKObject also has a __dict__ of its own that can be used to
  store arbitrary attributes.

  Memory management is done as follows. Each PySVTKObject has
  an entry along with a smart pointer to its svtkObjectBase in
  the svtkPythonUtil::ObjectMap.  When a PySVTKObject is destructed,
  it is removed along with the smart pointer from the ObjectMap.
-----------------------------------------------------------------------*/

#include "PySVTKObject.h"
#include "PySVTKMethodDescriptor.h"
#include "svtkDataArray.h"
#include "svtkObjectBase.h"
#include "svtkPythonCommand.h"
#include "svtkPythonUtil.h"

#include <cstddef>
#include <sstream>

// This will be set to the python type struct for svtkObjectBase
static PyTypeObject* PySVTKObject_Type = nullptr;

//--------------------------------------------------------------------
PySVTKClass::PySVTKClass(
  PyTypeObject* typeobj, PyMethodDef* methods, const char* classname, svtknewfunc constructor)
{
  this->py_type = typeobj;
  this->py_methods = methods;
  this->svtk_name = classname;
  this->svtk_new = constructor;
}

//--------------------------------------------------------------------
// C API

//--------------------------------------------------------------------
// Add a class, add methods and members to its type object.  A return
// value of nullptr signifies that the class was already added.
PyTypeObject* PySVTKClass_Add(
  PyTypeObject* pytype, PyMethodDef* methods, const char* classname, svtknewfunc constructor)
{
  // Check whether the type is already in the map (use classname as key),
  // and return it if so.  If not, then add it to the map.
  pytype = svtkPythonUtil::AddClassToMap(pytype, methods, classname, constructor);

  // Cache the type object for svtkObjectBase for quick access
  if (PySVTKObject_Type == nullptr && strcmp(classname, "svtkObjectBase") == 0)
  {
    PySVTKObject_Type = pytype;
  }

  // If type object already has a dict, we're done
  if (pytype->tp_dict)
  {
    return pytype;
  }

  // Create the dict
  pytype->tp_dict = PyDict_New();

  // Add special attribute __svtkname__
  PyObject* s = PyString_FromString(classname);
  PyDict_SetItemString(pytype->tp_dict, "__svtkname__", s);
  Py_DECREF(s);

  // Add all of the methods
  for (PyMethodDef* meth = methods; meth && meth->ml_name; meth++)
  {
    PyObject* func = PySVTKMethodDescriptor_New(pytype, meth);
    PyDict_SetItemString(pytype->tp_dict, meth->ml_name, func);
    Py_DECREF(func);
  }

  return pytype;
}

//--------------------------------------------------------------------
int PySVTKObject_Check(PyObject* op)
{
  return PyObject_TypeCheck(op, PySVTKObject_Type);
}

//--------------------------------------------------------------------
// Object protocol

//--------------------------------------------------------------------
PyObject* PySVTKObject_String(PyObject* op)
{
  std::ostringstream svtkmsg_with_warning_C4701;
  ((PySVTKObject*)op)->svtk_ptr->Print(svtkmsg_with_warning_C4701);
  svtkmsg_with_warning_C4701.put('\0');
  PyObject* res = PyString_FromString(svtkmsg_with_warning_C4701.str().c_str());
  return res;
}

//--------------------------------------------------------------------
PyObject* PySVTKObject_Repr(PyObject* op)
{
  char buf[255];
  snprintf(buf, sizeof(buf), "(%.200s)%p", Py_TYPE(op)->tp_name, static_cast<void*>(op));

  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
int PySVTKObject_Traverse(PyObject* o, visitproc visit, void* arg)
{
  PySVTKObject* self = (PySVTKObject*)o;
  int err = 0;

  if (self->svtk_observers != nullptr)
  {
    unsigned long* olist = self->svtk_observers;
    while (err == 0 && *olist != 0)
    {
      svtkObject* op = static_cast<svtkObject*>(self->svtk_ptr);
      svtkCommand* c = op->GetCommand(*olist);
      if (c == nullptr)
      {
        // observer is gone, remove from list
        unsigned long* tmp = olist;
        do
        {
          tmp++;
        } while (*tmp != 0);
        *olist = *--tmp;
        *tmp = 0;
      }
      else
      {
        // visit the observer
        svtkPythonCommand* cbc = static_cast<svtkPythonCommand*>(c);
        err = visit(cbc->obj, arg);
        olist++;
      }
    }
  }

  return err;
}

//--------------------------------------------------------------------
PyObject* PySVTKObject_New(PyTypeObject* tp, PyObject* args, PyObject* kwds)
{
  // If type was subclassed within python, then skip arg checks and
  // simply create a new object.
  if ((tp->tp_flags & Py_TPFLAGS_HEAPTYPE) == 0)
  {
    if (kwds != nullptr && PyDict_Size(kwds))
    {
      PyErr_SetString(PyExc_TypeError, "this function takes no keyword arguments");
      return nullptr;
    }

    PyObject* o = nullptr;
    if (!PyArg_UnpackTuple(args, tp->tp_name, 0, 1, &o))
    {
      return nullptr;
    }

    if (o)
    {
      // used to create a SVTK object from a SWIG pointer
      return svtkPythonUtil::GetObjectFromObject(o, svtkPythonUtil::StripModule(tp->tp_name));
    }
  }

  // if PySVTKObject_FromPointer gets nullptr, it creates a new object.
  return PySVTKObject_FromPointer(tp, nullptr, nullptr);
}

//--------------------------------------------------------------------
void PySVTKObject_Delete(PyObject* op)
{
  PySVTKObject* self = (PySVTKObject*)op;

  PyObject_GC_UnTrack(op);

  if (self->svtk_weakreflist != nullptr)
  {
    PyObject_ClearWeakRefs(op);
  }

  // A python object owning a SVTK object reference is getting
  // destroyed.  Remove the python object's SVTK object reference.
  svtkPythonUtil::RemoveObjectFromMap(op);

  Py_DECREF(self->svtk_dict);
  delete[] self->svtk_observers;
  delete[] self->svtk_buffer;

  PyObject_GC_Del(op);
}

//--------------------------------------------------------------------
// This defines any special attributes of wrapped SVTK objects.

static PyObject* PySVTKObject_GetDict(PyObject* op, void*)
{
  PySVTKObject* self = (PySVTKObject*)op;
  Py_INCREF(self->svtk_dict);
  return self->svtk_dict;
}

static PyObject* PySVTKObject_GetThis(PyObject* op, void*)
{
  PySVTKObject* self = (PySVTKObject*)op;
  const char* classname = self->svtk_ptr->GetClassName();
  const char* cp = classname;
  char buf[1024];
  // check to see if classname is a valid python identifier
  if (isalpha(*cp) || *cp == '_')
  {
    do
    {
      cp++;
    } while (isalnum(*cp) || *cp == '_');
  }
  // otherwise, use the pythonic form of the class name
  if (*cp != '\0')
  {
    classname = svtkPythonUtil::StripModule(Py_TYPE(op)->tp_name);
  }
  snprintf(buf, sizeof(buf), "p_%.500s", classname);
  return PyString_FromString(svtkPythonUtil::ManglePointer(self->svtk_ptr, buf));
}

PyGetSetDef PySVTKObject_GetSet[] = {
#if PY_VERSION_HEX >= 0x03070000
  { "__dict__", PySVTKObject_GetDict, nullptr, "Dictionary of attributes set by user.", nullptr },
  { "__this__", PySVTKObject_GetThis, nullptr, "Pointer to the C++ object.", nullptr },
#else
  { const_cast<char*>("__dict__"), PySVTKObject_GetDict, nullptr,
    const_cast<char*>("Dictionary of attributes set by user."), nullptr },
  { const_cast<char*>("__this__"), PySVTKObject_GetThis, nullptr,
    const_cast<char*>("Pointer to the C++ object."), nullptr },
#endif
  { nullptr, nullptr, nullptr, nullptr, nullptr }
};

//--------------------------------------------------------------------
// The following methods and struct define the "buffer" protocol
// for PySVTKObject, so that python can read from a svtkDataArray.
// This is particularly useful for NumPy.

#ifndef SVTK_PY3K
//--------------------------------------------------------------------
static Py_ssize_t PySVTKObject_AsBuffer_GetSegCount(PyObject* op, Py_ssize_t* lenp)
{
  PySVTKObject* self = (PySVTKObject*)op;
  svtkDataArray* da = svtkDataArray::SafeDownCast(self->svtk_ptr);
  if (da)
  {
    if (lenp)
    {
      *lenp = da->GetNumberOfTuples() * da->GetNumberOfComponents() * da->GetDataTypeSize();
    }

    return 1;
  }

  if (lenp)
  {
    *lenp = 0;
  }
  return 0;
}

//--------------------------------------------------------------------
static Py_ssize_t PySVTKObject_AsBuffer_GetReadBuf(PyObject* op, Py_ssize_t segment, void** ptrptr)
{
  if (segment != 0)
  {
    PyErr_SetString(PyExc_ValueError, "accessing non-existing array segment");
    return -1;
  }

  PySVTKObject* self = (PySVTKObject*)op;
  svtkDataArray* da = svtkDataArray::SafeDownCast(self->svtk_ptr);
  if (da)
  {
    *ptrptr = da->GetVoidPointer(0);
    return da->GetNumberOfTuples() * da->GetNumberOfComponents() * da->GetDataTypeSize();
  }

  return -1;
}

//--------------------------------------------------------------------
static Py_ssize_t PySVTKObject_AsBuffer_GetWriteBuf(PyObject* op, Py_ssize_t segment, void** ptrptr)
{
  return PySVTKObject_AsBuffer_GetReadBuf(op, segment, ptrptr);
}

#endif

#if PY_VERSION_HEX >= 0x02060000

//--------------------------------------------------------------------
// Convert a SVTK type to a python type char (struct module)
static const char* pythonTypeFormat(int t)
{
  const char* b = nullptr;

  switch (t)
  {
    case SVTK_CHAR:
      b = "c";
      break;
    case SVTK_SIGNED_CHAR:
      b = "b";
      break;
    case SVTK_UNSIGNED_CHAR:
      b = "B";
      break;
    case SVTK_SHORT:
      b = "h";
      break;
    case SVTK_UNSIGNED_SHORT:
      b = "H";
      break;
    case SVTK_INT:
      b = "i";
      break;
    case SVTK_UNSIGNED_INT:
      b = "I";
      break;
    case SVTK_LONG:
      b = "l";
      break;
    case SVTK_UNSIGNED_LONG:
      b = "L";
      break;
    case SVTK_LONG_LONG:
      b = "q";
      break;
    case SVTK_UNSIGNED_LONG_LONG:
      b = "Q";
      break;
#if !defined(SVTK_LEGACY_REMOVE)
    case SVTK___INT64:
      b = "q";
      break;
    case SVTK_UNSIGNED___INT64:
      b = "Q";
      break;
#endif
    case SVTK_FLOAT:
      b = "f";
      break;
    case SVTK_DOUBLE:
      b = "d";
      break;
#ifndef SVTK_USE_64BIT_IDS
    case SVTK_ID_TYPE:
      b = "i";
      break;
#else
    case SVTK_ID_TYPE:
      b = "q";
      break;
#endif
  }

  return b;
}

//--------------------------------------------------------------------
static int PySVTKObject_AsBuffer_GetBuffer(PyObject* obj, Py_buffer* view, int flags)
{
  PySVTKObject* self = (PySVTKObject*)obj;
  svtkDataArray* da = svtkDataArray::SafeDownCast(self->svtk_ptr);
  if (da)
  {
    void* ptr = da->GetVoidPointer(0);
    Py_ssize_t ntuples = da->GetNumberOfTuples();
    int ncomp = da->GetNumberOfComponents();
    int dsize = da->GetDataTypeSize();
    const char* format = pythonTypeFormat(da->GetDataType());
    Py_ssize_t size = ntuples * ncomp * dsize;

    if (da->GetDataType() == SVTK_BIT)
    {
      size = (ntuples * ncomp + 7) / 8;
    }

    // start by building a basic "unsigned char" buffer
    if (PyBuffer_FillInfo(view, obj, ptr, size, 0, flags) == -1)
    {
      return -1;
    }
    // check if a dimensioned array was requested
    if (format != nullptr && (flags & PyBUF_ND) != 0)
    {
      // first, build a simple 1D array
      view->itemsize = dsize;
      view->ndim = (ncomp > 1 ? 2 : 1);
      view->format = const_cast<char*>(format);

#if PY_VERSION_HEX >= 0x02070000 && PY_VERSION_HEX < 0x03030000
      // use "smalltable" for 1D arrays, like memoryobject.c
      view->shape = view->smalltable;
      view->strides = &view->smalltable[1];
      if (view->ndim > 1)
#endif
      {
        if (self->svtk_buffer && self->svtk_buffer[0] != view->ndim)
        {
          delete[] self->svtk_buffer;
          self->svtk_buffer = nullptr;
        }
        if (self->svtk_buffer == nullptr)
        {
          self->svtk_buffer = new Py_ssize_t[2 * view->ndim + 1];
          self->svtk_buffer[0] = view->ndim;
        }
        view->shape = &self->svtk_buffer[1];
        view->strides = &self->svtk_buffer[view->ndim + 1];
      }

      if (view->ndim == 1)
      {
        // simple one-dimensional array
        view->shape[0] = ntuples * ncomp;
        view->strides[0] = view->itemsize;
      }
      else
      {
        // use native C dimension ordering by default
        char order = 'C';
        if ((flags & PyBUF_ANY_CONTIGUOUS) == PyBUF_F_CONTIGUOUS)
        {
          // use fortran ordering only if explicitly requested
          order = 'F';
        }
        // need to allocate space for the strides and shape
        view->shape[0] = ntuples;
        view->shape[1] = ncomp;
        if (order == 'F')
        {
          view->shape[0] = ncomp;
          view->shape[1] = ntuples;
        }
        PyBuffer_FillContiguousStrides(view->ndim, view->shape, view->strides, dsize, order);
      }
    }
    return 0;
  }

  PyErr_Format(PyExc_ValueError, "Cannot get a buffer from %s.", Py_TYPE(obj)->tp_name);
  return -1;
}

//--------------------------------------------------------------------
static void PySVTKObject_AsBuffer_ReleaseBuffer(PyObject* obj, Py_buffer* view)
{
  // nothing to do, the caller will decref the obj
  (void)obj;
  (void)view;
}

#endif

//--------------------------------------------------------------------
PyBufferProcs PySVTKObject_AsBuffer = {
#ifndef SVTK_PY3K
  PySVTKObject_AsBuffer_GetReadBuf,  // bf_getreadbuffer
  PySVTKObject_AsBuffer_GetWriteBuf, // bf_getwritebuffer
  PySVTKObject_AsBuffer_GetSegCount, // bf_getsegcount
  nullptr,                          // bf_getcharbuffer
#endif
#if PY_VERSION_HEX >= 0x02060000
  PySVTKObject_AsBuffer_GetBuffer,    // bf_getbuffer
  PySVTKObject_AsBuffer_ReleaseBuffer // bf_releasebuffer
#endif
};

//--------------------------------------------------------------------
PyObject* PySVTKObject_FromPointer(PyTypeObject* pytype, PyObject* pydict, svtkObjectBase* ptr)
{
  // This will be set if we create a new C++ object
  bool created = false;
  std::string classname = svtkPythonUtil::StripModule(pytype->tp_name);
  PySVTKClass* cls = nullptr;

  if (ptr)
  {
    // If constructing from an existing C++ object, use its actual class
    classname = ptr->GetClassName();
    cls = svtkPythonUtil::FindClass(classname.c_str());
  }

  if (cls == nullptr)
  {
    // Use the svtkname of the supplied class type
    PyObject* s = PyObject_GetAttrString((PyObject*)pytype, "__svtkname__");
    if (s)
    {
#ifdef SVTK_PY3K
      PyObject* tmp = PyUnicode_AsUTF8String(s);
      if (tmp)
      {
        Py_DECREF(s);
        s = tmp;
      }
#endif
      const char* svtkname_classname = PyBytes_AsString(s);
      if (svtkname_classname == nullptr)
      {
        Py_DECREF(s);
        return nullptr;
      }
      classname = svtkname_classname;
      Py_DECREF(s);
    }
    cls = svtkPythonUtil::FindClass(classname.c_str());
    if (cls == nullptr)
    {
      PyErr_Format(PyExc_ValueError, "internal error, unknown SVTK class %.200s", classname.c_str());
      return nullptr;
    }
  }

  if (!ptr)
  {
    // Create a new instance of this class since we were not given one.
    if (cls->svtk_new)
    {
      ptr = cls->svtk_new();
      if (!ptr)
      {
        // The svtk_new() method returns null when a factory class has no
        // implementation (i.e. cannot provide a concrete class instance.)
        // NotImplementedError indicates a pure virtual method call.
        PyErr_SetString(
          PyExc_NotImplementedError, "no concrete implementation exists for this class");
        return nullptr;
      }

      // Check if the SVTK object already has a Python object
      // (e.g. svtk_new() might return a singleton instance)
      PyObject* obj = svtkPythonUtil::FindObject(ptr);
      if (obj)
      {
        ptr->Delete();
        return obj;
      }

      // flag to indicate that the SVTK object is a new instance
      created = true;

      // Check the type of the newly-created object
      const char* newclassname = ptr->GetClassName();
      if (std::string(newclassname) != classname)
      {
        PySVTKClass* newclass = svtkPythonUtil::FindClass(newclassname);
        if (newclass)
        {
          classname = newclassname;
          cls = newclass;
        }
      }
    }
    else
    {
      PyErr_SetString(PyExc_TypeError, "this is an abstract class and cannot be instantiated");
      return nullptr;
    }
  }

  if ((pytype->tp_flags & Py_TPFLAGS_HEAPTYPE) != 0)
  {
    // Incref if class was declared in python (see PyType_GenericAlloc).
    Py_INCREF(pytype);
  }
  else
  {
    // To support factory New methods, use the object's actual class
    pytype = cls->py_type;
  }

  // Create a new dict unless one was provided
  if (pydict)
  {
    Py_INCREF(pydict);
  }
  else
  {
    pydict = PyDict_New();
  }

  PySVTKObject* self = PyObject_GC_New(PySVTKObject, pytype);

  self->svtk_ptr = ptr;
  self->svtk_flags = 0;
  self->svtk_class = cls;
  self->svtk_dict = pydict;
  self->svtk_buffer = nullptr;
  self->svtk_observers = nullptr;
  self->svtk_weakreflist = nullptr;

  PyObject_GC_Track((PyObject*)self);

  // A python object owning a SVTK object reference is getting
  // created.  Add the python object's SVTK object reference.
  svtkPythonUtil::AddObjectToMap((PyObject*)self, ptr);

  // The hash now owns a reference so we can free ours.
  if (created)
  {
    ptr->Delete();
  }

  return (PyObject*)self;
}

svtkObjectBase* PySVTKObject_GetObject(PyObject* obj)
{
  return ((PySVTKObject*)obj)->svtk_ptr;
}

void PySVTKObject_AddObserver(PyObject* obj, unsigned long id)
{
  unsigned long* olist = ((PySVTKObject*)obj)->svtk_observers;
  unsigned long n = 0;
  if (olist == nullptr)
  {
    olist = new unsigned long[8];
    ((PySVTKObject*)obj)->svtk_observers = olist;
  }
  else
  {
    // count the number of items
    while (olist[n] != 0)
    {
      n++;
    }
    // check if n+1 is a power of two (base allocation is 8)
    unsigned long m = n + 1;
    if (m >= 8 && (n & m) == 0)
    {
      unsigned long* tmp = olist;
      olist = new unsigned long[2 * m];
      for (unsigned long i = 0; i < n; i++)
      {
        olist[i] = tmp[i];
      }
      delete[] tmp;
      ((PySVTKObject*)obj)->svtk_observers = olist;
    }
  }
  olist[n++] = id;
  olist[n] = 0;
}

unsigned int PySVTKObject_GetFlags(PyObject* obj)
{
  return ((PySVTKObject*)obj)->svtk_flags;
}

void PySVTKObject_SetFlag(PyObject* obj, unsigned int flag, int val)
{
  if (val)
  {
    ((PySVTKObject*)obj)->svtk_flags |= flag;
  }
  else
  {
    ((PySVTKObject*)obj)->svtk_flags &= ~flag;
  }
}
