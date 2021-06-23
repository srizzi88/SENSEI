/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWidgetCallbackMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWidgetCallbackMapper
 * @brief   map widget events into callbacks
 *
 * svtkWidgetCallbackMapper maps widget events (defined in svtkWidgetEvent.h)
 * into static class methods, and provides facilities to invoke the methods.
 * This class is templated and meant to be used as an internal helper class
 * by the widget classes. The class works in combination with the class
 * svtkWidgetEventTranslator, which translates SVTK events into widget events.
 *
 * @sa
 * svtkWidgetEvent svtkWidgetEventTranslator
 */

#ifndef svtkWidgetCallbackMapper_h
#define svtkWidgetCallbackMapper_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkObject.h"

class svtkWidgetEvent;
class svtkAbstractWidget;
class svtkWidgetEventTranslator;
class svtkCallbackMap; // PIMPL encapsulation of STL map
class svtkEventData;

class SVTKINTERACTIONWIDGETS_EXPORT svtkWidgetCallbackMapper : public svtkObject
{
public:
  /**
   * Instantiate the class.
   */
  static svtkWidgetCallbackMapper* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkWidgetCallbackMapper, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the svtkWidgetEventTranslator to coordinate with.
   */
  void SetEventTranslator(svtkWidgetEventTranslator* t);
  svtkGetObjectMacro(EventTranslator, svtkWidgetEventTranslator);
  //@}

  /**
   * Convenient typedef for working with callbacks.
   */
  typedef void (*CallbackType)(svtkAbstractWidget*);

  //@{
  /**
   * This class works with the class svtkWidgetEventTranslator to set up the
   * initial coorespondence between SVTK events, widget events, and callbacks.
   * Different flavors of the SetCallbackMethod() are available depending on
   * what sort of modifiers are to be associated with a particular event.
   * Typically the widgets should use this method to set up their event
   * callbacks. If modifiers are not provided (i.e., the SVTKEvent is a
   * unsigned long eventId) then modifiers are ignored. Otherwise, a svtkEvent
   * instance is used to fully quality the events.
   */
  void SetCallbackMethod(
    unsigned long SVTKEvent, unsigned long widgetEvent, svtkAbstractWidget* w, CallbackType f);
  void SetCallbackMethod(unsigned long SVTKEvent, int modifiers, char keyCode, int repeatCount,
    const char* keySym, unsigned long widgetEvent, svtkAbstractWidget* w, CallbackType f);
  void SetCallbackMethod(unsigned long SVTKEvent, svtkEventData* ed, unsigned long widgetEvent,
    svtkAbstractWidget* w, CallbackType f);
  // void SetCallbackMethod(svtkWidgetEvent *svtkEvent, unsigned long widgetEvent,
  //                       svtkAbstractWidget *w, CallbackType f);
  //@}

  /**
   * This method invokes the callback given a widget event. A non-zero value
   * is returned if the listed event is registered.
   */
  void InvokeCallback(unsigned long widgetEvent);

protected:
  svtkWidgetCallbackMapper();
  ~svtkWidgetCallbackMapper() override;

  // Translates SVTK events into widget events
  svtkWidgetEventTranslator* EventTranslator;

  // Invoke the method associated with a particular widget event
  svtkCallbackMap* CallbackMap;

  /**
   * This method is used to assign a callback (implemented as a static class
   * method) to a particular widget event. This is an internal method used by
   * widgets to map widget events into invocations of class methods.
   */
  void SetCallbackMethod(unsigned long widgetEvent, svtkAbstractWidget* w, CallbackType f);

private:
  svtkWidgetCallbackMapper(const svtkWidgetCallbackMapper&) = delete;
  void operator=(const svtkWidgetCallbackMapper&) = delete;
};

#endif /* svtkWidgetCallbackMapper_h */
