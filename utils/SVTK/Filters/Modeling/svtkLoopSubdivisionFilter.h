/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLoopSubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLoopSubdivisionFilter
 * @brief   generate a subdivision surface using the Loop Scheme
 *
 * svtkLoopSubdivisionFilter is an approximating subdivision scheme that
 * creates four new triangles for each triangle in the mesh. The user can
 * specify the NumberOfSubdivisions. Loop's subdivision scheme is
 * described in: Loop, C., "Smooth Subdivision surfaces based on
 * triangles,", Masters Thesis, University of Utah, August 1987.
 * For a nice summary of the technique see, Hoppe, H., et. al,
 * "Piecewise Smooth Surface Reconstruction,:, Proceedings of Siggraph 94
 * (Orlando, Florida, July 24-29, 1994). In Computer Graphics
 * Proceedings, Annual Conference Series, 1994, ACM SIGGRAPH,
 * pp. 295-302.
 * <P>
 * The filter only operates on triangles. Users should use the
 * svtkTriangleFilter to triangulate meshes that contain polygons or
 * triangle strips.
 * <P>
 * The filter approximates point data using the same scheme. New
 * triangles create at a subdivision step will have the cell data of
 * their parent cell.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * svtkApproximatingSubdivisionFilter
 */

#ifndef svtkLoopSubdivisionFilter_h
#define svtkLoopSubdivisionFilter_h

#include "svtkApproximatingSubdivisionFilter.h"
#include "svtkFiltersModelingModule.h" // For export macro

class svtkPolyData;
class svtkIntArray;
class svtkPoints;
class svtkIdList;

class SVTKFILTERSMODELING_EXPORT svtkLoopSubdivisionFilter : public svtkApproximatingSubdivisionFilter
{
public:
  //@{
  /**
   * Construct object with NumberOfSubdivisions set to 1.
   */
  static svtkLoopSubdivisionFilter* New();
  svtkTypeMacro(svtkLoopSubdivisionFilter, svtkApproximatingSubdivisionFilter);
  //@}

protected:
  svtkLoopSubdivisionFilter() {}
  ~svtkLoopSubdivisionFilter() override {}

  int GenerateSubdivisionPoints(svtkPolyData* inputDS, svtkIntArray* edgeData, svtkPoints* outputPts,
    svtkPointData* outputPD) override;
  int GenerateEvenStencil(svtkIdType p1, svtkPolyData* polys, svtkIdList* stencilIds, double* weights);
  void GenerateOddStencil(
    svtkIdType p1, svtkIdType p2, svtkPolyData* polys, svtkIdList* stencilIds, double* weights);

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkLoopSubdivisionFilter(const svtkLoopSubdivisionFilter&) = delete;
  void operator=(const svtkLoopSubdivisionFilter&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkLoopSubdivisionFilter.h
