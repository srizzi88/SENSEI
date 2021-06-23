/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPiecewiseFunctionAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPiecewiseFunctionAlgorithm
 * @brief   Superclass for algorithms that produce only piecewise function as output
 *
 *
 * svtkPiecewiseFunctionAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this classes
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be PiecewiseFunction. If that
 * isn't the case then please override this method in your subclass.
 * You should implement the subclass's algorithm into
 * RequestData( request, inputVec, outputVec).
 */

#ifndef svtkPiecewiseFunctionAlgorithm_h
#define svtkPiecewiseFunctionAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkPiecewiseFunction.h"          // makes things a bit easier

class svtkDataSet;
class svtkDataObject;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkPiecewiseFunctionAlgorithm : public svtkAlgorithm
{
public:
  static svtkPiecewiseFunctionAlgorithm* New();
  svtkTypeMacro(svtkPiecewiseFunctionAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkDataObject* GetOutput();
  svtkDataObject* GetOutput(int);
  virtual void SetOutput(svtkDataObject* d);
  //@}

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // this method is not recommended for use, but lots of old style filters
  // use it
  svtkDataObject* GetInput();
  svtkDataObject* GetInput(int port);

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
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(svtkDataObject*);
  void AddInputData(int, svtkDataObject*);
  //@}

protected:
  svtkPiecewiseFunctionAlgorithm();
  ~svtkPiecewiseFunctionAlgorithm() override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkPiecewiseFunctionAlgorithm(const svtkPiecewiseFunctionAlgorithm&) = delete;
  void operator=(const svtkPiecewiseFunctionAlgorithm&) = delete;
};

#endif
