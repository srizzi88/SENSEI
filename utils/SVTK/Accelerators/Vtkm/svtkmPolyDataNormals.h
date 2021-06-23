/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkmPolyDataNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkmPolyDataNormals
 * @brief   compute normals for polygonal mesh
 *
 * svtkmPolyDataNormals is a filter that computes point and/or cell normals
 * for a polygonal mesh. The user specifies if they would like the point
 * and/or cell normals to be computed by setting the ComputeCellNormals
 * and ComputePointNormals flags.
 *
 * The computed normals (a svtkFloatArray) are set to be the active normals
 * (using SetNormals()) of the PointData and/or the CellData (respectively)
 * of the output PolyData. The name of these arrays is "Normals".
 *
 * The algorithm works by determining normals for each polygon and then
 * averaging them at shared points.
 *
 * @warning
 * Normals are computed only for polygons and triangles. Normals are
 * not computed for lines, vertices, or triangle strips.
 *
 * @sa
 * For high-performance rendering, you could use svtkmTriangleMeshPointNormals
 * if you know that you have a triangle mesh which does not require splitting
 * nor consistency check on the cell orientations.
 *
 */

#ifndef svtkmPolyDataNormals_h
#define svtkmPolyDataNormals_h

#include "svtkAcceleratorsSVTKmModule.h" // for export macro
#include "svtkPolyDataNormals.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmPolyDataNormals : public svtkPolyDataNormals
{
public:
  svtkTypeMacro(svtkmPolyDataNormals, svtkPolyDataNormals);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmPolyDataNormals* New();

protected:
  svtkmPolyDataNormals();
  ~svtkmPolyDataNormals() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmPolyDataNormals(const svtkmPolyDataNormals&) = delete;
  void operator=(const svtkmPolyDataNormals&) = delete;
};

#endif // svtkmPolyDataNormals_h
// SVTK-HeaderTest-Exclude: svtkmPolyDataNormals.h
