/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageBSplineCoefficients.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageBSplineCoefficients
 * @brief   convert image to b-spline knots
 *
 * svtkImageBSplineCoefficients prepares an image for b-spline
 * interpolation by converting the image values into b-spline
 * knot coefficients.  It is a necessary pre-filtering step
 * before applying b-spline interpolation with svtkImageReslice.
 *
 * This class is based on code provided by Philippe Thevenaz of
 * EPFL, Lausanne, Switzerland.  Please acknowledge his contribution
 * by citing the following paper:
 * [1] P. Thevenaz, T. Blu, M. Unser, "Interpolation Revisited,"
 *     IEEE Transactions on Medical Imaging 19(7):739-758, 2000.
 *
 * The clamped boundary condition (which is the default) is taken
 * from code presented in the following paper:
 * [2] D. Ruijters, P. Thevenaz,
 *     "GPU Prefilter for Accurate Cubic B-spline Interpolation,"
 *     The Computer Journal, doi: 10.1093/comjnl/bxq086, 2010.
 *
 * @par Thanks:
 * This class was written by David Gobbi at the Seaman Family MR Research
 * Centre, Foothills Medical Centre, Calgary, Alberta.
 * DG Gobbi and YP Starreveld,
 * "Uniform B-Splines for the SVTK Imaging Pipeline,"
 * SVTK Journal, 2011,
 * http://hdl.handle.net/10380/3252
 */

#ifndef svtkImageBSplineCoefficients_h
#define svtkImageBSplineCoefficients_h

#include "svtkImageBSplineInterpolator.h" // for constants
#include "svtkImagingCoreModule.h"        // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageBSplineCoefficients : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageBSplineCoefficients* New();
  svtkTypeMacro(svtkImageBSplineCoefficients, svtkThreadedImageAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the degree of the spline polynomial.  The default value is 3,
   * and the maximum is 9.
   */
  svtkSetClampMacro(SplineDegree, int, 0, SVTK_IMAGE_BSPLINE_DEGREE_MAX);
  svtkGetMacro(SplineDegree, int);
  //@}

  //@{
  /**
   * Set the border mode.  The filter that is used to create the
   * coefficients must repeat the image somehow to make a theoritically
   * infinite input.  The default is to clamp values that are off the
   * edge of the image, to the value at the closest point on the edge.
   * The other ways of virtually extending the image are to produce
   * mirrored copies, which results in optimal smoothness at the boundary,
   * or to repeat the image, which results in a cyclic or periodic spline.
   */
  svtkSetClampMacro(BorderMode, int, SVTK_IMAGE_BORDER_CLAMP, SVTK_IMAGE_BORDER_MIRROR);
  void SetBorderModeToClamp() { this->SetBorderMode(SVTK_IMAGE_BORDER_CLAMP); }
  void SetBorderModeToRepeat() { this->SetBorderMode(SVTK_IMAGE_BORDER_REPEAT); }
  void SetBorderModeToMirror() { this->SetBorderMode(SVTK_IMAGE_BORDER_MIRROR); }
  svtkGetMacro(BorderMode, int);
  const char* GetBorderModeAsString();
  //@}

  //@{
  /**
   * Set the scalar type of the output.  Default is float.
   * Floating-point output is used to avoid overflow, since the
   * range of the output values is larger than the input values.
   */
  svtkSetClampMacro(OutputScalarType, int, SVTK_FLOAT, SVTK_DOUBLE);
  svtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(SVTK_FLOAT); }
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(SVTK_DOUBLE); }
  const char* GetOutputScalarTypeAsString();
  //@}

  //@{
  /**
   * Bypass the filter, do not do any processing.  If this is on,
   * then the output data will reference the input data directly,
   * and the output type will be the same as the input type.  This
   * is useful a downstream filter sometimes uses b-spline interpolation
   * and sometimes uses other forms of interpolation.
   */
  svtkSetMacro(Bypass, svtkTypeBool);
  svtkBooleanMacro(Bypass, svtkTypeBool);
  svtkGetMacro(Bypass, svtkTypeBool);
  //@}

  /**
   * Check a point against the image bounds.  Return 0 if out of bounds,
   * and 1 if inside bounds.  Calling Evaluate on a point outside the
   * bounds will not generate an error, but the value returned will
   * depend on the BorderMode.
   */
  int CheckBounds(const double point[3]);

  //@{
  /**
   * Interpolate a value from the image.  You must call Update() before
   * calling this method for the first time.  The first signature can
   * return multiple components, while the second signature is for use
   * on single-component images.
   */
  void Evaluate(const double point[3], double* value);
  double Evaluate(double x, double y, double z);
  double Evaluate(const double point[3]) { return this->Evaluate(point[0], point[1], point[2]); }
  //@}

protected:
  svtkImageBSplineCoefficients();
  ~svtkImageBSplineCoefficients() override;

  void AllocateOutputData(svtkImageData* out, svtkInformation* outInfo, int* uExtent) override;
  svtkImageData* AllocateOutputData(svtkDataObject* out, svtkInformation* outInfo) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedExecute(
    svtkImageData* inData, svtkImageData* outData, int outExt[6], int threadId) override;

  int SplineDegree;
  int BorderMode;
  int OutputScalarType;
  svtkTypeBool Bypass;
  int DataWasPassed;
  int Iteration;

private:
  svtkImageBSplineCoefficients(const svtkImageBSplineCoefficients&) = delete;
  void operator=(const svtkImageBSplineCoefficients&) = delete;
};

#endif
