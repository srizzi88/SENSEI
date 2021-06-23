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
/**
 * @class   svtkPythonAlgorithm
 * @brief   algorithm that can be implemented in Python
 *
 * svtkPythonAlgorithm is an algorithm that calls a Python object to do the actual
 * work.
 * It defers the following methods to Python:
 * - ProcessRequest()
 * - FillInputPortInformation()
 * - FillOutputPortInformation()
 *
 * Python signature of these methods is as follows:
 * - ProcessRequest(self, svtkself, request, inInfo, outInfo) : svtkself is the svtk object, inInfo is
 * a tuple of information objects
 * - FillInputPortInformation(self, svtkself, port, info)
 * - FillOutputPortInformation(self, svtkself, port, info)
 * - Initialize(self, svtkself)
 *
 * In addition, it calls an Initialize() method when setting the Python
 * object, which allows the initialization of number of input and output
 * ports etc.
 *
 * @sa
 * svtkProgrammableFilter
 */

#ifndef svtkPythonAlgorithm_h
#define svtkPythonAlgorithm_h
#if !defined(__SVTK_WRAP__) || defined(__SVTK_WRAP_HIERARCHY__) || defined(__SVTK_WRAP_PYTHON__)

#include "svtkPython.h" // Must be first

#include "svtkAlgorithm.h"
#include "svtkFiltersPythonModule.h" // For export macro

class svtkSmartPyObject;

class SVTKFILTERSPYTHON_EXPORT svtkPythonAlgorithm : public svtkAlgorithm
{
public:
  static svtkPythonAlgorithm* New();
  svtkTypeMacro(svtkPythonAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Specify the Python object to use to operate on the data. A reference will
   * be taken on the object. This will also invoke Initialize() on the Python
   * object, which is commonly used to set the number of input and output
   * ports as well as perform tasks commonly performed in the constructor
   * of C++ algorithm subclass.
   */
  void SetPythonObject(PyObject* obj);

  /**
   * Set the number of input ports used by the algorithm.
   * This is made public so that it can be called from Python.
   */
  void SetNumberOfInputPorts(int n) override;

  /**
   * Set the number of output ports provided by the algorithm.
   * This is made public so that it can be called from Python.
   */
  void SetNumberOfOutputPorts(int n) override;

protected:
  svtkPythonAlgorithm();
  ~svtkPythonAlgorithm() override;

  svtkTypeBool ProcessRequest(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkPythonAlgorithm(const svtkPythonAlgorithm&) = delete;
  void operator=(const svtkPythonAlgorithm&) = delete;

  int CheckResult(const char* method, const svtkSmartPyObject& res);

  PyObject* Object;
};

#endif
#endif
