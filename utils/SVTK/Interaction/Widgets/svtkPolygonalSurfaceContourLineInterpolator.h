/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolygonalSurfaceContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolygonalSurfaceContourLineInterpolator
 * @brief   Contour interpolator for to place points on polygonal surfaces.
 *
 *
 * svtkPolygonalSurfaceContourLineInterpolator interpolates and places
 * contour points on polygonal surfaces. The class interpolates nodes by
 * computing a \em graph \em geodesic laying on the polygonal data. By \em
 * graph \em Geodesic, we mean that the line interpolating the two end
 * points traverses along on the mesh edges so as to form the shortest
 * path. A Dijkstra algorithm is used to compute the path. See
 * svtkDijkstraGraphGeodesicPath.
 *
 * The class is mean to be used in conjunction with
 * svtkPolygonalSurfacePointPlacer. The reason for this weak coupling is a
 * performance issue, both classes need to perform a cell pick, and
 * coupling avoids multiple cell picks (cell picks are slow).
 *
 * @warning
 * You should have computed cell normals for the input polydata.
 *
 * @sa
 * svtkDijkstraGraphGeodesicPath, svtkPolyDataNormals
 */

#ifndef svtkPolygonalSurfaceContourLineInterpolator_h
#define svtkPolygonalSurfaceContourLineInterpolator_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPolyDataContourLineInterpolator.h"

class svtkDijkstraGraphGeodesicPath;
class svtkIdList;

class SVTKINTERACTIONWIDGETS_EXPORT svtkPolygonalSurfaceContourLineInterpolator
  : public svtkPolyDataContourLineInterpolator
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkPolygonalSurfaceContourLineInterpolator, svtkPolyDataContourLineInterpolator);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  static svtkPolygonalSurfaceContourLineInterpolator* New();

  /**
   * Subclasses that wish to interpolate a line segment must implement this.
   * For instance svtkBezierContourLineInterpolator adds nodes between idx1
   * and idx2, that allow the contour to adhere to a bezier curve.
   */
  int InterpolateLine(svtkRenderer* ren, svtkContourRepresentation* rep, int idx1, int idx2) override;

  /**
   * The interpolator is given a chance to update the node.
   * svtkImageContourLineInterpolator updates the idx'th node in the contour,
   * so it automatically sticks to edges in the vicinity as the user
   * constructs the contour.
   * Returns 0 if the node (world position) is unchanged.
   */
  int UpdateNode(svtkRenderer*, svtkContourRepresentation*, double* svtkNotUsed(node),
    int svtkNotUsed(idx)) override;

  //@{
  /**
   * Height offset at which points may be placed on the polygonal surface.
   * If you specify a non-zero value here, be sure to have computed vertex
   * normals on your input polygonal data. (easily done with
   * svtkPolyDataNormals).
   */
  svtkSetMacro(DistanceOffset, double);
  svtkGetMacro(DistanceOffset, double);
  //@}

  /**
   * Get the contour point ids. These point ids correspond to those on the
   * polygonal surface
   */
  void GetContourPointIds(svtkContourRepresentation* rep, svtkIdList* idList);

protected:
  svtkPolygonalSurfaceContourLineInterpolator();
  ~svtkPolygonalSurfaceContourLineInterpolator() override;

  /**
   * Draw the polyline at a certain height (in the direction of the vertex
   * normal) above the polydata.
   */
  double DistanceOffset;

private:
  svtkPolygonalSurfaceContourLineInterpolator(
    const svtkPolygonalSurfaceContourLineInterpolator&) = delete;
  void operator=(const svtkPolygonalSurfaceContourLineInterpolator&) = delete;

  // Cache the last used vertex id's (start and end).
  // If they are the same, don't recompute.
  svtkIdType LastInterpolatedVertexIds[2];

  svtkDijkstraGraphGeodesicPath* DijkstraGraphGeodesicPath;
};

#endif
