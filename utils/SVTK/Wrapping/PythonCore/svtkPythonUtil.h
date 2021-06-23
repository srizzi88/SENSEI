/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPythonUtil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPythonUtil
 */

#ifndef svtkPythonUtil_h
#define svtkPythonUtil_h

#include "PySVTKNamespace.h"
#include "PySVTKObject.h"
#include "PySVTKReference.h"
#include "PySVTKSpecialObject.h"
#include "svtkPython.h"
#include "svtkPythonCompatibility.h"

#if defined(_MSC_VER) // Visual Studio
// some docstrings trigger "decimal digit terminates octal escape sequence"
#pragma warning(disable : 4125)
#endif

class svtkPythonClassMap;
class svtkPythonCommand;
class svtkPythonCommandList;
class svtkPythonGhostMap;
class svtkPythonObjectMap;
class svtkPythonSpecialTypeMap;
class svtkPythonNamespaceMap;
class svtkPythonEnumMap;
class svtkPythonModuleList;
class svtkStdString;
class svtkUnicodeString;
class svtkVariant;

extern "C" void svtkPythonUtilDelete();

class SVTKWRAPPINGPYTHONCORE_EXPORT svtkPythonUtil
{
public:
  /**
   * If the name is templated or mangled, converts it into
   * a python-printable name.
   */
  static const char* PythonicClassName(const char* classname);

  /**
   * Given a qualified python name "module.name", remove "module.".
   */
  static const char* StripModule(const char* tpname);

  /**
   * Add a PySVTKClass to the type lookup table, this allows us to later
   * create object given only the class name.
   */
  static PyTypeObject* AddClassToMap(
    PyTypeObject* pytype, PyMethodDef* methods, const char* classname, svtknewfunc constructor);

  /**
   * Get information about a special SVTK type, given the type name.
   */
  static PySVTKClass* FindClass(const char* classname);

  /**
   * For an SVTK object whose class is not in the ClassMap, search
   * the whole ClassMap to find out which class is the closest base
   * class of the object.  Returns a PySVTKClass.
   */
  static PySVTKClass* FindNearestBaseClass(svtkObjectBase* ptr);

  /**
   * Extract the svtkObjectBase from a PySVTKObject.  If the PyObject is
   * not a PySVTKObject, or is not a PySVTKObject of the specified type,
   * the python error indicator will be set.
   * Special behavior: Py_None is converted to NULL without no error.
   */
  static svtkObjectBase* GetPointerFromObject(PyObject* obj, const char* classname);

  /**
   * Convert a svtkObjectBase to a PySVTKObject.  This will first check to
   * see if the PySVTKObject already exists, and create a new PySVTKObject
   * if necessary.  This function also passes ownership of the reference
   * to the PyObject.
   * Special behaviour: NULL is converted to Py_None.
   *
   * **Return value: New reference.**
   */
  static PyObject* GetObjectFromPointer(svtkObjectBase* ptr);

  /**
   * Try to convert some PyObject into a PySVTKObject, currently conversion
   * is supported for SWIG-style mangled pointer strings.
   */
  static PyObject* GetObjectFromObject(PyObject* arg, const char* type);

  /**
   * Add PySVTKObject/svtkObjectBase pairs to the internal mapping.
   * This methods do not change the reference counts of either the
   * svtkObjectBase or the PySVTKObject.
   */
  static void AddObjectToMap(PyObject* obj, svtkObjectBase* anInstance);

  /**
   * Remove a PySVTKObject from the internal mapping.  No reference
   * counts are changed.
   */
  static void RemoveObjectFromMap(PyObject* obj);

  /**
   * Find the PyObject for a SVTK object, return nullptr if not found.
   * If the object is found, then it is returned as a new reference.
   * Special behavior: If "ptr" is nullptr, then Py_None is returned.
   *
   * **Return value: New reference.**
   */
  static PyObject* FindObject(svtkObjectBase* ptr);

  /**
   * Add a special SVTK type to the type lookup table, this allows us to
   * later create object given only the class name.
   */
  static PyTypeObject* AddSpecialTypeToMap(
    PyTypeObject* pytype, PyMethodDef* methods, PyMethodDef* constructors, svtkcopyfunc copyfunc);

  /**
   * Get information about a special SVTK type, given the type name.
   */
  static PySVTKSpecialType* FindSpecialType(const char* classname);

  /**
   * Given a PyObject, convert it into a "result_type" object, where
   * "result_type" must be a wrapped type.  The C object is returned
   * as a void *, which must be cast to a pointer of the desired type.
   * If conversion was necessary, then the created python object is
   * returned in "newobj", but if the original python object was
   * already of the correct type, then "newobj" will be set to NULL.
   * If a python exception was raised, NULL will be returned.
   */
  static void* GetPointerFromSpecialObject(
    PyObject* obj, const char* result_type, PyObject** newobj);

  /**
   * Add a wrapped C++ namespace as a python module object.  This allows
   * the namespace to be retrieved and added to as necessary.
   */
  static void AddNamespaceToMap(PyObject* o);

  /**
   * Remove a wrapped C++ namespace from consideration.  This is called
   * from the namespace destructor.
   */
  static void RemoveNamespaceFromMap(PyObject* o);

  /**
   * Return an existing namespace, or NULL if it doesn't exist.
   */
  static PyObject* FindNamespace(const char* name);

  /**
   * Add a wrapped C++ enum as a python type object.
   */
  static void AddEnumToMap(PyTypeObject* o, const char* name);

  /**
   * Return an enum type object, or NULL if it doesn't exist.
   */
  static PyTypeObject* FindEnum(const char* name);

  /**
   * Find the PyTypeObject for a wrapped SVTK class.
   */
  static PyTypeObject* FindClassTypeObject(const char* name);

  /**
   * Find the PyTypeObject for a wrapped SVTK type (non-svtkObject class).
   */
  static PyTypeObject* FindSpecialTypeObject(const char* name);

  /**
   * Try to load an extension module, by looking in all the usual places.
   * The "globals" is the dict of the module that is doing the importing.
   * First, a relative import is performed, and if that fails, then a
   * global import is performed.  A return value of "false" indicates
   * failure, no exception is set.
   */
  static bool ImportModule(const char* name, PyObject* globals);

  /**
   * Modules call this to add themselves to the list of loaded modules.
   * This is needed because we do not know how the modules are arranged
   * within their package, so searching sys.modules is unreliable.  It is
   * best for us to keep our own list.
   */
  static void AddModule(const char* name);

  /**
   * Utility function to build a docstring by concatenating a series
   * of strings until a null string is found.
   */
  static PyObject* BuildDocString(const char* docstring[]);

  /**
   * Utility function for creating SWIG-style mangled pointer string.
   */
  static char* ManglePointer(const void* ptr, const char* type);

  /**
   * Utility function decoding a SWIG-style mangled pointer string.
   */
  static void* UnmanglePointer(char* ptrText, int* len, const char* type);

  /**
   * Compute a hash for a svtkVariant.
   */
  static Py_hash_t VariantHash(const svtkVariant* variant);

  //@{
  /**
   * Register a svtkPythonCommand. Registering svtkPythonCommand instances ensures
   * that when the interpreter is destroyed (and Py_AtExit() gets called), the
   * svtkPythonCommand state is updated to avoid referring to dangling Python
   * objects pointers. Note, this will not work with Py_NewInterpreter.
   */
  static void RegisterPythonCommand(svtkPythonCommand*);
  static void UnRegisterPythonCommand(svtkPythonCommand*);
  //@}

private:
  svtkPythonUtil();
  ~svtkPythonUtil();
  svtkPythonUtil(const svtkPythonUtil&) = delete;
  void operator=(const svtkPythonUtil&) = delete;

  svtkPythonObjectMap* ObjectMap;
  svtkPythonGhostMap* GhostMap;
  svtkPythonClassMap* ClassMap;
  svtkPythonSpecialTypeMap* SpecialTypeMap;
  svtkPythonNamespaceMap* NamespaceMap;
  svtkPythonEnumMap* EnumMap;
  svtkPythonModuleList* ModuleList;
  svtkPythonCommandList* PythonCommandList;

  friend void svtkPythonUtilDelete();
  friend void svtkPythonUtilCreateIfNeeded();
};

// For use by SetXXMethod() , SetXXMethodArgDelete()
extern SVTKWRAPPINGPYTHONCORE_EXPORT void svtkPythonVoidFunc(void*);
extern SVTKWRAPPINGPYTHONCORE_EXPORT void svtkPythonVoidFuncArgDelete(void*);

#endif
// SVTK-HeaderTest-Exclude: svtkPythonUtil.h
