/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkButterflySubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkButterflySubdivisionFilter
 * @brief   generate a subdivision surface using the Butterfly Scheme
 *
 * svtkButterflySubdivisionFilter is an interpolating subdivision scheme
 * that creates four new triangles for each triangle in the mesh. The
 * user can specify the NumberOfSubdivisions. This filter implements the
 * 8-point butterfly scheme described in: Zorin, D., Schroder, P., and
 * Sweldens, W., "Interpolating Subdivisions for Meshes with Arbitrary
 * Topology," Computer Graphics Proceedings, Annual Conference Series,
 * 1996, ACM SIGGRAPH, pp.189-192. This scheme improves previous
 * butterfly subdivisions with special treatment of vertices with valence
 * other than 6.
 *
 * Currently, the filter only operates on triangles. Users should use the
 * svtkTriangleFilter to triangulate meshes that contain polygons or
 * triangle strips.
 *
 * The filter interpolates point data using the same scheme. New
 * triangles created at a subdivision step will have the cell data of
 * their parent cell.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * svtkInterpolatingSubdivisionFilter svtkLinearSubdivisionFilter
 */

#ifndef svtkButterflySubdivisionFilter_h
#define svtkButterflySubdivisionFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkInterpolatingSubdivisionFilter.h"

class svtkCellArray;
class svtkIdList;
class svtkIntArray;

class SVTKFILTERSMODELING_EXPORT svtkButterflySubdivisionFilter
  : public svtkInterpolatingSubdivisionFilter
{
public:
  //@{
  /**
   * Construct object with NumberOfSubdivisions set to 1.
   */
  static svtkButterflySubdivisionFilter* New();
  svtkTypeMacro(svtkButterflySubdivisionFilter, svtkInterpolatingSubdivisionFilter);
  //@}

protected:
  svtkButterflySubdivisionFilter() {}
  ~svtkButterflySubdivisionFilter() override {}

private:
  int GenerateSubdivisionPoints(svtkPolyData* inputDS, svtkIntArray* edgeData, svtkPoints* outputPts,
    svtkPointData* outputPD) override;
  void GenerateButterflyStencil(
    svtkIdType p1, svtkIdType p2, svtkPolyData* polys, svtkIdList* stencilIds, double* weights);
  void GenerateLoopStencil(
    svtkIdType p1, svtkIdType p2, svtkPolyData* polys, svtkIdList* stencilIds, double* weights);
  void GenerateBoundaryStencil(
    svtkIdType p1, svtkIdType p2, svtkPolyData* polys, svtkIdList* stencilIds, double* weights);

private:
  svtkButterflySubdivisionFilter(const svtkButterflySubdivisionFilter&) = delete;
  void operator=(const svtkButterflySubdivisionFilter&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkButterflySubdivisionFilter.h
