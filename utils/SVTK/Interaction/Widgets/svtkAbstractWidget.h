/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractWidget.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractWidget
 * @brief   define the API for widget / widget representation
 *
 * The svtkAbstractWidget defines an API and implements methods common to all
 * widgets using the interaction/representation design. In this design, the
 * term interaction means that part of the widget that performs event
 * handling, while the representation corresponds to a svtkProp (or the
 * subclass svtkWidgetRepresentation) used to represent the
 * widget. svtkAbstractWidget also implements some methods common to all
 * subclasses.
 *
 * Note that svtkAbstractWidget provides access to the
 * svtkWidgetEventTranslator.  This class is responsible for translating SVTK
 * events (defined in svtkCommand.h) into widget events (defined in
 * svtkWidgetEvent.h). This class can be manipulated so that different SVTK
 * events can be mapped into widget events, thereby allowing the modification
 * of event bindings. Each subclass of svtkAbstractWidget defines the events
 * to which it responds.
 *
 * @warning
 * Note that the pair ( svtkAbstractWidget / svtkWidgetRepresentation ) is an
 * implementation of the second generation SVTK Widgets design. In the first
 * generation design, widgets were implemented in a single monolithic
 * class. This design was problematic because in client-server application
 * it was difficult to manage widgets properly. Also, new "representations"
 * or look-and-feel, for a widget required a whole new class, with a lot of
 * redundant code. The separation of the widget event handling and
 * representation enables users and developers to create new appearances for
 * the widget. It also facilitates parallel processing, where the client
 * application handles events, and remote representations of the widget are
 * slaves to the client (and do not handle events).
 *
 * @sa
 * svtkWidgetRepresentation svtkWidgetEventTranslator svtkWidgetCallbackMapper
 */

#ifndef svtkAbstractWidget_h
#define svtkAbstractWidget_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkInteractorObserver.h"

class svtkWidgetEventTranslator;
class svtkWidgetCallbackMapper;
class svtkWidgetRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkAbstractWidget : public svtkInteractorObserver
{
public:
  //@{
  /**
   * Standard macros implementing standard SVTK methods.
   */
  svtkTypeMacro(svtkAbstractWidget, svtkInteractorObserver);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Methods for activating this widget. Note that the widget representation
   * must be specified or the widget will not appear.
   * ProcessEvents (On by default) must be On for Enabled widget to respond
   * to interaction. If ProcessEvents is Off, enabling/disabling a widget
   * merely affects the visibility of the representation.
   */
  void SetEnabled(int) override;

  //@{
  /**
   * Methods to change whether the widget responds to interaction.
   * Set this to Off to disable interaction. On by default.
   * Subclasses must override SetProcessEvents() to make sure
   * that they pass on the flag to all component widgets.
   */
  svtkSetClampMacro(ProcessEvents, svtkTypeBool, 0, 1);
  svtkGetMacro(ProcessEvents, svtkTypeBool);
  svtkBooleanMacro(ProcessEvents, svtkTypeBool);
  //@}

  /**
   * Get the event translator. Careful manipulation of this class enables
   * the user to override the default event bindings.
   */
  svtkWidgetEventTranslator* GetEventTranslator() { return this->EventTranslator; }

  /**
   * Create the default widget representation if one is not set. The
   * representation defines the geometry of the widget (i.e., how it appears)
   * as well as providing special methods for manipulting the state and
   * appearance of the widget.
   */
  virtual void CreateDefaultRepresentation() = 0;

  /**
   * This method is called by subclasses when a render method is to be
   * invoked on the svtkRenderWindowInteractor. This method should be called
   * (instead of svtkRenderWindow::Render() because it has built into it
   * optimizations for minimizing renders and/or speeding renders.
   */
  void Render();

  /**
   * Specifying a parent to this widget is used when creating composite
   * widgets. It is an internal method not meant to be used by the public.
   * When a widget has a parent, it defers the rendering to the parent. It
   * may also defer managing the cursor (see ManagesCursor ivar).
   */
  void SetParent(svtkAbstractWidget* parent) { this->Parent = parent; }
  svtkGetObjectMacro(Parent, svtkAbstractWidget);

  //@{
  /**
   * Return an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of
   * svtkProp (typically a subclass of svtkWidgetRepresentation) so it can be
   * added to the renderer independent of the widget.
   */
  svtkWidgetRepresentation* GetRepresentation()
  {
    this->CreateDefaultRepresentation();
    return this->WidgetRep;
  }
  //@}

  //@{
  /**
   * Turn on or off the management of the cursor. Cursor management is
   * typically disabled for subclasses when composite widgets are
   * created. For example, svtkHandleWidgets are often used to create
   * composite widgets, and the parent widget takes over the cursor
   * management.
   */
  svtkSetMacro(ManagesCursor, svtkTypeBool);
  svtkGetMacro(ManagesCursor, svtkTypeBool);
  svtkBooleanMacro(ManagesCursor, svtkTypeBool);
  //@}

  /**
   * Override the superclass method. This will automatically change the
   * priority of the widget. Unlike the superclass documentation, no
   * methods such as SetInteractor to null and reset it etc. are necessary
   */
  void SetPriority(float) override;

protected:
  svtkAbstractWidget();
  ~svtkAbstractWidget() override;

  // Handles the events; centralized here for all widgets.
  static void ProcessEventsHandler(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  // The representation for the widget. This is typically called by the
  // SetRepresentation() methods particular to each widget (i.e. subclasses
  // of this class). This method does the actual work; the SetRepresentation()
  // methods constrain the type that can be set.
  void SetWidgetRepresentation(svtkWidgetRepresentation* r);
  svtkWidgetRepresentation* WidgetRep;

  // helper methods for cursor management
  svtkTypeBool ManagesCursor;
  virtual void SetCursor(int svtkNotUsed(state)) {}

  // For translating and invoking events
  svtkWidgetEventTranslator* EventTranslator;
  svtkWidgetCallbackMapper* CallbackMapper;

  // The parent, if any, for this widget
  svtkAbstractWidget* Parent;

  // Call data which can be retrieved by the widget. This data is set
  // by ProcessEvents() if call data is provided during a callback
  // sequence.
  void* CallData;

  // Flag indicating if the widget should handle interaction events.
  // On by default.
  svtkTypeBool ProcessEvents;

private:
  svtkAbstractWidget(const svtkAbstractWidget&) = delete;
  void operator=(const svtkAbstractWidget&) = delete;
};

#endif
