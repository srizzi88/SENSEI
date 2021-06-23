/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArchiver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPythonArchiver
 * @brief   A version of svtkArchiver that can be implemented in Python
 *
 * svtkPythonArchiver is an implementation of svtkArchiver that calls a Python
 * object to do the actual work.
 * It defers the following methods to Python:
 * - OpenArchive()
 * - CloseArchive()
 * - InsertIntoArchive()
 * - Contains()
 *
 * Python signature of these methods is as follows:
 * - OpenArchive(self, svtkself) : svtkself is the svtk object
 * - CloseArchive(self, svtkself)
 * - InsertIntoArchive(self, svtkself, relativePath, data, size)
 * - Contains()
 *
 * @sa
 * svtkPythonArchiver
 */

#ifndef svtkPythonArchiver_h
#define svtkPythonArchiver_h
#if !defined(__SVTK_WRAP__) || defined(__SVTK_WRAP_HIERARCHY__) || defined(__SVTK_WRAP_PYTHON__)

#include "svtkPython.h" // Must be first

#include "svtkArchiver.h"
#include "svtkCommonPythonModule.h" // For export macro

class svtkSmartPyObject;

class SVTKCOMMONPYTHON_EXPORT svtkPythonArchiver : public svtkArchiver
{
public:
  static svtkPythonArchiver* New();
  svtkTypeMacro(svtkPythonArchiver, svtkArchiver);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Specify the Python object to use to perform the archiving. A reference will
   * be taken on the object.
   */
  void SetPythonObject(PyObject* obj);

  //@{
  /**
   * Open the arhive for writing.
   */
  void OpenArchive() override;
  //@}

  //@{
  /**
   * Close the arhive.
   */
  void CloseArchive() override;
  //@}

  //@{
  /**
   * Insert \p data of size \p size into the archive at \p relativePath.
   */
  void InsertIntoArchive(
    const std::string& relativePath, const char* data, std::size_t size) override;
  //@}

  //@{
  /**
   * Checks if \p relativePath represents an entry in the archive.
   */
  bool Contains(const std::string& relativePath) override;
  //@}

protected:
  svtkPythonArchiver();
  ~svtkPythonArchiver() override;

private:
  svtkPythonArchiver(const svtkPythonArchiver&) = delete;
  void operator=(const svtkPythonArchiver&) = delete;

  int CheckResult(const char* method, const svtkSmartPyObject& res);

  PyObject* Object;
};

#endif
#endif
