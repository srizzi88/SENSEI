/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridAppend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/**
 * @class   svtkStructuredGridAppend
 * @brief   Collects data from multiple inputs into one structured grid.
 *
 * svtkStructuredGridAppend takes the components from multiple inputs and merges
 * them into one output. All inputs must have the same number of scalar components.
 * All inputs must have the same scalar type.
 */

#ifndef svtkStructuredGridAppend_h
#define svtkStructuredGridAppend_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkStructuredGridAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkStructuredGridAppend : public svtkStructuredGridAlgorithm
{
public:
  static svtkStructuredGridAppend* New();
  svtkTypeMacro(svtkStructuredGridAppend, svtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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
  svtkStructuredGridAppend();
  ~svtkStructuredGridAppend() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // see svtkAlgorithm for docs.
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkStructuredGridAppend(const svtkStructuredGridAppend&) = delete;
  void operator=(const svtkStructuredGridAppend&) = delete;
};

#endif
