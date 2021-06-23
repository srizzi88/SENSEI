/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkButtonWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkButtonWidget
 * @brief   activate an n-state button
 *
 * The svtkButtonWidget is used to interface with an n-state button. That is
 * each selection moves to the next button state (e.g., moves from "on" to
 * "off"). The widget uses modulo list traversal to transition through one or
 * more states. (A single state is simply a "selection" event; traversal
 * through the list can be in the forward or backward direction.)
 *
 * Depending on the nature of the representation the appearance of the button
 * can change dramatically, the specifics of appearance changes are a
 * function of the associated svtkButtonRepresentation (or subclass).
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - select button
 *   LeftButtonReleaseEvent - end the button selection process
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkButtonWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkButtonWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StateChangedEvent (on svtkWidgetEvent::EndSelect)
 * </pre>
 *
 */

#ifndef svtkButtonWidget_h
#define svtkButtonWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkButtonRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkButtonWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the class.
   */
  static svtkButtonWidget* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkButtonWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkButtonRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkButtonRepresentation.
   */
  svtkButtonRepresentation* GetSliderRepresentation()
  {
    return reinterpret_cast<svtkButtonRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it is a composite widget and does more than
   * its superclasses' svtkAbstractWidget::SetEnabled() method. The
   * method finds and sets the active viewport on the internal balloon
   * representation.
   */
  void SetEnabled(int) override;

protected:
  svtkButtonWidget();
  ~svtkButtonWidget() override {}

  // These are the events that are handled
  static void SelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Hovering,
    Selecting
  };

private:
  svtkButtonWidget(const svtkButtonWidget&) = delete;
  void operator=(const svtkButtonWidget&) = delete;
};

#endif
