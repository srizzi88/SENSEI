/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridBaseAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUnstructuredGridBaseAlgorithm
 * @brief   Superclass for algorithms that
 * produce only svtkUnstructureGridBase subclasses as output
 *
 * svtkUnstructuredGridBaseAlgorithm is a convenience class to make writing
 * algorithms easier. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this classes
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be UnstructuredGridBase. If
 * that isn't the case then please override this method in your subclass.
 */

#ifndef svtkUnstructuredGridBaseAlgorithm_h
#define svtkUnstructuredGridBaseAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkDataSet;
class svtkUnstructuredGridBase;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkUnstructuredGridBaseAlgorithm : public svtkAlgorithm
{
public:
  static svtkUnstructuredGridBaseAlgorithm* New();
  svtkTypeMacro(svtkUnstructuredGridBaseAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkUnstructuredGridBase* GetOutput();
  svtkUnstructuredGridBase* GetOutput(int);
  virtual void SetOutput(svtkDataObject* d);
  //@}

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(svtkDataObject*);
  void AddInputData(int, svtkDataObject*);
  //@}

protected:
  svtkUnstructuredGridBaseAlgorithm();
  ~svtkUnstructuredGridBaseAlgorithm() override;

  // convenience method
  virtual int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkUnstructuredGridBaseAlgorithm(const svtkUnstructuredGridBaseAlgorithm&) = delete;
  void operator=(const svtkUnstructuredGridBaseAlgorithm&) = delete;
};

#endif
