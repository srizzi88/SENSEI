/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWidgetEventTranslator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWidgetEventTranslator
 * @brief   map SVTK events into widget events
 *
 * svtkWidgetEventTranslator maps SVTK events (defined on svtkCommand) into
 * widget events (defined in svtkWidgetEvent.h). This class is typically used
 * in combination with svtkWidgetCallbackMapper, which is responsible for
 * translating widget events into method callbacks, and then invoking the
 * callbacks.
 *
 * This class can be used to define different mappings of SVTK events into
 * the widget events. Thus widgets can be reconfigured to use different
 * event bindings.
 *
 * @sa
 * svtkWidgetEvent svtkCommand svtkInteractorObserver
 */

#ifndef svtkWidgetEventTranslator_h
#define svtkWidgetEventTranslator_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkObject.h"

// Support PIMPL encapsulation of internal STL map
class svtkEventMap;
class svtkRenderWindowInteractor;
class svtkCallbackCommand;
class svtkEvent;
class svtkAbstractWidget;
class svtkEventData;

// This is a lightweight class that should be used internally by the widgets
class SVTKINTERACTIONWIDGETS_EXPORT svtkWidgetEventTranslator : public svtkObject
{
public:
  /**
   * Instantiate the object.
   */
  static svtkWidgetEventTranslator* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkWidgetEventTranslator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Use these methods to create the translation from a SVTK event to a widget
   * event. Specifying svtkWidgetEvent::NoEvent or an empty
   * string for the (toEvent) erases the mapping for the event.
   */
  void SetTranslation(unsigned long SVTKEvent, unsigned long widgetEvent);
  void SetTranslation(const char* SVTKEvent, const char* widgetEvent);
  void SetTranslation(unsigned long SVTKEvent, int modifier, char keyCode, int repeatCount,
    const char* keySym, unsigned long widgetEvent);
  void SetTranslation(svtkEvent* SVTKevent, unsigned long widgetEvent);
  void SetTranslation(unsigned long SVTKEvent, svtkEventData* edata, unsigned long widgetEvent);
  //@}

  //@{
  /**
   * Translate a SVTK event into a widget event. If no event mapping is found,
   * then the methods return svtkWidgetEvent::NoEvent or a nullptr string.
   */
  unsigned long GetTranslation(unsigned long SVTKEvent);
  const char* GetTranslation(const char* SVTKEvent);
  unsigned long GetTranslation(
    unsigned long SVTKEvent, int modifier, char keyCode, int repeatCount, const char* keySym);
  unsigned long GetTranslation(unsigned long SVTKEvent, svtkEventData* edata);
  unsigned long GetTranslation(svtkEvent* SVTKEvent);
  //@}

  //@{
  /**
   * Remove translations for a binding.
   * Returns the number of translations removed.
   */
  int RemoveTranslation(
    unsigned long SVTKEvent, int modifier, char keyCode, int repeatCount, const char* keySym);
  int RemoveTranslation(svtkEvent* e);
  int RemoveTranslation(svtkEventData* e);
  int RemoveTranslation(unsigned long SVTKEvent);
  int RemoveTranslation(const char* SVTKEvent);
  //@}

  /**
   * Clear all events from the translator (i.e., no events will be
   * translated).
   */
  void ClearEvents();

  //@{
  /**
   * Add the events in the current translation table to the interactor.
   */
  void AddEventsToParent(svtkAbstractWidget*, svtkCallbackCommand*, float priority);
  void AddEventsToInteractor(svtkRenderWindowInteractor*, svtkCallbackCommand*, float priority);
  //@}

protected:
  // Constructors/destructors made public for widgets to use
  svtkWidgetEventTranslator();
  ~svtkWidgetEventTranslator() override;

  // Map SVTK events to widget events
  svtkEventMap* EventMap;

  // Used for performance reasons to avoid object construction/deletion
  svtkEvent* Event;

private:
  svtkWidgetEventTranslator(const svtkWidgetEventTranslator&) = delete;
  void operator=(const svtkWidgetEventTranslator&) = delete;
};

#endif /* svtkWidgetEventTranslator_h */
