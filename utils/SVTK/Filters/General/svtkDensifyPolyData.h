/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDensifyPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkDensifyPolyData
 * @brief   Densify the input by adding points at the
 * centroid
 *
 *
 * The filter takes any polygonal data as input and will tessellate cells that
 * are planar polygons present by fanning out triangles from its centroid.
 * Other cells are simply passed through to the output.  PointData, if present,
 * is interpolated via linear interpolation. CellData for any tessellated cell
 * is simply copied over from its parent cell. Planar polygons are assumed to
 * be convex. Funny things will happen if they are not.
 *
 * The number of subdivisions can be controlled by the parameter
 * NumberOfSubdivisions.
 */

#ifndef svtkDensifyPolyData_h
#define svtkDensifyPolyData_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkDensifyPolyData : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkDensifyPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkDensifyPolyData* New();

  //@{
  /**
   * Number of recursive subdivisions. Initial value is 1.
   */
  svtkSetMacro(NumberOfSubdivisions, unsigned int);
  svtkGetMacro(NumberOfSubdivisions, unsigned int);
  //@}

protected:
  svtkDensifyPolyData();
  ~svtkDensifyPolyData() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  unsigned int NumberOfSubdivisions;

private:
  int FillInputPortInformation(int, svtkInformation*) override;

  svtkDensifyPolyData(const svtkDensifyPolyData&) = delete;
  void operator=(const svtkDensifyPolyData&) = delete;
};

#endif
