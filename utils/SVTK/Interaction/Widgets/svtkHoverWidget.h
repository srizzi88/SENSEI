/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHoverWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHoverWidget
 * @brief   invoke a svtkTimerEvent when hovering
 *
 * The svtkHoverWidget is used to invoke an event when hovering in a render window.
 * Hovering occurs when mouse motion (in the render window) does not occur
 * for a specified amount of time (i.e., TimerDuration). This class can be used
 * as is (by observing TimerEvents) or for class derivation for those classes
 * wishing to do more with the hover event.
 *
 * To use this widget, specify an instance of svtkHoverWidget and specify the
 * time (in milliseconds) defining the hover period. Unlike most widgets,
 * this widget does not require a representation (although subclasses like
 * svtkBalloonWidget do require a representation).
 *
 * @par Event Bindings:
 * By default, the widget observes the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 *   MouseMoveEvent - manages a timer used to determine whether the mouse
 *                    is hovering.
 *   TimerEvent - when the time between events (e.g., mouse move), then a
 *                timer event is invoked.
 *   KeyPressEvent - when the "Enter" key is pressed after the balloon appears,
 *                   a callback is activated (e.g., WidgetActivateEvent).
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkHoverWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Move -- start (or reset) the timer
 *   svtkWidgetEvent::TimedOut -- when enough time is elapsed between defined
 *                               SVTK events the hover event is invoked.
 *   svtkWidgetEvent::SelectAction -- activate any callbacks associated
 *                                   with the balloon.
 * </pre>
 *
 * @par Event Bindings:
 * This widget invokes the following SVTK events on itself when the widget
 * determines that it is hovering. Note that observers of this widget can
 * listen for these events and take appropriate action.
 * <pre>
 *   svtkCommand::TimerEvent (when hovering is determined to occur)
 *   svtkCommand::EndInteractionEvent (after a hover has occurred and the
 *                                    mouse begins moving again).
 *   svtkCommand::WidgetActivateEvent (when the balloon is selected with a
 *                                    keypress).
 * </pre>
 *
 * @sa
 * svtkAbstractWidget
 */

#ifndef svtkHoverWidget_h
#define svtkHoverWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class SVTKINTERACTIONWIDGETS_EXPORT svtkHoverWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkHoverWidget* New();

  //@{
  /**
   * Standard methods for a SVTK class.
   */
  svtkTypeMacro(svtkHoverWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the hovering interval (in milliseconds). If after moving the
   * mouse the pointer stays over a svtkProp for this duration, then a
   * svtkTimerEvent::TimerEvent is invoked.
   */
  svtkSetClampMacro(TimerDuration, int, 1, 100000);
  svtkGetMacro(TimerDuration, int);
  //@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it performs special timer-related operations.
   */
  void SetEnabled(int) override;

  /**
   * A default representation, of which there is none, is created. Note
   * that the superclasses svtkAbstractWidget::GetRepresentation()
   * method returns nullptr.
   */
  void CreateDefaultRepresentation() override { this->WidgetRep = nullptr; }

protected:
  svtkHoverWidget();
  ~svtkHoverWidget() override;

  // The state of the widget

  enum
  {
    Start = 0,
    Timing,
    TimedOut
  };

  int WidgetState;

  // Callback interface to execute events
  static void MoveAction(svtkAbstractWidget*);
  static void HoverAction(svtkAbstractWidget*);
  static void SelectAction(svtkAbstractWidget*);

  // Subclasses of this class invoke these methods. If a non-zero
  // value is returned, a subclass is handling the event.
  virtual int SubclassHoverAction() { return 0; }
  virtual int SubclassEndHoverAction() { return 0; }
  virtual int SubclassSelectAction() { return 0; }

  //@{
  /**
   * Helper methods for creating and destroying timers.
   */
  int TimerId;
  int TimerDuration;
  //@}

private:
  svtkHoverWidget(const svtkHoverWidget&) = delete;
  void operator=(const svtkHoverWidget&) = delete;
};

#endif
