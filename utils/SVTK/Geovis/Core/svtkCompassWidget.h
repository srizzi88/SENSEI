/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompassWidget.h

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
 * @class   svtkCompassWidget
 * @brief   set a value by manipulating something
 *
 * The svtkCompassWidget is used to adjust a scalar value in an
 * application. Note that the actual appearance of the widget depends on
 * the specific representation for the widget.
 *
 * To use this widget, set the widget representation. (the details may
 * vary depending on the particulars of the representation).
 *
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If the slider bead is selected:
 *   LeftButtonPressEvent - select slider
 *   LeftButtonReleaseEvent - release slider
 *   MouseMoveEvent - move slider
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkCompassWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkCompassWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 */

#ifndef svtkCompassWidget_h
#define svtkCompassWidget_h

#include "svtkAbstractWidget.h"
#include "svtkGeovisCoreModule.h" // For export macro

class svtkCompassRepresentation;

class SVTKGEOVISCORE_EXPORT svtkCompassWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the class.
   */
  static svtkCompassWidget* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkCompassWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkCompassRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  //@{
  /**
   * Get the value for this widget.
   */
  double GetHeading();
  void SetHeading(double v);
  double GetTilt();
  void SetTilt(double t);
  double GetDistance();
  void SetDistance(double t);
  //@}

protected:
  svtkCompassWidget();
  ~svtkCompassWidget() override {}

  // These are the events that are handled
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void TimerAction(svtkAbstractWidget*);

  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Highlighting,
    Adjusting,
    TiltAdjusting,
    DistanceAdjusting
  };

  int TimerId;
  int TimerDuration;
  double StartTime;

private:
  svtkCompassWidget(const svtkCompassWidget&) = delete;
  void operator=(const svtkCompassWidget&) = delete;
};

#endif
