/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSliderRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSliderRepresentation2D
 * @brief   provide the representation for a svtkSliderWidget with a 3D skin
 *
 * This class is used to represent and render a svtkSliderWidget. To use this
 * class, you must at a minimum specify the end points of the
 * slider. Optional instance variable can be used to modify the appearance of
 * the widget.
 *
 *
 * @sa
 * svtkSliderWidget
 */

#ifndef svtkSliderRepresentation2D_h
#define svtkSliderRepresentation2D_h

#include "svtkCoordinate.h"               // For svtkViewportCoordinateMacro
#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkSliderRepresentation.h"

class svtkPoints;
class svtkCellArray;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkActor2D;
class svtkCoordinate;
class svtkProperty2D;
class svtkPropCollection;
class svtkWindow;
class svtkViewport;
class svtkTransform;
class svtkTransformPolyDataFilter;
class svtkTextProperty;
class svtkTextMapper;
class svtkTextActor;

class SVTKINTERACTIONWIDGETS_EXPORT svtkSliderRepresentation2D : public svtkSliderRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkSliderRepresentation2D* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkSliderRepresentation2D, svtkSliderRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Position the first end point of the slider. Note that this point is an
   * instance of svtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point1Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  svtkCoordinate* GetPoint1Coordinate();

  /**
   * Position the second end point of the slider. Note that this point is an
   * instance of svtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point2Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  svtkCoordinate* GetPoint2Coordinate();

  //@{
  /**
   * Specify the label text for this widget. If the value is not set, or set
   * to the empty string "", then the label text is not displayed.
   */
  void SetTitleText(const char*) override;
  const char* GetTitleText() override;
  //@}

  //@{
  /**
   * Get the slider properties. The properties of the slider when selected
   * and unselected can be manipulated.
   */
  svtkGetObjectMacro(SliderProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Get the properties for the tube and end caps.
   */
  svtkGetObjectMacro(TubeProperty, svtkProperty2D);
  svtkGetObjectMacro(CapProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Get the selection property. This property is used to modify the appearance of
   * selected objects (e.g., the slider).
   */
  svtkGetObjectMacro(SelectedProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Set/Get the properties for the label and title text.
   */
  svtkGetObjectMacro(LabelProperty, svtkTextProperty);
  svtkGetObjectMacro(TitleProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Methods to interface with the svtkSliderWidget. The PlaceWidget() method
   * assumes that the parameter bounds[6] specifies the location in display space
   * where the widget should be placed.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double newEventPos[2]) override;
  void Highlight(int) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  void GetActors2D(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  //@}

protected:
  svtkSliderRepresentation2D();
  ~svtkSliderRepresentation2D() override;

  // Positioning the widget
  svtkCoordinate* Point1Coordinate;
  svtkCoordinate* Point2Coordinate;

  // Determine the parameter t along the slider
  virtual double ComputePickPosition(double eventPos[2]);

  // Define the geometry. It is constructed in canaonical position
  // along the x-axis and then rotated into position.
  svtkTransform* XForm;
  svtkPoints* Points;

  svtkCellArray* SliderCells;
  svtkPolyData* Slider;
  svtkTransformPolyDataFilter* SliderXForm;
  svtkPolyDataMapper2D* SliderMapper;
  svtkActor2D* SliderActor;
  svtkProperty2D* SliderProperty;

  svtkCellArray* TubeCells;
  svtkPolyData* Tube;
  svtkTransformPolyDataFilter* TubeXForm;
  svtkPolyDataMapper2D* TubeMapper;
  svtkActor2D* TubeActor;
  svtkProperty2D* TubeProperty;

  svtkCellArray* CapCells;
  svtkPolyData* Cap;
  svtkTransformPolyDataFilter* CapXForm;
  svtkPolyDataMapper2D* CapMapper;
  svtkActor2D* CapActor;
  svtkProperty2D* CapProperty;

  svtkTextProperty* LabelProperty;
  svtkTextMapper* LabelMapper;
  svtkActor2D* LabelActor;

  svtkTextProperty* TitleProperty;
  svtkTextMapper* TitleMapper;
  svtkActor2D* TitleActor;

  svtkProperty2D* SelectedProperty;

  // internal variables used for computation
  double X;

private:
  svtkSliderRepresentation2D(const svtkSliderRepresentation2D&) = delete;
  void operator=(const svtkSliderRepresentation2D&) = delete;
};

#endif
