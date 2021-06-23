/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBorderWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBorderWidget
 * @brief   place a border around a 2D rectangular region
 *
 * This class is a superclass for 2D widgets that may require a rectangular
 * border. Besides drawing a border, the widget provides methods for resizing
 * and moving the rectangular region (and associated border). The widget
 * provides methods and internal data members so that subclasses can take
 * advantage of this widgets capabilities, requiring only that the subclass
 * defines a "representation", i.e., some combination of props or actors
 * that can be managed in the 2D rectangular region.
 *
 * The class defines basic positioning functionality, including the ability
 * to size the widget with locked x/y proportions. The area within the border
 * may be made "selectable" as well, meaning that a selection event interior
 * to the widget invokes a virtual SelectRegion() method, which can be used
 * to pick objects or otherwise manipulate data interior to the widget.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * On the boundary of the widget:
 *   LeftButtonPressEvent - select boundary
 *   LeftButtonReleaseEvent - deselect boundary
 *   MouseMoveEvent - move/resize widget depending on which portion of the
 *                    boundary was selected.
 * On the interior of the widget:
 *   LeftButtonPressEvent - invoke SelectButton() callback (if the ivar
 *                          Selectable is on)
 * Anywhere on the widget:
 *   MiddleButtonPressEvent - move the widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkBorderWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Translate -- the widget is to be translated
 *   svtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, this widget invokes the
 * following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 * @sa
 * svtkInteractorObserver svtkCameraInterpolator
 */

#ifndef svtkBorderWidget_h
#define svtkBorderWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkBorderRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkBorderWidget : public svtkAbstractWidget
{
public:
  /**
   * Method to instantiate class.
   */
  static svtkBorderWidget* New();

  //@{
  /**
   * Standard methods for class.
   */
  svtkTypeMacro(svtkBorderWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Indicate whether the interior region of the widget can be selected or
   * not. If not, then events (such as left mouse down) allow the user to
   * "move" the widget, and no selection is possible. Otherwise the
   * SelectRegion() method is invoked.
   */
  svtkSetMacro(Selectable, svtkTypeBool);
  svtkGetMacro(Selectable, svtkTypeBool);
  svtkBooleanMacro(Selectable, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether the boundary of the widget can be resized.
   * If not, the cursor will not change to "resize" type when mouse
   * over the boundary.
   */
  svtkSetMacro(Resizable, svtkTypeBool);
  svtkGetMacro(Resizable, svtkTypeBool);
  svtkBooleanMacro(Resizable, svtkTypeBool);
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkBorderRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkBorderRepresentation.
   */
  svtkBorderRepresentation* GetBorderRepresentation()
  {
    return reinterpret_cast<svtkBorderRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkBorderWidget();
  ~svtkBorderWidget() override;

  /**
   * Subclasses generally implement this method. The SelectRegion() method
   * offers a subclass the chance to do something special if the interior
   * of the widget is selected.
   */
  virtual void SelectRegion(double eventPos[2]);

  // enable the selection of the region interior to the widget
  svtkTypeBool Selectable;
  svtkTypeBool Resizable;

  // processes the registered events
  static void SelectAction(svtkAbstractWidget*);
  static void TranslateAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);

  // Special internal methods to support subclasses handling events.
  // If a non-zero value is returned, the subclass is handling the event.
  virtual int SubclassSelectAction() { return 0; }
  virtual int SubclassTranslateAction() { return 0; }
  virtual int SubclassEndSelectAction() { return 0; }
  virtual int SubclassMoveAction() { return 0; }

  // helper methods for cursoe management
  void SetCursor(int State) override;

  // widget state
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Define,
    Manipulate,
    Selected
  };

private:
  svtkBorderWidget(const svtkBorderWidget&) = delete;
  void operator=(const svtkBorderWidget&) = delete;
};

#endif
