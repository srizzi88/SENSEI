/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellCentersPointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCellCentersPointPlacer.h"

#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkCell.h"
#include "svtkCellPicker.h"
#include "svtkDataSet.h"
#include "svtkInteractorObserver.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkProp.h"
#include "svtkPropCollection.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkCellCentersPointPlacer);

//----------------------------------------------------------------------
svtkCellCentersPointPlacer::svtkCellCentersPointPlacer()
{
  this->PickProps = svtkPropCollection::New();
  this->CellPicker = svtkCellPicker::New();
  this->CellPicker->PickFromListOn();

  this->Mode = svtkCellCentersPointPlacer::CellPointsMean;
}

//----------------------------------------------------------------------
svtkCellCentersPointPlacer::~svtkCellCentersPointPlacer()
{
  this->PickProps->Delete();
  this->CellPicker->Delete();
}

//----------------------------------------------------------------------
void svtkCellCentersPointPlacer::AddProp(svtkProp* prop)
{
  this->PickProps->AddItem(prop);
  this->CellPicker->AddPickList(prop);
}

//----------------------------------------------------------------------
void svtkCellCentersPointPlacer::RemoveViewProp(svtkProp* prop)
{
  this->PickProps->RemoveItem(prop);
  this->CellPicker->DeletePickList(prop);
}

//----------------------------------------------------------------------
void svtkCellCentersPointPlacer::RemoveAllProps()
{
  this->PickProps->RemoveAllItems();
  this->CellPicker->InitializePickList(); // clear the pick list.. remove
                                          // old props from it...
}

//----------------------------------------------------------------------
int svtkCellCentersPointPlacer::HasProp(svtkProp* prop)
{
  return this->PickProps->IsItemPresent(prop);
}

//----------------------------------------------------------------------
int svtkCellCentersPointPlacer::GetNumberOfProps()
{
  return this->PickProps->GetNumberOfItems();
}

//----------------------------------------------------------------------
int svtkCellCentersPointPlacer::ComputeWorldPosition(svtkRenderer* ren, double displayPos[2],
  double* svtkNotUsed(refWorldPos), double worldPos[3], double worldOrient[9])
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkCellCentersPointPlacer::ComputeWorldPosition(
  svtkRenderer* ren, double displayPos[2], double worldPos[3], double svtkNotUsed(worldOrient)[9])
{
  svtkDebugMacro(<< "Request for computing world position at "
                << "display position of " << displayPos[0] << "," << displayPos[1]);

  if (this->CellPicker->Pick(displayPos[0], displayPos[1], 0.0, ren))
  {
    if (svtkAssemblyPath* path = this->CellPicker->GetPath())
    {

      // We are checking if the prop present in the path is present
      // in the list supplied to us.. If it is, that prop will be picked.
      // If not, no prop will be picked.

      bool found = false;
      svtkAssemblyNode* node = nullptr;
      svtkCollectionSimpleIterator sit;
      this->PickProps->InitTraversal(sit);

      while (svtkProp* p = this->PickProps->GetNextProp(sit))
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
          svtkIdType pickedCellId = this->CellPicker->GetCellId();
          svtkCell* pickedCell = this->CellPicker->GetDataSet()->GetCell(pickedCellId);

          if (this->Mode == svtkCellCentersPointPlacer::ParametricCenter)
          {
            double pcoords[3];
            pickedCell->GetParametricCenter(pcoords);
            double* weights = new double[pickedCell->GetNumberOfPoints()];

            int subId;
            pickedCell->EvaluateLocation(subId, pcoords, worldPos, weights);
            delete[] weights;
          }

          if (this->Mode == svtkCellCentersPointPlacer::CellPointsMean)
          {
            const svtkIdType nPoints = pickedCell->GetNumberOfPoints();
            svtkPoints* points = pickedCell->GetPoints();
            worldPos[0] = worldPos[1] = worldPos[2] = 0.0;
            double pp[3];
            for (svtkIdType i = 0; i < nPoints; i++)
            {
              points->GetPoint(i, pp);
              worldPos[0] += pp[0];
              worldPos[1] += pp[1];
              worldPos[2] += pp[2];
            }

            worldPos[0] /= (static_cast<double>(nPoints));
            worldPos[1] /= (static_cast<double>(nPoints));
            worldPos[2] /= (static_cast<double>(nPoints));
          }

          if (this->Mode == svtkCellCentersPointPlacer::None)
          {
            this->CellPicker->GetPickPosition(worldPos);
          }

          return 1;
        }
      }
    }
  }

  return 0;
}

//----------------------------------------------------------------------
int svtkCellCentersPointPlacer::ValidateWorldPosition(
  double worldPos[3], double* svtkNotUsed(worldOrient))
{
  return this->ValidateWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int svtkCellCentersPointPlacer::ValidateWorldPosition(double svtkNotUsed(worldPos)[3])
{
  return 1;
}

//----------------------------------------------------------------------
int svtkCellCentersPointPlacer::ValidateDisplayPosition(
  svtkRenderer*, double svtkNotUsed(displayPos)[2])
{
  return 1;
}

//----------------------------------------------------------------------
void svtkCellCentersPointPlacer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CellPicker: " << this->CellPicker << endl;
  if (this->CellPicker)
  {
    this->CellPicker->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "PickProps: " << this->PickProps << endl;
  if (this->PickProps)
  {
    this->PickProps->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "Mode: " << this->Mode << endl;
}
