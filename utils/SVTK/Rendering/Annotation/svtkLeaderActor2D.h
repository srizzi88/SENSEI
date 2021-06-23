/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLeaderActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLeaderActor2D
 * @brief   create a leader with optional label and arrows
 *
 * svtkLeaderActor2D creates a leader with an optional label and arrows. (A
 * leader is typically used to indicate distance between points.)
 * svtkLeaderActor2D is a type of svtkActor2D; that is, it is drawn on the
 * overlay plane and is not occluded by 3D geometry. To use this class, you
 * typically specify two points defining the start and end points of the line
 * (x-y definition using svtkCoordinate class), whether to place arrows on one
 * or both end points, and whether to label the leader. Also, this class has a
 * special feature that allows curved leaders to be created by specifying a
 * radius.
 *
 * Use the svtkLeaderActor2D uses its superclass svtkActor2D instance variables
 * Position and Position2 svtkCoordinates to place an instance of
 * svtkLeaderActor2D (i.e., these two data members represent the start and end
 * points of the leader).  Using these svtkCoordinates you can specify the position
 * of the leader in a variety of coordinate systems.
 *
 * To control the appearance of the actor, use the superclasses
 * svtkActor2D::svtkProperty2D and the svtkTextProperty objects associated with
 * this actor.
 *
 * @sa
 * svtkAxisActor2D svtkActor2D svtkCoordinate svtkTextProperty
 */

#ifndef svtkLeaderActor2D_h
#define svtkLeaderActor2D_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkPoints;
class svtkCellArray;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkTextMapper;
class svtkTextProperty;

class SVTKRENDERINGANNOTATION_EXPORT svtkLeaderActor2D : public svtkActor2D
{
public:
  svtkTypeMacro(svtkLeaderActor2D, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object.
   */
  static svtkLeaderActor2D* New();

  //@{
  /**
   * Set/Get a radius which can be used to curve the leader.  If a radius is
   * specified whose absolute value is greater than one half the distance
   * between the two points defined by the superclasses' Position and
   * Position2 ivars, then the leader will be curved. A positive radius will
   * produce a curve such that the center is to the right of the line from
   * Position and Position2; a negative radius will produce a curve in the
   * opposite sense. By default, the radius is set to zero and thus there
   * is no curvature. Note that the radius is expresses as a multiple of
   * the distance between (Position,Position2); this avoids issues relative
   * to coordinate system transformations.
   */
  svtkSetMacro(Radius, double);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Set/Get the label for the leader. If the label is an empty string, then
   * it will not be drawn.
   */
  svtkSetStringMacro(Label);
  svtkGetStringMacro(Label);
  //@}

  //@{
  /**
   * Set/Get the text property of the label.
   */
  virtual void SetLabelTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(LabelTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the factor that controls the overall size of the fonts used
   * to label the leader.
   */
  svtkSetClampMacro(LabelFactor, double, 0.1, 2.0);
  svtkGetMacro(LabelFactor, double);
  //@}

  // Enums defined to support methods for control of arrow placement and
  // and appearance of arrow heads.
  enum
  {
    SVTK_ARROW_NONE = 0,
    SVTK_ARROW_POINT1,
    SVTK_ARROW_POINT2,
    SVTK_ARROW_BOTH
  };
  enum
  {
    SVTK_ARROW_FILLED = 0,
    SVTK_ARROW_OPEN,
    SVTK_ARROW_HOLLOW
  };

  //@{
  /**
   * Control whether arrow heads are drawn on the leader. Arrows may be
   * drawn on one end, both ends, or not at all.
   */
  svtkSetClampMacro(ArrowPlacement, int, SVTK_ARROW_NONE, SVTK_ARROW_BOTH);
  svtkGetMacro(ArrowPlacement, int);
  void SetArrowPlacementToNone() { this->SetArrowPlacement(SVTK_ARROW_NONE); }
  void SetArrowPlacementToPoint1() { this->SetArrowPlacement(SVTK_ARROW_POINT1); }
  void SetArrowPlacementToPoint2() { this->SetArrowPlacement(SVTK_ARROW_POINT2); }
  void SetArrowPlacementToBoth() { this->SetArrowPlacement(SVTK_ARROW_BOTH); }
  //@}

  //@{
  /**
   * Control the appearance of the arrow heads. A solid arrow head is a filled
   * triangle; a open arrow looks like a "V"; and a hollow arrow looks like a
   * non-filled triangle.
   */
  svtkSetClampMacro(ArrowStyle, int, SVTK_ARROW_FILLED, SVTK_ARROW_HOLLOW);
  svtkGetMacro(ArrowStyle, int);
  void SetArrowStyleToFilled() { this->SetArrowStyle(SVTK_ARROW_FILLED); }
  void SetArrowStyleToOpen() { this->SetArrowStyle(SVTK_ARROW_OPEN); }
  void SetArrowStyleToHollow() { this->SetArrowStyle(SVTK_ARROW_HOLLOW); }
  //@}

  //@{
  /**
   * Specify the arrow length and base width (in normalized viewport
   * coordinates).
   */
  svtkSetClampMacro(ArrowLength, double, 0.0, 1.0);
  svtkGetMacro(ArrowLength, double);
  svtkSetClampMacro(ArrowWidth, double, 0.0, 1.0);
  svtkGetMacro(ArrowWidth, double);
  //@}

  //@{
  /**
   * Limit the minimum and maximum size of the arrows. These values are
   * expressed in pixels and clamp the minimum/maximum possible size for the
   * width/length of the arrow head. (When clamped, the ratio between length
   * and width is preserved.)
   */
  svtkSetClampMacro(MinimumArrowSize, double, 1.0, SVTK_FLOAT_MAX);
  svtkGetMacro(MinimumArrowSize, double);
  svtkSetClampMacro(MaximumArrowSize, double, 1.0, SVTK_FLOAT_MAX);
  svtkGetMacro(MaximumArrowSize, double);
  //@}

  //@{
  /**
   * Enable auto-labelling. In this mode, the label is automatically updated
   * based on distance (in world coordinates) between the two end points; or
   * if a curved leader is being generated, the angle in degrees between the
   * two points.
   */
  svtkSetMacro(AutoLabel, svtkTypeBool);
  svtkGetMacro(AutoLabel, svtkTypeBool);
  svtkBooleanMacro(AutoLabel, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the format to use for auto-labelling.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Obtain the length of the leader if the leader is not curved,
   * otherwise obtain the angle that the leader circumscribes.
   */
  svtkGetMacro(Length, double);
  svtkGetMacro(Angle, double);
  //@}

  //@{
  /**
   * Methods required by svtkProp and svtkActor2D superclasses.
   */
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  void ReleaseGraphicsResources(svtkWindow*) override;
  void ShallowCopy(svtkProp* prop) override;

protected:
  svtkLeaderActor2D();
  ~svtkLeaderActor2D() override;

  // Internal helper methods
  virtual void BuildLeader(svtkViewport* viewport);
  int SetFontSize(svtkViewport* viewport, svtkTextMapper* textMapper, const int* targetSize,
    double factor, int* stringSize);
  int ClipLeader(
    double xL[3], int stringSize[2], double p1[3], double ray[3], double c1[3], double c2[3]);
  void BuildCurvedLeader(double p1[3], double p2[3], double ray[3], double rayLength, double theta,
    svtkViewport* viewport, int viewportChanged);
  int InStringBox(double center[3], int stringSize[2], double x[3]);

  // Characteristics of the leader
  double Radius;
  double Length;
  double Angle;

  svtkTypeBool AutoLabel;
  char* LabelFormat;
  char* Label;
  double LabelFactor;
  svtkTextMapper* LabelMapper;
  svtkActor2D* LabelActor;
  svtkTextProperty* LabelTextProperty;

  int ArrowPlacement;
  int ArrowStyle;
  double ArrowLength;
  double ArrowWidth;
  double MinimumArrowSize;
  double MaximumArrowSize;

  svtkPoints* LeaderPoints;
  svtkCellArray* LeaderLines;
  svtkCellArray* LeaderArrows;
  svtkPolyData* Leader;
  svtkPolyDataMapper2D* LeaderMapper;
  svtkActor2D* LeaderActor;

  // Internal ivars for tracking whether to rebuild
  int LastPosition[2];
  int LastPosition2[2];
  int LastSize[2];
  svtkTimeStamp BuildTime;

private:
  svtkLeaderActor2D(const svtkLeaderActor2D&) = delete;
  void operator=(const svtkLeaderActor2D&) = delete;
};

#endif
