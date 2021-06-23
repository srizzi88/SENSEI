/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAngleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAngleRepresentation3D
 * @brief   represent the svtkAngleWidget
 *
 * The svtkAngleRepresentation3D is a representation for the
 * svtkAngleWidget. This representation consists of two rays and three
 * svtkHandleRepresentations to place and manipulate the three points defining
 * the angle representation. (Note: the three points are referred to as Point1,
 * Center, and Point2, at the two end points (Point1 and Point2) and Center
 * (around which the angle is measured). This particular implementation is a
 * 3D representation, meaning that it draws in the overlay plane.
 *
 * @sa
 * svtkAngleWidget svtkHandleRepresentation
 */

#ifndef svtkAngleRepresentation3D_h
#define svtkAngleRepresentation3D_h

#include "svtkAngleRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkActor;
class svtkProperty;
class svtkPolyDataMapper;
class svtkLineSource;
class svtkArcSource;
class svtkFollower;
class svtkVectorText;
class svtkPolyDataMapper;
class svtkTextProperty;

class SVTKINTERACTIONWIDGETS_EXPORT svtkAngleRepresentation3D : public svtkAngleRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static svtkAngleRepresentation3D* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkAngleRepresentation3D, svtkAngleRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Satisfy the superclasses API. Angle returned is in radians.
   */
  double GetAngle() override;

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  void GetPoint1WorldPosition(double pos[3]) override;
  void GetCenterWorldPosition(double pos[3]) override;
  void GetPoint2WorldPosition(double pos[3]) override;
  virtual void SetPoint1WorldPosition(double pos[3]);
  void SetPoint1DisplayPosition(double pos[3]) override;
  virtual void SetCenterWorldPosition(double pos[3]);
  void SetCenterDisplayPosition(double pos[3]) override;
  virtual void SetPoint2WorldPosition(double pos[3]);
  void SetPoint2DisplayPosition(double pos[3]) override;
  void GetPoint1DisplayPosition(double pos[3]) override;
  void GetCenterDisplayPosition(double pos[3]) override;
  void GetPoint2DisplayPosition(double pos[3]) override;
  //@}

  //@{
  /**
   * Set/Get the three leaders used to create this representation.
   * By obtaining these leaders the user can set the appropriate
   * properties, etc.
   */
  svtkGetObjectMacro(Ray1, svtkActor);
  svtkGetObjectMacro(Ray2, svtkActor);
  svtkGetObjectMacro(Arc, svtkActor);
  svtkGetObjectMacro(TextActor, svtkFollower);
  //@}

  //@{
  /**
   * Scale text.
   */
  virtual void SetTextActorScale(double scale[3]);
  virtual double* GetTextActorScale();
  //@}

  /**
   * Method defined by svtkWidgetRepresentation superclass and
   * needed here.
   */
  void BuildRepresentation() override;

  //@{
  /**
   * Methods required by svtkProp superclass.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  svtkAngleRepresentation3D();
  ~svtkAngleRepresentation3D() override;

  // The pieces that make up the angle representations
  svtkLineSource* Line1Source;
  svtkLineSource* Line2Source;
  svtkArcSource* ArcSource;
  svtkPolyDataMapper* Line1Mapper;
  svtkPolyDataMapper* Line2Mapper;
  svtkPolyDataMapper* ArcMapper;
  svtkActor* Ray1;
  svtkActor* Ray2;
  svtkActor* Arc;
  svtkFollower* TextActor;
  svtkPolyDataMapper* TextMapper;
  svtkVectorText* TextInput;
  double Angle;
  bool ScaleInitialized;
  double TextPosition[3];

private:
  svtkAngleRepresentation3D(const svtkAngleRepresentation3D&) = delete;
  void operator=(const svtkAngleRepresentation3D&) = delete;
};

#endif
