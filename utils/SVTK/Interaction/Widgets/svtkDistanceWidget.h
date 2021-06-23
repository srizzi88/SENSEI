/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistanceWidget.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDistanceWidget
 * @brief   measure the distance between two points
 *
 * The svtkDistanceWidget is used to measure the distance between two points.
 * The two end points can be positioned independently, and when they are
 * released, a special PlacePointEvent is invoked so that special operations
 * may be take to reposition the point (snap to grid, etc.) The widget has
 * two different modes of interaction: when initially defined (i.e., placing
 * the two points) and then a manipulate mode (adjusting the position of
 * the two points).
 *
 * To use this widget, specify an instance of svtkDistanceWidget and a
 * representation (a subclass of svtkDistanceRepresentation). The widget is
 * implemented using two instances of svtkHandleWidget which are used to
 * position the end points of the line. The representations for these two
 * handle widgets are provided by the svtkDistanceRepresentation.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - add a point or select a handle
 *   MouseMoveEvent - position the second point or move a handle
 *   LeftButtonReleaseEvent - release the handle
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkDistanceWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::AddPoint -- add one point; depending on the state
 *                               it may the first or second point added. Or,
 *                               if near a handle, select the handle.
 *   svtkWidgetEvent::Move -- move the second point or handle depending on the state.
 *   svtkWidgetEvent::EndSelect -- the handle manipulation process has completed.
 * </pre>
 *
 * @par Event Bindings:
 * This widget invokes the following SVTK events on itself (which observers
 * can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (beginning to interact)
 *   svtkCommand::EndInteractionEvent (completing interaction)
 *   svtkCommand::InteractionEvent (moving after selecting something)
 *   svtkCommand::PlacePointEvent (after point is positioned;
 *                                call data includes handle id (0,1))
 * </pre>
 *
 * @sa
 * svtkHandleWidget
 */

#ifndef svtkDistanceWidget_h
#define svtkDistanceWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkDistanceRepresentation;
class svtkHandleWidget;
class svtkDistanceWidgetCallback;

class SVTKINTERACTIONWIDGETS_EXPORT svtkDistanceWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkDistanceWidget* New();

  //@{
  /**
   * Standard methods for a SVTK class.
   */
  svtkTypeMacro(svtkDistanceWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it is a composite widget and does more than
   * its superclasses' svtkAbstractWidget::SetEnabled() method.
   */
  void SetEnabled(int) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkDistanceRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkDistanceRepresentation.
   */
  svtkDistanceRepresentation* GetDistanceRepresentation()
  {
    return reinterpret_cast<svtkDistanceRepresentation*>(this->WidgetRep);
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

  /**
   * Description:
   * Enum defining the state of the widget. By default the widget is in Start mode,
   * and expects to be interactively placed. While placing the points the widget
   * transitions to Define state. Once placed, the widget enters the Manipulate state.
   */

  enum
  {
    Start = 0,
    Define,
    Manipulate
  };

  //@{
  /**
   * Set the state of the widget. If the state is set to "Manipulate" then it
   * is assumed that the widget and its representation will be initialized
   * programmatically and is not interactively placed. Initially the widget
   * state is set to "Start" which means nothing will appear and the user
   * must interactively place the widget with repeated mouse selections. Set
   * the state to "Start" if you want interactive placement. Generally state
   * changes must be followed by a Render() for things to visually take
   * effect.
   */
  virtual void SetWidgetStateToStart();
  virtual void SetWidgetStateToManipulate();
  //@}

  /**
   * Return the current widget state.
   */
  virtual int GetWidgetState() { return this->WidgetState; }

protected:
  svtkDistanceWidget();
  ~svtkDistanceWidget() override;

  // The state of the widget
  int WidgetState;
  int CurrentHandle;

  // Callback interface to capture events when
  // placing the widget.
  static void AddPointAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void AddPointAction3D(svtkAbstractWidget*);
  static void MoveAction3D(svtkAbstractWidget*);
  static void EndSelectAction3D(svtkAbstractWidget*);

  // The positioning handle widgets
  svtkHandleWidget* Point1Widget;
  svtkHandleWidget* Point2Widget;
  svtkDistanceWidgetCallback* DistanceWidgetCallback1;
  svtkDistanceWidgetCallback* DistanceWidgetCallback2;

  // Methods invoked when the handles at the
  // end points of the widget are manipulated
  void StartDistanceInteraction(int handleNum);
  void DistanceInteraction(int handleNum);
  void EndDistanceInteraction(int handleNum);

  friend class svtkDistanceWidgetCallback;

private:
  svtkDistanceWidget(const svtkDistanceWidget&) = delete;
  void operator=(const svtkDistanceWidget&) = delete;
};

#endif
