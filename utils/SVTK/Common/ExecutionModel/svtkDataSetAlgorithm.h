/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataSetAlgorithm
 * @brief   Superclass for algorithms that produce output of the same type as input
 *
 * svtkDataSetAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this classes
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be DataSet. If that isn't
 * the case then please override this method in your subclass. This class
 * breaks out the downstream requests into separate functions such as
 * RequestDataObject RequestData and RequestInformation. The default
 * implementation of RequestDataObject will create an output data of the
 * same type as the input.
 */

#ifndef svtkDataSetAlgorithm_h
#define svtkDataSetAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkDataSet;
class svtkImageData;
class svtkPolyData;
class svtkStructuredPoints;
class svtkStructuredGrid;
class svtkUnstructuredGrid;
class svtkRectilinearGrid;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkDataSetAlgorithm : public svtkAlgorithm
{
public:
  static svtkDataSetAlgorithm* New();
  svtkTypeMacro(svtkDataSetAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkDataSet* GetOutput();
  svtkDataSet* GetOutput(int);
  //@}

  /**
   * Get the input data object. This method is not recommended for use, but
   * lots of old style filters use it.
   */
  svtkDataObject* GetInput();

  /**
   * Get the output as svtkPolyData.
   */
  svtkPolyData* GetPolyDataOutput();

  /**
   * Get the output as svtkStructuredPoints.
   */
  svtkStructuredPoints* GetStructuredPointsOutput();

  /**
   * Get the output as svtkStructuredPoints.
   */
  svtkImageData* GetImageDataOutput();

  /**
   * Get the output as svtkStructuredGrid.
   */
  svtkStructuredGrid* GetStructuredGridOutput();

  /**
   * Get the output as svtkUnstructuredGrid.
   */
  svtkUnstructuredGrid* GetUnstructuredGridOutput();

  /**
   * Get the output as svtkRectilinearGrid.
   */
  svtkRectilinearGrid* GetRectilinearGridOutput();

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  void SetInputData(svtkDataSet*);
  void SetInputData(int, svtkDataSet*);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(svtkDataObject*);
  void AddInputData(svtkDataSet*);
  void AddInputData(int, svtkDataSet*);
  void AddInputData(int, svtkDataObject*);
  //@}

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkDataSetAlgorithm();
  ~svtkDataSetAlgorithm() override {}

  /**
   * This is called within ProcessRequest when a request asks for
   * Information. Typically an algorithm provides whatever lightweight
   * information about its output that it can here without doing any
   * lengthy computations. This happens in the first pass of the pipeline
   * execution.
   */
  virtual int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  //@{
  /**
   * This is called within ProcessRequest when each filter in the pipeline
   * decides what portion of its input is needed to create the portion of its
   * output that the downstream filter asks for. This happens during the
   * second pass in the pipeline execution process.
   */
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  //@}

  /**
   * This is called within ProcessRequest to when a request asks the
   * algorithm to create empty output data objects. This typically happens
   * early on in the execution of the pipeline. The default behavior is to
   * create an output DataSet of the same type as the input for each
   * output port. This method can be overridden to change the output
   * data type of an algorithm. This happens in the third pass of the
   * pipeline execution.
   */
  virtual int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * This is called within ProcessRequest when a request asks the algorithm
   * to do its work. This is the method you should override to do whatever the
   * algorithm is designed to do. This happens during the fourth pass in the
   * pipeline execution process.
   */
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkDataObject* GetInput(int port);

private:
  svtkDataSetAlgorithm(const svtkDataSetAlgorithm&) = delete;
  void operator=(const svtkDataSetAlgorithm&) = delete;
};

#endif
