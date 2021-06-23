/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTriangleMeshPointNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTriangleMeshPointNormals
 * @brief   compute point normals for triangle mesh
 *
 * svtkTriangleMeshPointNormals is a filter that computes point normals for
 * a triangle mesh to enable high-performance rendering. It is a fast-path
 * version of the svtkPolyDataNormals filter in order to be able to compute
 * normals for triangle meshes deforming rapidly.
 *
 * The computed normals (a svtkFloatArray) are set to be the active normals
 * (using SetNormals()) of the PointData. The array name is "Normals", so
 * they can be retrieved either with `output->GetPointData()->GetNormals()`
 * or with `output->GetPointData()->GetArray("Normals")`.
 *
 * The algorithm works by determining normals for each triangle and adding
 * these vectors to the triangle points. The resulting vectors at each
 * point are then normalized.
 *
 * @warning
 * Normals are computed only for triangular polygons: the filter can not
 * handle meshes with other types of cells (Verts, Lines, Strips) or Polys
 * with the wrong number of components (not equal to 3).
 *
 * @warning
 * Unlike the svtkPolyDataNormals filter, this filter does not apply any
 * splitting nor checks for cell orientation consistency in order to speed
 * up the computation. Moreover, normals are not calculated the exact same
 * way as the svtkPolyDataNormals filter since the triangle normals are not
 * normalized before being added to the point normals: those cell normals
 * are therefore weighted by the triangle area. This is not more nor less
 * correct than normalizing them before adding them, but it is much faster.
 *
 * @sa
 * If you do not need to do high-performance rendering, you should use
 * svtkPolyDataNormals if your mesh is not only triangular, if you need to
 * split vertices at sharp edges, or if you need to check that the cell
 * orientations are consistent to flip inverted normals.
 *
 * @sa
 * If you still need high-performance rendering but your input polydata is
 * not a triangular mesh and/or does not have consistent cell orientations
 * (causing inverted normals), you can still use this filter by using
 * svtkTriangleFilter and/or svtkCleanPolyData respectively beforehand. If
 * your mesh is deforming rapidly, you should be deforming the output mesh
 * of those two filters instead in order to only run them once.
 *
 */

#ifndef svtkTriangleMeshPointNormals_h
#define svtkTriangleMeshPointNormals_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPolyData;

class SVTKFILTERSCORE_EXPORT svtkTriangleMeshPointNormals : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkTriangleMeshPointNormals, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkTriangleMeshPointNormals* New();

protected:
  svtkTriangleMeshPointNormals() {}
  ~svtkTriangleMeshPointNormals() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkTriangleMeshPointNormals(const svtkTriangleMeshPointNormals&) = delete;
  void operator=(const svtkTriangleMeshPointNormals&) = delete;
};

#endif
