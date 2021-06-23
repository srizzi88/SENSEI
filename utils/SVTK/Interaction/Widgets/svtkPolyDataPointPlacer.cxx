/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataPointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyDataPointPlacer.h"

#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkInteractorObserver.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkPropCollection.h"
#include "svtkPropPicker.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkPolyDataPointPlacer);

//----------------------------------------------------------------------
svtkPolyDataPointPlacer::svtkPolyDataPointPlacer()
{
  this->SurfaceProps = svtkPropCollection::New();
  this->PropPicker = svtkPropPicker::New();
  this->PropPicker->PickFromListOn();
}

//----------------------------------------------------------------------
svtkPolyDataPointPlacer::~svtkPolyDataPointPlacer()
{
  this->SurfaceProps->Delete();
  this->PropPicker->Delete();
}

//----------------------------------------------------------------------
void svtkPolyDataPointPlacer::AddProp(svtkProp* prop)
{
  this->SurfaceProps->AddItem(prop);
  this->PropPicker->AddPickList(prop);
}

//----------------------------------------------------------------------
void svtkPolyDataPointPlacer::RemoveViewProp(svtkProp* prop)
{
  this->SurfaceProps->RemoveItem(prop);
  this->PropPicker->DeletePickList(prop);
}

//----------------------------------------------------------------------
void svtkPolyDataPointPlacer::RemoveAllProps()
{
  this->SurfaceProps->RemoveAllItems();
  this->PropPicker->InitializePickList(); // clear the pick list.. remove
                                          // old props from it...
}

//----------------------------------------------------------------------
int svtkPolyDataPointPlacer::HasProp(svtkProp* prop)
{
  return this->SurfaceProps->IsItemPresent(prop);
}

//----------------------------------------------------------------------
int svtkPolyDataPointPlacer::GetNumberOfProps()
{
  return this->SurfaceProps->GetNumberOfItems();
}

//----------------------------------------------------------------------
int svtkPolyDataPointPlacer::ComputeWorldPosition(svtkRenderer* ren, double displayPos[2],
  double* svtkNotUsed(refWorldPos), double worldPos[3], double worldOrient[9])
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkPolyDataPointPlacer::ComputeWorldPosition(
  svtkRenderer* ren, double displayPos[2], double worldPos[3], double svtkNotUsed(worldOrient)[9])
{
  if (this->PropPicker->Pick(displayPos[0], displayPos[1], 0.0, ren))
  {
    if (svtkAssemblyPath* path = this->PropPicker->GetPath())
    {

      // We are checking if the prop present in the path is present
      // in the list supplied to us.. If it is, that prop will be picked.
      // If not, no prop will be picked.

      bool found = false;
      svtkAssemblyNode* node = nullptr;
      svtkCollectionSimpleIterator sit;
      this->SurfaceProps->InitTraversal(sit);

      while (svtkProp* p = this->SurfaceProps->GetNextProp(sit))
      {
        svtkCollectionSimpleIterator psit;
        path->InitTraversal(psit);

        for (int i = 0; i < path->GetNumberOfItems() && !found; ++i)
        {
          node = path->GetNextNode(psit);
          found = (node->GetViewProp() == p);
        }

        if (found)
        {
          this->PropPicker->GetPickPosition(worldPos);

          // Raise height by 0.01 ... this should be a method..
          double displyPos[3];
          svtkInteractorObserver::ComputeWorldToDisplay(
            ren, worldPos[0], worldPos[1], worldPos[2], displyPos);
          displyPos[2] -= 0.01;
          double w[4];
          svtkInteractorObserver::ComputeDisplayToWorld(
            ren, displyPos[0], displyPos[1], displyPos[2], w);
          worldPos[0] = w[0];
          worldPos[1] = w[1];
          worldPos[2] = w[2];

          return 1;
        }
      }
    }
  }

  return 0;
}

//----------------------------------------------------------------------
int svtkPolyDataPointPlacer::ValidateWorldPosition(
  double worldPos[3], double* svtkNotUsed(worldOrient))
{
  return this->ValidateWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int svtkPolyDataPointPlacer::ValidateWorldPosition(double svtkNotUsed(worldPos)[3])
{
  return 1;
}

//----------------------------------------------------------------------
int svtkPolyDataPointPlacer::ValidateDisplayPosition(svtkRenderer*, double svtkNotUsed(displayPos)[2])
{
  // We could check here to ensure that the display point picks one of the
  // terrain props, but the contour representation always calls
  // ComputeWorldPosition followed by
  // ValidateDisplayPosition/ValidateWorldPosition when it needs to
  // update a node...
  //
  // So that would be wasting CPU cycles to perform
  // the same check twice..  Just return 1 here.

  return 1;
}

//----------------------------------------------------------------------
void svtkPolyDataPointPlacer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PropPicker: " << this->PropPicker << endl;
  if (this->PropPicker)
  {
    this->PropPicker->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "SurfaceProps: " << this->SurfaceProps << endl;
  if (this->SurfaceProps)
  {
    this->SurfaceProps->PrintSelf(os, indent.GetNextIndent());
  }
}
