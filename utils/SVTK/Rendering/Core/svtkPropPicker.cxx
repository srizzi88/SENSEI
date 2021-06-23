/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPropPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPropPicker.h"

#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkBox.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTransform.h"
#include "svtkWorldPointPicker.h"

svtkStandardNewMacro(svtkPropPicker);

svtkPropPicker::svtkPropPicker()
{
  this->PickFromProps = nullptr;
  this->WorldPointPicker = svtkWorldPointPicker::New();
}

svtkPropPicker::~svtkPropPicker()
{
  this->WorldPointPicker->Delete();
}

// set up for a pick
void svtkPropPicker::Initialize()
{
  this->svtkAbstractPropPicker::Initialize();
}

// Pick from the given collection
int svtkPropPicker::Pick(
  double selectionX, double selectionY, double svtkNotUsed(z), svtkRenderer* renderer)
{
  if (this->PickFromList)
  {
    return this->PickProp(selectionX, selectionY, renderer, this->PickList);
  }
  else
  {
    return this->PickProp(selectionX, selectionY, renderer);
  }
}

// Pick from the given collection
int svtkPropPicker::PickProp(
  double selectionX, double selectionY, svtkRenderer* renderer, svtkPropCollection* pickfrom)
{
  this->PickFromProps = pickfrom;
  int ret = this->PickProp(selectionX, selectionY, renderer);
  this->PickFromProps = nullptr;
  return ret;
}

// Perform pick operation with selection point provided. The z location
// is recovered from the zBuffer. Always returns 0 since no actors are picked.
int svtkPropPicker::PickProp(double selectionX, double selectionY, svtkRenderer* renderer)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = 0;

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  // Have the renderer do the hardware pick
  this->SetPath(renderer->PickPropFrom(selectionX, selectionY, this->PickFromProps));

  // If there was a pick then find the world x,y,z for the pick, and invoke
  // its pick method.
  if (this->Path)
  {
    this->WorldPointPicker->Pick(selectionX, selectionY, 0, renderer);
    this->WorldPointPicker->GetPickPosition(this->PickPosition);
    this->Path->GetLastNode()->GetViewProp()->Pick();
    this->InvokeEvent(svtkCommand::PickEvent, nullptr);
  }

  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);

  // Call Pick on the Prop that was picked, and return 1 for success
  if (this->Path)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// Pick from the given collection
int svtkPropPicker::Pick3DPoint(double pos[3], svtkRenderer* renderer)
{
  if (this->PickFromList)
  {
    return this->PickProp3DPoint(pos, renderer, this->PickList);
  }
  else
  {
    return this->PickProp3DPoint(pos, renderer);
  }
}

// Pick from the given collection
int svtkPropPicker::PickProp3DPoint(
  double pos[3], svtkRenderer* renderer, svtkPropCollection* pickfrom)
{
  this->PickFromProps = pickfrom;
  int ret = this->PickProp3DPoint(pos, renderer);
  this->PickFromProps = nullptr;
  return ret;
}

// Perform pick operation with selection point provided. The z location
// is recovered from the zBuffer. Always returns 0 since no actors are picked.
int svtkPropPicker::PickProp3DPoint(double pos[3], svtkRenderer* renderer)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = pos[0];
  this->SelectionPoint[1] = pos[1];
  this->SelectionPoint[2] = pos[2];

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  // for each prop, that is packable
  // find the prop whose bounds
  // contain the pick points and whole center is closest to the
  // selection point
  // TODO need to handle AssemblyPaths
  svtkPropCollection* props = renderer->GetViewProps();

  svtkAssemblyPath* result = nullptr;
  svtkCollectionSimpleIterator pit;
  props->InitTraversal(pit);
  svtkProp* prop = nullptr;
  while ((prop = props->GetNextProp(pit)))
  {
    if (prop->GetPickable() && prop->GetVisibility() && prop->GetUseBounds())
    {
      const double* bnds = prop->GetBounds();
      if (bnds)
      {
        if (pos[0] >= bnds[0] && pos[0] <= bnds[1] && pos[1] >= bnds[2] && pos[1] <= bnds[3] &&
          pos[2] >= bnds[4] && pos[2] <= bnds[5])
        {
          prop->InitPathTraversal();
          result = prop->GetNextPath();
        }
      }
    }
  }

  if (result)
  {
    result->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(svtkCommand::PickEvent, nullptr);
  }
  this->SetPath(result);

  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);

  // Call Pick on the Prop that was picked, and return 1 for success
  if (result)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// Pick from the given collection
int svtkPropPicker::Pick3DRay(double pos[3], double wori[4], svtkRenderer* renderer)
{
  // Compute event orientation
  if (this->PickFromList)
  {
    return this->PickProp3DRay(pos, wori, renderer, this->PickList);
  }
  else
  {
    return this->PickProp3DRay(pos, wori, renderer, renderer->GetViewProps());
  }
}

// Pick from the given collection
int svtkPropPicker::PickProp3DRay(
  double selectionPt[3], double wori[4], svtkRenderer* renderer, svtkPropCollection* propCollection)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  // Event position - Ray start position
  double p0[4];
  p0[0] = selectionPt[0];
  p0[1] = selectionPt[1];
  p0[2] = selectionPt[2];
  p0[3] = 1.0;

  // Compute ray direction
  svtkSmartPointer<svtkTransform> trans = svtkSmartPointer<svtkTransform>::New();
  trans->RotateWXYZ(wori[0], wori[1], wori[2], wori[3]);
  double* rayDirection = trans->TransformDoubleVector(0.0, 0.0, -1.0);

  svtkCamera* cam = renderer->GetActiveCamera();
  if (!cam)
  {
    return 0;
  }
  // Ray length
  double rayLength = cam->GetClippingRange()[1];

  // Ray end point
  double p1[4];
  p1[0] = p0[0] + rayLength * rayDirection[0];
  p1[1] = p0[1] + rayLength * rayDirection[1];
  p1[2] = p0[2] + rayLength * rayDirection[2];
  p1[3] = 1.0;

  // Construct the ray
  double ray[3];
  ray[0] = p1[0] - p0[0];
  ray[1] = p1[1] - p0[1];
  ray[2] = p1[2] - p0[2];

  svtkAssemblyPath* result = nullptr;
  svtkAssemblyPath* insideResult = nullptr;
  svtkCollectionSimpleIterator pit;
  svtkProp* prop = nullptr;
  svtkAssemblyPath* path;
  svtkProp* propCandidate;
  double t_min = SVTK_DOUBLE_MAX;
  double hitPos[3] = { 0.0, 0.0, 0.0 };

  // For all props, return the closest prop intersected by the ray.
  // If we pick inside a prop, it will be returned only if no other svtkProps are
  // intersected by the ray. WARNING: Intersection checking uses bounds. This is
  // confusing when the prop isn't fully filling its bounds. Improve this by :
  //-returning the prop which bounds center is the closest to the ray, or
  //-computing intersection with the geometry itself (see svtkCellPicker).
  for (propCollection->InitTraversal(pit); (prop = propCollection->GetNextProp(pit));)
  {
    for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
    {
      propCandidate = path->GetFirstNode()->GetViewProp();
      if (propCandidate->GetPickable() && propCandidate->GetVisibility() &&
        propCandidate->GetUseBounds())
      {
        double* bnds = propCandidate->GetBounds();
        if (bnds)
        {
          double t;
          double xyz[3];
          // Check for box intersection
          if (svtkBox::IntersectBox(bnds, const_cast<double*>(p0), ray, xyz, t))
          {
            // Inside a prop, save its path in case nothing else is picked
            if (!(t > 0))
            {
              insideResult = path;

              hitPos[0] = selectionPt[0];
              hitPos[1] = selectionPt[1];
              hitPos[2] = selectionPt[2];
            }
            // Something was picked by the ray, save its path and update t_min
            if (t > 0 && t < t_min)
            {
              result = path;
              t_min = t;
              hitPos[0] = xyz[0];
              hitPos[1] = xyz[1];
              hitPos[2] = xyz[2];
            }
          }
        }
      }
    }
  }

  // If the ray didn't intersect anything, we might be inside a prop
  if (!result)
  {
    result = insideResult;
  }

  // If something was picked..
  if (result)
  {
    result->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(svtkCommand::PickEvent, nullptr);

    // Update the picked position
    this->PickPosition[0] = hitPos[0];
    this->PickPosition[1] = hitPos[1];
    this->PickPosition[2] = hitPos[2];
  }

  this->SetPath(result);
  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);

  // Call Pick on the Prop that was picked, and return 1 for success
  if (result)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void svtkPropPicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->PickFromProps)
  {
    os << indent << "PickFrom List: " << this->PickFromProps << endl;
  }
  else
  {
    os << indent << "PickFrom List: (none)" << endl;
  }
}
