/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleRubberBandZoom.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleRubberBandZoom
 * @brief   zoom in by amount indicated by rubber band box
 *
 * This interactor style allows the user to draw a rectangle in the render
 * window using the left mouse button.  When the mouse button is released,
 * the current camera zooms by an amount determined from the shorter side of
 * the drawn rectangle.
 */

#ifndef svtkInteractorStyleRubberBandZoom_h
#define svtkInteractorStyleRubberBandZoom_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyle.h"
#include "svtkRect.h" // for svtkRecti

class svtkUnsignedCharArray;

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleRubberBandZoom : public svtkInteractorStyle
{
public:
  static svtkInteractorStyleRubberBandZoom* New();
  svtkTypeMacro(svtkInteractorStyleRubberBandZoom, svtkInteractorStyle);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * When set to true (default, false), the interactor will lock the rendered box to the
   * viewport's aspect ratio.
   */
  svtkSetMacro(LockAspectToViewport, bool);
  svtkGetMacro(LockAspectToViewport, bool);
  svtkBooleanMacro(LockAspectToViewport, bool);
  //@}

  //@{
  /**
   * When set to true (default, false), the position where the user starts the
   * interaction is treated as the center of the box rather that one of the
   * corners of the box.
   *
   * During interaction, modifier keys `Shift` or `Control` can be used to toggle
   * this flag temporarily. In other words, if `Shift` or `Control` key is pressed,
   * this class will act as if CenterAtStartPosition was opposite of what it is
   * set to.
   */
  svtkSetMacro(CenterAtStartPosition, bool);
  svtkGetMacro(CenterAtStartPosition, bool);
  svtkBooleanMacro(CenterAtStartPosition, bool);
  //@}

  //@{
  /**
   * If camera is in perspective projection mode, this interactor style uses
   * svtkCamera::Dolly to dolly the camera ahead for zooming. However, that can
   * have unintended consequences such as the camera entering into the data.
   * Another option is to use svtkCamera::Zoom instead. In that case, the camera
   * position is left unchanged, instead the focal point is changed to the
   * center of the target box and then the view angle is changed to zoom in.
   * To use this approach, set this parameter to false (default, true).
   */
  svtkSetMacro(UseDollyForPerspectiveProjection, bool);
  svtkGetMacro(UseDollyForPerspectiveProjection, bool);
  svtkBooleanMacro(UseDollyForPerspectiveProjection, bool);
  //@}

  //@{
  /**
   * Event bindings
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  //@}

protected:
  svtkInteractorStyleRubberBandZoom();
  ~svtkInteractorStyleRubberBandZoom() override;

  void Zoom() override;

  int StartPosition[2];
  int EndPosition[2];
  int Moving;
  bool LockAspectToViewport;
  bool CenterAtStartPosition;
  bool UseDollyForPerspectiveProjection;
  svtkUnsignedCharArray* PixelArray;

private:
  svtkInteractorStyleRubberBandZoom(const svtkInteractorStyleRubberBandZoom&) = delete;
  void operator=(const svtkInteractorStyleRubberBandZoom&) = delete;

  /**
   * Adjust the box based on this->LockAspectToViewport and
   * this->CenterAtStartPosition state. This may modify startPosition,
   * endPosition or both.
   */
  void AdjustBox(int startPosition[2], int endPosition[2]) const;

  void ZoomTraditional(const svtkRecti& box);
  void ZoomPerspectiveProjectionUsingViewAngle(const svtkRecti& box);
};

#endif
