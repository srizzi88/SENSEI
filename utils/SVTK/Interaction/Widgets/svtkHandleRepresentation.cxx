/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHandleRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHandleRepresentation.h"
#include "svtkCoordinate.h"
#include "svtkInteractorObserver.h"
#include "svtkObjectFactory.h"
#include "svtkPointPlacer.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

#include "assert.h"

svtkCxxSetObjectMacro(svtkHandleRepresentation, PointPlacer, svtkPointPlacer);

//----------------------------------------------------------------------
svtkHandleRepresentation::svtkHandleRepresentation()
{
  // Positions are maintained via a svtkCoordinate
  this->DisplayPosition = svtkCoordinate::New();
  this->DisplayPosition->SetCoordinateSystemToDisplay();

  this->WorldPosition = svtkCoordinate::New();
  this->WorldPosition->SetCoordinateSystemToWorld();

  this->InteractionState = svtkHandleRepresentation::Outside;
  this->Tolerance = 15;
  this->ActiveRepresentation = 0;
  this->Constrained = 0;
  this->PointPlacer = svtkPointPlacer::New();

  this->DisplayPositionTime.Modified();
  this->WorldPositionTime.Modified();

  this->TranslationAxis = Axis::NONE;
}

//----------------------------------------------------------------------
svtkHandleRepresentation::~svtkHandleRepresentation()
{
  this->DisplayPosition->Delete();
  this->WorldPosition->Delete();
  this->SetPointPlacer(nullptr);
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::SetDisplayPosition(double displyPos[3])
{
  if (this->Renderer && this->PointPlacer)
  {
    if (this->PointPlacer->ValidateDisplayPosition(this->Renderer, displyPos))
    {
      double worldPos[3], worldOrient[9];
      if (this->PointPlacer->ComputeWorldPosition(this->Renderer, displyPos, worldPos, worldOrient))
      {
        this->DisplayPosition->SetValue(displyPos);
        this->WorldPosition->SetValue(worldPos);
        this->DisplayPositionTime.Modified();
      }
    }
  }
  else
  {
    this->DisplayPosition->SetValue(displyPos);
    this->DisplayPositionTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::GetDisplayPosition(double pos[3])
{
  // The position is really represented in the world position; the display
  // position is a convenience to go back and forth between coordinate systems.
  // Also note that the window size may have changed, so it's important to
  // update the display position.
  if (this->Renderer &&
    (this->WorldPositionTime > this->DisplayPositionTime ||
      (this->Renderer->GetSVTKWindow() &&
        this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime)))
  {
    int* p = this->WorldPosition->GetComputedDisplayValue(this->Renderer);
    this->DisplayPosition->SetValue(p[0], p[1], p[2]);
  }
  this->DisplayPosition->GetValue(pos);
}

//----------------------------------------------------------------------
double* svtkHandleRepresentation::GetDisplayPosition()
{
  // The position is really represented in the world position; the display
  // position is a convenience to go back and forth between coordinate systems.
  // Also note that the window size may have changed, so it's important to
  // update the display position.
  if (this->Renderer &&
    (this->WorldPositionTime > this->DisplayPositionTime ||
      (this->Renderer->GetSVTKWindow() &&
        this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime)))
  {
    int* p = this->WorldPosition->GetComputedDisplayValue(this->Renderer);
    this->DisplayPosition->SetValue(p[0], p[1], p[2]);
  }
  return this->DisplayPosition->GetValue();
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::SetWorldPosition(double pos[3])
{
  if (this->Renderer && this->PointPlacer)
  {
    if (this->PointPlacer->ValidateWorldPosition(pos))
    {
      this->WorldPosition->SetValue(pos);
      this->WorldPositionTime.Modified();
    }
  }
  else
  {
    this->WorldPosition->SetValue(pos);
    this->WorldPositionTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::GetWorldPosition(double pos[3])
{
  this->WorldPosition->GetValue(pos);
}

//----------------------------------------------------------------------
double* svtkHandleRepresentation::GetWorldPosition()
{
  return this->WorldPosition->GetValue();
}

//----------------------------------------------------------------------
int svtkHandleRepresentation::CheckConstraint(
  svtkRenderer* svtkNotUsed(renderer), double svtkNotUsed(pos)[2])
{
  return 1;
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::SetRenderer(svtkRenderer* ren)
{
  this->DisplayPosition->SetViewport(ren);
  this->WorldPosition->SetViewport(ren);
  this->Superclass::SetRenderer(ren);

  // Okay this is weird. If a display position was set previously before
  // the renderer was specified, then the coordinate systems are not
  // synchronized.
  if (this->DisplayPositionTime > this->WorldPositionTime)
  {
    double p[3];
    this->DisplayPosition->GetValue(p);
    this->SetDisplayPosition(p); // side affect updated world pos
  }
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::GetTranslationVector(
  const double* p1, const double* p2, double* v) const
{
  if (this->TranslationAxis == Axis::NONE)
  {
    for (int i = 0; i < 3; ++i)
    {
      v[i] = p2[i] - p1[i];
    }
  }
  else
  {
    for (int i = 0; i < 3; ++i)
    {
      if (this->TranslationAxis == i)
      {
        v[i] = p2[i] - p1[i];
      }
      else
      {
        v[i] = 0.0;
      }
    }
  }
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::Translate(const double* p1, const double* p2)
{
  double v[3];
  this->GetTranslationVector(p1, p2, v);
  this->Translate(v);
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::Translate(const double* v)
{
  if (this->TranslationAxis == Axis::NONE)
  {
    for (int i = 0; i < 3; ++i)
    {
      WorldPosition->GetValue()[i] += v[i];
    }
  }
  else
  {
    assert(this->TranslationAxis > -1 && this->TranslationAxis < 3 &&
      "this->TranslationAxis out of bounds");
    WorldPosition->GetValue()[this->TranslationAxis] += v[this->TranslationAxis];
  }
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::DeepCopy(svtkProp* prop)
{
  svtkHandleRepresentation* rep = svtkHandleRepresentation::SafeDownCast(prop);
  if (rep)
  {
    this->SetTolerance(rep->GetTolerance());
    this->SetActiveRepresentation(rep->GetActiveRepresentation());
    this->SetConstrained(rep->GetConstrained());
    this->SetPointPlacer(rep->GetPointPlacer());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::ShallowCopy(svtkProp* prop)
{
  svtkHandleRepresentation* rep = svtkHandleRepresentation::SafeDownCast(prop);
  if (rep)
  {
    this->SetTolerance(rep->GetTolerance());
    this->SetActiveRepresentation(rep->GetActiveRepresentation());
    this->SetConstrained(rep->GetConstrained());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
svtkMTimeType svtkHandleRepresentation::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType wMTime = this->WorldPosition->GetMTime();
  mTime = (wMTime > mTime ? wMTime : mTime);
  svtkMTimeType dMTime = this->DisplayPosition->GetMTime();
  mTime = (dMTime > mTime ? dMTime : mTime);

  return mTime;
}

//----------------------------------------------------------------------
void svtkHandleRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  double p[3];
  this->GetDisplayPosition(p);
  os << indent << "Display Position: (" << p[0] << ", " << p[1] << ", " << p[2] << ")\n";

  this->GetWorldPosition(p);
  os << indent << "World Position: (" << p[0] << ", " << p[1] << ", " << p[2] << ")\n";

  os << indent << "Constrained: " << (this->Constrained ? "On" : "Off") << "\n";

  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "Active Representation: " << (this->ActiveRepresentation ? "On" : "Off") << "\n";

  if (this->PointPlacer)
  {
    os << indent << "PointPlacer:\n";
    this->PointPlacer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "PointPlacer: (none)\n";
  }

  // this->InteractionState is printed in superclass
  // this is commented to avoid PrintSelf errors
}
