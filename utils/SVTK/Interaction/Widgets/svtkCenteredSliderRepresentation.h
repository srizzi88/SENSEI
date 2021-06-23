/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCenteredSliderRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   svtkCenteredSliderRepresentation
 * @brief   provide the representation for a svtkCenteredSliderWidget
 *
 * This class is used to represent and render a svtkCenteredSliderWidget. To use this
 * class, you must at a minimum specify the end points of the
 * slider. Optional instance variable can be used to modify the appearance of
 * the widget.
 *
 *
 * @sa
 * svtkSliderWidget
 */

#ifndef svtkCenteredSliderRepresentation_h
#define svtkCenteredSliderRepresentation_h

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

class SVTKINTERACTIONWIDGETS_EXPORT svtkCenteredSliderRepresentation : public svtkSliderRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkCenteredSliderRepresentation* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkCenteredSliderRepresentation, svtkSliderRepresentation);
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
   * Get the properties for the tube and slider
   */
  svtkGetObjectMacro(TubeProperty, svtkProperty2D);
  svtkGetObjectMacro(SliderProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Get the selection property. This property is used to modify the
   * appearance of selected objects (e.g., the slider).
   */
  svtkGetObjectMacro(SelectedProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Set/Get the properties for the label and title text.
   */
  svtkGetObjectMacro(LabelProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Methods to interface with the svtkSliderWidget. The PlaceWidget() method
   * assumes that the parameter bounds[6] specifies the location in display
   * space where the widget should be placed.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void WidgetInteraction(double eventPos[2]) override;
  void Highlight(int) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  void GetActors(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  //@}

protected:
  svtkCenteredSliderRepresentation();
  ~svtkCenteredSliderRepresentation() override;

  // Positioning the widget
  svtkCoordinate* Point1Coordinate;
  svtkCoordinate* Point2Coordinate;

  // Determine the parameter t along the slider
  virtual double ComputePickPosition(double x, double y);

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

  svtkTextProperty* LabelProperty;
  svtkTextActor* LabelActor;

  svtkProperty2D* SelectedProperty;
  int HighlightState;

  // build the tube geometry
  void BuildTube();

private:
  // how many points along the tube
  int ArcCount;
  double ArcStart;
  double ArcEnd;
  double ButtonSize;
  double TubeSize;

  svtkCenteredSliderRepresentation(const svtkCenteredSliderRepresentation&) = delete;
  void operator=(const svtkCenteredSliderRepresentation&) = delete;
};

#endif
