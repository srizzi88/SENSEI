/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageAppendComponents.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageAppendComponents
 * @brief   Collects components from two inputs into
 * one output.
 *
 * svtkImageAppendComponents takes the components from two inputs and merges
 * them into one output. If Input1 has M components, and Input2 has N
 * components, the output will have M+N components with input1
 * components coming first.
 */

#ifndef svtkImageAppendComponents_h
#define svtkImageAppendComponents_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageAppendComponents : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageAppendComponents* New();
  svtkTypeMacro(svtkImageAppendComponents, svtkThreadedImageAlgorithm);

  /**
   * Replace one of the input connections with a new input.  You can
   * only replace input connections that you previously created with
   * AddInputConnection() or, in the case of the first input,
   * with SetInputConnection().
   */
  virtual void ReplaceNthInputConnection(int idx, svtkAlgorithmOutput* input);

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(int num, svtkDataObject* input);
  void SetInputData(svtkDataObject* input) { this->SetInputData(0, input); }
  //@}

  //@{
  /**
   * Get one input to this filter. This method is only for support of
   * old-style pipeline connections.  When writing new code you should
   * use svtkAlgorithm::GetInputConnection(0, num).
   */
  svtkDataObject* GetInput(int num);
  svtkDataObject* GetInput() { return this->GetInput(0); }
  //@}

  /**
   * Get the number of inputs to this filter. This method is only for
   * support of old-style pipeline connections.  When writing new code
   * you should use svtkAlgorithm::GetNumberOfInputConnections(0).
   */
  int GetNumberOfInputs() { return this->GetNumberOfInputConnections(0); }

protected:
  svtkImageAppendComponents() {}
  ~svtkImageAppendComponents() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

  // Implement methods required by svtkAlgorithm.
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkImageAppendComponents(const svtkImageAppendComponents&) = delete;
  void operator=(const svtkImageAppendComponents&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageAppendComponents.h
