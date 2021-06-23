/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetTriangleFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataSetTriangleFilter.h"

#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkOrderedTriangulator.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkDataSetTriangleFilter);

svtkDataSetTriangleFilter::svtkDataSetTriangleFilter()
{
  this->Triangulator = svtkOrderedTriangulator::New();
  this->Triangulator->PreSortedOff();
  this->Triangulator->UseTemplatesOn();
  this->TetrahedraOnly = 0;
}

svtkDataSetTriangleFilter::~svtkDataSetTriangleFilter()
{
  this->Triangulator->Delete();
  this->Triangulator = nullptr;
}

int svtkDataSetTriangleFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input->IsA("svtkStructuredPoints") || input->IsA("svtkStructuredGrid") ||
    input->IsA("svtkImageData") || input->IsA("svtkRectilinearGrid"))
  {
    this->StructuredExecute(input, output);
  }
  else
  {
    this->UnstructuredExecute(input, output);
  }

  svtkDebugMacro(<< "Produced " << this->GetOutput()->GetNumberOfCells() << " cells");

  return 1;
}

void svtkDataSetTriangleFilter::StructuredExecute(svtkDataSet* input, svtkUnstructuredGrid* output)
{
  int dimensions[3], i, j, k, l, m;
  svtkIdType newCellId, inId;
  svtkCellData* inCD = input->GetCellData();
  svtkCellData* outCD = output->GetCellData();
  svtkPoints* cellPts = svtkPoints::New();
  svtkPoints* newPoints = svtkPoints::New();
  svtkIdList* cellPtIds = svtkIdList::New();
  int numSimplices, numPts, dim, type;
  svtkIdType pts[4], num;

  // Create an array of points. This does an explicit creation
  // of each point.
  num = input->GetNumberOfPoints();
  newPoints->SetNumberOfPoints(num);
  for (i = 0; i < num; ++i)
  {
    newPoints->SetPoint(i, input->GetPoint(i));
  }

  outCD->CopyAllocate(inCD, input->GetNumberOfCells() * 5);
  output->Allocate(input->GetNumberOfCells() * 5);

  if (input->IsA("svtkStructuredPoints"))
  {
    static_cast<svtkStructuredPoints*>(input)->GetDimensions(dimensions);
  }
  else if (input->IsA("svtkStructuredGrid"))
  {
    static_cast<svtkStructuredGrid*>(input)->GetDimensions(dimensions);
  }
  else if (input->IsA("svtkImageData"))
  {
    static_cast<svtkImageData*>(input)->GetDimensions(dimensions);
  }
  else if (input->IsA("svtkRectilinearGrid"))
  {
    static_cast<svtkRectilinearGrid*>(input)->GetDimensions(dimensions);
  }
  else
  {
    // Every kind of structured data is listed above, this should never happen.
    // Report an error and produce no output.
    svtkErrorMacro("Unrecognized data set " << input->GetClassName());
    // Dimensions of 1x1x1 means a single point, i.e. dimensionality of zero.
    dimensions[0] = 1;
    dimensions[1] = 1;
    dimensions[2] = 1;
  }

  dimensions[0] = dimensions[0] - 1;
  dimensions[1] = dimensions[1] - 1;
  dimensions[2] = dimensions[2] - 1;

  svtkIdType numSlices = (dimensions[2] > 0 ? dimensions[2] : 1);
  int abort = 0;
  for (k = 0; k < numSlices && !abort; k++)
  {
    this->UpdateProgress(static_cast<double>(k) / numSlices);
    abort = this->GetAbortExecute();

    for (j = 0; j < dimensions[1]; j++)
    {
      for (i = 0; i < dimensions[0]; i++)
      {
        inId = i + (j + (k * dimensions[1])) * dimensions[0];
        svtkCell* cell = input->GetCell(i, j, k);
        if ((i + j + k) % 2 == 0)
        {
          cell->Triangulate(0, cellPtIds, cellPts);
        }
        else
        {
          cell->Triangulate(1, cellPtIds, cellPts);
        }

        dim = cell->GetCellDimension() + 1;

        numPts = cellPtIds->GetNumberOfIds();
        numSimplices = numPts / dim;
        type = 0;
        switch (dim)
        {
          case 1:
            type = SVTK_VERTEX;
            break;
          case 2:
            type = SVTK_LINE;
            break;
          case 3:
            type = SVTK_TRIANGLE;
            break;
          case 4:
            type = SVTK_TETRA;
            break;
        }
        if (!this->TetrahedraOnly || type == SVTK_TETRA)
        {
          for (l = 0; l < numSimplices; l++)
          {
            for (m = 0; m < dim; m++)
            {
              pts[m] = cellPtIds->GetId(dim * l + m);
            }
            // copy cell data
            newCellId = output->InsertNextCell(type, dim, pts);
            outCD->CopyData(inCD, inId, newCellId);
          } // for all simplices
        }
      } // i dimension
    }   // j dimension
  }     // k dimension

  // Update output
  output->SetPoints(newPoints);
  output->GetPointData()->PassData(input->GetPointData());
  output->Squeeze();

  newPoints->Delete();
  cellPts->Delete();
  cellPtIds->Delete();
}

// 3D cells use the ordered triangulator. The ordered triangulator is used
// to create templates on the fly. Once the templates are created then they
// are used to produce the final triangulation.
//
void svtkDataSetTriangleFilter::UnstructuredExecute(
  svtkDataSet* dataSetInput, svtkUnstructuredGrid* output)
{
  svtkPointSet* input = static_cast<svtkPointSet*>(dataSetInput); // has to be
  svtkIdType numCells = input->GetNumberOfCells();
  svtkGenericCell* cell;
  svtkIdType newCellId, j;
  int k;
  svtkCellData* inCD = input->GetCellData();
  svtkCellData* outCD = output->GetCellData();
  svtkPoints* cellPts;
  svtkIdList* cellPtIds;
  svtkIdType ptId, numTets, ncells;
  int numPts, type;
  int numSimplices, dim;
  svtkIdType pts[4];
  double x[3];

  if (numCells == 0)
  {
    outCD->CopyAllocate(inCD, 0);
    output->GetPointData()->CopyAllocate(input->GetPointData(), 0);
    return;
  }

  svtkUnstructuredGrid* inUgrid = svtkUnstructuredGrid::SafeDownCast(dataSetInput);
  if (inUgrid)
  {
    // avoid doing cell simplification if all cells are already simplices
    svtkUnsignedCharArray* cellTypes = inUgrid->GetCellTypesArray();
    if (cellTypes)
    {
      int allsimplices = 1;
      for (svtkIdType cellId = 0; cellId < cellTypes->GetSize() && allsimplices; cellId++)
      {
        switch (cellTypes->GetValue(cellId))
        {
          case SVTK_TETRA:
            break;
          case SVTK_VERTEX:
          case SVTK_LINE:
          case SVTK_TRIANGLE:
            if (this->TetrahedraOnly)
            {
              allsimplices = 0; // don't shallowcopy need to stip non tets
            }
            break;
          default:
            allsimplices = 0;
            break;
        }
      }
      if (allsimplices)
      {
        output->ShallowCopy(input);
        return;
      }
    }
  }

  cell = svtkGenericCell::New();
  cellPts = svtkPoints::New();
  cellPtIds = svtkIdList::New();

  // Create an array of points
  svtkCellData* tempCD = svtkCellData::New();
  tempCD->ShallowCopy(inCD);
  tempCD->SetActiveGlobalIds(nullptr);

  outCD->CopyAllocate(tempCD, input->GetNumberOfCells() * 5);
  output->Allocate(input->GetNumberOfCells() * 5);

  // Points are passed through
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());

  int abort = 0;
  svtkIdType updateTime = numCells / 20 + 1; // update roughly every 5%
  for (svtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
  {
    if (!(cellId % updateTime))
    {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    input->GetCell(cellId, cell);
    dim = cell->GetCellDimension();

    if (cell->GetCellType() == SVTK_POLYHEDRON) // polyhedron
    {
      dim = 4;
      cell->Triangulate(0, cellPtIds, cellPts);
      numPts = cellPtIds->GetNumberOfIds();

      numSimplices = numPts / dim;
      type = SVTK_TETRA;

      for (j = 0; j < numSimplices; j++)
      {
        for (k = 0; k < dim; k++)
        {
          pts[k] = cellPtIds->GetId(dim * j + k);
        }
        // copy cell data
        newCellId = output->InsertNextCell(type, dim, pts);
        outCD->CopyData(tempCD, cellId, newCellId);
      }
    }

    else if (dim == 3) // use ordered triangulation
    {
      numPts = cell->GetNumberOfPoints();
      double *p, *pPtr = cell->GetParametricCoords();
      this->Triangulator->InitTriangulation(0.0, 1.0, 0.0, 1.0, 0.0, 1.0, numPts);
      for (p = pPtr, j = 0; j < numPts; j++, p += 3)
      {
        // the wedge is "flipped" compared to other cells in that
        // the normal of the first face points out instead of in
        // so we flip the way we pass the points to the triangulator
        const svtkIdType wedgemap[18] = { 3, 4, 5, 0, 1, 2, 9, 10, 11, 6, 7, 8, 12, 13, 14, 15, 16,
          17 };
        type = cell->GetCellType();
        if (type == SVTK_WEDGE || type == SVTK_QUADRATIC_WEDGE ||
          type == SVTK_QUADRATIC_LINEAR_WEDGE || type == SVTK_BIQUADRATIC_QUADRATIC_WEDGE)
        {
          ptId = cell->PointIds->GetId(wedgemap[j]);
          cell->Points->GetPoint(wedgemap[j], x);
        }
        else
        {
          ptId = cell->PointIds->GetId(j);
          cell->Points->GetPoint(j, x);
        }
        this->Triangulator->InsertPoint(ptId, x, p, 0);
      }                          // for all cell points
      if (cell->IsPrimaryCell()) // use templates if topology is fixed
      {
        int numEdges = cell->GetNumberOfEdges();
        this->Triangulator->TemplateTriangulate(cell->GetCellType(), numPts, numEdges);
      }
      else // use ordered triangulator
      {
        this->Triangulator->Triangulate();
      }

      ncells = output->GetNumberOfCells();
      numTets = this->Triangulator->AddTetras(0, output);

      for (j = 0; j < numTets; j++)
      {
        outCD->CopyData(tempCD, cellId, ncells + j);
      }
    }

    else if (!this->TetrahedraOnly) // 2D or lower dimension
    {
      dim++;
      cell->Triangulate(0, cellPtIds, cellPts);
      numPts = cellPtIds->GetNumberOfIds();

      numSimplices = numPts / dim;
      type = 0;
      switch (dim)
      {
        case 1:
          type = SVTK_VERTEX;
          break;
        case 2:
          type = SVTK_LINE;
          break;
        case 3:
          type = SVTK_TRIANGLE;
          break;
      }

      for (j = 0; j < numSimplices; j++)
      {
        for (k = 0; k < dim; k++)
        {
          pts[k] = cellPtIds->GetId(dim * j + k);
        }
        // copy cell data
        newCellId = output->InsertNextCell(type, dim, pts);
        outCD->CopyData(tempCD, cellId, newCellId);
      }
    } // if 2D or less cell
  }   // for all cells

  // Update output
  output->Squeeze();

  tempCD->Delete();

  cellPts->Delete();
  cellPtIds->Delete();
  cell->Delete();
}

int svtkDataSetTriangleFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

void svtkDataSetTriangleFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TetrahedraOnly: " << (this->TetrahedraOnly ? "On" : "Off") << "\n";
}
