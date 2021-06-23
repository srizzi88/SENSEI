/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGeometryFilter.h"

#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkGenericCell.h"
#include "svtkHexagonalPrism.h"
#include "svtkHexahedron.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPentagonalPrism.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPyramid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkTetra.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVoxel.h"
#include "svtkWedge.h"

svtkStandardNewMacro(svtkGeometryFilter);
svtkCxxSetObjectMacro(svtkGeometryFilter, Locator, svtkIncrementalPointLocator);

//----------------------------------------------------------------------------
// Construct with all types of clipping turned off.
svtkGeometryFilter::svtkGeometryFilter()
{
  this->PointMinimum = 0;
  this->PointMaximum = SVTK_ID_MAX;

  this->CellMinimum = 0;
  this->CellMaximum = SVTK_ID_MAX;

  this->Extent[0] = -SVTK_DOUBLE_MAX;
  this->Extent[1] = SVTK_DOUBLE_MAX;
  this->Extent[2] = -SVTK_DOUBLE_MAX;
  this->Extent[3] = SVTK_DOUBLE_MAX;
  this->Extent[4] = -SVTK_DOUBLE_MAX;
  this->Extent[5] = SVTK_DOUBLE_MAX;

  this->PointClipping = 0;
  this->CellClipping = 0;
  this->ExtentClipping = 0;

  this->Merging = 1;
  this->Locator = nullptr;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
svtkGeometryFilter::~svtkGeometryFilter()
{
  this->SetLocator(nullptr);
}

//----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void svtkGeometryFilter::SetExtent(
  double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
  double extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

//----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void svtkGeometryFilter::SetExtent(double extent[6])
{
  int i;

  if (extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
    extent[2] != this->Extent[2] || extent[3] != this->Extent[3] || extent[4] != this->Extent[4] ||
    extent[5] != this->Extent[5])
  {
    this->Modified();
    for (i = 0; i < 3; i++)
    {
      if (extent[2 * i + 1] < extent[2 * i])
      {
        extent[2 * i + 1] = extent[2 * i];
      }
      this->Extent[2 * i] = extent[2 * i];
      this->Extent[2 * i + 1] = extent[2 * i + 1];
    }
  }
}

void svtkGeometryFilter::SetOutputPointsPrecision(int precision)
{
  if (this->OutputPointsPrecision != precision)
  {
    this->OutputPointsPrecision = precision;
    this->Modified();
  }
}

int svtkGeometryFilter::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//----------------------------------------------------------------------------
int svtkGeometryFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType cellId, newCellId;
  int i, j;
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  char* cellVis;
  svtkGenericCell* cell;
  svtkCell* face;
  double x[3];
  svtkIdList* ptIds;
  svtkIdList* cellIds;
  svtkIdList* pts;
  svtkPoints* newPts;
  svtkIdType ptId;
  int npts;
  svtkIdType pt = 0;
  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  int allVisible;
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  unsigned char* cellGhosts = nullptr;

  if (numCells == 0)
  {
    return 1;
  }

  switch (input->GetDataObjectType())
  {
    case SVTK_POLY_DATA:
      this->PolyDataExecute(input, output);
      return 1;
    case SVTK_UNSTRUCTURED_GRID:
      this->UnstructuredGridExecute(input, output);
      return 1;
    case SVTK_STRUCTURED_GRID:
      this->StructuredGridExecute(input, output, outInfo);
      return 1;
  }

  svtkDataArray* temp = nullptr;
  if (cd)
  {
    temp = cd->GetArray(svtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != SVTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    svtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts = static_cast<svtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  cellIds = svtkIdList::New();
  pts = svtkIdList::New();

  svtkDebugMacro(<< "Executing geometry filter");

  cell = svtkGenericCell::New();

  if ((!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping))
  {
    allVisible = 1;
    cellVis = nullptr;
  }
  else
  {
    allVisible = 0;
    cellVis = new char[numCells];
  }

  // Mark cells as being visible or not
  //
  if (!allVisible)
  {
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        input->GetCell(cellId, cell);
        ptIds = cell->GetPointIds();
        for (i = 0; i < ptIds->GetNumberOfIds(); i++)
        {
          ptId = ptIds->GetId(i);
          input->GetPoint(ptId, x);

          if ((this->PointClipping && (ptId < this->PointMinimum || ptId > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          }
        }
        if (i >= ptIds->GetNumberOfIds())
        {
          cellVis[cellId] = 1;
        }
      }
    }
  }

  // Allocate
  //
  newPts = svtkPoints::New();

  // set precision for the points in the output
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION ||
    this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->Allocate(numPts, numPts / 2);
  output->AllocateEstimate(numCells, 3);
  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(pd, numPts, numPts / 2);
  outputPD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd, numCells, numCells / 2);

  if (this->Merging)
  {
    if (this->Locator == nullptr)
    {
      this->CreateDefaultLocator();
    }
    this->Locator->InitPointInsertion(newPts, input->GetBounds());
  }

  // Traverse cells to extract geometry
  //
  int abort = 0;
  svtkIdType progressInterval = numCells / 20 + 1;
  for (cellId = 0; cellId < numCells && !abort; cellId++)
  {
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts && cellGhosts[cellId] & svtkDataSetAttributes::DUPLICATECELL)
    { // Do not create surfaces in outer ghost cells.
      continue;
    }

    if (allVisible || cellVis[cellId])
    {
      input->GetCell(cellId, cell);
      if (cell->GetCellType() != SVTK_EMPTY_CELL)
      {
        switch (cell->GetCellDimension())
        {
          // create new points and then cell
          case 0:
          case 1:
          case 2:

            npts = cell->GetNumberOfPoints();
            pts->Reset();
            for (i = 0; i < npts; i++)
            {
              ptId = cell->GetPointId(i);
              input->GetPoint(ptId, x);

              if (this->Merging && this->Locator->InsertUniquePoint(x, pt))
              {
                outputPD->CopyData(pd, ptId, pt);
              }
              else if (!this->Merging)
              {
                pt = newPts->InsertNextPoint(x);
                outputPD->CopyData(pd, ptId, pt);
              }
              pts->InsertId(i, pt);
            }
            newCellId = output->InsertNextCell(cell->GetCellType(), pts);
            outputCD->CopyData(cd, cellId, newCellId);
            break;

          case 3:
            int numFaces = cell->GetNumberOfFaces();
            for (j = 0; j < numFaces; j++)
            {
              face = cell->GetFace(j);
              input->GetCellNeighbors(cellId, face->PointIds, cellIds);
              if (cellIds->GetNumberOfIds() <= 0 || (!allVisible && !cellVis[cellIds->GetId(0)]))
              {
                npts = face->GetNumberOfPoints();
                pts->Reset();
                for (i = 0; i < npts; i++)
                {
                  ptId = face->GetPointId(i);
                  input->GetPoint(ptId, x);
                  if (this->Merging && this->Locator->InsertUniquePoint(x, pt))
                  {
                    outputPD->CopyData(pd, ptId, pt);
                  }
                  else if (!this->Merging)
                  {
                    pt = newPts->InsertNextPoint(x);
                    outputPD->CopyData(pd, ptId, pt);
                  }
                  pts->InsertId(i, pt);
                }
                newCellId = output->InsertNextCell(face->GetCellType(), pts);
                outputCD->CopyData(cd, cellId, newCellId);
              }
            }
            break;
        } // switch
      }
    } // if visible
  }   // for all cells

  svtkDebugMacro(<< "Extracted " << newPts->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  cell->Delete();
  output->SetPoints(newPts);
  newPts->Delete();

  // free storage
  if (!this->Merging && this->Locator)
  {
    this->Locator->Initialize();
  }
  output->Squeeze();

  cellIds->Delete();
  pts->Delete();
  delete[] cellVis;

  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of svtkMergePoints is used.
void svtkGeometryFilter::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
int svtkGeometryFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkGeometryFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Point Minimum : " << this->PointMinimum << "\n";
  os << indent << "Point Maximum : " << this->PointMaximum << "\n";

  os << indent << "Cell Minimum : " << this->CellMinimum << "\n";
  os << indent << "Cell Maximum : " << this->CellMaximum << "\n";

  os << indent << "Extent: \n";
  os << indent << "  Xmin,Xmax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "PointClipping: " << (this->PointClipping ? "On\n" : "Off\n");
  os << indent << "CellClipping: " << (this->CellClipping ? "On\n" : "Off\n");
  os << indent << "ExtentClipping: " << (this->ExtentClipping ? "On\n" : "Off\n");

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkGeometryFilter::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//----------------------------------------------------------------------------
void svtkGeometryFilter::PolyDataExecute(svtkDataSet* dataSetInput, svtkPolyData* output)
{
  svtkPolyData* input = static_cast<svtkPolyData*>(dataSetInput);
  svtkIdType cellId;
  int i;
  int allVisible;
  svtkIdType npts;
  const svtkIdType* pts;
  svtkPoints* p = input->GetPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkIdType newCellId, ptId;
  int visible, type;
  double x[3];
  unsigned char* cellGhosts = nullptr;

  svtkDebugMacro(<< "Executing geometry filter for poly data input");

  svtkDataArray* temp = nullptr;
  if (cd)
  {
    temp = cd->GetArray(svtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != SVTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    svtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts = static_cast<svtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  if ((!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping))
  {
    allVisible = 1;
  }
  else
  {
    allVisible = 0;
  }

  if (allVisible) // just pass input to output
  {
    output->CopyStructure(input);
    outputPD->PassData(pd);
    outputCD->PassData(cd);
    return;
  }

  // Always pass point data
  output->SetPoints(p);
  outputPD->PassData(pd);

  // Allocate
  //
  output->AllocateEstimate(numCells, 1);
  outputCD->CopyAllocate(cd, numCells, numCells / 2);
  input->BuildCells(); // needed for GetCellPoints()

  svtkIdType progressInterval = numCells / 20 + 1;
  for (cellId = 0; cellId < numCells; cellId++)
  {
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts && cellGhosts[cellId] & svtkDataSetAttributes::DUPLICATECELL)
    { // Do not create surfaces in outer ghost cells.
      continue;
    }

    input->GetCellPoints(cellId, npts, pts);
    visible = 1;
    if (!allVisible)
    {
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        visible = 0;
      }
      else
      {
        for (i = 0; i < npts; i++)
        {
          ptId = pts[i];
          input->GetPoint(ptId, x);

          if ((this->PointClipping && (ptId < this->PointMinimum || ptId > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            visible = 0;
            break;
          }
        }
      }
    }

    // now if visible extract geometry
    if (allVisible || visible)
    {
      type = input->GetCellType(cellId);
      newCellId = output->InsertNextCell(type, npts, pts);
      outputCD->CopyData(cd, cellId, newCellId);
    } // if visible
  }   // for all cells

  // Update ourselves and release memory
  //
  output->Squeeze();

  svtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");
}

//----------------------------------------------------------------------------
void svtkGeometryFilter::UnstructuredGridExecute(svtkDataSet* dataSetInput, svtkPolyData* output)
{
  svtkUnstructuredGrid* input = static_cast<svtkUnstructuredGrid*>(dataSetInput);
  svtkCellArray* connectivity = input->GetCells();
  if (connectivity == nullptr)
  {
    return;
  }
  auto cellIter = svtk::TakeSmartPointer(connectivity->NewIterator());
  svtkIdType cellId;
  int allVisible;
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  svtkPoints* p = input->GetPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkCellArray *verts, *lines, *polys, *strips;
  svtkIdList *cellIds, *faceIds;
  char* cellVis;
  int faceId, numFacePts;
  const svtkIdType* faceVerts;
  double x[3];
  int pixelConvert[4];
  unsigned char* cellGhosts = nullptr;

  pixelConvert[0] = 0;
  pixelConvert[1] = 1;
  pixelConvert[2] = 3;
  pixelConvert[3] = 2;

  svtkDebugMacro(<< "Executing geometry filter for unstructured grid input");

  svtkDataArray* temp = nullptr;
  if (cd)
  {
    temp = cd->GetArray(svtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != SVTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    svtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts = static_cast<svtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  // Check input
  if (connectivity == nullptr)
  {
    svtkDebugMacro(<< "Nothing to extract");
    return;
  }

  // Determine nature of what we have to do
  cellIds = svtkIdList::New();
  faceIds = svtkIdList::New();
  if ((!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping))
  {
    allVisible = 1;
    cellVis = nullptr;
  }
  else
  {
    allVisible = 0;
    cellVis = new char[numCells];
  }

  // Just pass points through, never merge
  output->SetPoints(input->GetPoints());
  outputPD->PassData(pd);

  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd, numCells, numCells / 2);

  verts = svtkCellArray::New();
  verts->AllocateEstimate(numCells / 4, 1);
  lines = svtkCellArray::New();
  lines->AllocateEstimate(numCells / 4, 1);
  polys = svtkCellArray::New();
  polys->AllocateEstimate(numCells / 4, 1);
  strips = svtkCellArray::New();
  strips->AllocateEstimate(numCells / 4, 1);

  // Loop over the cells determining what's visible
  if (!allVisible)
  {
    for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      cellId = cellIter->GetCurrentCellId();
      cellIter->GetCurrentCell(npts, pts);
      cellVis[cellId] = 1;
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        for (int i = 0; i < npts; i++)
        {
          p->GetPoint(pts[i], x);
          if ((this->PointClipping &&
                (pts[i] < this->PointMinimum || pts[i] > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          } // point/extent clipping
        }   // for each point
      }     // if point clipping needs checking
    }       // for all cells
  }         // if not all visible

  // Used for nonlinear cells only
  svtkNew<svtkGenericCell> cell;
  svtkNew<svtkIdList> ipts;
  svtkNew<svtkPoints> coords;
  svtkNew<svtkIdList> icellIds;

  // These store the cell ids of the input that map to the
  // new vert/line/poly/strip cells, for copying cell data
  // in appropriate order.
  std::vector<svtkIdType> vertCellIds;
  std::vector<svtkIdType> lineCellIds;
  std::vector<svtkIdType> polyCellIds;
  std::vector<svtkIdType> stripCellIds;
  vertCellIds.reserve(numCells);
  lineCellIds.reserve(numCells);
  polyCellIds.reserve(numCells);
  stripCellIds.reserve(numCells);

  // Loop over all cells now that visibility is known
  // (Have to compute visibility first for 3D cell boundaries)
  int progressInterval = numCells / 20 + 1;
  for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    cellId = cellIter->GetCurrentCellId();
    cellIter->GetCurrentCell(npts, pts);
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts && cellGhosts[cellId] & svtkDataSetAttributes::DUPLICATECELL)
    { // Do not create surfaces in outer ghost cells.
      continue;
    }

    if (allVisible || cellVis[cellId]) // now if visible extract geometry
    {
      // special code for nonlinear cells - rarely occurs, so right now it
      // is slow.
      switch (input->GetCellType(cellId))
      {
        case SVTK_EMPTY_CELL:
          break;

        case SVTK_VERTEX:
        case SVTK_POLY_VERTEX:
          verts->InsertNextCell(npts, pts);
          vertCellIds.push_back(cellId);
          break;

        case SVTK_LINE:
        case SVTK_POLY_LINE:
          lines->InsertNextCell(npts, pts);
          lineCellIds.push_back(cellId);
          break;

        case SVTK_TRIANGLE:
        case SVTK_QUAD:
        case SVTK_POLYGON:
          polys->InsertNextCell(npts, pts);
          polyCellIds.push_back(cellId);
          break;

        case SVTK_TRIANGLE_STRIP:
          strips->InsertNextCell(npts, pts);
          stripCellIds.push_back(cellId);
          break;

        case SVTK_PIXEL:
          polys->InsertNextCell(npts);
          // pixelConvert (in the following loop) is an int[4]. GCC 5.1.1
          // warns about pixelConvert[4] being uninitialized due to loop
          // unrolling -- forcibly restricting npts <= 4 prevents this warning.
          if (npts > 4)
          {
            npts = 4;
          }
          for (int i = 0; i < npts; i++)
          {
            polys->InsertCellPoint(pts[pixelConvert[i]]);
          }
          polyCellIds.push_back(cellId);
          break;

        case SVTK_TETRA:
          for (faceId = 0; faceId < 4; faceId++)
          {
            faceIds->Reset();
            faceVerts = svtkTetra::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            numFacePts = 3;
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if (cellIds->GetNumberOfIds() <= 0 || (!allVisible && !cellVis[cellIds->GetId(0)]))
            {
              polys->InsertNextCell(numFacePts);
              for (int i = 0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        case SVTK_VOXEL:
          for (faceId = 0; faceId < 6; faceId++)
          {
            faceIds->Reset();
            faceVerts = svtkVoxel::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            faceIds->InsertNextId(pts[faceVerts[3]]);
            numFacePts = 4;
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if (cellIds->GetNumberOfIds() <= 0 || (!allVisible && !cellVis[cellIds->GetId(0)]))
            {
              polys->InsertNextCell(numFacePts);
              for (int i = 0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[pixelConvert[i]]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        case SVTK_HEXAHEDRON:
          for (faceId = 0; faceId < 6; faceId++)
          {
            faceIds->Reset();
            faceVerts = svtkHexahedron::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            faceIds->InsertNextId(pts[faceVerts[3]]);
            numFacePts = 4;
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if (cellIds->GetNumberOfIds() <= 0 || (!allVisible && !cellVis[cellIds->GetId(0)]))
            {
              polys->InsertNextCell(numFacePts);
              for (int i = 0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        case SVTK_WEDGE:
          for (faceId = 0; faceId < 5; faceId++)
          {
            faceIds->Reset();
            faceVerts = svtkWedge::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            numFacePts = 3;
            if (faceVerts[3] >= 0)
            {
              faceIds->InsertNextId(pts[faceVerts[3]]);
              numFacePts = 4;
            }
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if (cellIds->GetNumberOfIds() <= 0 || (!allVisible && !cellVis[cellIds->GetId(0)]))
            {
              polys->InsertNextCell(numFacePts);
              for (int i = 0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        case SVTK_PYRAMID:
          for (faceId = 0; faceId < 5; faceId++)
          {
            faceIds->Reset();
            faceVerts = svtkPyramid::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            numFacePts = 3;
            if (faceVerts[3] >= 0)
            {
              faceIds->InsertNextId(pts[faceVerts[3]]);
              numFacePts = 4;
            }
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if (cellIds->GetNumberOfIds() <= 0 || (!allVisible && !cellVis[cellIds->GetId(0)]))
            {
              polys->InsertNextCell(numFacePts);
              for (int i = 0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        case SVTK_PENTAGONAL_PRISM:
          for (faceId = 0; faceId < 7; faceId++)
          {
            faceIds->Reset();
            faceVerts = svtkPentagonalPrism::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            faceIds->InsertNextId(pts[faceVerts[3]]);
            numFacePts = 4;
            if (faceVerts[4] >= 0)
            {
              faceIds->InsertNextId(pts[faceVerts[4]]);
              numFacePts = 5;
            }
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if (cellIds->GetNumberOfIds() <= 0 || (!allVisible && !cellVis[cellIds->GetId(0)]))
            {
              polys->InsertNextCell(numFacePts);
              for (int i = 0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        case SVTK_HEXAGONAL_PRISM:
          for (faceId = 0; faceId < 8; faceId++)
          {
            faceIds->Reset();
            faceVerts = svtkHexagonalPrism::GetFaceArray(faceId);
            faceIds->InsertNextId(pts[faceVerts[0]]);
            faceIds->InsertNextId(pts[faceVerts[1]]);
            faceIds->InsertNextId(pts[faceVerts[2]]);
            faceIds->InsertNextId(pts[faceVerts[3]]);
            numFacePts = 4;
            if (faceVerts[4] >= 0)
            {
              faceIds->InsertNextId(pts[faceVerts[4]]);
              faceIds->InsertNextId(pts[faceVerts[5]]);
              numFacePts = 6;
            }
            input->GetCellNeighbors(cellId, faceIds, cellIds);
            if (cellIds->GetNumberOfIds() <= 0 || (!allVisible && !cellVis[cellIds->GetId(0)]))
            {
              polys->InsertNextCell(numFacePts);
              for (int i = 0; i < numFacePts; i++)
              {
                polys->InsertCellPoint(pts[faceVerts[i]]);
              }
              polyCellIds.push_back(cellId);
            }
          }
          break;

        // Quadratic cells
        case SVTK_QUADRATIC_EDGE:
        case SVTK_CUBIC_LINE:
        case SVTK_QUADRATIC_TRIANGLE:
        case SVTK_QUADRATIC_QUAD:
        case SVTK_QUADRATIC_POLYGON:
        case SVTK_QUADRATIC_TETRA:
        case SVTK_QUADRATIC_HEXAHEDRON:
        case SVTK_QUADRATIC_WEDGE:
        case SVTK_QUADRATIC_PYRAMID:
        case SVTK_QUADRATIC_LINEAR_QUAD:
        case SVTK_BIQUADRATIC_TRIANGLE:
        case SVTK_BIQUADRATIC_QUAD:
        case SVTK_TRIQUADRATIC_HEXAHEDRON:
        case SVTK_QUADRATIC_LINEAR_WEDGE:
        case SVTK_BIQUADRATIC_QUADRATIC_WEDGE:
        case SVTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        {
          input->GetCell(cellId, cell);

          if (cell->GetCellDimension() == 1)
          {
            cell->Triangulate(0, ipts, coords);
            for (int i = 0; i < ipts->GetNumberOfIds(); i += 2)
            {
              lines->InsertNextCell(2);
              lines->InsertCellPoint(ipts->GetId(i));
              lines->InsertCellPoint(ipts->GetId(i + 1));
              lineCellIds.push_back(cellId);
            }
          }
          else if (cell->GetCellDimension() == 2)
          {
            cell->Triangulate(0, ipts, coords);
            for (int i = 0; i < ipts->GetNumberOfIds(); i += 3)
            {
              polys->InsertNextCell(3);
              polys->InsertCellPoint(ipts->GetId(i));
              polys->InsertCellPoint(ipts->GetId(i + 1));
              polys->InsertCellPoint(ipts->GetId(i + 2));
              polyCellIds.push_back(cellId);
            }
          }
          else // 3D nonlinear cell
          {
            svtkCell* face;
            for (int j = 0; j < cell->GetNumberOfFaces(); j++)
            {
              face = cell->GetFace(j);
              input->GetCellNeighbors(cellId, face->PointIds, icellIds);
              if (icellIds->GetNumberOfIds() <= 0)
              {
                face->Triangulate(0, ipts, coords);
                for (int i = 0; i < ipts->GetNumberOfIds(); i += 3)
                {
                  polys->InsertNextCell(3);
                  polys->InsertCellPoint(ipts->GetId(i));
                  polys->InsertCellPoint(ipts->GetId(i + 1));
                  polys->InsertCellPoint(ipts->GetId(i + 2));
                  polyCellIds.push_back(cellId);
                }
              }
            }
          } // 3d cell
        }
        break; // done with quadratic cells
      }        // switch
    }          // if visible
  }            // for all cells

  // Update ourselves and release memory
  //
  output->SetVerts(verts);
  verts->Delete();
  output->SetLines(lines);
  lines->Delete();
  output->SetPolys(polys);
  polys->Delete();
  output->SetStrips(strips);
  strips->Delete();

  // Copy the cell data in appropriate order : verts / lines / polys / strips
  size_t offset = 0;
  size_t size = vertCellIds.size();
  for (size_t i = 0; i < size; ++i)
  {
    outputCD->CopyData(cd, vertCellIds[i], static_cast<svtkIdType>(i));
  }
  offset += size;
  size = lineCellIds.size();
  for (size_t i = 0; i < size; ++i)
  {
    outputCD->CopyData(cd, lineCellIds[i], static_cast<svtkIdType>(i + offset));
  }
  offset += size;
  size = polyCellIds.size();
  for (size_t i = 0; i < size; ++i)
  {
    outputCD->CopyData(cd, polyCellIds[i], static_cast<svtkIdType>(i + offset));
  }
  offset += size;
  size = stripCellIds.size();
  for (size_t i = 0; i < size; ++i)
  {
    outputCD->CopyData(cd, stripCellIds[i], static_cast<svtkIdType>(i + offset));
  }

  output->Squeeze();

  svtkDebugMacro(<< "Extracted " << input->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  cellIds->Delete();
  faceIds->Delete();
  delete[] cellVis;
}

//----------------------------------------------------------------------------
void svtkGeometryFilter::StructuredGridExecute(
  svtkDataSet* dataSetInput, svtkPolyData* output, svtkInformation*)
{
  svtkIdType cellId, newCellId;
  int i;
  svtkStructuredGrid* input = static_cast<svtkStructuredGrid*>(dataSetInput);
  svtkIdType numCells = input->GetNumberOfCells();
  std::vector<char> cellVis;
  svtkGenericCell* cell;
  double x[3];
  svtkIdList* ptIds;
  svtkIdList* cellIds;
  svtkIdList* pts;
  svtkIdType ptId;
  const svtkIdType* faceVerts;
  int faceId, numFacePts;
  svtkIdType* facePts;
  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  int allVisible;
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkCellArray* cells;
  unsigned char* cellGhosts = nullptr;

  cellIds = svtkIdList::New();
  pts = svtkIdList::New();

  svtkDebugMacro(<< "Executing geometry filter with structured grid input");

  cell = svtkGenericCell::New();

  svtkDataArray* temp = nullptr;
  if (cd)
  {
    temp = cd->GetArray(svtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != SVTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    svtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    cellGhosts = static_cast<svtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  if ((!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping))
  {
    allVisible = 1;
  }
  else
  {
    allVisible = 0;
    cellVis.resize(numCells);
  }

  // Mark cells as being visible or not
  //
  if (!allVisible)
  {
    for (cellId = 0; cellId < numCells; cellId++)
    {
      cellVis[cellId] = 1;
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        input->GetCell(cellId, cell);
        ptIds = cell->GetPointIds();
        for (i = 0; i < ptIds->GetNumberOfIds(); i++)
        {
          ptId = ptIds->GetId(i);
          input->GetPoint(ptId, x);

          if ((this->PointClipping && (ptId < this->PointMinimum || ptId > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          }
        }
      }
    }
  }

  // Allocate - points are never merged
  //
  output->SetPoints(input->GetPoints());
  outputPD->PassData(pd);
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd, numCells, numCells / 2);

  cells = svtkCellArray::New();
  cells->AllocateEstimate(numCells, 1);

  // Traverse cells to extract geometry
  //
  svtkIdType progressInterval = numCells / 20 + 1;
  for (cellId = 0; cellId < numCells; cellId++)
  {
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
    }

    // Handle ghost cells here.  Another option was used cellVis array.
    if (cellGhosts && cellGhosts[cellId] & svtkDataSetAttributes::DUPLICATECELL)
    { // Do not create surfaces in outer ghost cells.
      continue;
    }

    if ((allVisible || cellVis[cellId]))
    {
      input->GetCell(cellId, cell);
      switch (cell->GetCellDimension())
      {
        // create new points and then cell
        case 0:
        case 1:
        case 2:
          newCellId = cells->InsertNextCell(cell);
          outputCD->CopyData(cd, cellId, newCellId);
          break;

        case 3: // must be hexahedron
          facePts = cell->GetPointIds()->GetPointer(0);
          for (faceId = 0; faceId < 6; faceId++)
          {
            pts->Reset();
            faceVerts = svtkHexahedron::GetFaceArray(faceId);
            pts->InsertNextId(facePts[faceVerts[0]]);
            pts->InsertNextId(facePts[faceVerts[1]]);
            pts->InsertNextId(facePts[faceVerts[2]]);
            pts->InsertNextId(facePts[faceVerts[3]]);
            numFacePts = 4;
            input->GetCellNeighbors(cellId, pts, cellIds);
            if (cellIds->GetNumberOfIds() <= 0 || (!allVisible && !cellVis[cellIds->GetId(0)]))
            {
              newCellId = cells->InsertNextCell(numFacePts);
              for (i = 0; i < numFacePts; i++)
              {
                cells->InsertCellPoint(facePts[faceVerts[i]]);
              }
              outputCD->CopyData(cd, cellId, newCellId);
            }
          }
          break;

      } // switch
    }   // if visible
  }     // for all cells

  switch (input->GetDataDimension())
  {
    case 0:
      output->SetVerts(cells);
      break;
    case 1:
      output->SetLines(cells);
      break;
    case 2:
    case 3:
      output->SetPolys(cells);
  }

  svtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  cells->Delete();
  cell->Delete();
  output->Squeeze();
  cellIds->Delete();
  pts->Delete();
}

//----------------------------------------------------------------------------
int svtkGeometryFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    ++ghostLevels;
  }

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}
