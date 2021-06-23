/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSmartPyObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkSmartPyObject
 *
 * The svtkSmartPyObject class serves as a smart pointer for PyObjects.
 */

#ifndef svtkSmartPyObject_h
#define svtkSmartPyObject_h

// this must be included first
#include "svtkPython.h" // PyObject can't be forward declared

#include "svtkWrappingPythonCoreModule.h"

class SVTKWRAPPINGPYTHONCORE_EXPORT svtkSmartPyObject
{
public:
  /**
   * Creates a new svtkSmartPyObject managing the existing reference
   * to the object given
   */
  svtkSmartPyObject(PyObject* obj = nullptr);
  /**
   * Creates a new svtkSmartPyObject to the object in the other smart
   * pointer and increments the reference count to the object
   */
  svtkSmartPyObject(const svtkSmartPyObject& other);
  /**
   * Decrements the reference count to the object
   */
  ~svtkSmartPyObject();

  /**
   * The internal pointer is copied from the other svtkSmartPyObject.
   * The reference count on the old object is decremented and the
   * reference count on the new object is incremented
   */
  svtkSmartPyObject& operator=(const svtkSmartPyObject& other);
  /**
   * Sets the internal pointer to the given PyObject.  The reference
   * count on the PyObject is incremented.  To take a reference without
   * incrementing the reference count use TakeReference.
   */
  svtkSmartPyObject& operator=(PyObject* obj);
  /**
   * Sets the internal pointer to the given PyObject without incrementing
   * the reference count
   */
  void TakeReference(PyObject* obj);

  /**
   * Provides normal pointer target member access using operator ->.
   */
  PyObject* operator->() const;
  /**
   * Get the contained pointer.
   */
  operator PyObject*() const;

  /**
   * Returns true if the internal pointer is to a valid PyObject.
   */
  operator bool() const;

  /**
   * Returns the pointer to a PyObject stored internally and clears the
   * internally stored pointer.  The caller is responsible for calling
   * Py_DECREF on the returned object when finished with it as this
   * does not change the reference count.
   */
  PyObject* ReleaseReference();
  /**
   * Returns the internal pointer to a PyObject with no effect on its
   * reference count
   */
  PyObject* GetPointer() const;
  /**
   * Returns the internal pointer to a PyObject and increments its reference
   * count
   */
  PyObject* GetAndIncreaseReferenceCount();

private:
  PyObject* Object;
};

#endif
// SVTK-HeaderTest-Exclude: svtkSmartPyObject.h
