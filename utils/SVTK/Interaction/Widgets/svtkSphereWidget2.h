/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereWidget2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSphereWidget2
 * @brief   3D widget for manipulating a point on a sphere
 *
 * This 3D widget interacts with a svtkSphereRepresentation class (i.e., it
 * handles the events that drive its corresponding representation). It can be
 * used to position a point on a sphere (for example, to place a light or
 * camera), or to position a sphere in a scene, including translating and
 * scaling the sphere.
 *
 * A nice feature of svtkSphereWidget2, like any 3D widget, is that it will
 * work in combination with the current interactor style (or any other
 * interactor observer). That is, if svtkSphereWidget2 does not handle an
 * event, then all other registered observers (including the interactor
 * style) have an opportunity to process the event. Otherwise, the
 * svtkSphereWidget2 will terminate the processing of the event that it
 * handles.
 *
 * To use this widget, you generally pair it with a svtkSphereRepresentation
 * (or a subclass). Various options are available in the representation for
 * controlling how the widget appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If the handle or sphere are selected:
 *   LeftButtonPressEvent - select the handle or sphere
 *   LeftButtonReleaseEvent - release the handle to sphere
 *   MouseMoveEvent - move the handle or translate the sphere
 * In all the cases, independent of what is picked, the widget responds to the
 * following SVTK events:
 *   MiddleButtonPressEvent - translate the representation
 *   MiddleButtonReleaseEvent - stop translating the representation
 *   RightButtonPressEvent - scale the widget's representation
 *   RightButtonReleaseEvent - stop scaling the representation
 *   MouseMoveEvent - scale (if right button) or move (if middle button) the widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkSphereWidget2's widget events:
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
 * In turn, when these widget events are processed, the svtkSphereWidget2
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 *
 * @par Event Bindings:
 * This class, and the affiliated svtkSphereRepresentation, are second generation
 * SVTK widgets. An earlier version of this functionality was defined in the
 * class svtkSphereWidget.
 *
 * @sa
 * svtkSphereRepresentation svtkSphereWidget
 */

#ifndef svtkSphereWidget2_h
#define svtkSphereWidget2_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkSphereRepresentation;
class svtkHandleWidget;

class SVTKINTERACTIONWIDGETS_EXPORT svtkSphereWidget2 : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkSphereWidget2* New();

  //@{
  /**
   * Standard class methods for type information and printing.
   */
  svtkTypeMacro(svtkSphereWidget2, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of
   * svtkProp so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkSphereRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  //@{
  /**
   * Control the behavior of the widget (i.e., how it processes
   * events). Translation, and scaling can all be enabled and disabled.
   */
  svtkSetMacro(TranslationEnabled, svtkTypeBool);
  svtkGetMacro(TranslationEnabled, svtkTypeBool);
  svtkBooleanMacro(TranslationEnabled, svtkTypeBool);
  svtkSetMacro(ScalingEnabled, svtkTypeBool);
  svtkGetMacro(ScalingEnabled, svtkTypeBool);
  svtkBooleanMacro(ScalingEnabled, svtkTypeBool);
  //@}

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the svtkSphereRepresentation class.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Override superclasses' SetEnabled() method because the line
   * widget must enable its internal handle widgets.
   */
  void SetEnabled(int enabling) override;

protected:
  svtkSphereWidget2();
  ~svtkSphereWidget2() override;

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

  // Control whether scaling and translation are supported
  svtkTypeBool TranslationEnabled;
  svtkTypeBool ScalingEnabled;

  svtkCallbackCommand* KeyEventCallbackCommand;
  static void ProcessKeyEvents(svtkObject*, unsigned long, void*, void*);

private:
  svtkSphereWidget2(const svtkSphereWidget2&) = delete;
  void operator=(const svtkSphereWidget2&) = delete;
};

#endif
