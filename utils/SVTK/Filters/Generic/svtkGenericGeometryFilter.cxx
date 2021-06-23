/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericGeometryFilter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkGenericAdaptorCell.h"
#include "svtkGenericAttribute.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericCell.h"
#include "svtkGenericCellIterator.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericDataSet.h"
#include "svtkGenericPointIterator.h"
#include "svtkHexahedron.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
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

svtkStandardNewMacro(svtkGenericGeometryFilter);

svtkCxxSetObjectMacro(svtkGenericGeometryFilter, Locator, svtkIncrementalPointLocator);
//----------------------------------------------------------------------------
// Construct with all types of clipping turned off.
svtkGenericGeometryFilter::svtkGenericGeometryFilter()
{
  this->PointMinimum = 0;
  this->PointMaximum = SVTK_ID_MAX;

  this->CellMinimum = 0;
  this->CellMaximum = SVTK_ID_MAX;

  this->Extent[0] = SVTK_DOUBLE_MIN;
  this->Extent[1] = SVTK_DOUBLE_MAX;
  this->Extent[2] = SVTK_DOUBLE_MIN;
  this->Extent[3] = SVTK_DOUBLE_MAX;
  this->Extent[4] = SVTK_DOUBLE_MIN;
  this->Extent[5] = SVTK_DOUBLE_MAX;

  this->PointClipping = 0;
  this->CellClipping = 0;
  this->ExtentClipping = 0;

  this->Merging = 1;
  this->Locator = nullptr;
  this->InternalPD = svtkPointData::New();

  this->PassThroughCellIds = 0;
}
//----------------------------------------------------------------------------
svtkGenericGeometryFilter::~svtkGenericGeometryFilter()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  this->InternalPD->Delete();
}

//----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void svtkGenericGeometryFilter::SetExtent(
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
void svtkGenericGeometryFilter::SetExtent(double extent[6])
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

//----------------------------------------------------------------------------
int svtkGenericGeometryFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGenericDataSet* input =
    svtkGenericDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType cellId;
  int i, j;
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  char* cellVis;
  svtkGenericAdaptorCell* cell;
  double x[3] = { 0, 0, 0 };
  svtkIdType ptIds[4];

  svtkIdType ptId;
  int allVisible;
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();

  if (numCells == 0)
  {
    svtkErrorMacro(<< "Number of cells is zero, no data to process.");
    return 1;
  }

  svtkDebugMacro(<< "Executing geometry filter");

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

  svtkGenericCellIterator* cellIt = input->NewCellIterator();
  // Mark cells as being visible or not
  //
  if (!allVisible)
  {
    for (cellIt->Begin(); !cellIt->IsAtEnd(); cellIt->Next())
    {
      cell = cellIt->GetCell();
      cellId = cell->GetId();
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        svtkGenericPointIterator* pointIt = input->NewPointIterator();
        cell->GetPointIterator(pointIt);
        pointIt->Begin();

        cell->GetPointIds(ptIds);
        for (i = 0; i < cell->GetNumberOfPoints(); i++)
        {
          ptId = ptIds[i];

          // Get point coordinate
          pointIt->GetPosition(x);
          pointIt->Next();

          if ((this->PointClipping && (ptId < this->PointMinimum || ptId > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          }
        }
        if (i >= cell->GetNumberOfPoints())
        {
          cellVis[cellId] = 1;
        }
        pointIt->Delete();
      }
    }
  }

  // Allocate
  //
  svtkIdType estimatedSize = input->GetEstimatedSize();
  estimatedSize = (estimatedSize / 1024 + 1) * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }
  output->AllocateEstimate(numCells, 1);

  svtkPoints* newPts = svtkPoints::New();
  svtkCellArray* cellArray = svtkCellArray::New();

  newPts->Allocate(estimatedSize, numPts);
  cellArray->AllocateEstimate(numCells, 1);

  // prepare the output attributes
  svtkGenericAttributeCollection* attributes = input->GetAttributes();
  svtkGenericAttribute* attribute;
  svtkDataArray* attributeArray;

  int c = attributes->GetNumberOfAttributes();
  svtkDataSetAttributes* dsAttributes;

  int attributeType;

  this->InternalPD->Initialize();
  for (i = 0; i < c; ++i)
  {
    attribute = attributes->GetAttribute(i);
    attributeType = attribute->GetType();
    int centering = attribute->GetCentering();
    if (centering != svtkPointCentered && centering != svtkCellCentered)
    { // skip boundary-centered attributes.
      continue;
    }

    if (centering == svtkPointCentered)
    {
      dsAttributes = outputPD;

      attributeArray = svtkDataArray::CreateDataArray(attribute->GetComponentType());
      attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
      attributeArray->SetName(attribute->GetName());
      this->InternalPD->AddArray(attributeArray);
      attributeArray->Delete();
      if (this->InternalPD->GetAttribute(attributeType) == nullptr)
      {
        this->InternalPD->SetActiveAttribute(
          this->InternalPD->GetNumberOfArrays() - 1, attributeType);
      }
    }
    else // svtkCellCentered
    {
      dsAttributes = outputCD;
    }
    attributeArray = svtkDataArray::CreateDataArray(attribute->GetComponentType());
    attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
    attributeArray->SetName(attribute->GetName());
    dsAttributes->AddArray(attributeArray);
    attributeArray->Delete();

    if (dsAttributes->GetAttribute(attributeType) == nullptr)
    {
      dsAttributes->SetActiveAttribute(dsAttributes->GetNumberOfArrays() - 1, attributeType);
    }
  }

  svtkIncrementalPointLocator* locator = nullptr;
  if (this->Merging)
  {
    if (this->Locator == nullptr)
    {
      this->CreateDefaultLocator();
    }
    this->Locator->InitPointInsertion(newPts, input->GetBounds());
    locator = this->Locator;
  }

  // Traverse cells to extract geometry
  //
  int abort = 0;
  svtkIdType progressInterval = numCells / 20 + 1;

  svtkIdList* faceList = svtkIdList::New();
  faceList->SetNumberOfIds(3);

  input->GetTessellator()->InitErrorMetrics(input);

  svtkIdTypeArray* originalCellIds = nullptr;
  if (this->PassThroughCellIds)
  {
    originalCellIds = svtkIdTypeArray::New();
    originalCellIds->SetName("svtkOriginalCellIds");
    originalCellIds->SetNumberOfComponents(1);
  }

  for (cellId = 0, cellIt->Begin(); !cellIt->IsAtEnd() && !abort; cellIt->Next(), cellId++)
  {
    cell = cellIt->GetCell();
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    svtkIdType BeginTopOutCId = outputCD->GetNumberOfTuples();
    if (allVisible || cellVis[cellId])
    {
      switch (cell->GetDimension())
      {
        // create new points and then cell
        case 0:
        case 1:
          svtkErrorMacro("Cell of dimension " << cell->GetDimension() << " not handled yet.");
          break;
        case 2:
          if (cell->IsOnBoundary())
          {
            cell->Tessellate(input->GetAttributes(), input->GetTessellator(), newPts, locator,
              cellArray, this->InternalPD, outputPD, outputCD, nullptr); // newScalars );
          }
          break;
        case 3:
          //          int numFaces = cell->GetNumberOfFaces();
          int numFaces = cell->GetNumberOfBoundaries(2);
          for (j = 0; j < numFaces; j++)
          {
            if (cell->IsFaceOnBoundary(j))
            {
              cell->TriangulateFace(input->GetAttributes(), input->GetTessellator(), j, newPts,
                locator, cellArray, this->InternalPD, outputPD, outputCD);
            }
          }
          break;

      } // switch
    }   // if visible

    svtkIdType EndTopOutCId = outputCD->GetNumberOfTuples();
    if (this->PassThroughCellIds)
    {
      for (svtkIdType cId = BeginTopOutCId; cId < EndTopOutCId; cId++)
      {
        originalCellIds->InsertNextValue(cellId);
      }
    }

  } // for all cells

  if (this->PassThroughCellIds)
  {
    outputCD->AddArray(originalCellIds);
    originalCellIds->Delete();
    originalCellIds = nullptr;
  }

  cellIt->Delete();
  svtkDebugMacro(<< "Extracted " << newPts->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  output->SetPolys(cellArray);

  cellArray->Delete();
  newPts->Delete();
  faceList->Delete();

  // free storage
  if (!this->Merging && this->Locator)
  {
    this->Locator->Initialize();
  }
  output->Squeeze();

  delete[] cellVis;

  return 1;
}

//----------------------------------------------------------------------------
int svtkGenericGeometryFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of svtkMergePoints is used.
void svtkGenericGeometryFilter::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

//----------------------------------------------------------------------------
void svtkGenericGeometryFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point Minimum : " << this->GetPointMinimum() << "\n";
  os << indent << "Point Maximum : " << this->GetPointMaximum() << "\n";

  os << indent << "Cell Minimum : " << this->GetCellMinimum() << "\n";
  os << indent << "Cell Maximum : " << this->GetCellMaximum() << "\n";

  os << indent << "Extent: \n";
  os << indent << "  Xmin,Xmax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "PointClipping: " << (this->GetPointClipping() ? "On\n" : "Off\n");
  os << indent << "CellClipping: " << (this->GetCellClipping() ? "On\n" : "Off\n");
  os << indent << "ExtentClipping: " << (this->GetExtentClipping() ? "On\n" : "Off\n");

  os << indent << "Merging: " << (this->GetMerging() ? "On\n" : "Off\n");
  if (this->GetLocator())
  {
    os << indent << "Locator: " << this->GetLocator() << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "PassThroughCellIds: " << (this->GetPassThroughCellIds() ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
svtkMTimeType svtkGenericGeometryFilter::GetMTime()
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
int svtkGenericGeometryFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
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
