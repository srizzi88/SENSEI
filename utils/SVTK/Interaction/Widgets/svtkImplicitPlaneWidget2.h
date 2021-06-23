/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitPlaneWidget2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImplicitPlaneWidget2
 * @brief   3D widget for manipulating an infinite plane
 *
 * This 3D widget defines an infinite plane that can be interactively placed
 * in a scene. The widget is assumed to consist of four parts: 1) a plane
 * contained in a 2) bounding box, with a 3) plane normal, which is rooted
 * at a 4) point on the plane. (The representation paired with this widget
 * determines the actual geometry of the widget.)
 *
 * To use this widget, you generally pair it with a svtkImplicitPlaneRepresentation
 * (or a subclass). Various options are available for controlling how the
 * representation appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If the mouse is over the plane normal:
 *   LeftButtonPressEvent - select normal
 *   LeftButtonReleaseEvent - release normal
 *   MouseMoveEvent - orient the normal vector
 * If the mouse is over the origin point (handle):
 *   LeftButtonPressEvent - select handle
 *   LeftButtonReleaseEvent - release handle (if selected)
 *   MouseMoveEvent - move the origin point (constrained to the plane)
 * If the mouse is over the plane:
 *   LeftButtonPressEvent - select plane
 *   LeftButtonReleaseEvent - release plane (if selected)
 *   MouseMoveEvent - move the plane
 * If the mouse is over the outline:
 *   LeftButtonPressEvent - select outline
 *   LeftButtonReleaseEvent - release outline (if selected)
 *   MouseMoveEvent - move the outline
 * If the keypress characters are used
 *   'Down/Left' Move plane down
 *   'Up/Right' Move plane up
 * In all the cases, independent of what is picked, the widget responds to the
 * following SVTK events:
 *   MiddleButtonPressEvent - move the plane
 *   MiddleButtonReleaseEvent - release the plane
 *   RightButtonPressEvent - scale the widget's representation
 *   RightButtonReleaseEvent - stop scaling the widget
 *   MouseMoveEvent - scale (if right button) or move (if middle button) the widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkImplicitPlaneWidget2's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for widget motion has been invoked
 *   svtkWidgetEvent::Up and svtkWidgetEvent::Down -- MovePlaneAction
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkImplicitPlaneWidget2
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 *
 * @par Event Bindings:
 * This class, and svtkImplicitPlaneRepresentation, are next generation SVTK
 * widgets. An earlier version of this functionality was defined in the class
 * svtkImplicitPlaneWidget.
 *
 * @sa
 * svtk3DWidget svtkBoxWidget svtkPlaneWidget svtkLineWidget svtkPointWidget
 * svtkSphereWidget svtkImagePlaneWidget svtkImplicitCylinderWidget
 */

#ifndef svtkImplicitPlaneWidget2_h
#define svtkImplicitPlaneWidget2_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkImplicitPlaneRepresentation;
class svtkInteractionCallback;

class SVTKINTERACTIONWIDGETS_EXPORT svtkImplicitPlaneWidget2 : public svtkAbstractWidget
{
  friend class svtkInteractionCallback;

public:
  /**
   * Instantiate the object.
   */
  static svtkImplicitPlaneWidget2* New();

  //@{
  /**
   * Standard svtkObject methods
   */
  svtkTypeMacro(svtkImplicitPlaneWidget2, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkImplicitPlaneRepresentation* rep);

  // Description:
  // Disable/Enable the widget if needed.
  // Unobserved the camera if the widget is disabled.
  void SetEnabled(int enabling) override;

  /**
   * Observe/Unobserve the camera if the widget is locked/unlocked to update the
   * svtkImplicitePlaneRepresentation's normal.
   */
  void SetLockNormalToCamera(int lock);

  /**
   * Return the representation as a svtkImplicitPlaneRepresentation.
   */
  svtkImplicitPlaneRepresentation* GetImplicitPlaneRepresentation()
  {
    return reinterpret_cast<svtkImplicitPlaneRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkImplicitPlaneWidget2();
  ~svtkImplicitPlaneWidget2() override;

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
  static void MovePlaneAction(svtkAbstractWidget*);
  static void SelectAction3D(svtkAbstractWidget*);
  static void EndSelectAction3D(svtkAbstractWidget*);
  static void MoveAction3D(svtkAbstractWidget*);
  static void TranslationAxisLock(svtkAbstractWidget*);
  static void TranslationAxisUnLock(svtkAbstractWidget*);

  /**
   * Update the cursor shape based on the interaction state. Returns 1
   * if the cursor shape requested is different from the existing one.
   */
  int UpdateCursorShape(int interactionState);

  //@{
  /**
   * Handle the interaction callback that may come from the representation.
   */
  svtkInteractionCallback* InteractionCallback;
  void InvokeInteractionCallback();
  //@}

private:
  svtkImplicitPlaneWidget2(const svtkImplicitPlaneWidget2&) = delete;
  void operator=(const svtkImplicitPlaneWidget2&) = delete;
};

#endif
