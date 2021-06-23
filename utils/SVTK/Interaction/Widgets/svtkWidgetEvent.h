/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWidgetEvent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWidgetEvent
 * @brief   define widget events
 *
 * svtkWidgetEvent defines widget events. These events are processed by
 * subclasses of svtkInteractorObserver.
 */

#ifndef svtkWidgetEvent_h
#define svtkWidgetEvent_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkObject.h"

class SVTKINTERACTIONWIDGETS_EXPORT svtkWidgetEvent : public svtkObject
{
public:
  /**
   * The object factory constructor.
   */
  static svtkWidgetEvent* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkWidgetEvent, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * All the widget events are defined here.
   */
  enum WidgetEventIds
  {
    NoEvent = 0,
    Select,
    EndSelect,
    Delete,
    Translate,
    EndTranslate,
    Scale,
    EndScale,
    Resize,
    EndResize,
    Rotate,
    EndRotate,
    Move,
    SizeHandles,
    AddPoint,
    AddFinalPoint,
    Completed,
    TimedOut,
    ModifyEvent,
    Reset,
    Up,
    Down,
    Left,
    Right,
    Select3D,
    EndSelect3D,
    Move3D,
    AddPoint3D,
    AddFinalPoint3D
  };

  //@{
  /**
   * Convenience methods for translating between event names and event ids.
   */
  static const char* GetStringFromEventId(unsigned long event);
  static unsigned long GetEventIdFromString(const char* event);
  //@}

protected:
  svtkWidgetEvent() {}
  ~svtkWidgetEvent() override {}

private:
  svtkWidgetEvent(const svtkWidgetEvent&) = delete;
  void operator=(const svtkWidgetEvent&) = delete;
};

#endif
