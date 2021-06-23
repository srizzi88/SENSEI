/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelCoordinatesInteractorStyle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkParallelCoordinatesInteractorStyle
 * @brief   interactive manipulation of the camera specialized for parallel coordinates
 *
 * svtkParallelCoordinatesInteractorStyle allows the user to interactively manipulate
 * (rotate, pan, zoom etc.) the camera.
 * Several events are overloaded from its superclass
 * svtkInteractorStyleTrackballCamera, hence the mouse bindings are different.
 * (The bindings keep the camera's view plane normal perpendicular to the x-y plane.)
 * In summary, the mouse events are as follows:
 * + Left Mouse button triggers window level events
 * + CTRL Left Mouse spins the camera around its view plane normal
 * + SHIFT Left Mouse pans the camera
 * + CTRL SHIFT Left Mouse dollys (a positional zoom) the camera
 * + Middle mouse button pans the camera
 * + Right mouse button dollys the camera.
 * + SHIFT Right Mouse triggers pick events
 *
 * Note that the renderer's actors are not moved; instead the camera is moved.
 *
 * @sa
 * svtkInteractorStyle svtkInteractorStyleTrackballActor
 * svtkInteractorStyleJoystickCamera svtkInteractorStyleJoystickActor
 */

#ifndef svtkParallelCoordinatesInteractorStyle_h
#define svtkParallelCoordinatesInteractorStyle_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyleTrackballCamera.h"

class svtkViewport;

class SVTKINTERACTIONSTYLE_EXPORT svtkParallelCoordinatesInteractorStyle
  : public svtkInteractorStyleTrackballCamera
{
public:
  static svtkParallelCoordinatesInteractorStyle* New();
  svtkTypeMacro(svtkParallelCoordinatesInteractorStyle, svtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    INTERACT_HOVER = 0,
    INTERACT_INSPECT,
    INTERACT_ZOOM,
    INTERACT_PAN
  };

  //@{
  /**
   * Get the cursor positions in pixel coords
   */
  svtkGetVector2Macro(CursorStartPosition, int);
  svtkGetVector2Macro(CursorCurrentPosition, int);
  svtkGetVector2Macro(CursorLastPosition, int);
  //@}

  //@{
  /**
   * Get the cursor positions in a given coordinate system
   */
  void GetCursorStartPosition(svtkViewport* viewport, double pos[2]);
  void GetCursorCurrentPosition(svtkViewport* viewport, double pos[2]);
  void GetCursorLastPosition(svtkViewport* viewport, double pos[2]);
  //@}

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnLeave() override;
  //@}

  //@{
  virtual void StartInspect(int x, int y);
  virtual void Inspect(int x, int y);
  virtual void EndInspect();
  //@}

  //@{
  void StartZoom() override;
  void Zoom() override;
  void EndZoom() override;
  //@}

  //@{
  void StartPan() override;
  void Pan() override;
  void EndPan() override;
  //@}

  /**
   * Override the "fly-to" (f keypress) for images.
   */
  void OnChar() override;

protected:
  svtkParallelCoordinatesInteractorStyle();
  ~svtkParallelCoordinatesInteractorStyle() override;

  int CursorStartPosition[2];
  int CursorCurrentPosition[2];
  int CursorLastPosition[2];

private:
  svtkParallelCoordinatesInteractorStyle(const svtkParallelCoordinatesInteractorStyle&) = delete;
  void operator=(const svtkParallelCoordinatesInteractorStyle&) = delete;
};

#endif
