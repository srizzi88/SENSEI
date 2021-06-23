/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedSizeHandleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFixedSizeHandleRepresentation
 * @brief   A marker that has the same size in pixels.
 *
 *
 * This class is a concrete implementation of svtkHandleRepresentation. It is
 * meant to be used as a representation for svtkHandleWidget. Unlike the other
 * representations, this can maintain a constant size in pixels, regardless of
 * the camera zoom parameters. The size in pixels may be set via
 * SetHandleSizeInPixels. This representation renders the markers as spherical
 * blobs in 3D space with the width as specified above, defaults to 10 pixels.
 * The handles will have the same size in pixels, give or take a certain
 * tolerance, as specified by SetHandleSizeToleranceInPixels. The tolerance
 * defaults to half a pixel. PointPlacers may be used to specify constraints on
 * the placement of markers. For instance a svtkPolygonalSurfacePointPlacer
 * will constrain placement of these spherical handles to a surface mesh.
 *
 * @sa
 * svtkHandleRepresentation svtkHandleWidget
 */

#ifndef svtkFixedSizeHandleRepresentation3D_h
#define svtkFixedSizeHandleRepresentation3D_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPolygonalHandleRepresentation3D.h"

class svtkSphereSource;

class SVTKINTERACTIONWIDGETS_EXPORT svtkFixedSizeHandleRepresentation3D
  : public svtkPolygonalHandleRepresentation3D
{
public:
  /**
   * Instantiate this class.
   */
  static svtkFixedSizeHandleRepresentation3D* New();

  //@{
  /**
   * Standard svtk methods
   */
  svtkTypeMacro(svtkFixedSizeHandleRepresentation3D, svtkPolygonalHandleRepresentation3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Get the object used to render the spherical handle marker
   */
  svtkGetObjectMacro(SphereSource, svtkSphereSource);
  //@}

  //@{
  /**
   * Set/Get the required handle size in pixels. Defaults to a width of
   * 10 pixels.
   */
  svtkSetMacro(HandleSizeInPixels, double);
  svtkGetMacro(HandleSizeInPixels, double);
  //@}

  //@{
  /**
   * Specify the acceptable handle size tolerance. During each render, the
   * handle 3D source will be updated to automatically match a display size
   * as specified by HandleSizeInPixels. This update will be done if the
   * handle size is larger than a tolerance. Default value of this
   * tolerance is half a pixel.
   */
  svtkSetMacro(HandleSizeToleranceInPixels, double);
  svtkGetMacro(HandleSizeToleranceInPixels, double);
  //@}

protected:
  svtkFixedSizeHandleRepresentation3D();
  ~svtkFixedSizeHandleRepresentation3D() override;

  /**
   * Recomputes the handle world size based on the set display size.
   */
  void BuildRepresentation() override;

  /**
   * Convenience method to convert from world to display
   */
  void WorldToDisplay(double w[4], double d[4]);

  /**
   * Convenience method to convert from display to world
   */
  void DisplayToWorld(double d[4], double w[4]);

  svtkSphereSource* SphereSource;
  double HandleSizeInPixels;
  double HandleSizeToleranceInPixels;

private:
  svtkFixedSizeHandleRepresentation3D(const svtkFixedSizeHandleRepresentation3D&) = delete;
  void operator=(const svtkFixedSizeHandleRepresentation3D&) = delete;
};

#endif
