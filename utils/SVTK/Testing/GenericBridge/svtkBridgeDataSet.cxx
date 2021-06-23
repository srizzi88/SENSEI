/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgeDataSet - Implementation of svtkGenericDataSet.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.

#include "svtkBridgeDataSet.h"

#include <cassert>

#include "svtkBridgeAttribute.h"
#include "svtkBridgeCell.h"
#include "svtkBridgeCellIterator.h"
#include "svtkBridgePointIterator.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkDataSet.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericCell.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericEdgeTable.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSimpleCellTessellator.h"

svtkStandardNewMacro(svtkBridgeDataSet);

//----------------------------------------------------------------------------
// Default constructor.
svtkBridgeDataSet::svtkBridgeDataSet()
{
  this->Implementation = nullptr;
  this->Types = svtkCellTypes::New();
  this->Tessellator = svtkSimpleCellTessellator::New();
}

//----------------------------------------------------------------------------
svtkBridgeDataSet::~svtkBridgeDataSet()
{
  if (this->Implementation)
  {
    this->Implementation->Delete();
  }
  this->Types->Delete();
  // this->Tessellator is deleted in the superclass
}

//----------------------------------------------------------------------------
void svtkBridgeDataSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "implementation: ";
  if (this->Implementation == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    this->Implementation->PrintSelf(os << endl, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
// Description:
// Return the dataset that will be manipulated through the adaptor interface.
svtkDataSet* svtkBridgeDataSet::GetDataSet()
{
  return this->Implementation;
}

//----------------------------------------------------------------------------
// Description:
// Set the dataset that will be manipulated through the adaptor interface.
// \pre ds_exists: ds!=0
void svtkBridgeDataSet::SetDataSet(svtkDataSet* ds)
{
  int i;
  int c;
  svtkPointData* pd;
  svtkCellData* cd;
  svtkBridgeAttribute* a;

  svtkSetObjectBodyMacro(Implementation, svtkDataSet, ds);
  // refresh the attribute collection
  this->Attributes->Reset();
  if (ds != nullptr)
  {
    // point data
    pd = ds->GetPointData();
    c = pd->GetNumberOfArrays();
    i = 0;
    while (i < c)
    {
      a = svtkBridgeAttribute::New();
      a->InitWithPointData(pd, i);
      this->Attributes->InsertNextAttribute(a);
      a->Delete();
      ++i;
    }
    // same thing for cell data.
    cd = ds->GetCellData();
    c = cd->GetNumberOfArrays();
    i = 0;
    while (i < c)
    {
      a = svtkBridgeAttribute::New();
      a->InitWithCellData(cd, i);
      this->Attributes->InsertNextAttribute(a);
      a->Delete();
      ++i;
    }
    this->Tessellator->Initialize(this);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// Number of points composing the dataset. See NewPointIterator for more
// details.
// \post positive_result: result>=0
svtkIdType svtkBridgeDataSet::GetNumberOfPoints()
{
  svtkIdType result = 0;
  if (this->Implementation)
  {
    result = this->Implementation->GetNumberOfPoints();
  }
  assert("post: positive_result" && result >= 0);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Compute the number of cells for each dimension and the list of types of
// cells.
// \pre implementation_exists: this->Implementation!=0
void svtkBridgeDataSet::ComputeNumberOfCellsAndTypes()
{
  unsigned char type;
  svtkIdType cellId;
  svtkIdType numCells;
  svtkCell* c;

  if (this->GetMTime() > this->ComputeNumberOfCellsTime) // cache is obsolete
  {
    numCells = this->GetNumberOfCells();
    this->NumberOf0DCells = 0;
    this->NumberOf1DCells = 0;
    this->NumberOf2DCells = 0;
    this->NumberOf3DCells = 0;

    this->Types->Reset();

    if (this->Implementation != nullptr)
    {
      cellId = 0;
      while (cellId < numCells)
      {
        c = this->Implementation->GetCell(cellId);
        switch (c->GetCellDimension())
        {
          case 0:
            this->NumberOf0DCells++;
            break;
          case 1:
            this->NumberOf1DCells++;
            break;
          case 2:
            this->NumberOf2DCells++;
            break;
          case 3:
            this->NumberOf3DCells++;
            break;
        }
        type = c->GetCellType();
        if (!this->Types->IsType(type))
        {
          this->Types->InsertNextType(type);
        }
        cellId++;
      }
    }

    this->ComputeNumberOfCellsTime.Modified(); // cache is up-to-date
    assert("check: positive_dim0" && this->NumberOf0DCells >= 0);
    assert("check: valid_dim0" && this->NumberOf0DCells <= numCells);
    assert("check: positive_dim1" && this->NumberOf1DCells >= 0);
    assert("check: valid_dim1" && this->NumberOf1DCells <= numCells);
    assert("check: positive_dim2" && this->NumberOf2DCells >= 0);
    assert("check: valid_dim2" && this->NumberOf2DCells <= numCells);
    assert("check: positive_dim3" && this->NumberOf3DCells >= 0);
    assert("check: valid_dim3" && this->NumberOf3DCells <= numCells);
  }
}

//----------------------------------------------------------------------------
// Description:
// Number of cells that explicitly define the dataset. See NewCellIterator
// for more details.
// \pre valid_dim_range: (dim>=-1) && (dim<=3)
// \post positive_result: result>=0
svtkIdType svtkBridgeDataSet::GetNumberOfCells(int dim)
{
  assert("pre: valid_dim_range" && (dim >= -1) && (dim <= 3));

  svtkIdType result = 0;
  if (this->Implementation != nullptr)
  {
    if (dim == -1)
    {
      result = this->Implementation->GetNumberOfCells();
    }
    else
    {
      this->ComputeNumberOfCellsAndTypes();
      switch (dim)
      {
        case 0:
          result = this->NumberOf0DCells;
          break;
        case 1:
          result = this->NumberOf1DCells;
          break;
        case 2:
          result = this->NumberOf2DCells;
          break;
        case 3:
          result = this->NumberOf3DCells;
          break;
      }
    }
  }

  assert("post: positive_result" && result >= 0);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Return -1 if the dataset is explicitly defined by cells of several
// dimensions or if there is no cell. If the dataset is explicitly defined by
// cells of a unique dimension, return this dimension.
// \post valid_range: (result>=-1) && (result<=3)
int svtkBridgeDataSet::GetCellDimension()
{
  int result = 0;
  int accu = 0;

  this->ComputeNumberOfCellsAndTypes();

  if (this->NumberOf0DCells != 0)
  {
    accu++;
    result = 0;
  }
  if (this->NumberOf1DCells != 0)
  {
    accu++;
    result = 1;
  }
  if (this->NumberOf2DCells != 0)
  {
    accu++;
    result = 2;
  }
  if (this->NumberOf3DCells != 0)
  {
    accu++;
    result = 3;
  }
  if (accu != 1) // no cells at all or several dimensions
  {
    result = -1;
  }
  assert("post: valid_range" && (result >= -1) && (result <= 3));
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Get a list of types of cells in a dataset. The list consists of an array
// of types (not necessarily in any order), with a single entry per type.
// For example a dataset 5 triangles, 3 lines, and 100 hexahedra would
// result a list of three entries, corresponding to the types SVTK_TRIANGLE,
// SVTK_LINE, and SVTK_HEXAHEDRON.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
// \pre types_exist: types!=0
void svtkBridgeDataSet::GetCellTypes(svtkCellTypes* types)
{
  assert("pre: types_exist" && types != nullptr);

  int i;
  int c;
  this->ComputeNumberOfCellsAndTypes();

  // copy from `this->Types' to `types'.
  types->Reset();
  c = this->Types->GetNumberOfTypes();
  i = 0;
  while (i < c)
  {
    types->InsertNextType(this->Types->GetCellType(i));
    ++i;
  }
}

//----------------------------------------------------------------------------
// Description:
// Cells of dimension `dim' (or all dimensions if -1) that explicitly define
// the dataset. For instance, it will return only tetrahedra if the mesh is
// defined by tetrahedra. If the mesh is composed of two parts, one with
// tetrahedra and another part with triangles, it will return both, but will
// not return edges and vertices.
// \pre valid_dim_range: (dim>=-1) && (dim<=3)
// \post result_exists: result!=0
svtkGenericCellIterator* svtkBridgeDataSet::NewCellIterator(int dim)
{
  assert("pre: valid_dim_range" && (dim >= -1) && (dim <= 3));

  svtkBridgeCellIterator* result = svtkBridgeCellIterator::New();
  result->InitWithDataSet(this, dim); // svtkBridgeCellIteratorOnDataSetCells

  assert("post: result_exists" && result != nullptr);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Boundaries of dimension `dim' (or all dimensions if -1) of the dataset.
// If `exteriorOnly' is true, only the exterior boundaries of the dataset
// will be returned, otherwise it will return exterior and interior
// boundaries.
// \pre valid_dim_range: (dim>=-1) && (dim<=2)
// \post result_exists: result!=0
svtkGenericCellIterator* svtkBridgeDataSet::NewBoundaryIterator(int dim, int exteriorOnly)
{
  assert("pre: valid_dim_range" && (dim >= -1) && (dim <= 2));

  svtkBridgeCellIterator* result = svtkBridgeCellIterator::New();
  result->InitWithDataSetBoundaries(
    this, dim, exteriorOnly); // svtkBridgeCellIteratorOnDataSetBoundaries(dim,exterior_only);

  assert("post: result_exists" && result != nullptr);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Points composing the dataset; they can be on a vertex or isolated.
// \post result_exists: result!=0
svtkGenericPointIterator* svtkBridgeDataSet::NewPointIterator()
{
  svtkBridgePointIterator* result = svtkBridgePointIterator::New();
  result->InitWithDataSet(this);
  assert("post: result_exists" && result != nullptr);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Estimated size needed after tessellation (or special operation)
svtkIdType svtkBridgeDataSet::GetEstimatedSize()
{
  return this->GetNumberOfPoints() * this->GetNumberOfCells();
}

//----------------------------------------------------------------------------
// Description:
// Locate closest cell to position `x' (global coordinates) with respect to
// a tolerance squared `tol2' and an initial guess `cell' (if valid). The
// result consists in the `cell', the `subId' of the sub-cell (0 if primary
// cell), the parametric coordinates `pcoord' of the position. It returns
// whether the position is inside the cell or not. Tolerance is used to
// control how close the point is to be considered "in" the cell.
// THIS METHOD IS NOT THREAD SAFE.
// \pre not_empty: GetNumberOfCells()>0
// \pre cell_exists: cell!=0
// \pre positive_tolerance: tol2>0
// \post clamped_pcoords: result implies (0<=pcoords[0]<=1 && )

int svtkBridgeDataSet::FindCell(
  double x[3], svtkGenericCellIterator*& cell, double tol2, int& subId, double pcoords[3])
{
  assert("pre: not_empty" && GetNumberOfCells() > 0);
  assert("pre: cell_exists" && cell != nullptr);
  assert("pre: positive_tolerance" && tol2 > 0);

  svtkIdType cellid;
  svtkBridgeCellIterator* it = static_cast<svtkBridgeCellIterator*>(cell);

  double* ignoredWeights = new double[this->Implementation->GetMaxCellSize()];

  cellid = this->Implementation->FindCell(x, nullptr, 0, tol2, subId, pcoords, ignoredWeights);

  delete[] ignoredWeights;
  if (cellid >= 0)
  {
    it->InitWithOneCell(this, cellid); // at end
    it->Begin();
    // clamp:
    int i = 0;
    while (i < 3)
    {
      if (pcoords[i] < 0)
      {
        pcoords[i] = 0;
      }
      else if (pcoords[i] > 1)
      {
        pcoords[i] = 1;
      }
      ++i;
    }
  }

  // A=>B: !A || B
  // result => clamped pcoords
  assert("post: clamped_pcoords" &&
    ((cellid < 0) ||
      (pcoords[0] >= 0 && pcoords[0] <= 1 && pcoords[1] >= 0 && pcoords[1] <= 1 &&
        pcoords[2] >= 0 && pcoords[2] <= 1)));

  return cellid >= 0; // bool
}

//----------------------------------------------------------------------------
// Description:
// Locate closest point `p' to position `x' (global coordinates)
// \pre not_empty: GetNumberOfPoints()>0
// \pre p_exists: p!=0
void svtkBridgeDataSet::FindPoint(double x[3], svtkGenericPointIterator* p)
{
  assert("pre: not_empty" && GetNumberOfPoints() > 0);
  assert("pre: p_exists" && p != nullptr);
  svtkBridgePointIterator* bp = static_cast<svtkBridgePointIterator*>(p);

  if (this->Implementation != nullptr)
  {
    svtkIdType pt = this->Implementation->FindPoint(x);
    bp->InitWithOnePoint(this, pt);
  }
  else
  {
    bp->InitWithOnePoint(this, -1);
  }
}

//----------------------------------------------------------------------------
// Description:
// Datasets are composite objects and need to check each part for MTime.
svtkMTimeType svtkBridgeDataSet::GetMTime()
{
  svtkMTimeType result;
  svtkMTimeType mtime;

  result = this->Superclass::GetMTime();

  if (this->Implementation != nullptr)
  {
    mtime = this->Implementation->GetMTime();
    result = (mtime > result ? mtime : result);
  }
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Compute the geometry bounding box.
void svtkBridgeDataSet::ComputeBounds()
{
  if (this->GetMTime() > this->ComputeTime)
  {
    if (this->Implementation != nullptr)
    {
      this->Implementation->ComputeBounds();
      this->ComputeTime.Modified();
      const double* bounds = this->Implementation->GetBounds();
      memcpy(this->Bounds, bounds, sizeof(double) * 6);
    }
    else
    {
      svtkMath::UninitializeBounds(this->Bounds);
    }
    this->ComputeTime.Modified();
  }
}
