/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTensorProbeRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTensorProbeRepresentation
 * @brief   Abstract class that serves as a representation for svtkTensorProbeWidget
 *
 * The class serves as an abstract geometrical representation for the
 * svtkTensorProbeWidget. It is left to the concrete implementation to render
 * the tensors as it desires. For instance,
 * svtkEllipsoidTensorProbeRepresentation renders the tensors as ellipsoids.
 *
 * @sa
 * svtkTensorProbeWidget
 */

#ifndef svtkTensorProbeRepresentation_h
#define svtkTensorProbeRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkGenericCell;

class SVTKINTERACTIONWIDGETS_EXPORT svtkTensorProbeRepresentation : public svtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkTensorProbeRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * See svtkWidgetRepresentation for details.
   */
  void BuildRepresentation() override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  //@}

  //@{
  /**
   * Set the position of the Tensor probe.
   */
  svtkSetVector3Macro(ProbePosition, double);
  svtkGetVector3Macro(ProbePosition, double);
  svtkSetMacro(ProbeCellId, svtkIdType);
  svtkGetMacro(ProbeCellId, svtkIdType);
  //@}

  /**
   * Set the trajectory that we are trying to probe tensors on
   */
  virtual void SetTrajectory(svtkPolyData*);

  /**
   * Set the probe position to a reasonable location on the trajectory.
   */
  void Initialize();

  /**
   * This method is invoked by the widget during user interaction.
   * Can we pick the tensor glyph at the current cursor pos
   */
  virtual int SelectProbe(int pos[2]) = 0;

  /**
   * INTERNAL - Do not use
   * This method is invoked by the widget during user interaction.
   * Move probe based on the position and the motion vector.
   */
  virtual int Move(double motionVector[2]);

  //@{
  /**
   * See svtkProp for details.
   */
  void GetActors(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  //@}

protected:
  svtkTensorProbeRepresentation();
  ~svtkTensorProbeRepresentation() override;

  void FindClosestPointOnPolyline(
    double displayPos[2], double closestWorldPos[3], svtkIdType& cellId, int maxSpeed = 10);

  svtkActor* TrajectoryActor;
  svtkPolyDataMapper* TrajectoryMapper;
  svtkPolyData* Trajectory;
  double ProbePosition[3];
  svtkIdType ProbeCellId;

private:
  svtkTensorProbeRepresentation(const svtkTensorProbeRepresentation&) = delete;
  void operator=(const svtkTensorProbeRepresentation&) = delete;
};

#endif
