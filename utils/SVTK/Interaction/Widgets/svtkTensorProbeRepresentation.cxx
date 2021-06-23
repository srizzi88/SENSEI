/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTensorProbeRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTensorProbeRepresentation.h"
#include "svtkActor.h"
#include "svtkCellArray.h"
#include "svtkCoordinate.h"
#include "svtkGenericCell.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

//----------------------------------------------------------------------
svtkTensorProbeRepresentation::svtkTensorProbeRepresentation()
{
  this->Trajectory = nullptr;
  this->TrajectoryMapper = svtkPolyDataMapper::New();
  this->TrajectoryActor = svtkActor::New();
  this->TrajectoryActor->SetMapper(this->TrajectoryMapper);
  this->ProbePosition[0] = SVTK_DOUBLE_MAX;
  this->ProbePosition[1] = SVTK_DOUBLE_MAX;
  this->ProbePosition[2] = SVTK_DOUBLE_MAX;
  this->ProbeCellId = -1;
}

//----------------------------------------------------------------------
svtkTensorProbeRepresentation::~svtkTensorProbeRepresentation()
{
  this->SetTrajectory(nullptr);
  this->TrajectoryMapper->Delete();
  this->TrajectoryActor->Delete();
}

//----------------------------------------------------------------------
void svtkTensorProbeRepresentation::SetTrajectory(svtkPolyData* args)
{
  if (this->Trajectory != args)
  {
    svtkPolyData* tempSGMacroVar = this->Trajectory;
    this->Trajectory = args;
    if (this->Trajectory != nullptr)
    {
      this->Trajectory->Register(this);
    }
    if (tempSGMacroVar != nullptr)
    {
      tempSGMacroVar->UnRegister(this);
    }
    this->TrajectoryMapper->SetInputData(this->Trajectory);
    this->Modified();
  }
}

//----------------------------------------------------------------------
int svtkTensorProbeRepresentation::Move(double motionVector[2])
{
  if (motionVector[0] == 0.0 && motionVector[1] == 0.0)
  {
    return 0;
  }

  svtkIdType cellId;
  double displayPos[4], p2[3], p[4];

  this->GetProbePosition(p);
  cellId = this->GetProbeCellId();

  p[3] = 1.0;
  this->Renderer->SetWorldPoint(p);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(displayPos);

  displayPos[0] += motionVector[0];
  displayPos[1] += motionVector[1];

  this->FindClosestPointOnPolyline(displayPos, p2, cellId);

  if (svtkMath::Distance2BetweenPoints(p, p2) > 0.0)
  {
    this->SetProbePosition(p2);
    this->SetProbeCellId(cellId);
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------
void svtkTensorProbeRepresentation ::FindClosestPointOnPolyline(
  double displayPos[2], double closestWorldPos[3], svtkIdType& cellId, int maxSpeed)
{
  svtkIdType npts = 0;
  const svtkIdType* ptIds = nullptr;

  this->Trajectory->GetLines()->GetCellAtId(0, npts, ptIds);
  svtkPoints* points = this->Trajectory->GetPoints();

  svtkIdType minCellId = svtkMath::Max(this->ProbeCellId - maxSpeed, static_cast<svtkIdType>(0));
  svtkIdType maxCellId = svtkMath::Min(this->ProbeCellId + maxSpeed, npts - 1);

  double closestT = 0.0, closestDist = SVTK_DOUBLE_MAX, pprev[3] = { 0.0, 0.0, 0.0 }, t,
         closestPt[3], dist, x[3] = { displayPos[0], displayPos[1], 0.0 };

  for (svtkIdType id = minCellId; id <= maxCellId; id++)
  {

    double p[4];
    points->GetPoint(id, p);
    p[3] = 1.0;

    this->Renderer->SetWorldPoint(p);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(p);

    if (id != minCellId)
    {
      p[2] = 0.0;
      dist = svtkLine::DistanceToLine(x, p, pprev, t, closestPt);
      if (t < 0.0 || t > 1.0)
      {
        double d1 = svtkMath::Distance2BetweenPoints(x, pprev);
        double d2 = svtkMath::Distance2BetweenPoints(x, p);
        if (d1 < d2)
        {
          t = 1.0;
          dist = d1;
        }
        else
        {
          t = 0.0;
          dist = d2;
        }
      }

      if (dist < closestDist)
      {
        closestDist = dist;
        closestT = t;
        closestPt[0] = p[0];
        closestPt[1] = p[1];
        closestPt[2] = p[2];
        cellId = id - 1;
      }
    }

    pprev[0] = p[0];
    pprev[1] = p[1];
  }

  double p1[3], p2[3];
  points->GetPoint(cellId, p1);
  points->GetPoint(cellId + 1, p2);

  closestWorldPos[0] = closestT * p1[0] + (1 - closestT) * p2[0];
  closestWorldPos[1] = closestT * p1[1] + (1 - closestT) * p2[1];
  closestWorldPos[2] = closestT * p1[2] + (1 - closestT) * p2[2];
}

//----------------------------------------------------------------------
// Set the probe position as the one closest to the center
void svtkTensorProbeRepresentation::Initialize()
{
  if (this->ProbePosition[0] == SVTK_DOUBLE_MAX && this->Trajectory)
  {
    double p[3];
    svtkPoints* points = this->Trajectory->GetPoints();
    points->GetPoint(0, p);

    this->SetProbeCellId(0);
    this->SetProbePosition(p);
  }
}

//----------------------------------------------------------------------
int svtkTensorProbeRepresentation ::RenderOpaqueGeometry(svtkViewport* viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();

  int count = 0;
  count += this->TrajectoryActor->RenderOpaqueGeometry(viewport);
  return count;
}

//----------------------------------------------------------------------
void svtkTensorProbeRepresentation::BuildRepresentation()
{
  this->Initialize();
}

//----------------------------------------------------------------------
void svtkTensorProbeRepresentation::GetActors(svtkPropCollection* pc)
{
  this->TrajectoryActor->GetActors(pc);
}

//----------------------------------------------------------------------
void svtkTensorProbeRepresentation::ReleaseGraphicsResources(svtkWindow* win)
{
  this->TrajectoryActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
void svtkTensorProbeRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TrajectoryActor: " << this->TrajectoryActor << endl;
  os << indent << "TrajectoryMapper: " << this->TrajectoryMapper << endl;
  os << indent << "Trajectory: " << this->Trajectory << endl;
  os << indent << "ProbePosition: (" << this->ProbePosition[0] << "," << this->ProbePosition[1]
     << "," << this->ProbePosition[2] << ")" << endl;
  os << indent << "ProbeCellId: " << this->ProbeCellId << endl;
}
