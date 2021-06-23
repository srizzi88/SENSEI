/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleUser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkInteractorStyleUser
 * @brief   provides customizable interaction routines
 *
 *
 * The most common way to customize user interaction is to write a subclass
 * of svtkInteractorStyle: svtkInteractorStyleUser allows you to customize
 * the interaction to without subclassing svtkInteractorStyle.  This is
 * particularly useful for setting up custom interaction modes in
 * scripting languages such as Python.  This class allows you
 * to hook into the MouseMove, ButtonPress/Release, KeyPress/Release,
 * etc. events.  If you want to hook into just a single mouse button,
 * but leave the interaction modes for the others unchanged, you
 * must use e.g. SetMiddleButtonPressMethod() instead of the more
 * general SetButtonPressMethod().
 */

#ifndef svtkInteractorStyleUser_h
#define svtkInteractorStyleUser_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyle.h"

// new motion flag
#define SVTKIS_USERINTERACTION 8

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleUser : public svtkInteractorStyle
{
public:
  static svtkInteractorStyleUser* New();
  svtkTypeMacro(svtkInteractorStyleUser, svtkInteractorStyle);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the most recent mouse position during mouse motion.
   * In your user interaction method, you must use this to track
   * the mouse movement.  Do not use GetEventPosition(), which records
   * the last position where a mouse button was pressed.
   */
  svtkGetVector2Macro(LastPos, int);
  //@}

  //@{
  /**
   * Get the previous mouse position during mouse motion, or after
   * a key press.  This can be used to calculate the relative
   * displacement of the mouse.
   */
  svtkGetVector2Macro(OldPos, int);
  //@}

  //@{
  /**
   * Test whether modifiers were held down when mouse button or key
   * was pressed.
   */
  svtkGetMacro(ShiftKey, int);
  svtkGetMacro(CtrlKey, int);
  //@}

  //@{
  /**
   * Get the character for a Char event.
   */
  svtkGetMacro(Char, int);
  //@}

  //@{
  /**
   * Get the KeySym (in the same format as svtkRenderWindowInteractor KeySyms)
   * for a KeyPress or KeyRelease method.
   */
  svtkGetStringMacro(KeySym);
  //@}

  //@{
  /**
   * Get the mouse button that was last pressed inside the window
   * (returns zero when the button is released).
   */
  svtkGetMacro(Button, int);
  //@}

  //@{
  /**
   * Generic event bindings
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnMouseWheelForward() override;
  void OnMouseWheelBackward() override;
  //@}

  //@{
  /**
   * Keyboard functions
   */
  void OnChar() override;
  void OnKeyPress() override;
  void OnKeyRelease() override;
  //@}

  //@{
  /**
   * These are more esoteric events, but are useful in some cases.
   */
  void OnExpose() override;
  void OnConfigure() override;
  void OnEnter() override;
  void OnLeave() override;
  //@}

  void OnTimer() override;

protected:
  svtkInteractorStyleUser();
  ~svtkInteractorStyleUser() override;

  int LastPos[2];
  int OldPos[2];

  int ShiftKey;
  int CtrlKey;
  int Char;
  char* KeySym;
  int Button;

private:
  svtkInteractorStyleUser(const svtkInteractorStyleUser&) = delete;
  void operator=(const svtkInteractorStyleUser&) = delete;
};

#endif
