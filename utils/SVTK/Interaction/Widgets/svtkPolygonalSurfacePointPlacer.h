/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolygonalSurfacePointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolygonalSurfacePointPlacer
 * @brief   Place points on the surface of polygonal data.
 *
 *
 * svtkPolygonalSurfacePointPlacer places points on polygonal data and is
 * meant to be used in conjunction with
 * svtkPolygonalSurfaceContourLineInterpolator.
 *
 * @warning
 * You should have computed cell normals for the input polydata if you are
 * specifying a distance offset.
 *
 * @sa
 * svtkPointPlacer svtkPolyDataNormals
 */

#ifndef svtkPolygonalSurfacePointPlacer_h
#define svtkPolygonalSurfacePointPlacer_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPolyDataPointPlacer.h"

class svtkPolyDataCollection;
class svtkCellPicker;
class svtkPolygonalSurfacePointPlacerInternals;
class svtkPolyData;

// The Node stores information about the point. This information is used by
// the interpolator. Reusing this information avoids the need for a second
// pick operation to regenerate it. (Cellpickers are slow).
struct svtkPolygonalSurfacePointPlacerNode
{
  double WorldPosition[3];
  double SurfaceWorldPosition[3];
  svtkIdType CellId;
  svtkIdType PointId;
  double ParametricCoords[3]; // parametric coords within cell
  svtkPolyData* PolyData;
};

class SVTKINTERACTIONWIDGETS_EXPORT svtkPolygonalSurfacePointPlacer : public svtkPolyDataPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static svtkPolygonalSurfacePointPlacer* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkPolygonalSurfacePointPlacer, svtkPolyDataPointPlacer);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  // Descuription:
  // Add /remove a prop, to place points on
  void AddProp(svtkProp*) override;
  void RemoveViewProp(svtkProp* prop) override;
  void RemoveAllProps() override;

  /**
   * Given a renderer and a display position in pixel coordinates,
   * compute the world position and orientation where this point
   * will be placed. This method is typically used by the
   * representation to place the point initially.
   * For the Terrain point placer this computes world points that
   * lie at the specified height above the terrain.
   */
  int ComputeWorldPosition(
    svtkRenderer* ren, double displayPos[2], double worldPos[3], double worldOrient[9]) override;

  /**
   * Given a renderer, a display position, and a reference world
   * position, compute the new world position and orientation
   * of this point. This method is typically used by the
   * representation to move the point.
   */
  int ComputeWorldPosition(svtkRenderer* ren, double displayPos[2], double refWorldPos[3],
    double worldPos[3], double worldOrient[9]) override;

  /**
   * Given a world position check the validity of this
   * position according to the constraints of the placer
   */
  int ValidateWorldPosition(double worldPos[3]) override;

  /**
   * Give the node a chance to update its auxiliary point id.
   */
  int UpdateNodeWorldPosition(double worldPos[3], svtkIdType nodePointId) override;

  /**
   * Given a display position, check the validity of this position.
   */
  int ValidateDisplayPosition(svtkRenderer*, double displayPos[2]) override;

  /**
   * Given a world position and a world orientation,
   * validate it according to the constraints of the placer.
   */
  int ValidateWorldPosition(double worldPos[3], double worldOrient[9]) override;

  //@{
  /**
   * Get the Prop picker.
   */
  svtkGetObjectMacro(CellPicker, svtkCellPicker);
  //@}

  //@{
  /**
   * Be sure to add polydata on which you wish to place points to this list
   * or they will not be considered for placement.
   */
  svtkGetObjectMacro(Polys, svtkPolyDataCollection);
  //@}

  //@{
  /**
   * Height offset at which points may be placed on the polygonal surface.
   * If you specify a non-zero value here, be sure to compute cell normals
   * on your input polygonal data (easily done with svtkPolyDataNormals).
   */
  svtkSetMacro(DistanceOffset, double);
  svtkGetMacro(DistanceOffset, double);
  //@}

  //@{
  /**
   * Snap to the closest point on the surface ?
   * This is useful for the svtkPolygonalSurfaceContourLineInterpolator, when
   * drawing contours along the edges of a surface mesh.
   * OFF by default.
   */
  svtkSetMacro(SnapToClosestPoint, svtkTypeBool);
  svtkGetMacro(SnapToClosestPoint, svtkTypeBool);
  svtkBooleanMacro(SnapToClosestPoint, svtkTypeBool);
  //@}

  //@{
  /**
   * Internally used by the interpolator.
   */
  typedef svtkPolygonalSurfacePointPlacerNode Node;
  Node* GetNodeAtWorldPosition(double worldPos[3]);
  //@}

protected:
  svtkPolygonalSurfacePointPlacer();
  ~svtkPolygonalSurfacePointPlacer() override;

  // The props that represents the terrain data (one or more) in a rendered
  // scene
  svtkCellPicker* CellPicker;
  svtkPolyDataCollection* Polys;
  svtkPolygonalSurfacePointPlacerInternals* Internals;
  double DistanceOffset;
  svtkTypeBool SnapToClosestPoint;

private:
  svtkPolygonalSurfacePointPlacer(const svtkPolygonalSurfacePointPlacer&) = delete;
  void operator=(const svtkPolygonalSurfacePointPlacer&) = delete;
};

#endif
