/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThresholdTextureCoords.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkThresholdTextureCoords
 * @brief   compute 1D, 2D, or 3D texture coordinates based on scalar threshold
 *
 * svtkThresholdTextureCoords is a filter that generates texture coordinates for
 * any input dataset type given a threshold criterion. The criterion can take
 * three forms: 1) greater than a particular value (ThresholdByUpper());
 * 2) less than a particular value (ThresholdByLower(); or 3) between two
 * values (ThresholdBetween(). If the threshold criterion is satisfied,
 * the "in" texture coordinate will be set (this can be specified by the
 * user). If the threshold criterion is not satisfied the "out" is set.
 *
 * @warning
 * There is a texture map - texThres.svtk - that can be used in conjunction
 * with this filter. This map defines a "transparent" region for texture
 * coordinates 0<=r<0.5, and an opaque full intensity map for texture
 * coordinates 0.5<r<=1.0. There is a small transition region for r=0.5.
 *
 * @sa
 * svtkThreshold svtkThresholdPoints svtkTextureMapToPlane svtkTextureMapToSphere
 * svtkTextureMapToCylinder
 */

#ifndef svtkThresholdTextureCoords_h
#define svtkThresholdTextureCoords_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersTextureModule.h" // For export macro

class SVTKFILTERSTEXTURE_EXPORT svtkThresholdTextureCoords : public svtkDataSetAlgorithm
{
public:
  static svtkThresholdTextureCoords* New();
  svtkTypeMacro(svtkThresholdTextureCoords, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Criterion is cells whose scalars are less than lower threshold.
   */
  void ThresholdByLower(double lower);

  /**
   * Criterion is cells whose scalars are less than upper threshold.
   */
  void ThresholdByUpper(double upper);

  /**
   * Criterion is cells whose scalars are between lower and upper thresholds.
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Return the upper and lower thresholds.
   */
  svtkGetMacro(UpperThreshold, double);
  svtkGetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Set the desired dimension of the texture map.
   */
  svtkSetClampMacro(TextureDimension, int, 1, 3);
  svtkGetMacro(TextureDimension, int);
  //@}

  //@{
  /**
   * Set the texture coordinate value for point satisfying threshold criterion.
   */
  svtkSetVector3Macro(InTextureCoord, double);
  svtkGetVectorMacro(InTextureCoord, double, 3);
  //@}

  //@{
  /**
   * Set the texture coordinate value for point NOT satisfying threshold
   * criterion.
   */
  svtkSetVector3Macro(OutTextureCoord, double);
  svtkGetVectorMacro(OutTextureCoord, double, 3);
  //@}

protected:
  svtkThresholdTextureCoords();
  ~svtkThresholdTextureCoords() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double LowerThreshold;
  double UpperThreshold;

  int TextureDimension;

  double InTextureCoord[3];
  double OutTextureCoord[3];

  int (svtkThresholdTextureCoords::*ThresholdFunction)(double s);

  int Lower(double s) { return (s <= this->LowerThreshold ? 1 : 0); }
  int Upper(double s) { return (s >= this->UpperThreshold ? 1 : 0); }
  int Between(double s)
  {
    return (s >= this->LowerThreshold ? (s <= this->UpperThreshold ? 1 : 0) : 0);
  }

private:
  svtkThresholdTextureCoords(const svtkThresholdTextureCoords&) = delete;
  void operator=(const svtkThresholdTextureCoords&) = delete;
};

#endif
