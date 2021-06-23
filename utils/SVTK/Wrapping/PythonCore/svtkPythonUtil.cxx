/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPythonUtil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPythonUtil.h"
#include "svtkPythonOverload.h"

#include "svtkSystemIncludes.h"

#include "svtkObject.h"
#include "svtkPythonCommand.h"
#include "svtkSmartPointerBase.h"
#include "svtkStdString.h"
#include "svtkToolkits.h"
#include "svtkUnicodeString.h"
#include "svtkVariant.h"
#include "svtkWeakPointer.h"
#include "svtkWindows.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// for uintptr_t
#ifdef _MSC_VER
#include <stddef.h>
#else
#include <cstdint>
#endif

//--------------------------------------------------------------------
// A ghost object, can be used to recreate a deleted PySVTKObject
class PySVTKObjectGhost
{
public:
  PySVTKObjectGhost()
    : svtk_ptr()
    , svtk_class(nullptr)
    , svtk_dict(nullptr)
  {
  }

  svtkWeakPointerBase svtk_ptr;
  PyTypeObject* svtk_class;
  PyObject* svtk_dict;
};

//--------------------------------------------------------------------
// There are six maps associated with the Python wrappers

// Map SVTK objects to python objects (this is also the cornerstone
// of the svtk/python garbage collection system, because it contains
// exactly one pointer reference for each SVTK object known to python)
class svtkPythonObjectMap
  : public std::map<svtkObjectBase*, std::pair<PyObject*, std::atomic<int32_t> > >
{
public:
  ~svtkPythonObjectMap();

  void add(svtkObjectBase* key, PyObject* value);
  void remove(svtkObjectBase* key);
};

// Call Delete instead of relying on svtkSmartPointer, so that crashes
// caused by deletion are easier to follow in the debug stack trace
svtkPythonObjectMap::~svtkPythonObjectMap()
{
  iterator i;
  for (i = this->begin(); i != this->end(); ++i)
  {
    for (int j = 0; j < i->second.second; ++j)
    {
      i->first->Delete();
    }
  }
}

void svtkPythonObjectMap::add(svtkObjectBase* key, PyObject* value)
{
  key->Register(nullptr);
  iterator i = this->find(key);
  if (i == this->end())
  {
    (*this)[key] = std::make_pair(value, 1);
  }
  else
  {
    i->second.first = value;
    ++i->second.second;
  }
}

void svtkPythonObjectMap::remove(svtkObjectBase* key)
{
  iterator i = this->find(key);
  if (i != this->end())
  {
    // Save the object. The iterator will become invalid if the iterator is
    // erased.
    svtkObjectBase* obj = i->first;
    // Remove it from the map if necessary.
    if (!--i->second.second)
    {
      this->erase(i);
    }
    // Remove a reference to the object. This must be done *after* removing it
    // from the map (if needed) because if there's a callback which reacts when
    // the reference is dropped, it might call RemoveObjectFromMap as well. If
    // it still exists in the map at that point, this becomes an infinite loop.
    obj->Delete();
  }
}

// Keep weak pointers to SVTK objects that python no longer has
// references to.  Python keeps the python 'dict' for SVTK objects
// even when they pass leave the python realm, so that if those
// SVTK objects come back, their 'dict' can be restored to them.
// Periodically the weak pointers are checked and the dicts of
// SVTK objects that have been deleted are tossed away.
class svtkPythonGhostMap : public std::map<svtkObjectBase*, PySVTKObjectGhost>
{
};

// Keep track of all the SVTK classes that python knows about.
class svtkPythonClassMap : public std::map<std::string, PySVTKClass>
{
};

// Like the ClassMap, for types not derived from svtkObjectBase.
class svtkPythonSpecialTypeMap : public std::map<std::string, PySVTKSpecialType>
{
};

// Keep track of all the C++ namespaces that have been wrapped.
class svtkPythonNamespaceMap : public std::map<std::string, PyObject*>
{
};

// Keep track of all the C++ enums that have been wrapped.
class svtkPythonEnumMap : public std::map<std::string, PyTypeObject*>
{
};

// Keep track of all the SVTK-Python extension modules
class svtkPythonModuleList : public std::vector<std::string>
{
};

// Keep track of all svtkPythonCommand instances.
class svtkPythonCommandList : public std::vector<svtkWeakPointer<svtkPythonCommand> >
{
public:
  ~svtkPythonCommandList()
  {
    iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter)
    {
      if (iter->GetPointer())
      {
        iter->GetPointer()->obj = nullptr;
        iter->GetPointer()->ThreadState = nullptr;
      }
    }
  }
  void findAndErase(svtkPythonCommand* ptr)
  {
    this->erase(std::remove(this->begin(), this->end(), ptr), this->end());
  }
};

//--------------------------------------------------------------------
// The singleton for svtkPythonUtil

static svtkPythonUtil* svtkPythonMap = nullptr;

// destructs the singleton when python exits
void svtkPythonUtilDelete()
{
  delete svtkPythonMap;
  svtkPythonMap = nullptr;
}

// constructs the singleton
void svtkPythonUtilCreateIfNeeded()
{
  if (svtkPythonMap == nullptr)
  {
    svtkPythonMap = new svtkPythonUtil();
    Py_AtExit(svtkPythonUtilDelete);
  }
}

//--------------------------------------------------------------------
svtkPythonUtil::svtkPythonUtil()
{
  this->ObjectMap = new svtkPythonObjectMap;
  this->GhostMap = new svtkPythonGhostMap;
  this->ClassMap = new svtkPythonClassMap;
  this->SpecialTypeMap = new svtkPythonSpecialTypeMap;
  this->NamespaceMap = new svtkPythonNamespaceMap;
  this->EnumMap = new svtkPythonEnumMap;
  this->ModuleList = new svtkPythonModuleList;
  this->PythonCommandList = new svtkPythonCommandList;
}

//--------------------------------------------------------------------
svtkPythonUtil::~svtkPythonUtil()
{
  delete this->ObjectMap;
  delete this->GhostMap;
  delete this->ClassMap;
  delete this->SpecialTypeMap;
  delete this->NamespaceMap;
  delete this->EnumMap;
  delete this->ModuleList;
  delete this->PythonCommandList;
}

//--------------------------------------------------------------------
void svtkPythonUtil::RegisterPythonCommand(svtkPythonCommand* cmd)
{
  if (cmd)
  {
    svtkPythonUtilCreateIfNeeded();
    svtkPythonMap->PythonCommandList->push_back(cmd);
  }
}

//--------------------------------------------------------------------
void svtkPythonUtil::UnRegisterPythonCommand(svtkPythonCommand* cmd)
{
  if (cmd && svtkPythonMap)
  {
    svtkPythonMap->PythonCommandList->findAndErase(cmd);
  }
}

//--------------------------------------------------------------------
PyTypeObject* svtkPythonUtil::AddSpecialTypeToMap(
  PyTypeObject* pytype, PyMethodDef* methods, PyMethodDef* constructors, svtkcopyfunc copyfunc)
{
  const char* classname = svtkPythonUtil::StripModule(pytype->tp_name);
  svtkPythonUtilCreateIfNeeded();

  // lets make sure it isn't already there
  svtkPythonSpecialTypeMap::iterator i = svtkPythonMap->SpecialTypeMap->find(classname);
  if (i == svtkPythonMap->SpecialTypeMap->end())
  {
    i = svtkPythonMap->SpecialTypeMap->insert(i,
      svtkPythonSpecialTypeMap::value_type(
        classname, PySVTKSpecialType(pytype, methods, constructors, copyfunc)));
  }

  return i->second.py_type;
}

//--------------------------------------------------------------------
PySVTKSpecialType* svtkPythonUtil::FindSpecialType(const char* classname)
{
  if (svtkPythonMap)
  {
    svtkPythonSpecialTypeMap::iterator it = svtkPythonMap->SpecialTypeMap->find(classname);

    if (it != svtkPythonMap->SpecialTypeMap->end())
    {
      return &it->second;
    }
  }

  return nullptr;
}

//--------------------------------------------------------------------
void svtkPythonUtil::AddObjectToMap(PyObject* obj, svtkObjectBase* ptr)
{
  svtkPythonUtilCreateIfNeeded();

#ifdef SVTKPYTHONDEBUG
  svtkGenericWarningMacro("Adding an object to map ptr = " << ptr);
#endif

  ((PySVTKObject*)obj)->svtk_ptr = ptr;
  svtkPythonMap->ObjectMap->add(ptr, obj);

#ifdef SVTKPYTHONDEBUG
  svtkGenericWarningMacro("Added object to map obj= " << obj << " " << ptr);
#endif
}

//--------------------------------------------------------------------
void svtkPythonUtil::RemoveObjectFromMap(PyObject* obj)
{
  PySVTKObject* pobj = (PySVTKObject*)obj;

#ifdef SVTKPYTHONDEBUG
  svtkGenericWarningMacro("Deleting an object from map obj = " << pobj << " " << pobj->svtk_ptr);
#endif

  if (svtkPythonMap && svtkPythonMap->ObjectMap->count(pobj->svtk_ptr))
  {
    svtkWeakPointerBase wptr;

    // check for customized class or dict
    if (pobj->svtk_class->py_type != Py_TYPE(pobj) || PyDict_Size(pobj->svtk_dict))
    {
      wptr = pobj->svtk_ptr;
    }

    svtkPythonMap->ObjectMap->remove(pobj->svtk_ptr);

    // if the SVTK object still exists, then make a ghost
    if (wptr.GetPointer())
    {
      // List of attrs to be deleted
      std::vector<PyObject*> delList;

      // Erase ghosts of SVTK objects that have been deleted
      svtkPythonGhostMap::iterator i = svtkPythonMap->GhostMap->begin();
      while (i != svtkPythonMap->GhostMap->end())
      {
        if (!i->second.svtk_ptr.GetPointer())
        {
          delList.push_back((PyObject*)i->second.svtk_class);
          delList.push_back(i->second.svtk_dict);
          svtkPythonMap->GhostMap->erase(i++);
        }
        else
        {
          ++i;
        }
      }

      // Add this new ghost to the map
      PySVTKObjectGhost& g = (*svtkPythonMap->GhostMap)[pobj->svtk_ptr];
      g.svtk_ptr = wptr;
      g.svtk_class = Py_TYPE(pobj);
      g.svtk_dict = pobj->svtk_dict;
      Py_INCREF(g.svtk_class);
      Py_INCREF(g.svtk_dict);

      // Delete attrs of erased objects.  Must be done at the end.
      for (size_t j = 0; j < delList.size(); j++)
      {
        Py_DECREF(delList[j]);
      }
    }
  }
}

//--------------------------------------------------------------------
PyObject* svtkPythonUtil::FindObject(svtkObjectBase* ptr)
{
  PyObject* obj = nullptr;

  if (ptr && svtkPythonMap)
  {
    svtkPythonObjectMap::iterator i = svtkPythonMap->ObjectMap->find(ptr);
    if (i != svtkPythonMap->ObjectMap->end())
    {
      obj = i->second.first;
    }
    if (obj)
    {
      Py_INCREF(obj);
      return obj;
    }
  }
  else
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // search weak list for object, resurrect if it is there
  svtkPythonGhostMap::iterator j = svtkPythonMap->GhostMap->find(ptr);
  if (j != svtkPythonMap->GhostMap->end())
  {
    if (j->second.svtk_ptr.GetPointer())
    {
      obj = PySVTKObject_FromPointer(j->second.svtk_class, j->second.svtk_dict, ptr);
    }
    Py_DECREF(j->second.svtk_class);
    Py_DECREF(j->second.svtk_dict);
    svtkPythonMap->GhostMap->erase(j);
  }

  return obj;
}

//--------------------------------------------------------------------
PyObject* svtkPythonUtil::GetObjectFromPointer(svtkObjectBase* ptr)
{
  PyObject* obj = svtkPythonUtil::FindObject(ptr);

  if (obj == nullptr)
  {
    // create a new object
    PySVTKClass* svtkclass = nullptr;
    svtkPythonClassMap::iterator k = svtkPythonMap->ClassMap->find(ptr->GetClassName());
    if (k != svtkPythonMap->ClassMap->end())
    {
      svtkclass = &k->second;
    }

    // if the class was not in the map, then find the nearest base class
    // that is, and associate ptr->GetClassName() with that base class
    if (svtkclass == nullptr)
    {
      const char* classname = ptr->GetClassName();
      svtkclass = svtkPythonUtil::FindNearestBaseClass(ptr);
      svtkPythonClassMap::iterator i = svtkPythonMap->ClassMap->find(classname);
      if (i == svtkPythonMap->ClassMap->end())
      {
        svtkPythonMap->ClassMap->insert(i, svtkPythonClassMap::value_type(classname, *svtkclass));
      }
    }

    obj = PySVTKObject_FromPointer(svtkclass->py_type, nullptr, ptr);
  }

  return obj;
}

//--------------------------------------------------------------------
const char* svtkPythonUtil::PythonicClassName(const char* classname)
{
  const char* cp = classname;

  /* check for non-alphanumeric chars */
  if (isalpha(*cp) || *cp == '_')
  {
    do
    {
      cp++;
    } while (isalnum(*cp) || *cp == '_');
  }

  if (*cp != '\0')
  {
    /* look up class and get its pythonic name */
    PyTypeObject* pytype = svtkPythonUtil::FindClassTypeObject(classname);
    if (pytype)
    {
      classname = svtkPythonUtil::StripModule(pytype->tp_name);
    }
  }

  return classname;
}

//--------------------------------------------------------------------
const char* svtkPythonUtil::StripModule(const char* tpname)
{
  const char* cp = tpname;
  const char* strippedname = tpname;
  while (*cp != '\0')
  {
    if (*cp++ == '.')
    {
      strippedname = cp;
    }
  }
  return strippedname;
}

//--------------------------------------------------------------------
PyTypeObject* svtkPythonUtil::AddClassToMap(
  PyTypeObject* pytype, PyMethodDef* methods, const char* classname, svtknewfunc constructor)
{
  svtkPythonUtilCreateIfNeeded();

  // lets make sure it isn't already there
  svtkPythonClassMap::iterator i = svtkPythonMap->ClassMap->find(classname);
  if (i == svtkPythonMap->ClassMap->end())
  {
    i = svtkPythonMap->ClassMap->insert(i,
      svtkPythonClassMap::value_type(
        classname, PySVTKClass(pytype, methods, classname, constructor)));
  }

  return i->second.py_type;
}

//--------------------------------------------------------------------
PySVTKClass* svtkPythonUtil::FindClass(const char* classname)
{
  if (svtkPythonMap)
  {
    svtkPythonClassMap::iterator it = svtkPythonMap->ClassMap->find(classname);
    if (it != svtkPythonMap->ClassMap->end())
    {
      return &it->second;
    }
  }

  return nullptr;
}

//--------------------------------------------------------------------
// this is a helper function to find the nearest base class for an
// object whose class is not in the ClassDict
PySVTKClass* svtkPythonUtil::FindNearestBaseClass(svtkObjectBase* ptr)
{
  PySVTKClass* nearestbase = nullptr;
  int maxdepth = 0;
  int depth;

  for (svtkPythonClassMap::iterator classes = svtkPythonMap->ClassMap->begin();
       classes != svtkPythonMap->ClassMap->end(); ++classes)
  {
    PySVTKClass* pyclass = &classes->second;

    if (ptr->IsA(pyclass->svtk_name))
    {
      PyTypeObject* base = pyclass->py_type->tp_base;
      // count the hierarchy depth for this class
      for (depth = 0; base != nullptr; depth++)
      {
        base = base->tp_base;
      }
      // we want the class that is furthest from svtkObjectBase
      if (depth > maxdepth)
      {
        maxdepth = depth;
        nearestbase = pyclass;
      }
    }
  }

  return nearestbase;
}

//--------------------------------------------------------------------
svtkObjectBase* svtkPythonUtil::GetPointerFromObject(PyObject* obj, const char* result_type)
{
  svtkObjectBase* ptr;

  // convert Py_None to nullptr every time
  if (obj == Py_None)
  {
    return nullptr;
  }

  // check to ensure it is a svtk object
  if (!PySVTKObject_Check(obj))
  {
    obj = PyObject_GetAttrString(obj, "__svtk__");
    if (obj)
    {
      PyObject* arglist = Py_BuildValue("()");
      PyObject* result = PyEval_CallObject(obj, arglist);
      Py_DECREF(arglist);
      Py_DECREF(obj);
      if (result == nullptr)
      {
        return nullptr;
      }
      if (!PySVTKObject_Check(result))
      {
        PyErr_SetString(PyExc_TypeError, "__svtk__() doesn't return a SVTK object");
        Py_DECREF(result);
        return nullptr;
      }
      else
      {
        ptr = ((PySVTKObject*)result)->svtk_ptr;
        Py_DECREF(result);
      }
    }
    else
    {
#ifdef SVTKPYTHONDEBUG
      svtkGenericWarningMacro("Object " << obj << " is not a SVTK object!!");
#endif
      PyErr_SetString(PyExc_TypeError, "method requires a SVTK object");
      return nullptr;
    }
  }
  else
  {
    ptr = ((PySVTKObject*)obj)->svtk_ptr;
  }

#ifdef SVTKPYTHONDEBUG
  svtkGenericWarningMacro("Checking into obj " << obj << " ptr = " << ptr);
#endif

  if (ptr->IsA(result_type))
  {
#ifdef SVTKPYTHONDEBUG
    svtkGenericWarningMacro("Got obj= " << obj << " ptr= " << ptr << " " << result_type);
#endif
    return ptr;
  }
  else
  {
    char error_string[2048];
#ifdef SVTKPYTHONDEBUG
    svtkGenericWarningMacro("svtk bad argument, type conversion failed.");
#endif
    snprintf(error_string, sizeof(error_string), "method requires a %.500s, a %.500s was provided.",
      svtkPythonUtil::PythonicClassName(result_type),
      svtkPythonUtil::PythonicClassName(((svtkObjectBase*)ptr)->GetClassName()));
    PyErr_SetString(PyExc_TypeError, error_string);
    return nullptr;
  }
}

//----------------
// union of long int and pointer
union svtkPythonUtilPointerUnion {
  void* p;
  uintptr_t l;
};

//----------------
// union of long int and pointer
union svtkPythonUtilConstPointerUnion {
  const void* p;
  uintptr_t l;
};

//--------------------------------------------------------------------
PyObject* svtkPythonUtil::GetObjectFromObject(PyObject* arg, const char* type)
{
  union svtkPythonUtilPointerUnion u;
  PyObject* tmp = nullptr;

#ifdef Py_USING_UNICODE
  if (PyUnicode_Check(arg))
  {
    tmp = PyUnicode_AsUTF8String(arg);
    arg = tmp;
  }
#endif

  if (PyBytes_Check(arg))
  {
    svtkObjectBase* ptr;
    char* ptrText = PyBytes_AsString(arg);

    char typeCheck[1024]; // typeCheck is currently not used
    unsigned long long l;
    int i = sscanf(ptrText, "_%llx_%s", &l, typeCheck);
    u.l = static_cast<uintptr_t>(l);

    if (i <= 0)
    {
      i = sscanf(ptrText, "Addr=0x%llx", &l);
      u.l = static_cast<uintptr_t>(l);
    }
    if (i <= 0)
    {
      i = sscanf(ptrText, "%p", &u.p);
    }
    if (i <= 0)
    {
      Py_XDECREF(tmp);
      PyErr_SetString(
        PyExc_ValueError, "could not extract hexadecimal address from argument string");
      return nullptr;
    }

    ptr = static_cast<svtkObjectBase*>(u.p);

    if (!ptr->IsA(type))
    {
      char error_string[2048];
      snprintf(error_string, sizeof(error_string),
        "method requires a %.500s address, a %.500s address was provided.", type,
        ptr->GetClassName());
      Py_XDECREF(tmp);
      PyErr_SetString(PyExc_TypeError, error_string);
      return nullptr;
    }

    Py_XDECREF(tmp);
    return svtkPythonUtil::GetObjectFromPointer(ptr);
  }

  Py_XDECREF(tmp);
  PyErr_SetString(PyExc_TypeError, "method requires a string argument");
  return nullptr;
}

//--------------------------------------------------------------------
void* svtkPythonUtil::GetPointerFromSpecialObject(
  PyObject* obj, const char* result_type, PyObject** newobj)
{
  if (svtkPythonMap == nullptr)
  {
    PyErr_SetString(PyExc_TypeError, "method requires a svtkPythonMap");
    return nullptr;
  }

  const char* object_type = svtkPythonUtil::StripModule(Py_TYPE(obj)->tp_name);

  // do a lookup on the desired type
  svtkPythonSpecialTypeMap::iterator it = svtkPythonMap->SpecialTypeMap->find(result_type);
  if (it != svtkPythonMap->SpecialTypeMap->end())
  {
    PySVTKSpecialType* info = &it->second;

    // first, check if object is the desired type
    if (PyObject_TypeCheck(obj, info->py_type))
    {
      return ((PySVTKSpecialObject*)obj)->svtk_ptr;
    }

    // try to construct the special object from the supplied object
    PyObject* sobj = nullptr;

    PyMethodDef* meth = svtkPythonOverload::FindConversionMethod(info->svtk_constructors, obj);

    // If a constructor signature exists for "obj", call it
    if (meth && meth->ml_meth)
    {
      PyObject* args = PyTuple_New(1);
      PyTuple_SET_ITEM(args, 0, obj);
      Py_INCREF(obj);

      sobj = meth->ml_meth(nullptr, args);

      Py_DECREF(args);
    }

    if (sobj && newobj)
    {
      *newobj = sobj;
      return ((PySVTKSpecialObject*)sobj)->svtk_ptr;
    }
    else if (sobj)
    {
      char error_text[2048];
      Py_DECREF(sobj);
      snprintf(error_text, sizeof(error_text), "cannot pass %.500s as a non-const %.500s reference",
        object_type, result_type);
      PyErr_SetString(PyExc_TypeError, error_text);
      return nullptr;
    }

    // If a TypeError occurred, clear it and set our own error
    PyObject* ex = PyErr_Occurred();
    if (ex != nullptr)
    {
      if (PyErr_GivenExceptionMatches(ex, PyExc_TypeError))
      {
        PyErr_Clear();
      }
      else
      {
        return nullptr;
      }
    }
  }

#ifdef SVTKPYTHONDEBUG
  svtkGenericWarningMacro("svtk bad argument, type conversion failed.");
#endif

  char error_string[2048];
  snprintf(error_string, sizeof(error_string), "method requires a %.500s, a %.500s was provided.",
    result_type, object_type);
  PyErr_SetString(PyExc_TypeError, error_string);

  return nullptr;
}

//--------------------------------------------------------------------
void svtkPythonUtil::AddNamespaceToMap(PyObject* module)
{
  if (!PySVTKNamespace_Check(module))
  {
    return;
  }

  svtkPythonUtilCreateIfNeeded();

  const char* name = PySVTKNamespace_GetName(module);
  // let's make sure it isn't already there
  svtkPythonNamespaceMap::iterator i = svtkPythonMap->NamespaceMap->find(name);
  if (i != svtkPythonMap->NamespaceMap->end())
  {
    return;
  }

  (*svtkPythonMap->NamespaceMap)[name] = module;
}

//--------------------------------------------------------------------
// This method is called from PySVTKNamespace_Delete
void svtkPythonUtil::RemoveNamespaceFromMap(PyObject* obj)
{
  if (svtkPythonMap && PySVTKNamespace_Check(obj))
  {
    const char* name = PySVTKNamespace_GetName(obj);
    svtkPythonNamespaceMap::iterator it = svtkPythonMap->NamespaceMap->find(name);
    if (it != svtkPythonMap->NamespaceMap->end() && it->second == obj)
    {
      // The map has a pointer to the object, but does not hold a
      // reference, therefore there is no decref.
      svtkPythonMap->NamespaceMap->erase(it);
    }
  }
}

//--------------------------------------------------------------------
PyObject* svtkPythonUtil::FindNamespace(const char* name)
{
  if (svtkPythonMap)
  {
    svtkPythonNamespaceMap::iterator it = svtkPythonMap->NamespaceMap->find(name);
    if (it != svtkPythonMap->NamespaceMap->end())
    {
      return it->second;
    }
  }

  return nullptr;
}

//--------------------------------------------------------------------
void svtkPythonUtil::AddEnumToMap(PyTypeObject* enumtype, const char* name)
{
  svtkPythonUtilCreateIfNeeded();

  // Only add to map if it isn't already there
  svtkPythonEnumMap::iterator i = svtkPythonMap->EnumMap->find(name);
  if (i == svtkPythonMap->EnumMap->end())
  {
    (*svtkPythonMap->EnumMap)[name] = enumtype;
  }
}

//--------------------------------------------------------------------
PyTypeObject* svtkPythonUtil::FindEnum(const char* name)
{
  PyTypeObject* pytype = nullptr;

  if (svtkPythonMap)
  {
    svtkPythonEnumMap::iterator it = svtkPythonMap->EnumMap->find(name);
    if (it != svtkPythonMap->EnumMap->end())
    {
      pytype = it->second;
    }
  }

  return pytype;
}

//--------------------------------------------------------------------
PyTypeObject* svtkPythonUtil::FindClassTypeObject(const char* name)
{
  PySVTKClass* info = svtkPythonUtil::FindClass(name);
  if (info)
  {
    return info->py_type;
  }

  return nullptr;
}

//--------------------------------------------------------------------
PyTypeObject* svtkPythonUtil::FindSpecialTypeObject(const char* name)
{
  PySVTKSpecialType* info = svtkPythonUtil::FindSpecialType(name);
  if (info)
  {
    return info->py_type;
  }

  return nullptr;
}

//--------------------------------------------------------------------
bool svtkPythonUtil::ImportModule(const char* fullname, PyObject* globals)
{
  // strip all but the final part of the path
  const char* name = std::strrchr(fullname, '.');
  if (name == nullptr)
  {
    name = fullname;
  }
  else if (name[0] == '.')
  {
    name++;
  }

  // check whether the module is already loaded
  if (svtkPythonMap)
  {
    svtkPythonModuleList* ml = svtkPythonMap->ModuleList;
    if (std::find(ml->begin(), ml->end(), name) != ml->end())
    {
      return true;
    }
  }

  PyObject* m = nullptr;

  if (fullname == name || (fullname[0] == '.' && &fullname[1] == name))
  {
    // try relative import (const-cast is needed for Python 2.x only)
    m = PyImport_ImportModuleLevel(const_cast<char*>(name), globals, nullptr, nullptr, 1);
    if (!m)
    {
      PyErr_Clear();
    }
  }

  if (!m)
  {
    // try absolute import
    m = PyImport_ImportModule(fullname);
  }

  if (!m)
  {
    PyErr_Clear();
    return false;
  }

  Py_DECREF(m);
  return true;
}

//--------------------------------------------------------------------
void svtkPythonUtil::AddModule(const char* name)
{
  svtkPythonUtilCreateIfNeeded();

  svtkPythonMap->ModuleList->push_back(name);
}

//--------------------------------------------------------------------
// mangle a void pointer into a SWIG-style string
char* svtkPythonUtil::ManglePointer(const void* ptr, const char* type)
{
  static char ptrText[128];
  int ndigits = 2 * (int)sizeof(void*);
  union svtkPythonUtilConstPointerUnion u;
  u.p = ptr;
  snprintf(ptrText, sizeof(ptrText), "_%*.*llx_%s", ndigits, ndigits,
    static_cast<unsigned long long>(u.l), type);

  return ptrText;
}

//--------------------------------------------------------------------
// unmangle a void pointer from a SWIG-style string
void* svtkPythonUtil::UnmanglePointer(char* ptrText, int* len, const char* type)
{
  int i;
  union svtkPythonUtilPointerUnion u;
  char text[1024];
  char typeCheck[1024];
  typeCheck[0] = '\0';

  // Do some minimal checks that it might be a swig pointer.
  if (*len < 256 && *len > 4 && ptrText[0] == '_')
  {
    strncpy(text, ptrText, *len);
    text[*len] = '\0';
    i = *len;
    // Allow one null byte, in case trailing null is part of *len
    if (i > 0 && text[i - 1] == '\0')
    {
      i--;
    }
    // Verify that there are no other null bytes
    while (i > 0 && text[i - 1] != '\0')
    {
      i--;
    }

    // If no null bytes, then do a full check for a swig pointer
    if (i == 0)
    {
      unsigned long long l;
      i = sscanf(text, "_%llx_%s", &l, typeCheck);
      u.l = static_cast<uintptr_t>(l);

      if (strcmp(type, typeCheck) == 0)
      { // successfully unmangle
        *len = 0;
        return u.p;
      }
      else if (i == 2)
      { // mangled pointer of wrong type
        *len = -1;
        return nullptr;
      }
    }
  }

  // couldn't unmangle: return string as void pointer if it didn't look
  // like a SWIG mangled pointer
  return (void*)ptrText;
}

//--------------------------------------------------------------------
Py_hash_t svtkPythonUtil::VariantHash(const svtkVariant* v)
{
  Py_hash_t h = -1;

  // This uses the same rules as the svtkVariant "==" operator.
  // All types except for svtkObject are converted to strings.
  // Quite inefficient, but it gets the job done.  Fortunately,
  // the python svtkVariant is immutable, so its hash can be cached.

  switch (v->GetType())
  {
    case SVTK_OBJECT:
    {
      h = _Py_HashPointer(v->ToSVTKObject());
      break;
    }

#ifdef Py_USING_UNICODE
    case SVTK_UNICODE_STRING:
    {
      svtkUnicodeString u = v->ToUnicodeString();
      const char* s = u.utf8_str();
      PyObject* tmp = PyUnicode_DecodeUTF8(s, strlen(s), "strict");
      if (tmp == nullptr)
      {
        PyErr_Clear();
        return 0;
      }
      h = PyObject_Hash(tmp);
      Py_DECREF(tmp);
      break;
    }
#endif

    default:
    {
      svtkStdString s = v->ToString();
      PyObject* tmp = PyString_FromString(s.c_str());
      h = PyObject_Hash(tmp);
      Py_DECREF(tmp);
      break;
    }
  }

  return h;
}

//--------------------------------------------------------------------
void svtkPythonVoidFunc(void* arg)
{
  PyObject *arglist, *result;
  PyObject* func = (PyObject*)arg;

  // Sometimes it is possible for the function to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
  {
    return;
  }

#ifndef SVTK_NO_PYTHON_THREADS
  svtkPythonScopeGilEnsurer gilEnsurer(true);
#endif

  arglist = Py_BuildValue("()");

  result = PyEval_CallObject(func, arglist);
  Py_DECREF(arglist);

  if (result)
  {
    Py_XDECREF(result);
  }
  else
  {
    if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
    {
      cerr << "Caught a Ctrl-C within python, exiting program.\n";
      Py_Exit(1);
    }
    PyErr_Print();
  }
}

//--------------------------------------------------------------------
void svtkPythonVoidFuncArgDelete(void* arg)
{
  PyObject* func = (PyObject*)arg;

  // Sometimes it is possible for the function to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
  {
    return;
  }

#ifndef SVTK_NO_PYTHON_THREADS
  svtkPythonScopeGilEnsurer gilEnsurer(true);
#endif

  if (func)
  {
    Py_DECREF(func);
  }
}
