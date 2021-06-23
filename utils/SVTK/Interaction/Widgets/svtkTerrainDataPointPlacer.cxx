/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTerrainDataPointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTerrainDataPointPlacer.h"

#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkPropCollection.h"
#include "svtkPropPicker.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkTerrainDataPointPlacer);

//----------------------------------------------------------------------
svtkTerrainDataPointPlacer::svtkTerrainDataPointPlacer()
{
  this->TerrainProps = svtkPropCollection::New();
  this->PropPicker = svtkPropPicker::New();
  this->PropPicker->PickFromListOn();

  this->HeightOffset = 0.0;
}

//----------------------------------------------------------------------
svtkTerrainDataPointPlacer::~svtkTerrainDataPointPlacer()
{
  this->TerrainProps->Delete();
  this->PropPicker->Delete();
}

//----------------------------------------------------------------------
void svtkTerrainDataPointPlacer::AddProp(svtkProp* prop)
{
  this->TerrainProps->AddItem(prop);
  this->PropPicker->AddPickList(prop);
}

//----------------------------------------------------------------------
void svtkTerrainDataPointPlacer::RemoveAllProps()
{
  this->TerrainProps->RemoveAllItems();
  this->PropPicker->InitializePickList(); // clear the pick list.. remove
                                          // old props from it...
}

//----------------------------------------------------------------------
int svtkTerrainDataPointPlacer::ComputeWorldPosition(svtkRenderer* ren, double displayPos[2],
  double* svtkNotUsed(refWorldPos), double worldPos[3], double worldOrient[9])
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkTerrainDataPointPlacer::ComputeWorldPosition(
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
      this->TerrainProps->InitTraversal(sit);

      while (svtkProp* p = this->TerrainProps->GetNextProp(sit))
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
          worldPos[2] += this->HeightOffset;
          return 1;
        }
      }
    }
  }

  return 0;
}

//----------------------------------------------------------------------
int svtkTerrainDataPointPlacer::ValidateWorldPosition(
  double worldPos[3], double* svtkNotUsed(worldOrient))
{
  return this->ValidateWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int svtkTerrainDataPointPlacer::ValidateWorldPosition(double svtkNotUsed(worldPos)[3])
{
  return 1;
}

//----------------------------------------------------------------------
int svtkTerrainDataPointPlacer::ValidateDisplayPosition(
  svtkRenderer*, double svtkNotUsed(displayPos)[2])
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
void svtkTerrainDataPointPlacer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PropPicker: " << this->PropPicker << endl;
  if (this->PropPicker)
  {
    this->PropPicker->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "TerrainProps: " << this->TerrainProps << endl;
  if (this->TerrainProps)
  {
    this->TerrainProps->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "HeightOffset: " << this->HeightOffset << endl;
}
