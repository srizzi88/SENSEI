/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLegendScaleActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLegendScaleActor
 * @brief   annotate the render window with scale and distance information
 *
 * This class is used to annotate the render window. Its basic goal is to
 * provide an indication of the scale of the scene. Four axes surrounding the
 * render window indicate (in a variety of ways) the scale of what the camera
 * is viewing. An option also exists for displaying a scale legend.
 *
 * The axes can be programmed either to display distance scales or x-y
 * coordinate values. By default, the scales display a distance. However,
 * if you know that the view is down the z-axis, the scales can be programmed
 * to display x-y coordinate values.
 *
 * @warning
 * Please be aware that the axes and scale values are subject to perspective
 * effects. The distances are computed in the focal plane of the camera.
 * When there are large view angles (i.e., perspective projection), the
 * computed distances may provide users the wrong sense of scale. These
 * effects are not present when parallel projection is enabled.
 */

#ifndef svtkLegendScaleActor_h
#define svtkLegendScaleActor_h

#include "svtkCoordinate.h" // For svtkViewportCoordinateMacro
#include "svtkProp.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkAxisActor2D;
class svtkTextProperty;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkActor2D;
class svtkTextMapper;
class svtkPoints;
class svtkCoordinate;

class SVTKRENDERINGANNOTATION_EXPORT svtkLegendScaleActor : public svtkProp
{
public:
  /**
   * Instantiate the class.
   */
  static svtkLegendScaleActor* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkLegendScaleActor, svtkProp);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  enum AttributeLocation
  {
    DISTANCE = 0,
    XY_COORDINATES = 1
  };

  //@{
  /**
   * Specify the mode for labeling the scale axes. By default, the axes are
   * labeled with the distance between points (centered at a distance of
   * 0.0). Alternatively if you know that the view is down the z-axis; the
   * axes can be labeled with x-y coordinate values.
   */
  svtkSetClampMacro(LabelMode, int, DISTANCE, XY_COORDINATES);
  svtkGetMacro(LabelMode, int);
  void SetLabelModeToDistance() { this->SetLabelMode(DISTANCE); }
  void SetLabelModeToXYCoordinates() { this->SetLabelMode(XY_COORDINATES); }
  //@}

  //@{
  /**
   * Set/Get the flags that control which of the four axes to display (top,
   * bottom, left and right). By default, all the axes are displayed.
   */
  svtkSetMacro(RightAxisVisibility, svtkTypeBool);
  svtkGetMacro(RightAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(RightAxisVisibility, svtkTypeBool);
  svtkSetMacro(TopAxisVisibility, svtkTypeBool);
  svtkGetMacro(TopAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(TopAxisVisibility, svtkTypeBool);
  svtkSetMacro(LeftAxisVisibility, svtkTypeBool);
  svtkGetMacro(LeftAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(LeftAxisVisibility, svtkTypeBool);
  svtkSetMacro(BottomAxisVisibility, svtkTypeBool);
  svtkGetMacro(BottomAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(BottomAxisVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether the legend scale should be displayed or not.
   * The default is On.
   */
  svtkSetMacro(LegendVisibility, svtkTypeBool);
  svtkGetMacro(LegendVisibility, svtkTypeBool);
  svtkBooleanMacro(LegendVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Convenience method that turns all the axes either on or off.
   */
  void AllAxesOn();
  void AllAxesOff();
  //@}

  //@{
  /**
   * Convenience method that turns all the axes and the legend scale.
   */
  void AllAnnotationsOn();
  void AllAnnotationsOff();
  //@}

  //@{
  /**
   * Set/Get the offset of the right axis from the border. This number is expressed in
   * pixels, and represents the approximate distance of the axes from the sides
   * of the renderer. The default is 50.
   */
  svtkSetClampMacro(RightBorderOffset, int, 5, SVTK_INT_MAX);
  svtkGetMacro(RightBorderOffset, int);
  //@}

  //@{
  /**
   * Set/Get the offset of the top axis from the border. This number is expressed in
   * pixels, and represents the approximate distance of the axes from the sides
   * of the renderer. The default is 30.
   */
  svtkSetClampMacro(TopBorderOffset, int, 5, SVTK_INT_MAX);
  svtkGetMacro(TopBorderOffset, int);
  //@}

  //@{
  /**
   * Set/Get the offset of the left axis from the border. This number is expressed in
   * pixels, and represents the approximate distance of the axes from the sides
   * of the renderer. The default is 50.
   */
  svtkSetClampMacro(LeftBorderOffset, int, 5, SVTK_INT_MAX);
  svtkGetMacro(LeftBorderOffset, int);
  //@}

  //@{
  /**
   * Set/Get the offset of the bottom axis from the border. This number is expressed in
   * pixels, and represents the approximate distance of the axes from the sides
   * of the renderer. The default is 30.
   */
  svtkSetClampMacro(BottomBorderOffset, int, 5, SVTK_INT_MAX);
  svtkGetMacro(BottomBorderOffset, int);
  //@}

  //@{
  /**
   * Get/Set the corner offset. This is the offset factor used to offset the
   * axes at the corners. Default value is 2.0.
   */
  svtkSetClampMacro(CornerOffsetFactor, double, 1.0, 10.0);
  svtkGetMacro(CornerOffsetFactor, double);
  //@}

  //@{
  /**
   * Set/Get the labels text properties for the legend title and labels.
   */
  svtkGetObjectMacro(LegendTitleProperty, svtkTextProperty);
  svtkGetObjectMacro(LegendLabelProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * These are methods to retrieve the svtkAxisActors used to represent
   * the four axes that form this representation. Users may retrieve and
   * then modify these axes to control their appearance.
   */
  svtkGetObjectMacro(RightAxis, svtkAxisActor2D);
  svtkGetObjectMacro(TopAxis, svtkAxisActor2D);
  svtkGetObjectMacro(LeftAxis, svtkAxisActor2D);
  svtkGetObjectMacro(BottomAxis, svtkAxisActor2D);
  //@}

  //@{
  /**
   * Standard methods supporting the rendering process.
   */
  virtual void BuildRepresentation(svtkViewport* viewport);
  void GetActors2D(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  //@}

protected:
  svtkLegendScaleActor();
  ~svtkLegendScaleActor() override;

  int LabelMode;
  int RightBorderOffset;
  int TopBorderOffset;
  int LeftBorderOffset;
  int BottomBorderOffset;
  double CornerOffsetFactor;

  // The four axes around the borders of the renderer
  svtkAxisActor2D* RightAxis;
  svtkAxisActor2D* TopAxis;
  svtkAxisActor2D* LeftAxis;
  svtkAxisActor2D* BottomAxis;

  // Control the display of the axes
  svtkTypeBool RightAxisVisibility;
  svtkTypeBool TopAxisVisibility;
  svtkTypeBool LeftAxisVisibility;
  svtkTypeBool BottomAxisVisibility;

  // Support for the legend.
  svtkTypeBool LegendVisibility;
  svtkPolyData* Legend;
  svtkPoints* LegendPoints;
  svtkPolyDataMapper2D* LegendMapper;
  svtkActor2D* LegendActor;
  svtkTextMapper* LabelMappers[6];
  svtkActor2D* LabelActors[6];
  svtkTextProperty* LegendTitleProperty;
  svtkTextProperty* LegendLabelProperty;
  svtkCoordinate* Coordinate;

  svtkTimeStamp BuildTime;

private:
  svtkLegendScaleActor(const svtkLegendScaleActor&) = delete;
  void operator=(const svtkLegendScaleActor&) = delete;
};

#endif
