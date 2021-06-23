/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMandelbrotSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMandelbrotSource
 * @brief   Mandelbrot image.
 *
 * svtkImageMandelbrotSource creates an unsigned char image of the Mandelbrot
 * set.  The values in the image are the number of iterations it takes for
 * the magnitude of the value to get over 2.  The equation repeated is
 * z = z^2 + C (z and C are complex).  Initial value of z is zero, and the
 * real value of C is mapped onto the x axis, and the imaginary value of C
 * is mapped onto the Y Axis.  I was thinking of extending this source
 * to generate Julia Sets (initial value of Z varies).  This would be 4
 * possible parameters to vary, but there are no more 4d images :(
 * The third dimension (z axis) is the imaginary value of the initial value.
 */

#ifndef svtkImageMandelbrotSource_h
#define svtkImageMandelbrotSource_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingSourcesModule.h" // For export macro

class SVTKIMAGINGSOURCES_EXPORT svtkImageMandelbrotSource : public svtkImageAlgorithm
{
public:
  static svtkImageMandelbrotSource* New();
  svtkTypeMacro(svtkImageMandelbrotSource, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the extent of the whole output Volume.
   */
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  svtkGetVector6Macro(WholeExtent, int);
  //@}

  //@{
  /**
   * This flag determines whether the Size or spacing of
   * a data set remain constant (when extent is changed).
   * By default, size remains constant.
   */
  svtkSetMacro(ConstantSize, svtkTypeBool);
  svtkGetMacro(ConstantSize, svtkTypeBool);
  svtkBooleanMacro(ConstantSize, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the projection from the 4D space (4 parameters / 2 imaginary numbers)
   * to the axes of the 3D Volume.
   * 0=C_Real, 1=C_Imaginary, 2=X_Real, 4=X_Imaginary
   */
  void SetProjectionAxes(int x, int y, int z);
  void SetProjectionAxes(int a[3]) { this->SetProjectionAxes(a[0], a[1], a[2]); }
  svtkGetVector3Macro(ProjectionAxes, int);
  //@}

  //@{
  /**
   * Imaginary and real value for C (constant in equation)
   * and X (initial value).
   */
  svtkSetVector4Macro(OriginCX, double);
  // void SetOriginCX(double cReal, double cImag, double xReal, double xImag);
  svtkGetVector4Macro(OriginCX, double);
  //@}

  //@{
  /**
   * Imaginary and real value for C (constant in equation)
   * and X (initial value).
   */
  svtkSetVector4Macro(SampleCX, double);
  // void SetOriginCX(double cReal, double cImag, double xReal, double xImag);
  svtkGetVector4Macro(SampleCX, double);
  //@}

  //@{
  /**
   * Just a different way of setting the sample.
   * This sets the size of the 4D volume.
   * SampleCX is computed from size and extent.
   * Size is ignored when a dimension i 0 (collapsed).
   */
  void SetSizeCX(double cReal, double cImag, double xReal, double xImag);
  double* GetSizeCX() SVTK_SIZEHINT(4);
  void GetSizeCX(double s[4]);
  //@}

  //@{
  /**
   * The maximum number of cycles run to see if the value goes over 2
   */
  svtkSetClampMacro(MaximumNumberOfIterations, unsigned short, static_cast<unsigned short>(1),
    static_cast<unsigned short>(5000));
  svtkGetMacro(MaximumNumberOfIterations, unsigned short);
  //@}

  //@{
  /**
   * Convenience for Viewer.  Pan 3D volume relative to spacing.
   * Zoom constant factor.
   */
  void Zoom(double factor);
  void Pan(double x, double y, double z);
  //@}

  /**
   * Convenience for Viewer.  Copy the OriginCX and the SpacingCX.
   * What about other parameters ???
   */
  void CopyOriginAndSample(svtkImageMandelbrotSource* source);

  //@{
  /**
   * Set/Get a subsample rate.
   */
  svtkSetClampMacro(SubsampleRate, int, 1, SVTK_INT_MAX);
  svtkGetMacro(SubsampleRate, int);
  //@}

protected:
  svtkImageMandelbrotSource();
  ~svtkImageMandelbrotSource() override;

  int ProjectionAxes[3];

  // WholeExtent in 3 space (after projection).
  int WholeExtent[6];

  // Complex constant/initial-value at origin.
  double OriginCX[4];
  // Initial complex value at origin.
  double SampleCX[4];
  unsigned short MaximumNumberOfIterations;

  // A temporary vector that is computed as needed.
  // It is used to return a vector.
  double SizeCX[4];

  // A flag for keeping size constant (vs. keeping the spacing).
  svtkTypeBool ConstantSize;

  int SubsampleRate;

  // see svtkAlgorithm for details
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double EvaluateSet(double p[4]);

private:
  svtkImageMandelbrotSource(const svtkImageMandelbrotSource&) = delete;
  void operator=(const svtkImageMandelbrotSource&) = delete;
};

#endif
