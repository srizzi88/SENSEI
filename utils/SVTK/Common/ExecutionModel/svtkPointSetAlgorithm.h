/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointSetAlgorithm
 * @brief   Superclass for algorithms that produce output of the same type as input
 *
 * svtkPointSetAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this classes
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be PointSet. If that
 * isn't the case then please override this method in your subclass.
 * You should implement the subclass's algorithm into
 * RequestData( request, inputVec, outputVec).
 */

#ifndef svtkPointSetAlgorithm_h
#define svtkPointSetAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkPointSet;
class svtkPolyData;
class svtkStructuredGrid;
class svtkUnstructuredGrid;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkPointSetAlgorithm : public svtkAlgorithm
{
public:
  static svtkPointSetAlgorithm* New();
  svtkTypeMacro(svtkPointSetAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkPointSet* GetOutput();
  svtkPointSet* GetOutput(int);
  //@}

  /**
   * Get the output as svtkPolyData.
   */
  svtkPolyData* GetPolyDataOutput();

  /**
   * Get the output as svtkStructuredGrid.
   */
  svtkStructuredGrid* GetStructuredGridOutput();

  /**
   * Get the output as svtkUnstructuredGrid.
   */
  svtkUnstructuredGrid* GetUnstructuredGridOutput();

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  void SetInputData(svtkPointSet*);
  void SetInputData(int, svtkPointSet*);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(svtkDataObject*);
  void AddInputData(svtkPointSet*);
  void AddInputData(int, svtkPointSet*);
  void AddInputData(int, svtkDataObject*);
  //@}

  // this method is not recommended for use, but lots of old style filters
  // use it
  svtkDataObject* GetInput();

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkPointSetAlgorithm();
  ~svtkPointSetAlgorithm() override {}

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
  virtual int ExecuteInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  //@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int ComputeInputUpdateExtent(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  //@}

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkPointSetAlgorithm(const svtkPointSetAlgorithm&) = delete;
  void operator=(const svtkPointSetAlgorithm&) = delete;
};

#endif
