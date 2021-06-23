/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleRubberBandPick.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleRubberBandPick
 * @brief   Like TrackBallCamera, but this can pick props underneath a rubber band selection
 * rectangle.
 *
 *
 * This interactor style allows the user to draw a rectangle in the render
 * window by hitting 'r' and then using the left mouse button.
 * When the mouse button is released, the attached picker operates on the pixel
 * in the center of the selection rectangle. If the picker happens to be a
 * svtkAreaPicker it will operate on the entire selection rectangle.
 * When the 'p' key is hit the above pick operation occurs on a 1x1 rectangle.
 * In other respects it behaves the same as its parent class.
 *
 * @sa
 * svtkAreaPicker
 */

#ifndef svtkInteractorStyleRubberBandPick_h
#define svtkInteractorStyleRubberBandPick_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyleTrackballCamera.h"

class svtkUnsignedCharArray;

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleRubberBandPick
  : public svtkInteractorStyleTrackballCamera
{
public:
  static svtkInteractorStyleRubberBandPick* New();
  svtkTypeMacro(svtkInteractorStyleRubberBandPick, svtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void StartSelect();

  //@{
  /**
   * Event bindings
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnChar() override;
  //@}

protected:
  svtkInteractorStyleRubberBandPick();
  ~svtkInteractorStyleRubberBandPick() override;

  virtual void Pick();
  void RedrawRubberBand();

  int StartPosition[2];
  int EndPosition[2];

  int Moving;

  svtkUnsignedCharArray* PixelArray;

  int CurrentMode;

private:
  svtkInteractorStyleRubberBandPick(const svtkInteractorStyleRubberBandPick&) = delete;
  void operator=(const svtkInteractorStyleRubberBandPick&) = delete;
};

#endif
