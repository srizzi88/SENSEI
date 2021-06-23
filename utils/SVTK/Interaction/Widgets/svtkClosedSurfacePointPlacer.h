/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClosedSurfacePointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkClosedSurfacePointPlacer
 * @brief   PointPlacer to constrain validity within a set of convex planes
 *
 * This placer takes a set of boudning planes and constraints the validity
 * within the supplied convex planes. It is used by the
 * ParallelopPipedRepresentation to place constraints on the motion the
 * handles within the parallelopiped.
 *
 * @sa
 * svtkParallelopipedRepresentation
 */

#ifndef svtkClosedSurfacePointPlacer_h
#define svtkClosedSurfacePointPlacer_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPointPlacer.h"

class svtkPlane;
class svtkPlaneCollection;
class svtkPlanes;
class svtkRenderer;

class SVTKINTERACTIONWIDGETS_EXPORT svtkClosedSurfacePointPlacer : public svtkPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static svtkClosedSurfacePointPlacer* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkClosedSurfacePointPlacer, svtkPointPlacer);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * A collection of plane equations used to bound the position of the point.
   * This is in addition to confining the point to a plane - these constraints
   * are meant to, for example, keep a point within the extent of an image.
   * Using a set of plane equations allows for more complex bounds (such as
   * bounding a point to an oblique reliced image that has hexagonal shape)
   * than a simple extent.
   */
  void AddBoundingPlane(svtkPlane* plane);
  void RemoveBoundingPlane(svtkPlane* plane);
  void RemoveAllBoundingPlanes();
  virtual void SetBoundingPlanes(svtkPlaneCollection*);
  svtkGetObjectMacro(BoundingPlanes, svtkPlaneCollection);
  void SetBoundingPlanes(svtkPlanes* planes);
  //@}

  /**
   * Given a renderer and a display position, compute the
   * world position and world orientation for this point.
   * A plane is defined by a combination of the
   * ProjectionNormal, ProjectionOrigin, and ObliquePlane
   * ivars. The display position is projected onto this
   * plane to determine a world position, and the
   * orientation is set to the normal of the plane. If
   * the point cannot project onto the plane or if it
   * falls outside the bounds imposed by the
   * BoundingPlanes, then 0 is returned, otherwise 1 is
   * returned to indicate a valid return position and
   * orientation.
   */
  int ComputeWorldPosition(
    svtkRenderer* ren, double displayPos[2], double worldPos[3], double worldOrient[9]) override;

  /**
   * Given a renderer, a display position and a reference position, "worldPos"
   * is calculated as :
   * Consider the line "L" that passes through the supplied "displayPos" and
   * is parallel to the direction of projection of the camera. Clip this line
   * segment with the parallelopiped, let's call it "L_segment". The computed
   * world position, "worldPos" will be the point on "L_segment" that is
   * closest to refWorldPos.
   * NOTE: Note that a set of bounding planes must be supplied. The Oblique
   * plane, if supplied is ignored.
   */
  int ComputeWorldPosition(svtkRenderer* ren, double displayPos[2], double refWorldPos[2],
    double worldPos[3], double worldOrient[9]) override;

  /**
   * Give a world position check if it is valid - does
   * it lie on the plane and within the bounds? Returns
   * 1 if it is valid, 0 otherwise.
   */
  int ValidateWorldPosition(double worldPos[3]) override;

  // Descrption:
  // Orientationation is ignored, and the above method
  // is called instead.
  int ValidateWorldPosition(double worldPos[3], double worldOrient[9]) override;

  // Descrption:
  // The minimum distance the object should be from the faces of the object.
  // Must be greater than 0. Default is 0.
  svtkSetClampMacro(MinimumDistance, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(MinimumDistance, double);

protected:
  svtkClosedSurfacePointPlacer();
  ~svtkClosedSurfacePointPlacer() override;

  // A collection of planes used to bound the projection
  // plane
  svtkPlaneCollection* BoundingPlanes;

  // Calculate the distance of a point from the Object. Negative
  // values imply that the point is outside. Positive values imply that it is
  // inside. The closest point to the object is returned in closestPt.
  static double GetDistanceFromObject(double pos[3], svtkPlaneCollection* pc, double closestPt[3]);

  void BuildPlanes();

  double MinimumDistance;
  svtkPlaneCollection* InnerBoundingPlanes;

private:
  svtkClosedSurfacePointPlacer(const svtkClosedSurfacePointPlacer&) = delete;
  void operator=(const svtkClosedSurfacePointPlacer&) = delete;
};

#endif
