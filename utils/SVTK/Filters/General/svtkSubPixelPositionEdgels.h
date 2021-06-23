/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSubPixelPositionEdgels.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSubPixelPositionEdgels
 * @brief   adjust edgel locations based on gradients.
 *
 * svtkSubPixelPositionEdgels is a filter that takes a series of linked
 * edgels (digital curves) and gradient maps as input. It then adjusts
 * the edgel locations based on the gradient data. Specifically, the
 * algorithm first determines the neighboring gradient magnitudes of
 * an edgel using simple interpolation of its neighbors. It then fits
 * the following three data points: negative gradient direction
 * gradient magnitude, edgel gradient magnitude and positive gradient
 * direction gradient magnitude to a quadratic function. It then
 * solves this quadratic to find the maximum gradient location along
 * the gradient orientation.  It then modifies the edgels location
 * along the gradient orientation to the calculated maximum
 * location. This algorithm does not adjust an edgel in the direction
 * orthogonal to its gradient vector.
 *
 * @sa
 * svtkImageData svtkImageGradient svtkLinkEdgels
 */

#ifndef svtkSubPixelPositionEdgels_h
#define svtkSubPixelPositionEdgels_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkStructuredPoints;
class svtkDataArray;

class SVTKFILTERSGENERAL_EXPORT svtkSubPixelPositionEdgels : public svtkPolyDataAlgorithm
{
public:
  static svtkSubPixelPositionEdgels* New();
  svtkTypeMacro(svtkSubPixelPositionEdgels, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the gradient data for doing the position adjustments.
   */
  void SetGradMapsData(svtkStructuredPoints* gm);
  svtkStructuredPoints* GetGradMaps();
  //@}

  //@{
  /**
   * These methods can make the positioning look for a target scalar value
   * instead of looking for a maximum.
   */
  svtkSetMacro(TargetFlag, svtkTypeBool);
  svtkGetMacro(TargetFlag, svtkTypeBool);
  svtkBooleanMacro(TargetFlag, svtkTypeBool);
  svtkSetMacro(TargetValue, double);
  svtkGetMacro(TargetValue, double);
  //@}

protected:
  svtkSubPixelPositionEdgels();
  ~svtkSubPixelPositionEdgels() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void Move(int xdim, int ydim, int zdim, int x, int y, float* img, svtkDataArray* inVecs,
    double* result, int z, double* aspect, double* resultNormal);
  void Move(int xdim, int ydim, int zdim, int x, int y, double* img, svtkDataArray* inVecs,
    double* result, int z, double* aspect, double* resultNormal);
  // extension for target instead of maximum
  svtkTypeBool TargetFlag;
  double TargetValue;

private:
  svtkSubPixelPositionEdgels(const svtkSubPixelPositionEdgels&) = delete;
  void operator=(const svtkSubPixelPositionEdgels&) = delete;
};

#endif
