/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEdgePoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEdgePoints
 * @brief   generate points on isosurface
 *
 * svtkEdgePoints is a filter that takes as input any dataset and
 * generates for output a set of points that lie on an isosurface. The
 * points are created by interpolation along cells edges whose end-points are
 * below and above the contour value.
 * @warning
 * svtkEdgePoints can be considered a "poor man's" dividing cubes algorithm
 * (see svtkDividingCubes). Points are generated only on the edges of cells,
 * not in the interior, and at lower density than dividing cubes. However, it
 * is more general than dividing cubes since it treats any type of dataset.
 */

#ifndef svtkEdgePoints_h
#define svtkEdgePoints_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkMergePoints;

class SVTKFILTERSGENERAL_EXPORT svtkEdgePoints : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkEdgePoints, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with contour value of 0.0.
   */
  static svtkEdgePoints* New();

  //@{
  /**
   * Set/get the contour value.
   */
  svtkSetMacro(Value, double);
  svtkGetMacro(Value, double);
  //@}

protected:
  svtkEdgePoints();
  ~svtkEdgePoints() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  double Value;
  svtkMergePoints* Locator;

private:
  svtkEdgePoints(const svtkEdgePoints&) = delete;
  void operator=(const svtkEdgePoints&) = delete;
};

#endif
