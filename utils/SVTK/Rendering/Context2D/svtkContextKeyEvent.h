/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextScene.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextKeyEvent
 * @brief   data structure to represent key events.
 *
 *
 * Provides a convenient data structure to represent key events in the
 * svtkContextScene. Passed to svtkAbstractContextItem objects.
 */

#ifndef svtkContextKeyEvent_h
#define svtkContextKeyEvent_h

#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkVector.h"                   // For svtkVector2i
#include "svtkWeakPointer.h"              // For svtkWeakPointer

class svtkRenderWindowInteractor;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextKeyEvent
{
public:
  svtkContextKeyEvent();

  /**
   * Set the interactor for the key event.
   */
  void SetInteractor(svtkRenderWindowInteractor* interactor);

  /**
   * Get the interactor for the key event. This can be null, and is provided
   * only for convenience.
   */
  svtkRenderWindowInteractor* GetInteractor() const;

  /**
   * Set the position of the mouse when the key was pressed.
   */
  void SetPosition(const svtkVector2i& position) { this->Position = position; }

  /**
   * Get the position of the mouse when the key was pressed.
   */
  svtkVector2i GetPosition() const { return this->Position; }

  char GetKeyCode() const;

protected:
  svtkWeakPointer<svtkRenderWindowInteractor> Interactor;
  svtkVector2i Position;
};

#endif // svtkContextKeyEvent_h
// SVTK-HeaderTest-Exclude: svtkContextKeyEvent.h
