/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPythonItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPythonItem
 * @brief   A svtkContextItem that can be implemented in Python
 *
 *
 * This class allows implementation of arbitrary context items in Python.
 *
 * @sa
 * svtkAbstractContextItem
 */

#ifndef svtkPythonItem_h
#define svtkPythonItem_h
#if !defined(__SVTK_WRAP__) || defined(__SVTK_WRAP_HIERARCHY__) || defined(__SVTK_WRAP_PYTHON__)

#include "svtkPython.h" // Must be first

#include "svtkContextItem.h"
#include "svtkPythonContext2DModule.h" // For export macro

class svtkSmartPyObject;

class SVTKPYTHONCONTEXT2D_EXPORT svtkPythonItem : public svtkContextItem
{
public:
  svtkTypeMacro(svtkPythonItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPythonItem* New();

  /**
   * Specify the Python object to use to operate on the data. A reference will
   * be taken on the object. This will also invoke Initialize() on the Python
   * object, providing an opportunity to perform tasks commonly done in the
   * constructor of C++ svtkContextItem subclasses.
   */
  void SetPythonObject(PyObject* obj);

  bool Paint(svtkContext2D* painter) override;

protected:
  svtkPythonItem();
  ~svtkPythonItem() override;

private:
  svtkPythonItem(const svtkPythonItem&) = delete;
  void operator=(const svtkPythonItem&) = delete;

  bool CheckResult(const char* method, const svtkSmartPyObject& res);

  PyObject* Object;
};

#endif // #ifndef svtkPythonItem_h
#endif
