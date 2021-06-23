/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkmTriangleMeshPointNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkmTriangleMeshPointNormals
 * @brief   compute point normals for triangle mesh
 *
 * svtkmTriangleMeshPointNormals is a filter that computes point normals for
 * a triangle mesh to enable high-performance rendering. It is a fast-path
 * version of the svtkmPolyDataNormals filter in order to be able to compute
 * normals for triangle meshes deforming rapidly.
 *
 * The computed normals (a svtkFloatArray) are set to be the active normals
 * (using SetNormals()) of the PointData. The array name is "Normals".
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
 */

#ifndef svtkmTriangleMeshPointNormals_h
#define svtkmTriangleMeshPointNormals_h

#include "svtkAcceleratorsSVTKmModule.h" // for export macro
#include "svtkTriangleMeshPointNormals.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmTriangleMeshPointNormals : public svtkTriangleMeshPointNormals
{
public:
  svtkTypeMacro(svtkmTriangleMeshPointNormals, svtkTriangleMeshPointNormals);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmTriangleMeshPointNormals* New();

protected:
  svtkmTriangleMeshPointNormals();
  ~svtkmTriangleMeshPointNormals() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmTriangleMeshPointNormals(const svtkmTriangleMeshPointNormals&) = delete;
  void operator=(const svtkmTriangleMeshPointNormals&) = delete;
};

#endif // svtkmTriangleMeshPointNormals_h
// SVTK-HeaderTest-Exclude: svtkmTriangleMeshPointNormals.h
