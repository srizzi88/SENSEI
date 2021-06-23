/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEvent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEvent
 * @brief   a complete specification of a SVTK event including all modifiers
 *
 * svtkEvent is a class that fully describes a SVTK event. It is used by the
 * widgets to help specify the mapping between SVTK events and widget events.
 */

#ifndef svtkEvent_h
#define svtkEvent_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkObject.h"

class svtkRenderWindowInteractor;

class SVTKINTERACTIONWIDGETS_EXPORT svtkEvent : public svtkObject
{
public:
  /**
   * The object factory constructor.
   */
  static svtkEvent* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkEvent, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Ways to specify modifiers to SVTK events. These can be logical OR'd to
   * produce combinations of modifiers.
   */
  enum EventModifiers
  {
    AnyModifier = -1,
    NoModifier = 0,
    ShiftModifier = 1,
    ControlModifier = 2,
    AltModifier = 4
  };

  //@{
  /**
   * Set the modifier for the event.
   */
  svtkSetMacro(EventId, unsigned long);
  svtkGetMacro(EventId, unsigned long);
  //@}

  //@{
  /**
   * Set the modifier for the event.
   */
  svtkSetMacro(Modifier, int);
  svtkGetMacro(Modifier, int);
  //@}

  //@{
  /**
   * Set the KeyCode for the event.
   */
  svtkSetMacro(KeyCode, char);
  svtkGetMacro(KeyCode, char);
  //@}

  //@{
  /**
   * Set the repease count for the event.
   */
  svtkSetMacro(RepeatCount, int);
  svtkGetMacro(RepeatCount, int);
  //@}

  //@{
  /**
   * Set the complex key symbol (compound key strokes) for the event.
   */
  svtkSetStringMacro(KeySym);
  svtkGetStringMacro(KeySym);
  //@}

  /**
   * Convenience method computes the event modifier from an interactor.
   */
  static int GetModifier(svtkRenderWindowInteractor*);

  /**
   * Used to compare whether two events are equal. Takes into account
   * the EventId as well as the various modifiers.
   */
  bool operator==(svtkEvent*);
  bool operator==(unsigned long SVTKEvent); // event with no modifiers

protected:
  svtkEvent();
  ~svtkEvent() override;

  unsigned long EventId;
  int Modifier;
  char KeyCode;
  int RepeatCount;
  char* KeySym;

private:
  svtkEvent(const svtkEvent&) = delete;
  void operator=(const svtkEvent&) = delete;
};

#endif
