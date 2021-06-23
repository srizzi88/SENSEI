/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageReslice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageReslice
 * @brief   Reslices a volume along a new set of axes.
 *
 * svtkImageReslice is the swiss-army-knife of image geometry filters:
 * It can permute, rotate, flip, scale, resample, deform, and pad image
 * data in any combination with reasonably high efficiency.  Simple
 * operations such as permutation, resampling and padding are done
 * with similar efficiently to the specialized svtkImagePermute,
 * svtkImageResample, and svtkImagePad filters.  There are a number of
 * tasks that svtkImageReslice is well suited for:
 * <p>1) Application of simple rotations, scales, and translations to
 * an image. It is often a good idea to use svtkImageChangeInformation
 * to center the image first, so that scales and rotations occur around
 * the center rather than around the lower-left corner of the image.
 * <p>2) Resampling of one data set to match the voxel sampling of
 * a second data set via the SetInformationInput() method, e.g. for
 * the purpose of comparing two images or combining two images.
 * A transformation, either linear or nonlinear, can be applied
 * at the same time via the SetResliceTransform method if the two
 * images are not in the same coordinate space.
 * <p>3) Extraction of slices from an image volume.  The most convenient
 * way to do this is to use SetResliceAxesDirectionCosines() to
 * specify the orientation of the slice.  The direction cosines give
 * the x, y, and z axes for the output volume.  The method
 * SetOutputDimensionality(2) is used to specify that want to output a
 * slice rather than a volume.  The SetResliceAxesOrigin() command is
 * used to provide an (x,y,z) point that the slice will pass through.
 * You can use both the ResliceAxes and the ResliceTransform at the
 * same time, in order to extract slices from a volume that you have
 * applied a transformation to.
 * @warning
 * This filter is very inefficient if the output X dimension is 1.
 * @sa
 * svtkAbstractTransform svtkMatrix4x4
 */

#ifndef svtkImageReslice_h
#define svtkImageReslice_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

// interpolation mode constants
#define SVTK_RESLICE_NEAREST SVTK_NEAREST_INTERPOLATION
#define SVTK_RESLICE_LINEAR SVTK_LINEAR_INTERPOLATION
#define SVTK_RESLICE_CUBIC SVTK_CUBIC_INTERPOLATION

class svtkImageData;
class svtkAbstractTransform;
class svtkMatrix4x4;
class svtkImageStencilData;
class svtkScalarsToColors;
class svtkAbstractImageInterpolator;

class SVTKIMAGINGCORE_EXPORT svtkImageReslice : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageReslice* New();
  svtkTypeMacro(svtkImageReslice, svtkThreadedImageAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * This method is used to set up the axes for the output voxels.
   * The output Spacing, Origin, and Extent specify the locations
   * of the voxels within the coordinate system defined by the axes.
   * The ResliceAxes are used most often to permute the data, e.g.
   * to extract ZY or XZ slices of a volume as 2D XY images.
   * <p>The first column of the matrix specifies the x-axis
   * vector (the fourth element must be set to zero), the second
   * column specifies the y-axis, and the third column the
   * z-axis.  The fourth column is the origin of the
   * axes (the fourth element must be set to one).
   * <p>An alternative to SetResliceAxes() is to use
   * SetResliceAxesDirectionCosines() to set the directions of the
   * axes and SetResliceAxesOrigin() to set the origin of the axes.
   */
  virtual void SetResliceAxes(svtkMatrix4x4*);
  svtkGetObjectMacro(ResliceAxes, svtkMatrix4x4);
  //@}

  //@{
  /**
   * Specify the direction cosines for the ResliceAxes (i.e. the
   * first three elements of each of the first three columns of
   * the ResliceAxes matrix).  This will modify the current
   * ResliceAxes matrix, or create a new matrix if none exists.
   */
  void SetResliceAxesDirectionCosines(double x0, double x1, double x2, double y0, double y1,
    double y2, double z0, double z1, double z2);
  void SetResliceAxesDirectionCosines(const double x[3], const double y[3], const double z[3])
  {
    this->SetResliceAxesDirectionCosines(x[0], x[1], x[2], y[0], y[1], y[2], z[0], z[1], z[2]);
  }
  void SetResliceAxesDirectionCosines(const double xyz[9])
  {
    this->SetResliceAxesDirectionCosines(
      xyz[0], xyz[1], xyz[2], xyz[3], xyz[4], xyz[5], xyz[6], xyz[7], xyz[8]);
  }
  void GetResliceAxesDirectionCosines(double x[3], double y[3], double z[3]);
  void GetResliceAxesDirectionCosines(double xyz[9])
  {
    this->GetResliceAxesDirectionCosines(&xyz[0], &xyz[3], &xyz[6]);
  }
  double* GetResliceAxesDirectionCosines() SVTK_SIZEHINT(9)
  {
    this->GetResliceAxesDirectionCosines(this->ResliceAxesDirectionCosines);
    return this->ResliceAxesDirectionCosines;
  }
  //@}

  //@{
  /**
   * Specify the origin for the ResliceAxes (i.e. the first three
   * elements of the final column of the ResliceAxes matrix).
   * This will modify the current ResliceAxes matrix, or create
   * new matrix if none exists.
   */
  void SetResliceAxesOrigin(double x, double y, double z);
  void SetResliceAxesOrigin(const double xyz[3])
  {
    this->SetResliceAxesOrigin(xyz[0], xyz[1], xyz[2]);
  }
  void GetResliceAxesOrigin(double xyz[3]);
  double* GetResliceAxesOrigin() SVTK_SIZEHINT(3)
  {
    this->GetResliceAxesOrigin(this->ResliceAxesOrigin);
    return this->ResliceAxesOrigin;
  }
  //@}

  //@{
  /**
   * Set a transform to be applied to the resampling grid that has
   * been defined via the ResliceAxes and the output Origin, Spacing
   * and Extent.  Note that applying a transform to the resampling
   * grid (which lies in the output coordinate system) is
   * equivalent to applying the inverse of that transform to
   * the input volume.  Nonlinear transforms such as svtkGridTransform
   * and svtkThinPlateSplineTransform can be used here.
   */
  virtual void SetResliceTransform(svtkAbstractTransform*);
  svtkGetObjectMacro(ResliceTransform, svtkAbstractTransform);
  //@}

  //@{
  /**
   * Set a svtkImageData from which the default Spacing, Origin,
   * and WholeExtent of the output will be copied.  The spacing,
   * origin, and extent will be permuted according to the
   * ResliceAxes.  Any values set via SetOutputSpacing,
   * SetOutputOrigin, and SetOutputExtent will override these
   * values.  By default, the Spacing, Origin, and WholeExtent
   * of the Input are used.
   */
  virtual void SetInformationInput(svtkImageData*);
  svtkGetObjectMacro(InformationInput, svtkImageData);
  //@}

  //@{
  /**
   * Specify whether to transform the spacing, origin and extent
   * of the Input (or the InformationInput) according to the
   * direction cosines and origin of the ResliceAxes before applying
   * them as the default output spacing, origin and extent
   * (default: On).
   */
  svtkSetMacro(TransformInputSampling, svtkTypeBool);
  svtkBooleanMacro(TransformInputSampling, svtkTypeBool);
  svtkGetMacro(TransformInputSampling, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn this on if you want to guarantee that the extent of the
   * output will be large enough to ensure that none of the
   * data will be cropped (default: Off).
   */
  svtkSetMacro(AutoCropOutput, svtkTypeBool);
  svtkBooleanMacro(AutoCropOutput, svtkTypeBool);
  svtkGetMacro(AutoCropOutput, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on wrap-pad feature (default: Off).
   */
  svtkSetMacro(Wrap, svtkTypeBool);
  svtkGetMacro(Wrap, svtkTypeBool);
  svtkBooleanMacro(Wrap, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on mirror-pad feature (default: Off).
   * This will override the wrap-pad.
   */
  svtkSetMacro(Mirror, svtkTypeBool);
  svtkGetMacro(Mirror, svtkTypeBool);
  svtkBooleanMacro(Mirror, svtkTypeBool);
  //@}

  //@{
  /**
   * Extend the apparent input border by a half voxel (default: On).
   * This changes how interpolation is handled at the borders of the
   * input image: if the center of an output voxel is beyond the edge
   * of the input image, but is within a half voxel width of the edge
   * (using the input voxel width), then the value of the output voxel
   * is calculated as if the input's edge voxels were duplicated past
   * the edges of the input.
   * This has no effect if Mirror or Wrap are on.
   */
  svtkSetMacro(Border, svtkTypeBool);
  svtkGetMacro(Border, svtkTypeBool);
  svtkBooleanMacro(Border, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the border thickness for BorderOn() (default: 0.5).
   * See SetBorder() for more information.
   */
  //@{
  svtkSetMacro(BorderThickness, double);
  svtkGetMacro(BorderThickness, double);
  //@}

  /**
   * Set interpolation mode (default: nearest neighbor).
   */
  svtkSetClampMacro(InterpolationMode, int, SVTK_RESLICE_NEAREST, SVTK_RESLICE_CUBIC);
  svtkGetMacro(InterpolationMode, int);
  void SetInterpolationModeToNearestNeighbor() { this->SetInterpolationMode(SVTK_RESLICE_NEAREST); }
  void SetInterpolationModeToLinear() { this->SetInterpolationMode(SVTK_RESLICE_LINEAR); }
  void SetInterpolationModeToCubic() { this->SetInterpolationMode(SVTK_RESLICE_CUBIC); }
  virtual const char* GetInterpolationModeAsString();
  //@}

  //@{
  /**
   * Set the interpolator to use.  The default interpolator
   * supports the Nearest, Linear, and Cubic interpolation modes.
   */
  virtual void SetInterpolator(svtkAbstractImageInterpolator* sampler);
  virtual svtkAbstractImageInterpolator* GetInterpolator();
  //@}

  //@{
  /**
   * Set the slab mode, for generating thick slices. The default is Mean.
   * If SetSlabNumberOfSlices(N) is called with N greater than one, then
   * each output slice will actually be a composite of N slices.  This method
   * specifies the compositing mode to be used.
   */
  svtkSetClampMacro(SlabMode, int, SVTK_IMAGE_SLAB_MIN, SVTK_IMAGE_SLAB_SUM);
  svtkGetMacro(SlabMode, int);
  void SetSlabModeToMin() { this->SetSlabMode(SVTK_IMAGE_SLAB_MIN); }
  void SetSlabModeToMax() { this->SetSlabMode(SVTK_IMAGE_SLAB_MAX); }
  void SetSlabModeToMean() { this->SetSlabMode(SVTK_IMAGE_SLAB_MEAN); }
  void SetSlabModeToSum() { this->SetSlabMode(SVTK_IMAGE_SLAB_SUM); }
  virtual const char* GetSlabModeAsString();
  //@}

  //@{
  /**
   * Set the number of slices that will be combined to create the slab.
   */
  svtkSetMacro(SlabNumberOfSlices, int);
  svtkGetMacro(SlabNumberOfSlices, int);
  //@}

  //@{
  /**
   * Use trapezoid integration for slab computation.  All this does is
   * weigh the first and last slices by half when doing sum and mean.
   * It is off by default.
   */
  svtkSetMacro(SlabTrapezoidIntegration, svtkTypeBool);
  svtkBooleanMacro(SlabTrapezoidIntegration, svtkTypeBool);
  svtkGetMacro(SlabTrapezoidIntegration, svtkTypeBool);
  //@}

  //@{
  /**
   * The slab spacing as a fraction of the output slice spacing.
   * When one of the various slab modes is chosen, each output slice is
   * produced by generating several "temporary" output slices and then
   * combining them according to the slab mode.  By default, the spacing
   * between these temporary slices is the Z component of the OutputSpacing.
   * This method sets the spacing between these temporary slices to be a
   * fraction of the output spacing.
   */
  svtkSetMacro(SlabSliceSpacingFraction, double);
  svtkGetMacro(SlabSliceSpacingFraction, double);
  //@}

  //@{
  /**
   * Turn on and off optimizations (default on, they should only be
   * turned off for testing purposes).
   */
  svtkSetMacro(Optimization, svtkTypeBool);
  svtkGetMacro(Optimization, svtkTypeBool);
  svtkBooleanMacro(Optimization, svtkTypeBool);
  //@}

  //@{
  /**
   * Set a value to add to all the output voxels.
   * After a sample value has been interpolated from the input image, the
   * equation u = (v + ScalarShift)*ScalarScale will be applied to it before
   * it is written to the output image.  The result will always be clamped to
   * the limits of the output data type.
   */
  svtkSetMacro(ScalarShift, double);
  svtkGetMacro(ScalarShift, double);
  //@}

  //@{
  /**
   * Set multiplication factor to apply to all the output voxels.
   * After a sample value has been interpolated from the input image, the
   * equation u = (v + ScalarShift)*ScalarScale will be applied to it before
   * it is written to the output image.  The result will always be clamped to
   * the limits of the output data type.
   */
  svtkSetMacro(ScalarScale, double);
  svtkGetMacro(ScalarScale, double);
  //@}

  //@{
  /**
   * Set the scalar type of the output to be different from the input.
   * The default value is -1, which means that the input scalar type will be
   * used to set the output scalar type.  Otherwise, this must be set to one
   * of the following types: SVTK_CHAR, SVTK_SIGNED_CHAR, SVTK_UNSIGNED_CHAR,
   * SVTK_SHORT, SVTK_UNSIGNED_SHORT, SVTK_INT, SVTK_UNSIGNED_INT, SVTK_FLOAT,
   * or SVTK_DOUBLE.  Other types are not permitted.  If the output type
   * is an integer type, the output will be rounded and clamped to the
   * limits of the type.
   */
  svtkSetMacro(OutputScalarType, int);
  svtkGetMacro(OutputScalarType, int);
  //@}

  //@{
  /**
   * Set the background color (for multi-component images).
   */
  svtkSetVector4Macro(BackgroundColor, double);
  svtkGetVector4Macro(BackgroundColor, double);
  //@}

  //@{
  /**
   * Set background grey level (for single-component images).
   */
  void SetBackgroundLevel(double v) { this->SetBackgroundColor(v, v, v, v); }
  double GetBackgroundLevel() { return this->GetBackgroundColor()[0]; }
  //@}

  //@{
  /**
   * Set the voxel spacing for the output data.  The default output
   * spacing is the input spacing permuted through the ResliceAxes.
   */
  virtual void SetOutputSpacing(double x, double y, double z);
  virtual void SetOutputSpacing(const double a[3]) { this->SetOutputSpacing(a[0], a[1], a[2]); }
  svtkGetVector3Macro(OutputSpacing, double);
  void SetOutputSpacingToDefault();
  //@}

  //@{
  /**
   * Set the origin for the output data.  The default output origin
   * is the input origin permuted through the ResliceAxes.
   */
  virtual void SetOutputOrigin(double x, double y, double z);
  virtual void SetOutputOrigin(const double a[3]) { this->SetOutputOrigin(a[0], a[1], a[2]); }
  svtkGetVector3Macro(OutputOrigin, double);
  void SetOutputOriginToDefault();
  //@}

  //@{
  /**
   * Set the extent for the output data.  The default output extent
   * is the input extent permuted through the ResliceAxes.
   */
  virtual void SetOutputExtent(int a, int b, int c, int d, int e, int f);
  virtual void SetOutputExtent(const int a[6])
  {
    this->SetOutputExtent(a[0], a[1], a[2], a[3], a[4], a[5]);
  }
  svtkGetVector6Macro(OutputExtent, int);
  void SetOutputExtentToDefault();
  //@}

  //@{
  /**
   * Force the dimensionality of the output to either 1, 2,
   * 3 or 0 (default: 3).  If the dimensionality is 2D, then
   * the Z extent of the output is forced to (0,0) and the Z
   * origin of the output is forced to 0.0 (i.e. the output
   * extent is confined to the xy plane).  If the dimensionality
   * is 1D, the output extent is confined to the x axis.
   * For 0D, the output extent consists of a single voxel at
   * (0,0,0).
   */
  svtkSetMacro(OutputDimensionality, int);
  svtkGetMacro(OutputDimensionality, int);
  //@}

  /**
   * When determining the modified time of the filter,
   * this check the modified time of the transform and matrix.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Report object referenced by instances of this class.
   */
  void ReportReferences(svtkGarbageCollector*) override;

  //@{
  /**
   * Convenient methods for switching between nearest-neighbor and linear
   * interpolation.
   * InterpolateOn() is equivalent to SetInterpolationModeToLinear() and
   * InterpolateOff() is equivalent to SetInterpolationModeToNearestNeighbor()
   * You should not use these methods if you use the SetInterpolationMode
   * methods.
   */
  void SetInterpolate(int t)
  {
    if (t && !this->GetInterpolate())
    {
      this->SetInterpolationModeToLinear();
    }
    else if (!t && this->GetInterpolate())
    {
      this->SetInterpolationModeToNearestNeighbor();
    }
  }
  void InterpolateOn() { this->SetInterpolate(1); }
  void InterpolateOff() { this->SetInterpolate(0); }
  int GetInterpolate() { return (this->GetInterpolationMode() != SVTK_RESLICE_NEAREST); }
  //@}

  //@{
  /**
   * Use a stencil to limit the calculations to a specific region of
   * the output.  Portions of the output that are 'outside' the stencil
   * will be cleared to the background color.
   */
  void SetStencilData(svtkImageStencilData* stencil);
  svtkImageStencilData* GetStencil();
  //@}

  //@{
  /**
   * Generate an output stencil that defines which pixels were
   * interpolated and which pixels were out-of-bounds of the input.
   */
  svtkSetMacro(GenerateStencilOutput, svtkTypeBool);
  svtkGetMacro(GenerateStencilOutput, svtkTypeBool);
  svtkBooleanMacro(GenerateStencilOutput, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the output stencil.
   */
  svtkAlgorithmOutput* GetStencilOutputPort() { return this->GetOutputPort(1); }
  svtkImageStencilData* GetStencilOutput();
  void SetStencilOutput(svtkImageStencilData* stencil);
  //@}

protected:
  svtkImageReslice();
  ~svtkImageReslice() override;

  svtkMatrix4x4* ResliceAxes;
  double ResliceAxesDirectionCosines[9];
  double ResliceAxesOrigin[3];
  svtkAbstractTransform* ResliceTransform;
  svtkAbstractImageInterpolator* Interpolator;
  svtkImageData* InformationInput;
  svtkTypeBool Wrap;
  svtkTypeBool Mirror;
  svtkTypeBool Border;
  int InterpolationMode;
  svtkTypeBool Optimization;
  int SlabMode;
  int SlabNumberOfSlices;
  svtkTypeBool SlabTrapezoidIntegration;
  double SlabSliceSpacingFraction;
  double ScalarShift;
  double ScalarScale;
  double BorderThickness;
  double BackgroundColor[4];
  double OutputOrigin[3];
  double OutputSpacing[3];
  int OutputExtent[6];
  int OutputScalarType;
  int OutputDimensionality;
  svtkTypeBool TransformInputSampling;
  svtkTypeBool AutoCropOutput;
  int HitInputExtent;
  int UsePermuteExecute;
  int ComputeOutputSpacing;
  int ComputeOutputOrigin;
  int ComputeOutputExtent;
  svtkTypeBool GenerateStencilOutput;

  svtkMatrix4x4* IndexMatrix;
  svtkAbstractTransform* OptimizedTransform;

  /**
   * This should be set to 1 by derived classes that override the
   * ConvertScalars method.
   */
  int HasConvertScalars;

  /**
   * This should be overridden by derived classes that operate on
   * the interpolated data before it is placed in the output.
   */
  virtual int ConvertScalarInfo(int& scalarType, int& numComponents);

  /**
   * This should be overridden by derived classes that operate on
   * the interpolated data before it is placed in the output.
   * The input data will usually be double or float (since the
   * interpolation routines use floating-point) but it could be
   * of any type.  This method will be called from multiple threads,
   * so it must be thread-safe in derived classes.
   */
  virtual void ConvertScalars(void* inPtr, void* outPtr, int inputType, int inputNumComponents,
    int count, int idX, int idY, int idZ, int threadId);

  void ConvertScalarsBase(void* inPtr, void* outPtr, int inputType, int inputNumComponents,
    int count, int idX, int idY, int idZ, int threadId)
  {
    this->ConvertScalars(
      inPtr, outPtr, inputType, inputNumComponents, count, idX, idY, idZ, threadId);
  }

  void GetAutoCroppedOutputBounds(svtkInformation* inInfo, double bounds[6]);
  void AllocateOutputData(svtkImageData* output, svtkInformation* outInfo, int* uExtent) override;
  svtkImageData* AllocateOutputData(svtkDataObject*, svtkInformation*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  svtkMatrix4x4* GetIndexMatrix(svtkInformation* inInfo, svtkInformation* outInfo);
  svtkAbstractTransform* GetOptimizedTransform() { return this->OptimizedTransform; }

private:
  svtkImageReslice(const svtkImageReslice&) = delete;
  void operator=(const svtkImageReslice&) = delete;
};

#endif
