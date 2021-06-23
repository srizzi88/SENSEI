/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxesTransformWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAxesTransformWidget
 * @brief   3D widget for performing 3D transformations around an axes
 *
 * This 3D widget defines an axes which is used to guide transformation. The
 * widget can translate, scale, and rotate around one of the three coordinate
 * axes. The widget consists of a handle at the origin (used for
 * translation), three axes (around which rotations occur), and three end
 * arrows (or cones depending on the representation) that can be stretched to
 * scale an object.  Optionally a text label can be used to indicate the
 * amount of the transformation.
 *
 * To use this widget, you generally pair it with a
 * svtkAxesTransformRepresentation (or a subclass). Various options are
 * available in the representation for controlling how the widget appears,
 * and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If the origin handle is selected:
 *   LeftButtonPressEvent - activate the associated handle widget
 *   LeftButtonReleaseEvent - release the handle widget associated with the point
 *   MouseMoveEvent - move the handle and hence the origin and the widget
 * If one of the lines is selected:
 *   LeftButtonPressEvent - activate rotation by selecting one of the three axes.
 *   LeftButtonReleaseEvent - end rotation
 *   MouseMoveEvent - moving along the selected axis causes rotation to occur.
 * If one of the arrows/cones is selected:
 *   LeftButtonPressEvent - activate scaling by selecting the ends of one of the three axes.
 *   LeftButtonReleaseEvent - end scaling
 *   MouseMoveEvent - moving along the selected axis causes scaling to occur.
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkAxesTransformWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkAxesTransformWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 *
 * @warning
 * Note that the widget can be picked even when it is "behind"
 * other actors.  This is an intended feature and not a bug.
 *
 * @warning
 * This class, and svtkAxesTransformRepresentation, are next generation SVTK widgets.
 *
 * @sa
 * svtkAxesTransformRepresentation svtkAffineWidget svtkBoxWidget2
 */

#ifndef svtkAxesTransformWidget_h
#define svtkAxesTransformWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkAxesTransformRepresentation;
class svtkHandleWidget;

class SVTKINTERACTIONWIDGETS_EXPORT svtkAxesTransformWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkAxesTransformWidget* New();

  //@{
  /**
   * Standard svtkObject methods
   */
  svtkTypeMacro(svtkAxesTransformWidget, svtkAbstractWidget);
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
  void SetRepresentation(svtkAxesTransformRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkAxesTransformRepresentation.
   */
  svtkAxesTransformRepresentation* GetLineRepresentation()
  {
    return reinterpret_cast<svtkAxesTransformRepresentation*>(this->WidgetRep);
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
  svtkAxesTransformWidget();
  ~svtkAxesTransformWidget() override;

  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };
  int CurrentHandle;

  // These methods handle events
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);

  // The positioning handle widgets
  svtkHandleWidget* OriginWidget;    // first end point
  svtkHandleWidget* SelectionWidget; // used when selecting any one of the axes

private:
  svtkAxesTransformWidget(const svtkAxesTransformWidget&) = delete;
  void operator=(const svtkAxesTransformWidget&) = delete;
};

#endif
