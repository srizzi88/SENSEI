/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAnnotationLayersAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAnnotationLayersAlgorithm
 * @brief   Superclass for algorithms that produce only svtkAnnotationLayers as output
 *
 *
 * svtkAnnotationLayersAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this class
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be svtkAnnotationLayers. If that
 * isn't the case then please override this method in your subclass.
 * You should implement the subclass's algorithm into
 * RequestData( request, inputVec, outputVec).
 */

#ifndef svtkAnnotationLayersAlgorithm_h
#define svtkAnnotationLayersAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkAnnotationLayers.h"           // makes things a bit easier
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkDataSet;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkAnnotationLayersAlgorithm : public svtkAlgorithm
{
public:
  static svtkAnnotationLayersAlgorithm* New();
  svtkTypeMacro(svtkAnnotationLayersAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkAnnotationLayers* GetOutput() { return this->GetOutput(0); }
  svtkAnnotationLayers* GetOutput(int index);

  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject* obj) { this->SetInputData(0, obj); }
  void SetInputData(int index, svtkDataObject* obj);

protected:
  svtkAnnotationLayersAlgorithm();
  ~svtkAnnotationLayersAlgorithm() override;

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
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkAnnotationLayersAlgorithm(const svtkAnnotationLayersAlgorithm&) = delete;
  void operator=(const svtkAnnotationLayersAlgorithm&) = delete;
};

#endif
