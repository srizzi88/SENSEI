/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHandleWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHandleWidget
 * @brief   a general widget for moving handles
 *
 * The svtkHandleWidget is used to position a handle.  A handle is a widget
 * with a position (in display and world space). Various appearances are
 * available depending on its associated representation. The widget provides
 * methods for translation, including constrained translation along
 * coordinate axes. To use this widget, create and associate a representation
 * with the widget.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - select focal point of widget
 *   LeftButtonReleaseEvent - end selection
 *   MiddleButtonPressEvent - translate widget
 *   MiddleButtonReleaseEvent - end translation
 *   RightButtonPressEvent - scale widget
 *   RightButtonReleaseEvent - end scaling
 *   MouseMoveEvent - interactive movement across widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkHandleWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- focal point is being selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Translate -- translate the widget
 *   svtkWidgetEvent::EndTranslate -- end widget translation
 *   svtkWidgetEvent::Scale -- scale the widget
 *   svtkWidgetEvent::EndScale -- end scaling the widget
 *   svtkWidgetEvent::Move -- a request for widget motion
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkHandleWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 */

#ifndef svtkHandleWidget_h
#define svtkHandleWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkHandleRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkHandleWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkHandleWidget* New();

  //@{
  /**
   * Standard SVTK class macros.
   */
  svtkTypeMacro(svtkHandleWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkHandleRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkHandleRepresentation.
   */
  svtkHandleRepresentation* GetHandleRepresentation()
  {
    return reinterpret_cast<svtkHandleRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set. By default
   * an instance of svtkPointHandleRepresentation3D is created.
   */
  void CreateDefaultRepresentation() override;

  //@{
  /**
   * Enable / disable axis constrained motion of the handles. By default the
   * widget responds to the shift modifier to constrain the handle along the
   * axis closest aligned with the motion vector.
   */
  svtkSetMacro(EnableAxisConstraint, svtkTypeBool);
  svtkGetMacro(EnableAxisConstraint, svtkTypeBool);
  svtkBooleanMacro(EnableAxisConstraint, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable moving of handles. By default, the handle can be moved.
   */
  svtkSetMacro(EnableTranslation, svtkTypeBool);
  svtkGetMacro(EnableTranslation, svtkTypeBool);
  svtkBooleanMacro(EnableTranslation, svtkTypeBool);
  //@}

  //@{
  /**
   * Allow resizing of handles ? By default the right mouse button scales
   * the handle size.
   */
  svtkSetMacro(AllowHandleResize, svtkTypeBool);
  svtkGetMacro(AllowHandleResize, svtkTypeBool);
  svtkBooleanMacro(AllowHandleResize, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the widget state.
   */
  svtkGetMacro(WidgetState, int);
  //@}

  //@{
  /**
   * Allow the widget to be visible as an inactive representation when disabled.
   * By default, this is false i.e. the representation is not visible when the
   * widget is disabled.
   */
  svtkSetMacro(ShowInactive, svtkTypeBool);
  svtkGetMacro(ShowInactive, svtkTypeBool);
  svtkBooleanMacro(ShowInactive, svtkTypeBool);
  //@}

  // Manage the state of the widget
  enum _WidgetState
  {
    Start = 0,
    Active,
    Inactive
  };

  /**
   * Enable/disable widget.
   * Custom override for the SetEnabled method to allow for the inactive state.
   **/
  void SetEnabled(int enabling) override;

protected:
  svtkHandleWidget();
  ~svtkHandleWidget() override;

  // These are the callbacks for this widget
  static void GenericAction(svtkHandleWidget*);
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void TranslateAction(svtkAbstractWidget*);
  static void ScaleAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void SelectAction3D(svtkAbstractWidget*);
  static void MoveAction3D(svtkAbstractWidget*);
  static void ProcessKeyEvents(svtkObject*, unsigned long, void*, void*);

  // helper methods for cursor management
  void SetCursor(int state) override;

  int WidgetState;
  svtkTypeBool EnableAxisConstraint;
  svtkTypeBool EnableTranslation;

  // Allow resizing of handles.
  svtkTypeBool AllowHandleResize;

  // Keep representation visible when disabled
  svtkTypeBool ShowInactive;

  svtkCallbackCommand* KeyEventCallbackCommand;

private:
  svtkHandleWidget(const svtkHandleWidget&) = delete;
  void operator=(const svtkHandleWidget&) = delete;
};

#endif
