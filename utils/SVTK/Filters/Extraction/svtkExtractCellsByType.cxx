/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractCellsByType.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractCellsByType.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkUniformGrid.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkExtractCellsByType);

#include <set>

// Special token marks any cell type
#define SVTK_ANY_CELL_TYPE 1000000

struct svtkCellTypeSet : public std::set<unsigned int>
{
};

//----------------------------------------------------------------------------
svtkExtractCellsByType::svtkExtractCellsByType()
{
  this->CellTypes = new svtkCellTypeSet;
}

//----------------------------------------------------------------------------
svtkExtractCellsByType::~svtkExtractCellsByType()
{
  delete this->CellTypes;
}

//----------------------------------------------------------------------------
void svtkExtractCellsByType::AddCellType(unsigned int cellType)
{
  auto prevSize = this->CellTypes->size();
  this->CellTypes->insert(cellType);
  if (prevSize != this->CellTypes->size())
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkExtractCellsByType::RemoveCellType(unsigned int cellType)
{
  auto prevSize = this->CellTypes->size();
  this->CellTypes->erase(cellType);
  this->CellTypes->erase(SVTK_ANY_CELL_TYPE);
  if (prevSize != this->CellTypes->size())
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkExtractCellsByType::RemoveAllCellTypes()
{
  if (!this->CellTypes->empty())
  {
    this->CellTypes->clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Special value indicates that all cells are to be selected. This is better
// than populating from the list svtkCellType.h due to the associated
// maintenance burden.
void svtkExtractCellsByType::AddAllCellTypes()
{
  auto prevSize = this->CellTypes->size();
  this->CellTypes->insert(SVTK_ANY_CELL_TYPE);
  if (prevSize != this->CellTypes->size())
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool svtkExtractCellsByType::ExtractCellType(unsigned int cellType)
{
  if (this->CellTypes->find(cellType) != this->CellTypes->end() ||
    this->CellTypes->find(SVTK_ANY_CELL_TYPE) != this->CellTypes->end())
  {
    return true;
  }
  else
  {
    return false;
  }
}

//----------------------------------------------------------------------------
void svtkExtractCellsByType::ExtractUnstructuredData(svtkDataSet* inDS, svtkDataSet* outDS)
{
  svtkPointData* inPD = inDS->GetPointData();
  svtkPointData* outPD = outDS->GetPointData();

  svtkIdType numPts = inDS->GetNumberOfPoints();

  // We are going some maps to indicate where the points and cells originated
  // from. Values <0 mean that the points or cells are not mapped to the
  // output.
  svtkIdType* ptMap = new svtkIdType[numPts];
  std::fill_n(ptMap, numPts, -1);

  // Now dispatch to specific unstructured type
  svtkIdType numNewPts = 0;
  if (inDS->GetDataObjectType() == SVTK_POLY_DATA)
  {
    this->ExtractPolyDataCells(inDS, outDS, ptMap, numNewPts);
  }

  else if (inDS->GetDataObjectType() == SVTK_UNSTRUCTURED_GRID)
  {
    this->ExtractUnstructuredGridCells(inDS, outDS, ptMap, numNewPts);
  }

  // Copy referenced input points to new points array
  outPD->CopyAllocate(inPD);
  svtkPointSet* inPtSet = svtkPointSet::SafeDownCast(inDS);
  svtkPointSet* outPtSet = svtkPointSet::SafeDownCast(outDS);
  svtkPoints* inPts = inPtSet->GetPoints();
  svtkPoints* outPts = svtkPoints::New();
  outPts->SetNumberOfPoints(numNewPts);
  for (svtkIdType ptId = 0; ptId < numPts; ++ptId)
  {
    if (ptMap[ptId] >= 0)
    {
      outPts->SetPoint(ptMap[ptId], inPts->GetPoint(ptId));
      outPD->CopyData(inPD, ptId, ptMap[ptId]);
    }
  }
  outPtSet->SetPoints(outPts);

  // Clean up
  outPts->Delete();
  delete[] ptMap;
}

//----------------------------------------------------------------------------
void svtkExtractCellsByType::ExtractPolyDataCells(
  svtkDataSet* inDS, svtkDataSet* outDS, svtkIdType* ptMap, svtkIdType& numNewPts)
{
  svtkPolyData* input = svtkPolyData::SafeDownCast(inDS);
  svtkCellData* inCD = input->GetCellData();
  svtkPolyData* output = svtkPolyData::SafeDownCast(outDS);
  svtkCellData* outCD = output->GetCellData();

  // Treat the four cell arrays separately. If the array might have cells of
  // the specified types, then traverse it, copying the input cells to the
  // output cells. Also keep track of the point map.

  // The cellIds are numbered across the four arrays: verts, lines, polys,
  // strips. Have to carefully coordinate the cell ids with traversal of each
  // array.
  svtkIdType cellId, currentCellId = 0;
  svtkIdList* ptIds = svtkIdList::New();

  // Verts
  outCD->CopyAllocate(inCD);
  svtkIdType i;
  svtkIdType npts;
  const svtkIdType* pts;
  svtkCellArray* inVerts = input->GetVerts();
  if (this->ExtractCellType(SVTK_VERTEX) || this->ExtractCellType(SVTK_POLY_VERTEX))
  {
    svtkCellArray* verts = svtkCellArray::New();
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts, pts); ++currentCellId)
    {
      if (this->ExtractCellType(input->GetCellType(currentCellId)))
      {
        ptIds->Reset();
        for (i = 0; i < npts; ++i)
        {
          if (ptMap[pts[i]] < 0)
          {
            ptMap[pts[i]] = numNewPts++;
          }
          ptIds->InsertId(i, ptMap[pts[i]]);
        }
        cellId = verts->InsertNextCell(ptIds);
        outCD->CopyData(inCD, currentCellId, cellId);
      }
    }
    output->SetVerts(verts);
    verts->Delete();
  }
  else
  {
    currentCellId += inVerts->GetNumberOfCells();
  }

  // Lines
  svtkCellArray* inLines = input->GetLines();
  if (this->ExtractCellType(SVTK_LINE) || this->ExtractCellType(SVTK_POLY_LINE))
  {
    svtkCellArray* lines = svtkCellArray::New();
    for (inLines->InitTraversal(); inLines->GetNextCell(npts, pts); ++currentCellId)
    {
      if (this->ExtractCellType(input->GetCellType(currentCellId)))
      {
        ptIds->Reset();
        for (i = 0; i < npts; ++i)
        {
          if (ptMap[pts[i]] < 0)
          {
            ptMap[pts[i]] = numNewPts++;
          }
          ptIds->InsertId(i, ptMap[pts[i]]);
        }
        cellId = lines->InsertNextCell(ptIds);
        outCD->CopyData(inCD, currentCellId, cellId);
      }
    }
    output->SetLines(lines);
    lines->Delete();
  }
  else
  {
    currentCellId += inLines->GetNumberOfCells();
  }

  // Polys
  svtkCellArray* inPolys = input->GetPolys();
  if (this->ExtractCellType(SVTK_TRIANGLE) || this->ExtractCellType(SVTK_QUAD) ||
    this->ExtractCellType(SVTK_POLYGON))
  {
    svtkCellArray* polys = svtkCellArray::New();
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts); ++currentCellId)
    {
      if (this->ExtractCellType(input->GetCellType(currentCellId)))
      {
        ptIds->Reset();
        for (i = 0; i < npts; ++i)
        {
          if (ptMap[pts[i]] < 0)
          {
            ptMap[pts[i]] = numNewPts++;
          }
          ptIds->InsertId(i, ptMap[pts[i]]);
        }
        cellId = polys->InsertNextCell(ptIds);
        outCD->CopyData(inCD, currentCellId, cellId);
      }
    }
    output->SetPolys(polys);
    polys->Delete();
  }
  else
  {
    currentCellId += inPolys->GetNumberOfCells();
  }

  // Triangle strips
  svtkCellArray* inStrips = input->GetStrips();
  if (this->ExtractCellType(SVTK_TRIANGLE_STRIP))
  {
    svtkCellArray* strips = svtkCellArray::New();
    // All cells are of type SVTK_TRIANGLE_STRIP
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts); ++currentCellId)
    {
      ptIds->Reset();
      for (i = 0; i < npts; ++i)
      {
        if (ptMap[pts[i]] < 0)
        {
          ptMap[pts[i]] = numNewPts++;
        }
        ptIds->InsertId(i, ptMap[pts[i]]);
      }
      cellId = strips->InsertNextCell(ptIds);
      outCD->CopyData(inCD, currentCellId, cellId);
    }
    output->SetStrips(strips);
    strips->Delete();
  }

  // Clean up
  ptIds->Delete();
}

//----------------------------------------------------------------------------
void svtkExtractCellsByType::ExtractUnstructuredGridCells(
  svtkDataSet* inDS, svtkDataSet* outDS, svtkIdType* ptMap, svtkIdType& numNewPts)
{
  svtkUnstructuredGrid* input = svtkUnstructuredGrid::SafeDownCast(inDS);
  svtkCellData* inCD = input->GetCellData();
  svtkUnstructuredGrid* output = svtkUnstructuredGrid::SafeDownCast(outDS);
  svtkCellData* outCD = output->GetCellData();

  svtkIdType numCells = input->GetNumberOfCells();

  // Check for trivial cases: either all in or all out
  if (input->IsHomogeneous())
  {
    if (this->ExtractCellType(input->GetCellType(0)))
    {
      output->ShallowCopy(input);
    }
    else
    {
      output->Initialize();
    }
    return;
  }

  // Mixed collection of cells so simply loop over all cells, copying
  // appropriate types to the output. Along the way keep track of the points
  // that are used.
  svtkIdType i, cellId, newCellId, npts, ptId;
  svtkIdList* ptIds = svtkIdList::New();
  int cellType;
  output->Allocate(numCells);
  for (cellId = 0; cellId < numCells; ++cellId)
  {
    cellType = input->GetCellType(cellId);
    if (this->ExtractCellType(cellType))
    {
      input->GetCellPoints(cellId, ptIds);
      npts = ptIds->GetNumberOfIds();
      for (i = 0; i < npts; ++i)
      {
        ptId = ptIds->GetId(i);
        if (ptMap[ptId] < 0)
        {
          ptMap[ptId] = numNewPts++;
        }
        ptIds->InsertId(i, ptMap[ptId]);
      }
      newCellId = output->InsertNextCell(cellType, ptIds);
      outCD->CopyData(inCD, cellId, newCellId);
    }
  }

  // Clean up
  ptIds->Delete();
}

//----------------------------------------------------------------------------
int svtkExtractCellsByType::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Handle the trivial case
  svtkIdType numCells = input->GetNumberOfCells();
  if (this->CellTypes->empty() || numCells <= 0)
  {
    output->Initialize(); // output is empty
    return 1;
  }

  // Dispatch to appropriate type. This filter does not directly handle
  // composite dataset types, composite types should be looped over by
  // the pipeline executive.
  if (input->GetDataObjectType() == SVTK_POLY_DATA ||
    input->GetDataObjectType() == SVTK_UNSTRUCTURED_GRID)
  {
    this->ExtractUnstructuredData(input, output);
  }

  // Structured data has only one cell type per dataset
  else if (input->GetDataObjectType() == SVTK_IMAGE_DATA ||
    input->GetDataObjectType() == SVTK_STRUCTURED_POINTS ||
    input->GetDataObjectType() == SVTK_RECTILINEAR_GRID ||
    input->GetDataObjectType() == SVTK_STRUCTURED_GRID ||
    input->GetDataObjectType() == SVTK_UNIFORM_GRID ||
    input->GetDataObjectType() == SVTK_HYPER_TREE_GRID)
  {
    if (this->ExtractCellType(input->GetCellType(0)))
    {
      output->ShallowCopy(input);
    }
    else
    {
      output->Initialize(); // output is empty
    }
  }

  else
  {
    svtkErrorMacro("Unknown dataset type");
    output->Initialize(); // output is empty
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractCellsByType::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractCellsByType::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // Output the number of types specified
  os << indent << "Number of types specified: " << this->CellTypes->size() << "\n";
}
