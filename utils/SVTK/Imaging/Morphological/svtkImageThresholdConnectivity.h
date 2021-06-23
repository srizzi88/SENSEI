/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageThresholdConnectivity.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageThresholdConnectivity
 * @brief   Flood fill an image region.
 *
 * svtkImageThresholdConnectivity will perform a flood fill on an image,
 * given upper and lower pixel intensity thresholds. It works similarly
 * to svtkImageThreshold, but also allows the user to set seed points
 * to limit the threshold operation to contiguous regions of the image.
 * The filled region, or the "inside", will be passed through to the
 * output by default, while the "outside" will be replaced with zeros.
 * This behavior can be changed by using the ReplaceIn() and ReplaceOut()
 * methods.  The scalar type of the output is the same as the input.
 * @sa
 * svtkImageThreshold
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to SVTK.
 */

#ifndef svtkImageThresholdConnectivity_h
#define svtkImageThresholdConnectivity_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingMorphologicalModule.h" // For export macro

class svtkPoints;
class svtkImageData;
class svtkImageStencilData;

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageThresholdConnectivity : public svtkImageAlgorithm
{
public:
  static svtkImageThresholdConnectivity* New();
  svtkTypeMacro(svtkImageThresholdConnectivity, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the seeds.  The seeds are in real data coordinates, not in
   * voxel index locations.
   */
  void SetSeedPoints(svtkPoints* points);
  svtkGetObjectMacro(SeedPoints, svtkPoints);
  //@}

  /**
   * Values greater than or equal to this threshold will be filled.
   */
  void ThresholdByUpper(double thresh);

  /**
   * Values less than or equal to this threshold will be filled.
   */
  void ThresholdByLower(double thresh);

  /**
   * Values within this range will be filled, where the range includes
   * values that are exactly equal to the lower and upper thresholds.
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Replace the filled region by the value set by SetInValue().
   */
  svtkSetMacro(ReplaceIn, svtkTypeBool);
  svtkGetMacro(ReplaceIn, svtkTypeBool);
  svtkBooleanMacro(ReplaceIn, svtkTypeBool);
  //@}

  //@{
  /**
   * If ReplaceIn is set, the filled region will be replaced by this value.
   */
  void SetInValue(double val);
  svtkGetMacro(InValue, double);
  //@}

  //@{
  /**
   * Replace the filled region by the value set by SetInValue().
   */
  svtkSetMacro(ReplaceOut, svtkTypeBool);
  svtkGetMacro(ReplaceOut, svtkTypeBool);
  svtkBooleanMacro(ReplaceOut, svtkTypeBool);
  //@}

  //@{
  /**
   * If ReplaceOut is set, outside the fill will be replaced by this value.
   */
  void SetOutValue(double val);
  svtkGetMacro(OutValue, double);
  //@}

  //@{
  /**
   * Get the Upper and Lower thresholds.
   */
  svtkGetMacro(UpperThreshold, double);
  svtkGetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Limit the flood to a range of slices in the specified direction.
   */
  svtkSetVector2Macro(SliceRangeX, int);
  svtkGetVector2Macro(SliceRangeX, int);
  svtkSetVector2Macro(SliceRangeY, int);
  svtkGetVector2Macro(SliceRangeY, int);
  svtkSetVector2Macro(SliceRangeZ, int);
  svtkGetVector2Macro(SliceRangeZ, int);
  //@}

  //@{
  /**
   * Specify a stencil that will be used to limit the flood fill to
   * an arbitrarily-shaped region of the image.
   */
  virtual void SetStencilData(svtkImageStencilData* stencil);
  svtkImageStencilData* GetStencil();
  //@}

  //@{
  /**
   * For multi-component images, you can set which component will be
   * used for the threshold checks.
   */
  svtkSetMacro(ActiveComponent, int);
  svtkGetMacro(ActiveComponent, int);
  //@}

  //@{
  /**
   * The radius of the neighborhood that must be within the threshold
   * values in order for the voxel to be included in the mask.  The
   * default radius is zero (one single voxel).  The radius is measured
   * in voxels.
   */
  svtkSetVector3Macro(NeighborhoodRadius, double);
  svtkGetVector3Macro(NeighborhoodRadius, double);
  //@}

  //@{
  /**
   * The fraction of the neighborhood that must be within the thresholds.
   * The default value is 0.5.
   */
  svtkSetClampMacro(NeighborhoodFraction, double, 0.0, 1.0);
  svtkGetMacro(NeighborhoodFraction, double);
  //@}

  /**
   * Override the MTime to account for the seed points.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * After the filter has executed, use GetNumberOfVoxels() to find
   * out how many voxels were filled.
   */
  svtkGetMacro(NumberOfInVoxels, int);
  //@}

protected:
  svtkImageThresholdConnectivity();
  ~svtkImageThresholdConnectivity() override;

  double UpperThreshold;
  double LowerThreshold;
  double InValue;
  double OutValue;
  svtkTypeBool ReplaceIn;
  svtkTypeBool ReplaceOut;

  double NeighborhoodRadius[3];
  double NeighborhoodFraction;

  svtkPoints* SeedPoints;

  int SliceRangeX[2];
  int SliceRangeY[2];
  int SliceRangeZ[2];

  int NumberOfInVoxels;

  int ActiveComponent;

  svtkImageData* ImageMask;

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageThresholdConnectivity(const svtkImageThresholdConnectivity&) = delete;
  void operator=(const svtkImageThresholdConnectivity&) = delete;
};

#endif
