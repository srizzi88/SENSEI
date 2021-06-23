/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFrustumSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFrustumSelector.h
 *
 * svtkFrustumSelector is a svtkSelector that selects elements based
 * on whether they are inside or intersect a frustum of interest.  This handles
 * the svtkSelectionNode::FRUSTUM selection type.
 *
 */

#ifndef svtkFrustumSelector_h
#define svtkFrustumSelector_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkSelector.h"

#include "svtkSmartPointer.h" // for smart pointer

class svtkDataSet;
class svtkPlanes;
class svtkSignedCharArray;

class SVTKFILTERSEXTRACTION_EXPORT svtkFrustumSelector : public svtkSelector
{
public:
  static svtkFrustumSelector* New();
  svtkTypeMacro(svtkFrustumSelector, svtkSelector);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Initialize(svtkSelectionNode* node) override;

  /**
   * Return the MTime taking into account changes to the Frustum
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set the selection frustum. The planes object must contain six planes.
   */
  void SetFrustum(svtkPlanes*);
  svtkPlanes* GetFrustum();
  //@}

protected:
  svtkFrustumSelector(svtkPlanes* f = nullptr);
  ~svtkFrustumSelector() override;

  svtkSmartPointer<svtkPlanes> Frustum;

  bool ComputeSelectedElements(svtkDataObject* input, svtkSignedCharArray* insidednessArray) override;

  /**
   * Given eight vertices, creates a frustum.
   * each pt is x,y,z,1
   * in the following order
   * near lower left, far lower left
   * near upper left, far upper left
   * near lower right, far lower right
   * near upper right, far upper right
   */
  void CreateFrustum(double vertices[32]);

  /**
   * Computes which points in the dataset are inside the frustum and populates the pointsInside
   * array with 1 for inside and 0 for outside.
   */
  void ComputeSelectedPoints(svtkDataSet* input, svtkSignedCharArray* pointsInside);
  /**
   * Computes which cells in the dataset are inside or intersect the frustum and populates
   * the cellsInside array with 1 for inside/intersecting and 0 for outside.
   */
  void ComputeSelectedCells(svtkDataSet* input, svtkSignedCharArray* cellsInside);

  int OverallBoundsTest(double bounds[6]);

private:
  svtkFrustumSelector(const svtkFrustumSelector&) = delete;
  void operator=(const svtkFrustumSelector&) = delete;
};

#endif
