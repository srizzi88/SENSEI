/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCenteredSliderWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCenteredSliderWidget
 * @brief   set a value by manipulating a slider
 *
 * The svtkCenteredSliderWidget is used to adjust a scalar value in an application.
 * This class measures deviations form the center point on the slider.
 * Moving the slider
 * modifies the value of the widget, which can be used to set parameters on
 * other objects. Note that the actual appearance of the widget depends on
 * the specific representation for the widget.
 *
 * To use this widget, set the widget representation. The representation is
 * assumed to consist of a tube, two end caps, and a slider (the details may
 * vary depending on the particulars of the representation). Then in the
 * representation you will typically set minimum and maximum value, as well
 * as the current value. The position of the slider must also be set, as well
 * as various properties.
 *
 * Note that the value should be obtain from the widget, not from the
 * representation. Also note that Minimum and Maximum values are in terms of
 * value per second. The value you get from this widget's GetValue method is
 * multiplied by time.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If the slider bead is selected:
 *   LeftButtonPressEvent - select slider (if on slider)
 *   LeftButtonReleaseEvent - release slider (if selected)
 *   MouseMoveEvent - move slider
 * If the end caps or slider tube are selected:
 *   LeftButtonPressEvent - move (or animate) to cap or point on tube;
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkCenteredSliderWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkCenteredSliderWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 */

#ifndef svtkCenteredSliderWidget_h
#define svtkCenteredSliderWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkSliderRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkCenteredSliderWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the class.
   */
  static svtkCenteredSliderWidget* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkCenteredSliderWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkSliderRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkSliderRepresentation.
   */
  svtkSliderRepresentation* GetSliderRepresentation()
  {
    return reinterpret_cast<svtkSliderRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Get the value fo this widget.
   */
  double GetValue() { return this->Value; }

protected:
  svtkCenteredSliderWidget();
  ~svtkCenteredSliderWidget() override {}

  // These are the events that are handled
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void TimerAction(svtkAbstractWidget*);

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Sliding
  };

  int TimerId;
  int TimerDuration;
  double StartTime;
  double Value;

private:
  svtkCenteredSliderWidget(const svtkCenteredSliderWidget&) = delete;
  void operator=(const svtkCenteredSliderWidget&) = delete;
};

#endif
