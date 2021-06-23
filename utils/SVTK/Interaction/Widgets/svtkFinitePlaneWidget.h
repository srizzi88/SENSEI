/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFinitePlaneWidget.h

  Copyright (c)
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFinitePlaneWidget
 * @brief   3D widget for manipulating a finite plane
 *
 * This 3D widget interacts with a svtkFinitePlaneRepresentation class (i.e., it
 * handles the events that drive its corresponding representation). This 3D
 * widget defines a finite plane that can be interactively placed in a scene.
 * The widget is assumed to consist of four parts: 1) a plane with 2) a normal
 * and 3) three handles that can be moused on and manipulated.
 * The green and red handles represent the semi finite plane definition,
 * the third is in the center of the plane.
 * Operation like rotation of the plane (using normal), origin translation and
 * geometry plane modification using green and red handles are availables.
 *
 * To use this widget, you generally pair it with a svtkFinitePlaneRepresentation
 * (or a subclass). Various options are available in the representation for
 * controlling how the widget appears, and how the widget reacts.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If one of the 3 handles are selected:
 *   LeftButtonPressEvent - select the appropriate handle
 *   LeftButtonReleaseEvent - release the currently selected handle
 *   MouseMoveEvent - move the handle
 * In all the cases, independent of what is picked, the widget responds to the
 * following SVTK events:
 *   LeftButtonPressEvent - start select action
 *   LeftButtonReleaseEvent - stop select action
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkFinitePlaneWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkFinitePlaneWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 * @sa
 * svtkFinitePlaneRepresentation
 */

#ifndef svtkFinitePlaneWidget_h
#define svtkFinitePlaneWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkFinitePlaneRepresentation;
class svtkHandleWidget;

class SVTKINTERACTIONWIDGETS_EXPORT svtkFinitePlaneWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkFinitePlaneWidget* New();

  //@{
  /**
   * Standard svtkObject methods
   */
  svtkTypeMacro(svtkFinitePlaneWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkFinitePlaneRepresentation* r);

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the svtkFinitePlaneRepresentation class.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkFinitePlaneWidget();
  ~svtkFinitePlaneWidget() override;

  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  // These methods handle events
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);

  /**
   * Update the cursor shape based on the interaction state. Returns 1
   * if the cursor shape requested is different from the existing one.
   */
  int UpdateCursorShape(int interactionState);

private:
  svtkFinitePlaneWidget(const svtkFinitePlaneWidget&) = delete;
  void operator=(const svtkFinitePlaneWidget&) = delete;
};

#endif
