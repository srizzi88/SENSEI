/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitCylinderWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImplicitCylinderWidget
 * @brief   3D widget for manipulating an infinite cylinder
 *
 * This 3D widget defines an infinite cylinder that can be
 * interactively placed in a scene. The widget is assumed to consist
 * of four parts: 1) a cylinder contained in a 2) bounding box, with a
 * 3) cylinder axis, which is rooted at a 4) center point in the bounding
 * box. (The representation paired with this widget determines the
 * actual geometry of the widget.)
 *
 * To use this widget, you generally pair it with a svtkImplicitCylinderRepresentation
 * (or a subclass). Various options are available for controlling how the
 * representation appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If the cylinder axis is selected:
 *   LeftButtonPressEvent - select normal
 *   LeftButtonReleaseEvent - release (end select) normal
 *   MouseMoveEvent - orient the normal vector
 * If the center point (handle) is selected:
 *   LeftButtonPressEvent - select handle (if on slider)
 *   LeftButtonReleaseEvent - release handle (if selected)
 *   MouseMoveEvent - move the center point (constrained to plane or on the
 *                    axis if CTRL key is pressed)
 * If the cylinder is selected:
 *   LeftButtonPressEvent - select cylinder
 *   LeftButtonReleaseEvent - release cylinder
 *   MouseMoveEvent - increase/decrease cylinder radius
 * If the outline is selected:
 *   LeftButtonPressEvent - select outline
 *   LeftButtonReleaseEvent - release outline
 *   MouseMoveEvent - move the outline
 * If the keypress characters are used
 *   'Down/Left' Move cylinder away from viewer
 *   'Up/Right' Move cylinder towards viewer
 * In all the cases, independent of what is picked, the widget responds to the
 * following SVTK events:
 *   MiddleButtonPressEvent - move the cylinder
 *   MiddleButtonReleaseEvent - release the cylinder
 *   RightButtonPressEvent - scale the widget's representation
 *   RightButtonReleaseEvent - stop scaling the widget
 *   MouseMoveEvent - scale (if right button) or move (if middle button) the widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkImplicitCylinderWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for widget motion has been invoked
 *   svtkWidgetEvent::Up and svtkWidgetEvent::Down -- MoveCylinderAction
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkImplicitCylinderWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 *
 * @sa
 * svtk3DWidget svtkImplicitPlaneWidget
 */

#ifndef svtkImplicitCylinderWidget_h
#define svtkImplicitCylinderWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkImplicitCylinderRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkImplicitCylinderWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkImplicitCylinderWidget* New();

  //@{
  /**
   * Standard svtkObject methods
   */
  svtkTypeMacro(svtkImplicitCylinderWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkImplicitCylinderRepresentation* rep);

  /**
   * Return the representation as a svtkImplicitCylinderRepresentation.
   */
  svtkImplicitCylinderRepresentation* GetCylinderRepresentation()
  {
    return reinterpret_cast<svtkImplicitCylinderRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkImplicitCylinderWidget();
  ~svtkImplicitCylinderWidget() override = default;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  // These methods handle events
  static void SelectAction(svtkAbstractWidget*);
  static void TranslateAction(svtkAbstractWidget*);
  static void ScaleAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void MoveCylinderAction(svtkAbstractWidget*);
  static void TranslationAxisLock(svtkAbstractWidget*);
  static void TranslationAxisUnLock(svtkAbstractWidget*);

  /**
   * Update the cursor shape based on the interaction state. Returns 1
   * if the cursor shape requested is different from the existing one.
   */
  int UpdateCursorShape(int interactionState);

private:
  svtkImplicitCylinderWidget(const svtkImplicitCylinderWidget&) = delete;
  void operator=(const svtkImplicitCylinderWidget&) = delete;
};

#endif
