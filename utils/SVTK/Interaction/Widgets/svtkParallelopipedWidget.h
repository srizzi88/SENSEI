/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelopipedWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParallelopipedWidget
 * @brief   a widget to manipulate 3D parallelopipeds
 *
 *
 * This widget was designed with the aim of visualizing / probing cuts on
 * a skewed image data / structured grid.
 *
 * @par Interaction:
 * The widget allows you to create a parallelopiped (defined by 8 handles).
 * The widget is initially placed by using the "PlaceWidget" method in the
 * representation class. After the widget has been created, the following
 * interactions may be used to manipulate it :
 * 1) Click on a handle and drag it around moves the handle in space, while
 *    keeping the same axis alignment of the parallelopiped
 * 2) Dragging a handle with the shift button pressed resizes the piped
 *    along an axis.
 * 3) Control-click on a handle creates a chair at that position. (A chair
 *    is a depression in the piped that allows you to visualize cuts in the
 *    volume).
 * 4) Clicking on a chair and dragging it around moves the chair within the
 *    piped.
 * 5) Shift-click on the piped enables you to translate it.
 *
 */

#ifndef svtkParallelopipedWidget_h
#define svtkParallelopipedWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkParallelopipedRepresentation;
class svtkHandleWidget;
class svtkWidgetSet;

class SVTKINTERACTIONWIDGETS_EXPORT svtkParallelopipedWidget : public svtkAbstractWidget
{

  friend class svtkWidgetSet;

public:
  /**
   * Instantiate the object.
   */
  static svtkParallelopipedWidget* New();

  svtkTypeMacro(svtkParallelopipedWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Override the superclass method. This is a composite widget, (it internally
   * consists of handle widgets). We will override the superclass method, so
   * that we can pass the enabled state to the internal widgets as well.
   */
  void SetEnabled(int) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkParallelopipedRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkParallelopipedRepresentation.
   */
  svtkParallelopipedRepresentation* GetParallelopipedRepresentation()
  {
    return reinterpret_cast<svtkParallelopipedRepresentation*>(this->WidgetRep);
  }

  //@{
  /**
   * Enable/disable the creation of a chair on this widget. If off,
   * chairs cannot be created.
   */
  svtkSetMacro(EnableChairCreation, svtkTypeBool);
  svtkGetMacro(EnableChairCreation, svtkTypeBool);
  svtkBooleanMacro(EnableChairCreation, svtkTypeBool);
  //@}

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
  svtkParallelopipedWidget();
  ~svtkParallelopipedWidget() override;

  static void RequestResizeCallback(svtkAbstractWidget*);
  static void RequestResizeAlongAnAxisCallback(svtkAbstractWidget*);
  static void RequestChairModeCallback(svtkAbstractWidget*);
  static void TranslateCallback(svtkAbstractWidget*);
  static void OnMouseMoveCallback(svtkAbstractWidget*);
  static void OnLeftButtonUpCallback(svtkAbstractWidget*);

  // Control whether chairs can be created
  svtkTypeBool EnableChairCreation;

  //@{
  void BeginTranslateAction(svtkParallelopipedWidget* dispatcher);
  void TranslateAction(svtkParallelopipedWidget* dispatcher);
  //@}

  // helper methods for cursor management
  void SetCursor(int state) override;

  // To break reference count loops
  void ReportReferences(svtkGarbageCollector* collector) override;

  // The positioning handle widgets
  svtkHandleWidget** HandleWidgets;

  /**
   * Events invoked by this widget
   */
  enum WidgetEventIds
  {
    RequestResizeEvent = 10000,
    RequestResizeAlongAnAxisEvent,
    RequestChairModeEvent
  };

  svtkWidgetSet* WidgetSet;

private:
  svtkParallelopipedWidget(const svtkParallelopipedWidget&) = delete;
  void operator=(const svtkParallelopipedWidget&) = delete;
};

#endif
