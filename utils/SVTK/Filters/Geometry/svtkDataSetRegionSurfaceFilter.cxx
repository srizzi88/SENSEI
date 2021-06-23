//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "svtkDataSetRegionSurfaceFilter.h"

#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkCharArray.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridGeometryFilter.h"

#include <map>

class svtkDataSetRegionSurfaceFilter::Internals
{
public:
  Internals()
    : NextRegion(0)
  {
    this->OldToNew[-1] = -1;
  };
  ~Internals() = default;

  // place to pass a material id back but still subclass
  int NextRegion;

  // pair entries are two materials that a polygon bounds (-1 if external)
  // content is index into output material array for this pair
  std::map<std::pair<int, int>, int> NewRegions;

  // map old material ids into new locations
  std::map<int, int> OldToNew;
};
svtkStandardNewMacro(svtkDataSetRegionSurfaceFilter);

//----------------------------------------------------------------------------
svtkDataSetRegionSurfaceFilter::svtkDataSetRegionSurfaceFilter()
{
  this->RegionArray = nullptr;
  this->RegionArrayName = nullptr;
  this->SetRegionArrayName("material");
  this->MaterialPropertiesName = nullptr;
  this->SetMaterialPropertiesName("material_properties");
  this->MaterialIDsName = nullptr;
  this->SetMaterialIDsName("material_ids");
  this->MaterialPIDsName = nullptr;
  this->SetMaterialPIDsName("material_ancestors");
  this->InterfaceIDsName = nullptr;
  this->SetInterfaceIDsName("interface_ids");
  this->OrigCellIds = svtkIdTypeArray::New();
  this->OrigCellIds->SetName("OrigCellIds");
  this->OrigCellIds->SetNumberOfComponents(1);
  this->CellFaceIds = svtkCharArray::New();
  this->CellFaceIds->SetName("CellFaceIds");
  this->CellFaceIds->SetNumberOfComponents(1);
  this->Internal = new svtkDataSetRegionSurfaceFilter::Internals;
  this->SingleSided = true;
}

//----------------------------------------------------------------------------
svtkDataSetRegionSurfaceFilter::~svtkDataSetRegionSurfaceFilter()
{
  this->SetRegionArrayName(nullptr);
  this->SetMaterialPropertiesName(nullptr);
  this->SetMaterialIDsName(nullptr);
  this->SetMaterialPIDsName(nullptr);
  this->SetInterfaceIDsName(nullptr);
  this->OrigCellIds->Delete();
  this->CellFaceIds->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
int svtkDataSetRegionSurfaceFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkDataSetRegionSurfaceFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkUnstructuredGrid* input = svtkUnstructuredGrid::GetData(inputVector[0], 0);

  if (!input)
  {
    svtkErrorMacro("Input not specified!");
    return 0;
  }

  if (this->RegionArrayName)
  {
    this->RegionArray =
      svtkIntArray::SafeDownCast(input->GetCellData()->GetArray(this->RegionArrayName));
  }

  // assume all tets, and that the tets are small relative to the size of the
  // regions (absolute max number of faces in output would be
  // input->GetNumberOfCells() * 4)
  this->OrigCellIds->Reset();
  this->OrigCellIds->Allocate(input->GetNumberOfCells());
  this->CellFaceIds->Reset();
  this->CellFaceIds->Allocate(input->GetNumberOfCells());

  this->Superclass::RequestData(request, inputVector, outputVector);

  // if any tets, then we'll have CellFaceIds... assume all tets and that we
  // want to addthe cell data
  if (this->CellFaceIds->GetNumberOfTuples() > 0)
  {
    svtkPolyData* output = svtkPolyData::GetData(outputVector, 0);
    if (output->GetNumberOfCells() != this->CellFaceIds->GetNumberOfTuples())
    {
      svtkErrorMacro("Unable to add CellData because wrong # of values!");
    }
    else
    {
      output->GetCellData()->AddArray(this->OrigCellIds);
      output->GetCellData()->AddArray(this->CellFaceIds);
    }
  }
  else
  { // get rid of point mapping information
    svtkPolyData* output = svtkPolyData::GetData(outputVector, 0);
    output->GetPointData()->RemoveArray("svtkOriginalPointIds");
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkDataSetRegionSurfaceFilter::UnstructuredGridExecute(
  svtkDataSet* dataSetInput, svtkPolyData* output)
{
  svtkUnstructuredGrid* input = svtkUnstructuredGrid::SafeDownCast(dataSetInput);

  // Before we start doing anything interesting, check if we need handle
  // non-linear cells using sub-division.
  bool handleSubdivision = false;
  if (this->NonlinearSubdivisionLevel >= 1)
  {
    // Check to see if the data actually has nonlinear cells.  Handling
    // nonlinear cells adds unnecessary work if we only have linear cells.
    svtkIdType numCells = input->GetNumberOfCells();
    unsigned char* cellTypes = input->GetCellTypesArray()->GetPointer(0);
    for (svtkIdType i = 0; i < numCells; i++)
    {
      if (!svtkCellTypes::IsLinear(cellTypes[i]))
      {
        handleSubdivision = true;
        break;
      }
    }
  }

  svtkSmartPointer<svtkUnstructuredGrid> tempInput;
  if (handleSubdivision)
  {
    // Since this filter only properly subdivides 2D cells past
    // level 1, we convert 3D cells to 2D by using
    // svtkUnstructuredGridGeometryFilter.
    svtkNew<svtkUnstructuredGridGeometryFilter> uggf;
    svtkNew<svtkUnstructuredGrid> clone;
    clone->ShallowCopy(input);
    uggf->SetInputData(clone);
    uggf->SetPassThroughCellIds(this->PassThroughCellIds);
    uggf->SetPassThroughPointIds(this->PassThroughPointIds);
    uggf->Update();

    tempInput = svtkSmartPointer<svtkUnstructuredGrid>::New();
    tempInput->ShallowCopy(uggf->GetOutputDataObject(0));
    input = tempInput;
  }

  svtkCellArray* newVerts;
  svtkCellArray* newLines;
  svtkCellArray* newPolys;
  svtkPoints* newPts;
  const svtkIdType* ids;
  int progressCount;
  int i, j;
  int cellType;
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkGenericCell* cell;
  int numFacePts;
  svtkIdType numCellPts;
  svtkIdType inPtId, outPtId;
  svtkPointData* inputPD = input->GetPointData();
  svtkCellData* inputCD = input->GetCellData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkFastGeomQuad* q;
  unsigned char* cellTypes = input->GetCellTypesArray()->GetPointer(0);

  // These are for the default case/
  svtkIdList* pts;
  svtkPoints* coords;
  svtkCell* face;
  int flag2D = 0;

  // These are for subdividing quadratic cells
  svtkDoubleArray* parametricCoords;
  svtkDoubleArray* parametricCoords2;
  svtkIdList* outPts;
  svtkIdList* outPts2;

  pts = svtkIdList::New();
  coords = svtkPoints::New();
  parametricCoords = svtkDoubleArray::New();
  parametricCoords2 = svtkDoubleArray::New();
  outPts = svtkIdList::New();
  outPts2 = svtkIdList::New();
  // might not be necessary to set the data type for coords
  // but certainly safer to do so
  coords->SetDataType(input->GetPoints()->GetData()->GetDataType());
  cell = svtkGenericCell::New();

  this->NumberOfNewCells = 0;
  this->InitializeQuadHash(numPts);

  // Allocate
  //
  newPts = svtkPoints::New();
  newPts->SetDataType(input->GetPoints()->GetData()->GetDataType());
  newPts->Allocate(numPts);
  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(numCells, 3);
  newVerts = svtkCellArray::New();
  newLines = svtkCellArray::New();

  if (this->NonlinearSubdivisionLevel < 2)
  {
    outputPD->CopyGlobalIdsOn();
    outputPD->CopyAllocate(inputPD, numPts, numPts / 2);
  }
  else
  {
    outputPD->InterpolateAllocate(inputPD, numPts, numPts / 2);
  }
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(inputCD, numCells, numCells / 2);

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = svtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = svtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
  }

  // First insert all points.  Points have to come first in poly data.
  auto cellIter = svtkSmartPointer<svtkCellArrayIterator>::Take(input->GetCells()->NewIterator());
  for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    const svtkIdType cellId = cellIter->GetCurrentCellId();
    cellIter->GetCurrentCell(numCellPts, ids);

    cellType = cellTypes[cellId];

    // A couple of common cases to see if things go faster.
    if (cellType == SVTK_VERTEX || cellType == SVTK_POLY_VERTEX)
    {
      newVerts->InsertNextCell(numCellPts);
      for (i = 0; i < numCellPts; ++i)
      {
        inPtId = ids[i];
        outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
        newVerts->InsertCellPoint(outPtId);
      }
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
  }

  // Traverse cells to extract geometry
  //
  progressCount = 0;
  int abort = 0;
  svtkIdType progressInterval = numCells / 20 + 1;

  // First insert all points lines in output and 3D geometry in hash.
  // Save 2D geometry for second pass.
  // initialize the pointer to the cells for fast traversal.
  for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal() && !abort;
       cellIter->GoToNextCell())
  {
    const svtkIdType cellId = cellIter->GetCurrentCellId();

    // Progress and abort method support
    if (progressCount >= progressInterval)
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
      progressCount = 0;
    }
    progressCount++;

    cellIter->GetCurrentCell(numCellPts, ids);
    cellType = cellTypes[cellId];

    // A couple of common cases to see if things go faster.
    if (cellType == SVTK_VERTEX || cellType == SVTK_POLY_VERTEX)
    {
      // Do nothing.  This case was handled in the previous loop.
    }
    else if (cellType == SVTK_LINE || cellType == SVTK_POLY_LINE)
    {
      newLines->InsertNextCell(numCellPts);
      for (i = 0; i < numCellPts; ++i)
      {
        inPtId = ids[i];
        outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
        newLines->InsertCellPoint(outPtId);
      }
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
    else if (cellType == SVTK_HEXAHEDRON)
    {
      this->InsertQuadInHash(ids[0], ids[1], ids[5], ids[4], cellId, 2);
      this->InsertQuadInHash(ids[0], ids[3], ids[2], ids[1], cellId, 4);
      this->InsertQuadInHash(ids[0], ids[4], ids[7], ids[3], cellId, 0);
      this->InsertQuadInHash(ids[1], ids[2], ids[6], ids[5], cellId, 1);
      this->InsertQuadInHash(ids[2], ids[3], ids[7], ids[6], cellId, 3);
      this->InsertQuadInHash(ids[4], ids[5], ids[6], ids[7], cellId, 5);
    }
    else if (cellType == SVTK_VOXEL)
    {
      this->InsertQuadInHash(ids[0], ids[1], ids[5], ids[4], cellId, 2);
      this->InsertQuadInHash(ids[0], ids[2], ids[3], ids[1], cellId, 4);
      this->InsertQuadInHash(ids[0], ids[4], ids[6], ids[2], cellId, 0);
      this->InsertQuadInHash(ids[1], ids[3], ids[7], ids[5], cellId, 1);
      this->InsertQuadInHash(ids[2], ids[6], ids[7], ids[3], cellId, 3);
      this->InsertQuadInHash(ids[4], ids[5], ids[7], ids[6], cellId, 5);
    }
    else if (cellType == SVTK_TETRA)
    {
      this->InsertTriInHash(ids[0], ids[1], ids[2], cellId, 3);
      this->InsertTriInHash(ids[0], ids[1], ids[3], cellId, 0);
      this->InsertTriInHash(ids[0], ids[2], ids[3], cellId, 2);
      this->InsertTriInHash(ids[1], ids[2], ids[3], cellId, 1);
    }
    else if (cellType == SVTK_PENTAGONAL_PRISM)
    {
      // The quads :
      this->InsertQuadInHash(ids[0], ids[1], ids[6], ids[5], cellId, 2);
      this->InsertQuadInHash(ids[1], ids[2], ids[7], ids[6], cellId, 3);
      this->InsertQuadInHash(ids[2], ids[3], ids[8], ids[7], cellId, 4);
      this->InsertQuadInHash(ids[3], ids[4], ids[9], ids[8], cellId, 5);
      this->InsertQuadInHash(ids[4], ids[0], ids[5], ids[9], cellId, 6);
      this->InsertPolygonInHash(ids, 5, cellId);
      this->InsertPolygonInHash(&ids[5], 5, cellId);
    }
    else if (cellType == SVTK_HEXAGONAL_PRISM)
    {
      // The quads :

      this->InsertQuadInHash(ids[0], ids[1], ids[7], ids[6], cellId, 2);
      this->InsertQuadInHash(ids[1], ids[2], ids[8], ids[7], cellId, 3);
      this->InsertQuadInHash(ids[2], ids[3], ids[9], ids[8], cellId, 4);
      this->InsertQuadInHash(ids[3], ids[4], ids[10], ids[9], cellId, 5);
      this->InsertQuadInHash(ids[4], ids[5], ids[11], ids[10], cellId, 6);
      this->InsertQuadInHash(ids[5], ids[0], ids[6], ids[11], cellId, 7);
      this->InsertPolygonInHash(ids, 6, cellId);
      this->InsertPolygonInHash(&ids[6], 6, cellId);
    }
    else if (cellType == SVTK_PIXEL || cellType == SVTK_QUAD || cellType == SVTK_TRIANGLE ||
      cellType == SVTK_POLYGON || cellType == SVTK_TRIANGLE_STRIP ||
      cellType == SVTK_QUADRATIC_TRIANGLE || cellType == SVTK_BIQUADRATIC_TRIANGLE ||
      cellType == SVTK_QUADRATIC_QUAD || cellType == SVTK_QUADRATIC_LINEAR_QUAD ||
      cellType == SVTK_BIQUADRATIC_QUAD)
    { // save 2D cells for second pass
      flag2D = 1;
    }
    else
    // Default way of getting faces. Differentiates between linear
    // and higher order cells.
    {
      input->GetCell(cellId, cell);
      if (cell->IsLinear())
      {
        if (cell->GetCellDimension() == 3)
        {
          int numFaces = cell->GetNumberOfFaces();
          for (j = 0; j < numFaces; j++)
          {
            face = cell->GetFace(j);
            numFacePts = face->GetNumberOfPoints();
            if (numFacePts == 4)
            {
              this->InsertQuadInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                face->PointIds->GetId(2), face->PointIds->GetId(3), cellId, j);
            }
            else if (numFacePts == 3)
            {
              this->InsertTriInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                face->PointIds->GetId(2), cellId, j);
            }
            else
            {
              this->InsertPolygonInHash(
                face->PointIds->GetPointer(0), face->PointIds->GetNumberOfIds(), cellId);
            }
          } // for all cell faces
        }   // if 3D
        else
        {
          svtkDebugMacro("Missing cell type.");
        }
      } // a linear cell type

      else // process nonlinear cells via triangulation
      {
        if (cell->GetCellDimension() == 1)
        {
          cell->Triangulate(0, pts, coords);
          for (i = 0; i < pts->GetNumberOfIds(); i += 2)
          {
            newLines->InsertNextCell(2);
            inPtId = pts->GetId(i);
            this->RecordOrigCellId(this->NumberOfNewCells, cellId);
            outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
            outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
            newLines->InsertCellPoint(outPtId);
            inPtId = pts->GetId(i + 1);
            outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
            newLines->InsertCellPoint(outPtId);
          }
        }
        else if (cell->GetCellDimension() == 2)
        {
          svtkWarningMacro(<< "2-D nonlinear cells must be processed with all other 2-D cells.");
        }
        else // 3D nonlinear cell
        {
          svtkIdList* cellIds = svtkIdList::New();
          int numFaces = cell->GetNumberOfFaces();
          for (j = 0; j < numFaces; j++)
          {
            face = cell->GetFace(j);
            input->GetCellNeighbors(cellId, face->PointIds, cellIds);
            if (cellIds->GetNumberOfIds() <= 0)
            {
              // FIXME: Face could not be consistent. svtkOrderedTriangulator is a better option
              if (this->NonlinearSubdivisionLevel >= 1)
              {
                // TODO: Handle NonlinearSubdivisionLevel > 1 correctly.
                face->Triangulate(0, pts, coords);
                for (i = 0; i < pts->GetNumberOfIds(); i += 3)
                {
                  this->InsertTriInHash(
                    pts->GetId(i), pts->GetId(i + 1), pts->GetId(i + 2), cellId, j);
                }
              }
              else
              {
                switch (face->GetCellType())
                {
                  case SVTK_QUADRATIC_TRIANGLE:
                    this->InsertTriInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                      face->PointIds->GetId(2), cellId, j);
                    break;
                  case SVTK_QUADRATIC_QUAD:
                  case SVTK_BIQUADRATIC_QUAD:
                  case SVTK_QUADRATIC_LINEAR_QUAD:
                    this->InsertQuadInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                      face->PointIds->GetId(2), face->PointIds->GetId(3), cellId, j);
                    break;
                  default:
                    svtkErrorMacro(<< "Encountered unknown nonlinear face.");
                    break;
                } // switch cell type
              }   // subdivision level
            }     // cell has ids
          }       // for faces
          cellIds->Delete();
        } // 3d cell
      }   // nonlinear cell
    }     // Cell type else.
  }       // for all cells.

  // It would be possible to add these (except for polygons with 5+ sides)
  // to the hashes.  Alternatively, the higher order 2d cells could be handled
  // in the following loop.

  // Now insert 2DCells.  Because of poly datas (cell data) ordering,
  // the 2D cells have to come after points and lines.
  // initialize the pointer to the cells for fast traversal.
  for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal() && !abort && flag2D;
       cellIter->GoToNextCell())
  {
    const svtkIdType cellId = cellIter->GetCurrentCellId();
    cellIter->GetCurrentCell(numCellPts, ids);
    cellType = input->GetCellType(cellId);

    // If we have a quadratic face and our subdivision level is zero, just treat
    // it as a linear cell.  This should work so long as the first points of the
    // quadratic cell correspond to all those of the equivalent linear cell
    // (which all the current definitions do).
    if (this->NonlinearSubdivisionLevel < 1)
    {
      switch (cellType)
      {
        case SVTK_QUADRATIC_TRIANGLE:
          cellType = SVTK_TRIANGLE;
          numCellPts = 3;
          break;
        case SVTK_QUADRATIC_QUAD:
        case SVTK_BIQUADRATIC_QUAD:
        case SVTK_QUADRATIC_LINEAR_QUAD:
          cellType = SVTK_POLYGON;
          numCellPts = 4;
          break;
      }
    }

    // A couple of common cases to see if things go faster.
    if (cellType == SVTK_PIXEL)
    { // Do we really want to insert the 2D cells into a hash?
      pts->Reset();
      pts->InsertId(0, this->GetOutputPointId(ids[0], input, newPts, outputPD));
      pts->InsertId(1, this->GetOutputPointId(ids[1], input, newPts, outputPD));
      pts->InsertId(2, this->GetOutputPointId(ids[3], input, newPts, outputPD));
      pts->InsertId(3, this->GetOutputPointId(ids[2], input, newPts, outputPD));
      newPolys->InsertNextCell(pts);
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
    else if (cellType == SVTK_POLYGON || cellType == SVTK_TRIANGLE || cellType == SVTK_QUAD)
    {
      pts->Reset();
      for (i = 0; i < numCellPts; i++)
      {
        inPtId = ids[i];
        outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
        pts->InsertId(i, outPtId);
      }
      newPolys->InsertNextCell(pts);
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
    else if (cellType == SVTK_TRIANGLE_STRIP)
    {
      // Change strips to triangles so we do not have to worry about order.
      int toggle = 0;
      svtkIdType ptIds[3];
      // This check is not really necessary.  It was put here because of another (now fixed) bug.
      if (numCellPts > 1)
      {
        ptIds[0] = this->GetOutputPointId(ids[0], input, newPts, outputPD);
        ptIds[1] = this->GetOutputPointId(ids[1], input, newPts, outputPD);
        for (i = 2; i < numCellPts; ++i)
        {
          ptIds[2] = this->GetOutputPointId(ids[i], input, newPts, outputPD);
          newPolys->InsertNextCell(3, ptIds);
          this->RecordOrigCellId(this->NumberOfNewCells, cellId);
          outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
          ptIds[toggle] = ptIds[2];
          toggle = !toggle;
        }
      }
    }
    else if (cellType == SVTK_QUADRATIC_TRIANGLE || cellType == SVTK_BIQUADRATIC_TRIANGLE ||
      cellType == SVTK_QUADRATIC_QUAD || cellType == SVTK_BIQUADRATIC_QUAD ||
      cellType == SVTK_QUADRATIC_LINEAR_QUAD)
    {
      // Note: we should not be here if this->NonlinearSubdivisionLevel is less
      // than 1.  See the check above.
      input->GetCell(cellId, cell);
      cell->Triangulate(0, pts, coords);
      // Copy the level 1 subdivision points (which also exist in the input and
      // can therefore just be copied over.  Note that the output of Triangulate
      // records triangles in pts where each 3 points defines a triangle.  We
      // will keep this invariant and also keep the same invariant in
      // parametericCoords and outPts later.
      outPts->Reset();
      for (i = 0; i < pts->GetNumberOfIds(); i++)
      {
        svtkIdType op;
        op = this->GetOutputPointId(pts->GetId(i), input, newPts, outputPD);
        outPts->InsertNextId(op);
      }
      // Do any further subdivision if necessary.
      if (this->NonlinearSubdivisionLevel > 1)
      {
        // We are going to need parametric coordinates to further subdivide.
        double* pc = cell->GetParametricCoords();
        parametricCoords->Reset();
        parametricCoords->SetNumberOfComponents(3);
        for (i = 0; i < pts->GetNumberOfIds(); i++)
        {
          svtkIdType ptId = pts->GetId(i);
          svtkIdType cellPtId;
          for (cellPtId = 0; cell->GetPointId(cellPtId) != ptId; cellPtId++)
          {
          }
          parametricCoords->InsertNextTypedTuple(pc + 3 * cellPtId);
        }
        // Subdivide these triangles as many more times as necessary.  Remember
        // that we have already done the first subdivision.
        for (j = 1; j < this->NonlinearSubdivisionLevel; j++)
        {
          parametricCoords2->Reset();
          parametricCoords2->SetNumberOfComponents(3);
          outPts2->Reset();
          // Each triangle will be split into 4 triangles.
          for (i = 0; i < outPts->GetNumberOfIds(); i += 3)
          {
            // Hold the input point ids and parametric coordinates.  First 3
            // indices are the original points.  Second three are the midpoints
            // in the edges (0,1), (1,2) and (2,0), respectively (see comment
            // below).
            svtkIdType inPts[6];
            double inParamCoords[6][3];
            int k;
            for (k = 0; k < 3; k++)
            {
              inPts[k] = outPts->GetId(i + k);
              parametricCoords->GetTypedTuple(i + k, inParamCoords[k]);
            }
            for (k = 3; k < 6; k++)
            {
              int pt1 = k - 3;
              int pt2 = (pt1 < 2) ? (pt1 + 1) : 0;
              inParamCoords[k][0] = 0.5 * (inParamCoords[pt1][0] + inParamCoords[pt2][0]);
              inParamCoords[k][1] = 0.5 * (inParamCoords[pt1][1] + inParamCoords[pt2][1]);
              inParamCoords[k][2] = 0.5 * (inParamCoords[pt1][2] + inParamCoords[pt2][2]);
              inPts[k] = GetInterpolatedPointId(
                inPts[pt1], inPts[pt2], input, cell, inParamCoords[k], newPts, outputPD);
            }
            //       * 0
            //      / \        Use the 6 points recorded
            //     /   \       in inPts and inParamCoords
            //  3 *-----* 5    to create the 4 triangles
            //   / \   / \     shown here.
            //  /   \ /   \    .
            // *-----*-----*
            // 1     4     2
            const int subtriangles[12] = { 0, 3, 5, 3, 1, 4, 3, 4, 5, 5, 4, 2 };
            for (k = 0; k < 12; k++)
            {
              int localId = subtriangles[k];
              outPts2->InsertNextId(inPts[localId]);
              parametricCoords2->InsertNextTypedTuple(inParamCoords[localId]);
            }
          } // Iterate over triangles
          // Now that we have recorded the subdivided triangles in outPts2 and
          // parametricCoords2, swap them with outPts and parametricCoords to
          // make them the current ones.
          std::swap(outPts, outPts2);
          std::swap(parametricCoords, parametricCoords2);
        } // Iterate over subdivision levels
      }   // If further subdivision

      // Now that we have done all the subdivisions and created all of the
      // points, record the triangles.
      for (i = 0; i < outPts->GetNumberOfIds(); i += 3)
      {
        newPolys->InsertNextCell(3, outPts->GetPointer(i));
        this->RecordOrigCellId(this->NumberOfNewCells, cellId);
        outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
      }
    }
  } // for all cells.

  // Now transfer geometry from hash to output (only triangles and quads).
  this->InitQuadHashTraversal();
  svtkIntArray* outRegionArray = nullptr;
  if (this->RegionArrayName)
  {
    outRegionArray = svtkIntArray::SafeDownCast(outputCD->GetArray(this->RegionArrayName));
  }
  while ((q = this->GetNextVisibleQuadFromHash()))
  {
    // handle all polys
    for (i = 0; i < q->numPts; i++)
    {
      q->ptArray[i] = this->GetOutputPointId(q->ptArray[i], input, newPts, outputPD);
    }
    newPolys->InsertNextCell(q->numPts, q->ptArray);
    this->RecordOrigCellId(this->NumberOfNewCells, q);
    outputCD->CopyData(inputCD, q->SourceId, this->NumberOfNewCells);
    if (outRegionArray)
    {
      outRegionArray->SetValue(this->NumberOfNewCells, this->Internal->NextRegion);
    }
    this->NumberOfNewCells++;
  }

  if (this->PassThroughCellIds)
  {
    outputCD->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    outputPD->AddArray(this->OriginalPointIds);
  }

  // wrangle materials
  if (outRegionArray)
  {
    int nummats = static_cast<int>(this->Internal->NewRegions.size());

    // place to keep track of two parent materials
    svtkIntArray* outMatPIDs = svtkIntArray::New();
    outMatPIDs->SetName(this->GetMaterialPIDsName());
    outMatPIDs->SetNumberOfComponents(2);
    outMatPIDs->SetNumberOfTuples(nummats);
    output->GetFieldData()->AddArray(outMatPIDs);
    outMatPIDs->Delete();

    // place to copy or construct material specifications
    svtkStringArray* outMaterialSpecs = nullptr;
    svtkStringArray* inMaterialSpecs = svtkStringArray::SafeDownCast(
      input->GetFieldData()->GetAbstractArray(this->GetMaterialPropertiesName()));
    if (inMaterialSpecs)
    {
      outMaterialSpecs = svtkStringArray::New();
      outMaterialSpecs->SetName(this->GetMaterialPropertiesName());
      outMaterialSpecs->SetNumberOfComponents(1);
      outMaterialSpecs->SetNumberOfTuples(nummats);
      output->GetFieldData()->AddArray(outMaterialSpecs);
      outMaterialSpecs->Delete();
    }

    // indices into material specifications
    svtkIntArray* outMaterialIDs = svtkIntArray::New();
    outMaterialIDs->SetName(this->GetMaterialIDsName());
    outMaterialIDs->SetNumberOfComponents(1);
    outMaterialIDs->SetNumberOfTuples(nummats);
    output->GetFieldData()->AddArray(outMaterialIDs);
    outMaterialIDs->Delete();
    svtkIntArray* inMaterialIDs =
      svtkIntArray::SafeDownCast(input->GetFieldData()->GetArray(this->GetMaterialIDsName()));
    // make a map for quick lookup of material spec for each material later
    std::map<int, int> reverseids;
    if (inMaterialIDs && inMaterialSpecs)
    {
      for (i = 0; i < inMaterialSpecs->GetNumberOfTuples(); i++)
      {
        reverseids[inMaterialIDs->GetValue(i)] = i;
      }
    }
    else
    {
      if (inMaterialSpecs)
      {
        for (i = 0; i < inMaterialSpecs->GetNumberOfTuples(); i++)
        {
          reverseids[i] = i;
        }
      }
    }

    // go through all the materials we've made
    std::map<std::pair<int, int>, int>::iterator it;
    for (it = this->Internal->NewRegions.begin(); it != this->Internal->NewRegions.end(); ++it)
    {
      int index = it->second;
      outMaterialIDs->SetValue(index, index);

      // keep record of parents
      int pid0_orig = it->first.first;
      int pid0 = this->Internal->OldToNew[pid0_orig];
      int pid1 = this->Internal->OldToNew[it->first.second];
      outMatPIDs->SetTuple2(index, pid0, pid1);

      if (inMaterialSpecs && outMaterialSpecs)
      {
        // keep record of material specifications
        if (pid1 == -1 && inMaterialSpecs)
        {
          // copy border materials across
          int location = reverseids[pid0_orig];
          outMaterialSpecs->SetValue(index, inMaterialSpecs->GetValue(location));
        }
        else
        {
          // make a note for materials with two parents
          outMaterialSpecs->SetValue(index, "interface");
        }
      }
    }

    // translate any user provided interfaces too
    svtkIntArray* inInterfaceIDs =
      svtkIntArray::SafeDownCast(input->GetFieldData()->GetArray(this->GetInterfaceIDsName()));
    if (inInterfaceIDs)
    {
      int nOverrides = inInterfaceIDs->GetNumberOfTuples();
      svtkIntArray* outInterfaceIDs = svtkIntArray::New();
      outInterfaceIDs->SetName(this->GetInterfaceIDsName());
      outInterfaceIDs->SetNumberOfComponents(2);
      outInterfaceIDs->SetNumberOfTuples(nOverrides);
      output->GetFieldData()->AddArray(outInterfaceIDs);
      outInterfaceIDs->Delete();
      for (i = 0; i < nOverrides; i++)
      {
        double* old = inInterfaceIDs->GetTuple2(i);
        int pid0 = this->Internal->OldToNew[(int)old[0]];
        int pid1 = this->Internal->OldToNew[(int)old[1]];
        outInterfaceIDs->SetTuple2(i, pid0, pid1);
      }
    }
  }

  // Update ourselves and release memory
  cell->Delete();
  coords->Delete();
  pts->Delete();
  parametricCoords->Delete();
  parametricCoords2->Delete();
  outPts->Delete();
  outPts2->Delete();

  output->SetPoints(newPts);
  newPts->Delete();
  output->SetPolys(newPolys);
  newPolys->Delete();
  if (newVerts->GetNumberOfCells() > 0)
  {
    output->SetVerts(newVerts);
  }
  newVerts->Delete();
  newVerts = nullptr;
  if (newLines->GetNumberOfCells() > 0)
  {
    output->SetLines(newLines);
  }
  newLines->Delete();

  // free storage
  output->Squeeze();
  if (this->OriginalCellIds != nullptr)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  if (this->OriginalPointIds != nullptr)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }
  if (this->PieceInvariant)
  {
    output->RemoveGhostCells();
  }

  this->DeleteQuadHash();

  return 1;
}

//----------------------------------------------------------------------------
void svtkDataSetRegionSurfaceFilter::InsertQuadInHash(
  svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType d, svtkIdType sourceId, svtkIdType faceId)
{

  svtkIdType tmp;
  svtkFastGeomQuad *quad, **end;

  // Reorder to get smallest id in a.
  if (b < a && b < c && b < d)
  {
    tmp = a;
    a = b;
    b = c;
    c = d;
    d = tmp;
  }
  else if (c < a && c < b && c < d)
  {
    tmp = a;
    a = c;
    c = tmp;
    tmp = b;
    b = d;
    d = tmp;
  }
  else if (d < a && d < b && d < c)
  {
    tmp = a;
    a = d;
    d = c;
    c = b;
    b = tmp;
  }

  // Look for existing quad in the hash;
  end = this->QuadHash + a;
  quad = *end;
  svtkIdType regionId = -1;
  if (this->RegionArray)
  {
    regionId = this->RegionArray->GetValue(sourceId);
  }
  while (quad)
  {
    end = &(quad->Next);
    const svtkIdType* quadsRegionId = (quad->ptArray + quad->numPts);
    // a has to match in this bin.
    // c should be independent of point order.
    if (quad->numPts == 4 && c == quad->ptArray[2])
    {
      // Check boh orders for b and d.
      if (((b == quad->ptArray[1] && d == quad->ptArray[3]) ||
            (b == quad->ptArray[3] && d == quad->ptArray[1])) &&
        (regionId == -1 || regionId == *quadsRegionId))

      {
        // We have a match.
        quad->SourceId = -1;
        // That is all we need to do.  Hide any quad shared by two or more cells.
        return;
      }
    }
    quad = *end;
  }

  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(6);
  quad->Next = nullptr;
  quad->SourceId = sourceId;
  quad->ptArray[0] = a;
  quad->ptArray[1] = b;
  quad->ptArray[2] = c;
  quad->ptArray[3] = d;

  // assign the face id to ptArray[5], and the region id to ptArray[4],
  // but using pointer math, so that we don't generate a warning about accessing
  // ptArray out of bounds
  const int quadRealNumPts(4);
  svtkIdType* quadsRegionId = (quad->ptArray + quadRealNumPts);
  svtkIdType* quadsFaceId = (quad->ptArray + quadRealNumPts + 1);
  *quadsRegionId = regionId;
  *quadsFaceId = faceId;

  quad->numPts = quadRealNumPts;
  *end = quad;
}

//----------------------------------------------------------------------------
void svtkDataSetRegionSurfaceFilter::InsertTriInHash(
  svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType sourceId, svtkIdType faceId)
{
  int tmp;
  svtkFastGeomQuad *quad, **end;

  // Reorder to get smallest id in a.
  if (b < a && b < c)
  {
    tmp = a;
    a = b;
    b = c;
    c = tmp;
  }
  else if (c < a && c < b)
  {
    tmp = a;
    a = c;
    c = b;
    b = tmp;
  }
  // We can't put the second smnallest in b because it might change the order
  // of the vertices in the final triangle.

  // Look for existing tri in the hash;
  end = this->QuadHash + a;
  quad = *end;
  svtkIdType regionId = -1;
  if (this->RegionArray)
  {
    regionId = this->RegionArray->GetValue(sourceId);
  }
  while (quad)
  {
    end = &(quad->Next);
    const svtkIdType* quadsRegionId = (quad->ptArray + quad->numPts);
    // a has to match in this bin.
    if (quad->numPts == 3)
    {
      if (((b == quad->ptArray[1] && c == quad->ptArray[2]) ||
            (b == quad->ptArray[2] && c == quad->ptArray[1])) &&
        (regionId == -1 ||
          regionId == *quadsRegionId)) // internal cells, but on a material interface
      {
        // We have a match.
        quad->SourceId = -1;
        // That is all we need to do. Hide any tri shared by two or more cells (that also are from
        // same region)
        return;
      }
    }
    quad = *end;
  }

  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(5);
  quad->Next = nullptr;
  quad->SourceId = sourceId;
  quad->ptArray[0] = a;
  quad->ptArray[1] = b;
  quad->ptArray[2] = c;
  quad->ptArray[3] = regionId;

  // assign the face id to ptArray[4], but using pointer math,
  // so that we don't generate a warning about accessing ptArray
  // out of bounds
  const int quadRealNumPts(3);
  svtkIdType* quadsFaceId = (quad->ptArray + quadRealNumPts + 1);
  *quadsFaceId = faceId;
  quad->numPts = quadRealNumPts;
  *end = quad;
}

//----------------------------------------------------------------------------
void svtkDataSetRegionSurfaceFilter::RecordOrigCellId(svtkIdType destIndex, svtkFastGeomQuad* quad)
{
  this->OrigCellIds->InsertValue(destIndex, quad->SourceId);
  const svtkIdType* faceId = (quad->ptArray + (quad->numPts + 1));
  this->CellFaceIds->InsertValue(destIndex, *faceId);
}

//----------------------------------------------------------------------------
void svtkDataSetRegionSurfaceFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkFastGeomQuad* svtkDataSetRegionSurfaceFilter::GetNextVisibleQuadFromHash()
{
  if (!this->RegionArray)
  {
    this->Internal->NextRegion = -1;
    return this->Superclass::GetNextVisibleQuadFromHash();
  }

  svtkFastGeomQuad* quad;
  quad = this->QuadHashTraversal;
  // Move till traversal until we have a quad to return.
  // Note: the current traversal has not been returned yet.
  while (quad == nullptr || quad->SourceId == -1)
  {
    if (quad)
    { // The quad must be hidden.  Move to the next.
      quad = quad->Next;
    }
    else
    { // must be the end of the linked list.  Move to the next bin.
      this->QuadHashTraversalIndex += 1;
      if (this->QuadHashTraversalIndex >= this->QuadHashLength)
      { // There are no more bins.
        this->QuadHashTraversal = nullptr;
        return nullptr;
      }
      quad = this->QuadHash[this->QuadHashTraversalIndex];
    }
  }

  int mat1 = this->RegionArray->GetValue(quad->SourceId);

  if (!this->SingleSided)
  {
    this->Internal->NextRegion = mat1;
  }
  else
  {
    // preserve this quad's material in isolation (external faces)
    int matidx;
    std::pair<int, int> p = std::make_pair(mat1, -1);
    if (this->Internal->NewRegions.find(p) == this->Internal->NewRegions.end())
    {
      matidx = static_cast<int>(this->Internal->NewRegions.size());
      this->Internal->NewRegions[p] = matidx;
      this->Internal->OldToNew[mat1] = matidx;
    }
    matidx = this->Internal->NewRegions[p];

    // look for this quad's twin across material interface.
    svtkFastGeomQuad* quad2;
    quad2 = quad->Next;
    int npts = quad->numPts;
    while (quad2)
    {
      bool match = false;
      if ((npts == 3) && (quad2->numPts == 3) &&
        ((quad->ptArray[1] == quad2->ptArray[1] && quad->ptArray[2] == quad2->ptArray[2]) ||
          (quad->ptArray[1] == quad2->ptArray[2] && quad->ptArray[2] == quad2->ptArray[1])))
      {
        match = true;
      }
      if ((npts == 4) && (quad2->numPts == 4) &&
        ((quad->ptArray[1] == quad2->ptArray[1] && quad->ptArray[3] == quad2->ptArray[3]) ||
          (quad->ptArray[1] == quad2->ptArray[3] && quad->ptArray[3] == quad2->ptArray[1])))
      {
        match = true;
      }
      if (match)
      {
        int mat2 = this->RegionArray->GetValue(quad2->SourceId);
        if (mat2 > mat1)
        {
          // pick greater material to ensure a consistent ordering for normals
          quad->SourceId = quad2->SourceId;
          quad->ptArray[0] = quad2->ptArray[0];
          quad->ptArray[1] = quad2->ptArray[1];
          quad->ptArray[2] = quad2->ptArray[2];
          if (npts == 4)
          {
            quad->ptArray[3] = quad2->ptArray[3];
          }
        }
        // preserve the joined quad's material
        int m1 = (mat1 > mat2 ? mat1 : mat2);
        int m2 = (mat1 > mat2 ? mat2 : mat1);
        p = std::make_pair(m1, m2);
        if (this->Internal->NewRegions.find(p) == this->Internal->NewRegions.end())
        {
          matidx = static_cast<int>(this->Internal->NewRegions.size());
          this->Internal->NewRegions[p] = matidx;
        }
        matidx = this->Internal->NewRegions[p];

        quad2->SourceId = -1; // don't visit the twin
        quad2 = nullptr;
      }
      else
      { // not a match
        quad2 = quad2->Next;
      }
    }
    this->Internal->NextRegion = matidx;
  }

  // Now we have a quad to return.  Set the traversal to the next entry.
  this->QuadHashTraversal = quad->Next;

  return quad;
}
