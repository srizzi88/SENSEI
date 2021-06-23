/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextMouseEvent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextMouseEvent
 * @brief   data structure to represent mouse events.
 *
 *
 * Provides a convenient data structure to represent mouse events in the
 * svtkContextScene. Passed to svtkAbstractContextItem objects.
 */

#ifndef svtkContextMouseEvent_h
#define svtkContextMouseEvent_h

#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkVector.h"                   // Needed for svtkVector2f and svtkVector2i
#include "svtkWin32Header.h"              // For export macros.

class svtkRenderWindowInteractor;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextMouseEvent
{
public:
  /**
   * Enumeration of mouse buttons.
   */
  enum
  {
    NO_BUTTON = 0,
    LEFT_BUTTON = 1,
    MIDDLE_BUTTON = 2,
    RIGHT_BUTTON = 4
  };

  /**
   * Enumeration of modifier keys.
   */
  enum
  {
    NO_MODIFIER = 0,
    ALT_MODIFIER = 1,
    SHIFT_MODIFIER = 2,
    CONTROL_MODIFIER = 4
  };

  svtkContextMouseEvent() {}

  /**
   * Set the interactor for the mouse event.
   */
  void SetInteractor(svtkRenderWindowInteractor* interactor) { this->Interactor = interactor; }

  /**
   * Get the interactor for the mouse event. This can be null, and is provided
   * only for convenience.
   */
  svtkRenderWindowInteractor* GetInteractor() const { return this->Interactor; }

  /**
   * Set/get the position of the mouse in the item's coordinates.
   */
  void SetPos(const svtkVector2f& pos) { this->Pos = pos; }
  svtkVector2f GetPos() const { return this->Pos; }

  /**
   * Set/get the position of the mouse in scene coordinates.
   */
  void SetScenePos(const svtkVector2f& pos) { this->ScenePos = pos; }
  svtkVector2f GetScenePos() const { return this->ScenePos; }

  /**
   * Set/get the position of the mouse in screen coordinates.
   */
  void SetScreenPos(const svtkVector2i& pos) { this->ScreenPos = pos; }
  svtkVector2i GetScreenPos() const { return this->ScreenPos; }

  /**
   * Set/get the position of the mouse in the item's coordinates.
   */
  void SetLastPos(const svtkVector2f& pos) { this->LastPos = pos; }
  svtkVector2f GetLastPos() const { return this->LastPos; }

  /**
   * Set/get the position of the mouse in scene coordinates.
   */
  void SetLastScenePos(const svtkVector2f& pos) { this->LastScenePos = pos; }
  svtkVector2f GetLastScenePos() const { return this->LastScenePos; }

  /**
   * Set/get the position of the mouse in screen coordinates.
   */
  void SetLastScreenPos(const svtkVector2i& pos) { this->LastScreenPos = pos; }
  svtkVector2i GetLastScreenPos() const { return this->LastScreenPos; }

  /**
   * Set/get the mouse button that caused the event, with possible values being
   * NO_BUTTON, LEFT_BUTTON, MIDDLE_BUTTON and RIGHT_BUTTON.
   */
  void SetButton(int button) { this->Button = button; }
  int GetButton() const { return this->Button; }

  /**
   * Return the modifier keys, if any, ORed together. Valid modifier enum values
   * are NO_MODIFIER, ALT_MODIFIER, SHIFT_MODIFIER and/or CONTROL_MODIFIER.
   */
  int GetModifiers() const;

protected:
  /**
   * Position of the mouse in item coordinate system.
   */
  svtkVector2f Pos;

  /**
   * Position of the mouse the scene coordinate system.
   */
  svtkVector2f ScenePos;

  /**
   * Position of the mouse in screen coordinates
   */
  svtkVector2i ScreenPos;

  /**
   * `Pos' at the previous mouse event.
   */
  svtkVector2f LastPos;

  /**
   * `ScenePos'at the previous mouse event.
   */
  svtkVector2f LastScenePos;

  /**
   * `ScreenPos' at the previous mouse event.
   */
  svtkVector2i LastScreenPos;

  /**
   * Mouse button that caused the event, using the anonymous enumeration.
   */
  int Button;

protected:
  svtkRenderWindowInteractor* Interactor;
};

#endif // svtkContextMouseEvent_h
// SVTK-HeaderTest-Exclude: svtkContextMouseEvent.h
