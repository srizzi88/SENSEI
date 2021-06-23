/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageAppend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageAppend
 * @brief   Collects data from multiple inputs into one image.
 *
 * svtkImageAppend takes the components from multiple inputs and merges
 * them into one output. The output images are append along the "AppendAxis".
 * Except for the append axis, all inputs must have the same extent.
 * All inputs must have the same number of scalar components.
 * A future extension might be to pad or clip inputs to have the same extent.
 * The output has the same origin and spacing as the first input.
 * The origin and spacing of all other inputs are ignored.  All inputs
 * must have the same scalar type.
 */

#ifndef svtkImageAppend_h
#define svtkImageAppend_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkImageAppend : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageAppend* New();
  svtkTypeMacro(svtkImageAppend, svtkThreadedImageAlgorithm);
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

  //@{
  /**
   * This axis is expanded to hold the multiple images.
   * The default AppendAxis is the X axis.
   * If you want to create a volue from a series of XY images, then you should
   * set the AppendAxis to 2 (Z axis).
   */
  svtkSetMacro(AppendAxis, int);
  svtkGetMacro(AppendAxis, int);
  //@}

  //@{
  /**
   * By default "PreserveExtents" is off and the append axis is used.
   * When "PreseveExtents" is on, the extent of the inputs is used to
   * place the image in the output.  The whole extent of the output is
   * the union of the input whole extents.  Any portion of the
   * output not covered by the inputs is set to zero.  The origin and
   * spacing is taken from the first input.
   */
  svtkSetMacro(PreserveExtents, svtkTypeBool);
  svtkGetMacro(PreserveExtents, svtkTypeBool);
  svtkBooleanMacro(PreserveExtents, svtkTypeBool);
  //@}

protected:
  svtkImageAppend();
  ~svtkImageAppend() override;

  svtkTypeBool PreserveExtents;
  int AppendAxis;
  // Array holds the AppendAxisExtent shift for each input.
  int* Shifts;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

  // see svtkAlgorithm for docs.
  int FillInputPortInformation(int, svtkInformation*) override;

  void InitOutput(int outExt[6], svtkImageData* outData);

  void InternalComputeInputUpdateExtent(int* inExt, int* outExt, int* inWextent, int whichInput);

  // overridden to allocate all of the output arrays, not just active scalars
  void AllocateOutputData(svtkImageData* out, svtkInformation* outInfo, int* uExtent) override;
  svtkImageData* AllocateOutputData(svtkDataObject* out, svtkInformation* outInfo) override;

  // overridden to prevent shallow copies across, since we have to do it elementwise
  void CopyAttributeData(
    svtkImageData* in, svtkImageData* out, svtkInformationVector** inputVector) override;

private:
  svtkImageAppend(const svtkImageAppend&) = delete;
  void operator=(const svtkImageAppend&) = delete;
};

#endif
