/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoxWidget2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBoxWidget2
 * @brief   3D widget for manipulating a box
 *
 * This 3D widget interacts with a svtkBoxRepresentation class (i.e., it
 * handles the events that drive its corresponding representation). The
 * representation is assumed to represent a region of interest that is
 * represented by an arbitrarily oriented hexahedron (or box) with interior
 * face angles of 90 degrees (i.e., orthogonal faces). The representation
 * manifests seven handles that can be moused on and manipulated, plus the
 * six faces can also be interacted with. The first six handles are placed on
 * the six faces, the seventh is in the center of the box. In addition, a
 * bounding box outline is shown, the "faces" of which can be selected for
 * object rotation or scaling. A nice feature of svtkBoxWidget2, like any 3D
 * widget, will work with the current interactor style. That is, if
 * svtkBoxWidget2 does not handle an event, then all other registered
 * observers (including the interactor style) have an opportunity to process
 * the event. Otherwise, the svtkBoxWidget will terminate the processing of
 * the event that it handles.
 *
 * To use this widget, you generally pair it with a svtkBoxRepresentation
 * (or a subclass). Various options are available in the representation for
 * controlling how the widget appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If one of the seven handles are selected:
 *   LeftButtonPressEvent - select the appropriate handle
 *   LeftButtonReleaseEvent - release the currently selected handle
 *   MouseMoveEvent - move the handle
 * If one of the faces is selected:
 *   LeftButtonPressEvent - select a box face
 *   LeftButtonReleaseEvent - release the box face
 *   MouseMoveEvent - rotate the box
 * In all the cases, independent of what is picked, the widget responds to the
 * following SVTK events:
 *   MiddleButtonPressEvent - translate the widget
 *   MiddleButtonReleaseEvent - release the widget
 *   RightButtonPressEvent - scale the widget's representation
 *   RightButtonReleaseEvent - stop scaling the widget
 *   MouseMoveEvent - scale (if right button) or move (if middle button) the widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkBoxWidget2's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Scale -- some part of the widget has been selected
 *   svtkWidgetEvent::EndScale -- the selection process has completed
 *   svtkWidgetEvent::Translate -- some part of the widget has been selected
 *   svtkWidgetEvent::EndTranslate -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkBoxWidget2
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 *
 * @par Event Bindings:
 * This class, and the affiliated svtkBoxRepresentation, are second generation
 * SVTK widgets. An earlier version of this functionality was defined in the
 * class svtkBoxWidget.
 *
 * @sa
 * svtkBoxRepresentation svtkBoxWidget
 */

#ifndef svtkBoxWidget2_h
#define svtkBoxWidget2_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkBoxRepresentation;
class svtkHandleWidget;

class SVTKINTERACTIONWIDGETS_EXPORT svtkBoxWidget2 : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkBoxWidget2* New();

  //@{
  /**
   * Standard class methods for type information and printing.
   */
  svtkTypeMacro(svtkBoxWidget2, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkBoxRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  //@{
  /**
   * Control the behavior of the widget (i.e., how it processes
   * events). Translation, rotation, scaling and face movement can all be enabled and
   * disabled. Scaling refers to scaling of the whole widget at once,
   * (default is through right mouse button) while face movement refers to
   * scaling of the widget one face (axis) at a time (default through grabbing
   * one of the representation spherical handles).
   */
  svtkSetMacro(TranslationEnabled, svtkTypeBool);
  svtkGetMacro(TranslationEnabled, svtkTypeBool);
  svtkBooleanMacro(TranslationEnabled, svtkTypeBool);
  svtkSetMacro(ScalingEnabled, svtkTypeBool);
  svtkGetMacro(ScalingEnabled, svtkTypeBool);
  svtkBooleanMacro(ScalingEnabled, svtkTypeBool);
  svtkSetMacro(RotationEnabled, svtkTypeBool);
  svtkGetMacro(RotationEnabled, svtkTypeBool);
  svtkBooleanMacro(RotationEnabled, svtkTypeBool);
  svtkSetMacro(MoveFacesEnabled, svtkTypeBool);
  svtkGetMacro(MoveFacesEnabled, svtkTypeBool);
  svtkBooleanMacro(MoveFacesEnabled, svtkTypeBool);
  //@}

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the svtkBoxRepresentation class.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Override superclasses' SetEnabled() method because the line
   * widget must enable its internal handle widgets.
   */
  void SetEnabled(int enabling) override;

protected:
  svtkBoxWidget2();
  ~svtkBoxWidget2() override;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  // These methods handle events
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void TranslateAction(svtkAbstractWidget*);
  static void ScaleAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void SelectAction3D(svtkAbstractWidget*);
  static void EndSelectAction3D(svtkAbstractWidget*);
  static void MoveAction3D(svtkAbstractWidget*);
  static void StepAction3D(svtkAbstractWidget*);

  // Control whether scaling, rotation, and translation are supported
  svtkTypeBool TranslationEnabled;
  svtkTypeBool ScalingEnabled;
  svtkTypeBool RotationEnabled;
  svtkTypeBool MoveFacesEnabled;

  svtkCallbackCommand* KeyEventCallbackCommand;
  static void ProcessKeyEvents(svtkObject*, unsigned long, void*, void*);

private:
  svtkBoxWidget2(const svtkBoxWidget2&) = delete;
  void operator=(const svtkBoxWidget2&) = delete;
};

#endif
