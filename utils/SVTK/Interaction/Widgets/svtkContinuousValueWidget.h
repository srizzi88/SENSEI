/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContinuousValueWidget.h

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
 * @class   svtkContinuousValueWidget
 * @brief   set a value by manipulating something
 *
 * The svtkContinuousValueWidget is used to adjust a scalar value in an
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
 * into the svtkContinuousValueWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkContinuousValueWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 */

#ifndef svtkContinuousValueWidget_h
#define svtkContinuousValueWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkContinuousValueWidgetRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkContinuousValueWidget : public svtkAbstractWidget
{
public:
  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkContinuousValueWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkContinuousValueWidgetRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkContinuousValueWidgetRepresentation.
   */
  svtkContinuousValueWidgetRepresentation* GetContinuousValueWidgetRepresentation()
  {
    return reinterpret_cast<svtkContinuousValueWidgetRepresentation*>(this->WidgetRep);
  }

  //@{
  /**
   * Get the value for this widget.
   */
  double GetValue();
  void SetValue(double v);
  //@}

protected:
  svtkContinuousValueWidget();
  ~svtkContinuousValueWidget() override {}

  // These are the events that are handled
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Highlighting,
    Adjusting
  };

  double Value;

private:
  svtkContinuousValueWidget(const svtkContinuousValueWidget&) = delete;
  void operator=(const svtkContinuousValueWidget&) = delete;
};

#endif
