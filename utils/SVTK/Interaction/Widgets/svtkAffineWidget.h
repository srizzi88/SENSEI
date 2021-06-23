/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAffineWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAffineWidget
 * @brief   perform affine transformations
 *
 * The svtkAffineWidget is used to perform affine transformations on objects.
 * (Affine transformations are transformations that keep parallel lines parallel.
 * Affine transformations include translation, scaling, rotation, and shearing.)
 *
 * To use this widget, set the widget representation. The representation
 * maintains a transformation matrix and other instance variables consistent
 * with the transformations applied by this widget.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - select widget: depending on which part is selected
 *                          translation, rotation, scaling, or shearing may follow.
 *   LeftButtonReleaseEvent - end selection of widget.
 *   MouseMoveEvent - interactive movement across widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkAffineWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- focal point is being selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for widget motion
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkAffineWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 */

#ifndef svtkAffineWidget_h
#define svtkAffineWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkAffineRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkAffineWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkAffineWidget* New();

  //@{
  /**
   * Standard SVTK class macros.
   */
  svtkTypeMacro(svtkAffineWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkAffineRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkAffineRepresentation.
   */
  svtkAffineRepresentation* GetAffineRepresentation()
  {
    return reinterpret_cast<svtkAffineRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Methods for activating this widget. This implementation extends the
   * superclasses' in order to resize the widget handles due to a render
   * start event.
   */
  void SetEnabled(int) override;

protected:
  svtkAffineWidget();
  ~svtkAffineWidget() override;

  // These are the callbacks for this widget
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void ModifyEventAction(svtkAbstractWidget*);

  // helper methods for cursor management
  void SetCursor(int state) override;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  // Keep track whether key modifier key is pressed
  int ModifierActive;

private:
  svtkAffineWidget(const svtkAffineWidget&) = delete;
  void operator=(const svtkAffineWidget&) = delete;
};

#endif
