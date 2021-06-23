/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkContourHelper.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkIdListCollection.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkPointData.h"
#include "svtkPolygonBuilder.h"

svtkContourHelper::svtkContourHelper(svtkIncrementalPointLocator* locator, svtkCellArray* verts,
  svtkCellArray* lines, svtkCellArray* polys, svtkPointData* inPd, svtkCellData* inCd,
  svtkPointData* outPd, svtkCellData* outCd, int estimatedSize, bool outputTriangles)
  : Locator(locator)
  , Verts(verts)
  , Lines(lines)
  , Polys(polys)
  , InPd(inPd)
  , InCd(inCd)
  , OutPd(outPd)
  , OutCd(outCd)
  , GenerateTriangles(outputTriangles)
{
  this->Tris = svtkCellArray::New();
  this->TriOutCd = svtkCellData::New();
  if (this->GenerateTriangles)
  {
    this->Tris->AllocateEstimate(estimatedSize, 3);
    this->TriOutCd->Initialize();
  }
  this->PolyCollection = svtkIdListCollection::New();
}

svtkContourHelper::~svtkContourHelper()
{
  this->Tris->Delete();
  this->TriOutCd->Delete();
  this->PolyCollection->Delete();
}

void svtkContourHelper::Contour(
  svtkCell* cell, double value, svtkDataArray* cellScalars, svtkIdType cellId)
{
  bool mergeTriangles = (!this->GenerateTriangles) && cell->GetCellDimension() == 3;
  svtkCellData* outCD = nullptr;
  svtkCellArray* outPoly = nullptr;
  if (mergeTriangles)
  {
    outPoly = this->Tris;
    outCD = this->TriOutCd;
  }
  else
  {
    outPoly = this->Polys;
    outCD = this->OutCd;
  }
  cell->Contour(value, cellScalars, this->Locator, this->Verts, this->Lines, outPoly, this->InPd,
    this->OutPd, this->InCd, cellId, outCD);
  if (mergeTriangles)
  {
    this->PolyBuilder.Reset();

    svtkIdType cellSize;
    const svtkIdType* cellVerts;
    while (this->Tris->GetNextCell(cellSize, cellVerts))
    {
      if (cellSize == 3)
      {
        this->PolyBuilder.InsertTriangle(cellVerts);
      }
      else // for whatever reason, the cell contouring is already outputting polys
      {
        svtkIdType outCellId = this->Polys->InsertNextCell(cellSize, cellVerts);
        this->OutCd->CopyData(this->InCd, cellId,
          outCellId + this->Verts->GetNumberOfCells() + this->Lines->GetNumberOfCells());
      }
    }

    this->PolyBuilder.GetPolygons(this->PolyCollection);
    int nPolys = this->PolyCollection->GetNumberOfItems();
    for (int polyId = 0; polyId < nPolys; ++polyId)
    {
      svtkIdList* poly = this->PolyCollection->GetItem(polyId);
      if (poly->GetNumberOfIds() != 0)
      {
        svtkIdType outCellId = this->Polys->InsertNextCell(poly);
        this->OutCd->CopyData(this->InCd, cellId,
          outCellId + this->Verts->GetNumberOfCells() + this->Lines->GetNumberOfCells());
      }
      poly->Delete();
    }
    this->PolyCollection->RemoveAllItems();
  }
}
