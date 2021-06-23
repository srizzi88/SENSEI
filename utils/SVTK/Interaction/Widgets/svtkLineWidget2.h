/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLineWidget2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLineWidget2
 * @brief   3D widget for manipulating a finite, straight line
 *
 * This 3D widget defines a straight line that can be interactively placed in
 * a scene. The widget is assumed to consist of two parts: 1) two end points
 * and 2) a straight line connecting the two points. (The representation
 * paired with this widget determines the actual geometry of the widget.) The
 * positioning of the two end points is facilitated by using svtkHandleWidgets
 * to position the points.
 *
 * To use this widget, you generally pair it with a svtkLineRepresentation
 * (or a subclass). Various options are available in the representation for
 * controlling how the widget appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If one of the two end points are selected:
 *   LeftButtonPressEvent - activate the associated handle widget
 *   LeftButtonReleaseEvent - release the handle widget associated with the point
 *   MouseMoveEvent - move the point
 * If the line is selected:
 *   LeftButtonPressEvent - activate a handle widget accociated with the line
 *   LeftButtonReleaseEvent - release the handle widget associated with the line
 *   MouseMoveEvent - translate the line
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
 * into the svtkLineWidget2's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkLineWidget2
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 *
 *
 * @par Event Bindings:
 * This class, and svtkLineRepresentation, are next generation SVTK widgets. An
 * earlier version of this functionality was defined in the class
 * svtkLineWidget.
 *
 * @sa
 * svtkLineRepresentation svtkLineWidget svtk3DWidget svtkImplicitPlaneWidget
 * svtkImplicitPlaneWidget2
 */

#ifndef svtkLineWidget2_h
#define svtkLineWidget2_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkLineRepresentation;
class svtkHandleWidget;

class SVTKINTERACTIONWIDGETS_EXPORT svtkLineWidget2 : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkLineWidget2* New();

  //@{
  /**
   * Standard svtkObject methods
   */
  svtkTypeMacro(svtkLineWidget2, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Override superclasses' SetEnabled() method because the line
   * widget must enable its internal handle widgets.
   */
  void SetEnabled(int enabling) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkLineRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkLineRepresentation.
   */
  svtkLineRepresentation* GetLineRepresentation()
  {
    return reinterpret_cast<svtkLineRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Methods to change the whether the widget responds to interaction.
   * Overridden to pass the state to component widgets.
   */
  void SetProcessEvents(svtkTypeBool) override;

protected:
  svtkLineWidget2();
  ~svtkLineWidget2() override;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };
  int CurrentHandle;

  // These methods handle events
  static void SelectAction(svtkAbstractWidget*);
  static void TranslateAction(svtkAbstractWidget*);
  static void ScaleAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);

  // The positioning handle widgets
  svtkHandleWidget* Point1Widget; // first end point
  svtkHandleWidget* Point2Widget; // second end point
  svtkHandleWidget* LineHandle;   // used when selecting the line

  svtkCallbackCommand* KeyEventCallbackCommand;
  static void ProcessKeyEvents(svtkObject*, unsigned long, void*, void*);

private:
  svtkLineWidget2(const svtkLineWidget2&) = delete;
  void operator=(const svtkLineWidget2&) = delete;
};

#endif
