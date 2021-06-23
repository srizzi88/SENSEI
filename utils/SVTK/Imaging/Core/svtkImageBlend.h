/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageBlend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageBlend
 * @brief   blend images together using alpha or opacity
 *
 * svtkImageBlend takes L, LA, RGB, or RGBA images as input and blends them
 * according to the alpha values and/or the opacity setting for each input.
 *
 * The spacing, origin, extent, and number of components of the output are
 * the same as those for the first input.  If the input has an alpha
 * component, then this component is copied unchanged into the output.
 * In addition, if the first input has either one component or two
 * components i.e. if it is either L (greyscale) or LA (greyscale + alpha)
 * then all other inputs must also be L or LA.
 *
 * Different blending modes are available:
 *
 * \em Normal (default) :
 * This is the standard blending mode used by OpenGL and other graphics
 * packages.  The output always has the same number of components
 * and the same extent as the first input.  The alpha value of the first
 * input is not used in the blending computation, instead it is copied
 * directly to the output.
 *
 * \code
 * output <- input[0]
 * foreach input i {
 *   foreach pixel px {
 *     r <- input[i](px)(alpha) * opacity[i]
 *     f <- (255 - r)
 *     output(px) <- output(px) * f + input(px) * r
 *   }
 * }
 * \endcode
 *
 * \em Compound :
 * Images are compounded together and each component is scaled by the sum of
 * the alpha/opacity values. Use the CompoundThreshold method to set
 * specify a threshold in compound mode. Pixels with opacity*alpha less
 * or equal than this threshold are ignored.
 * The alpha value of the first input, if present, is NOT copied to the alpha
 * value of the output.  The output always has the same number of components
 * and the same extent as the first input.
 * If CompoundAlpha is set, the alpha value of the output is also computed using
 * the alpha weighted blend calculation.
 *
 * \code
 * output <- 0
 * foreach pixel px {
 *   sum <- 0
 *   foreach input i {
 *     r <- input[i](px)(alpha) * opacity(i)
 *     sum <- sum + r
 *     if r > threshold {
 *       output(px) <- output(px) + input(px) * r
 *     }
 *   }
 *   output(px) <- output(px) / sum
 * }
 * \endcode
 */

#ifndef svtkImageBlend_h
#define svtkImageBlend_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class svtkImageStencilData;

#define SVTK_IMAGE_BLEND_MODE_NORMAL 0
#define SVTK_IMAGE_BLEND_MODE_COMPOUND 1

class SVTKIMAGINGCORE_EXPORT svtkImageBlend : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageBlend* New();
  svtkTypeMacro(svtkImageBlend, svtkThreadedImageAlgorithm);
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
   * Set the opacity of an input image: the alpha values of the image are
   * multiplied by the opacity.  The opacity of image idx=0 is ignored.
   */
  void SetOpacity(int idx, double opacity);
  double GetOpacity(int idx);
  //@}

  /**
   * Set a stencil to apply when blending the data.
   * Create a pipeline connection.
   */
  void SetStencilConnection(svtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Set a stencil to apply when blending the data.
   */
  void SetStencilData(svtkImageStencilData* stencil);
  svtkImageStencilData* GetStencil();
  //@}

  //@{
  /**
   * Set the blend mode
   */
  svtkSetClampMacro(BlendMode, int, SVTK_IMAGE_BLEND_MODE_NORMAL, SVTK_IMAGE_BLEND_MODE_COMPOUND);
  svtkGetMacro(BlendMode, int);
  void SetBlendModeToNormal() { this->SetBlendMode(SVTK_IMAGE_BLEND_MODE_NORMAL); }
  void SetBlendModeToCompound() { this->SetBlendMode(SVTK_IMAGE_BLEND_MODE_COMPOUND); }
  const char* GetBlendModeAsString(void);
  //@}

  //@{
  /**
   * Specify a threshold in compound mode. Pixels with opacity*alpha less
   * or equal the threshold are ignored.
   */
  svtkSetMacro(CompoundThreshold, double);
  svtkGetMacro(CompoundThreshold, double);
  //@}

  //@{
  /**
   * Set whether to use the alpha weighted blending calculation on the alpha
   * component. If false, the alpha component is set to the sum of the product
   * of opacity and alpha from all inputs.
   */
  svtkSetMacro(CompoundAlpha, svtkTypeBool);
  svtkGetMacro(CompoundAlpha, svtkTypeBool);
  svtkBooleanMacro(CompoundAlpha, svtkTypeBool);
  //@}

protected:
  svtkImageBlend();
  ~svtkImageBlend() override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void InternalComputeInputUpdateExtent(int inExt[6], int outExt[6], int inWExtent[6]);

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

  // see svtkAlgorithm for docs.
  int FillInputPortInformation(int, svtkInformation*) override;

  // see svtkAlgorithm for docs.
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  double* Opacity;
  int OpacityArrayLength;
  int BlendMode;
  double CompoundThreshold;
  int DataWasPassed;
  svtkTypeBool CompoundAlpha;

private:
  svtkImageBlend(const svtkImageBlend&) = delete;
  void operator=(const svtkImageBlend&) = delete;
};

//@{
/**
 * Get the blending mode as a descriptive string
 */
inline const char* svtkImageBlend::GetBlendModeAsString()
{
  switch (this->BlendMode)
  {
    case SVTK_IMAGE_BLEND_MODE_NORMAL:
      return "Normal";
    case SVTK_IMAGE_BLEND_MODE_COMPOUND:
      return "Compound";
    default:
      return "Unknown Blend Mode";
  }
}
//@}

#endif
