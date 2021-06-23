/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebInteractionEvent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWebInteractionEvent
 *
 *
 */

#ifndef svtkWebInteractionEvent_h
#define svtkWebInteractionEvent_h

#include "svtkObject.h"
#include "svtkWebCoreModule.h" // needed for exports

class SVTKWEBCORE_EXPORT svtkWebInteractionEvent : public svtkObject
{
public:
  static svtkWebInteractionEvent* New();
  svtkTypeMacro(svtkWebInteractionEvent, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum MouseButton
  {
    LEFT_BUTTON = 0x01,
    MIDDLE_BUTTON = 0x02,
    RIGHT_BUTTON = 0x04
  };

  enum ModifierKeys
  {
    SHIFT_KEY = 0x01,
    CTRL_KEY = 0x02,
    ALT_KEY = 0x04,
    META_KEY = 0x08
  };

  //@{
  /**
   * Set/Get the mouse buttons state.
   */
  svtkSetMacro(Buttons, unsigned int);
  svtkGetMacro(Buttons, unsigned int);
  //@}

  //@{
  /**
   * Set/Get modifier state.
   */
  svtkSetMacro(Modifiers, unsigned int);
  svtkGetMacro(Modifiers, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the chart code.
   */
  svtkSetMacro(KeyCode, char);
  svtkGetMacro(KeyCode, char);
  //@}

  //@{
  /**
   * Set/Get event position.
   */
  svtkSetMacro(X, double);
  svtkGetMacro(X, double);
  svtkSetMacro(Y, double);
  svtkGetMacro(Y, double);
  svtkSetMacro(Scroll, double);
  svtkGetMacro(Scroll, double);
  //@}

  // Handle double click
  svtkSetMacro(RepeatCount, int);
  svtkGetMacro(RepeatCount, int);

protected:
  svtkWebInteractionEvent();
  ~svtkWebInteractionEvent() override;

  unsigned int Buttons;
  unsigned int Modifiers;
  char KeyCode;
  double X;
  double Y;
  double Scroll;
  int RepeatCount;

private:
  svtkWebInteractionEvent(const svtkWebInteractionEvent&) = delete;
  void operator=(const svtkWebInteractionEvent&) = delete;
};

#endif
