/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageIslandRemoval2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageIslandRemoval2D
 * @brief   Removes small clusters in masks.
 *
 * svtkImageIslandRemoval2D computes the area of separate islands in
 * a mask image.  It removes any island that has less than AreaThreshold
 * pixels.  Output has the same ScalarType as input.  It generates
 * the whole 2D output image for any output request.
 */

#ifndef svtkImageIslandRemoval2D_h
#define svtkImageIslandRemoval2D_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingMorphologicalModule.h" // For export macro

typedef struct
{
  void* inPtr;
  void* outPtr;
  int idx0;
  int idx1;
} svtkImage2DIslandPixel;

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageIslandRemoval2D : public svtkImageAlgorithm
{
public:
  //@{
  /**
   * Constructor: Sets default filter to be identity.
   */
  static svtkImageIslandRemoval2D* New();
  svtkTypeMacro(svtkImageIslandRemoval2D, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the cutoff area for removal
   */
  svtkSetMacro(AreaThreshold, int);
  svtkGetMacro(AreaThreshold, int);
  //@}

  //@{
  /**
   * Set/Get whether to use 4 or 8 neighbors
   */
  svtkSetMacro(SquareNeighborhood, svtkTypeBool);
  svtkGetMacro(SquareNeighborhood, svtkTypeBool);
  svtkBooleanMacro(SquareNeighborhood, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the value to remove.
   */
  svtkSetMacro(IslandValue, double);
  svtkGetMacro(IslandValue, double);
  //@}

  //@{
  /**
   * Set/Get the value to put in the place of removed pixels.
   */
  svtkSetMacro(ReplaceValue, double);
  svtkGetMacro(ReplaceValue, double);
  //@}

protected:
  svtkImageIslandRemoval2D();
  ~svtkImageIslandRemoval2D() override {}

  int AreaThreshold;
  svtkTypeBool SquareNeighborhood;
  double IslandValue;
  double ReplaceValue;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageIslandRemoval2D(const svtkImageIslandRemoval2D&) = delete;
  void operator=(const svtkImageIslandRemoval2D&) = delete;
};

#endif
