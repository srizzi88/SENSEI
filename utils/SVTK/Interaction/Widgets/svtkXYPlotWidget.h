/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXYPlotWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXYPlotWidget
 * @brief   2D widget for manipulating a XY plot
 *
 * This class provides support for interactively manipulating the position,
 * size, and orientation of a XY Plot. It listens to Left mouse events and
 * mouse movement. It will change the cursor shape based on its location. If
 * the cursor is over an edge of thea XY plot it will change the cursor shape
 * to a resize edge shape. If the position of a XY plot is moved to be close to
 * the center of one of the four edges of the viewport, then the XY plot will
 * change its orientation to align with that edge. This orientation is sticky
 * in that it will stay that orientation until the position is moved close to
 * another edge.
 *
 * @sa
 * svtkInteractorObserver
 */

#ifndef svtkXYPlotWidget_h
#define svtkXYPlotWidget_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkInteractorObserver.h"
class svtkXYPlotActor;

class SVTKINTERACTIONWIDGETS_EXPORT svtkXYPlotWidget : public svtkInteractorObserver
{
public:
  static svtkXYPlotWidget* New();
  svtkTypeMacro(svtkXYPlotWidget, svtkInteractorObserver);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the XY plot used by this Widget. One is created automatically.
   */
  virtual void SetXYPlotActor(svtkXYPlotActor*);
  svtkGetObjectMacro(XYPlotActor, svtkXYPlotActor);
  //@}

  /**
   * Methods for turning the interactor observer on and off.
   */
  void SetEnabled(int) override;

protected:
  svtkXYPlotWidget();
  ~svtkXYPlotWidget() override;

  // the actor that is used
  svtkXYPlotActor* XYPlotActor;

  // handles the events
  static void ProcessEvents(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown();
  void OnLeftButtonUp();
  void OnMouseMove();

  // used to compute relative movements
  float StartPosition[2];

  // Manage the state of the widget
  int State;
  enum WidgetState
  {
    Moving = 0,
    AdjustingP1,
    AdjustingP2,
    AdjustingP3,
    AdjustingP4,
    AdjustingE1,
    AdjustingE2,
    AdjustingE3,
    AdjustingE4,
    Inside,
    Outside
  };

  // use to determine what state the mouse is over, edge1 p1, etc.
  // returns a state from the WidgetState enum above
  int ComputeStateBasedOnPosition(int X, int Y, int* pos1, int* pos2);

  // set the cursor to the correct shape based on State argument
  void SetCursor(int State);

private:
  svtkXYPlotWidget(const svtkXYPlotWidget&) = delete;
  void operator=(const svtkXYPlotWidget&) = delete;
};

#endif
