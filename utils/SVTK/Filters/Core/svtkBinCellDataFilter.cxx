/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBinCellDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBinCellDataFilter.h"

#include "svtkAbstractCellLocator.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkDataSet.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStaticCellLocator.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <map>
#include <sstream>

svtkStandardNewMacro(svtkBinCellDataFilter);

//---------------------------------------------------------------------------
// Specify a spatial locator for speeding the search process. By
// default an instance of svtkStaticCellLocator is used.
svtkCxxSetObjectMacro(svtkBinCellDataFilter, CellLocator, svtkAbstractCellLocator);

#define CELL_TOLERANCE_FACTOR_SQR 1e-6

//----------------------------------------------------------------------------
namespace
{
typedef std::map<svtkIdType, svtkIdType> IdMap;
bool CompareIdMapCounts(const IdMap::value_type& p1, const IdMap::value_type& p2)
{
  return p1.second < p2.second;
}

int MostFrequentId(svtkIdType* idList, svtkIdType nIds)
{
  IdMap idMap;
  for (svtkIdType i = 0; i < nIds; i++)
  {
    if (idList[i] != -1)
    {
      idMap[idList[i]]++;
    }
  }
  if (idMap.empty())
  {
    return -1;
  }
  return std::max_element(idMap.begin(), idMap.end(), CompareIdMapCounts)->first;
}

int GetBinId(double value, double* binValues, int nBins)
{
  double* lb = std::lower_bound(binValues, binValues + nBins, value);
  return (lb - binValues);
}
} // namespace

//----------------------------------------------------------------------------
svtkBinCellDataFilter::svtkBinCellDataFilter()
{
  this->BinValues = svtkBinValues::New();
  this->BinValues->GenerateValues(2, SVTK_DOUBLE_MIN, SVTK_DOUBLE_MAX);

  this->CellLocator = nullptr;

  this->StoreNumberOfNonzeroBins = true;
  this->NumberOfNonzeroBinsArrayName = nullptr;
  this->SetNumberOfNonzeroBinsArrayName("NumberOfNonzeroBins");

  this->SpatialMatch = 0;
  this->SetNumberOfInputPorts(2);

  this->CellOverlapMethod = svtkBinCellDataFilter::CELL_CENTROID;
  this->Tolerance = 1.0;
  this->ComputeTolerance = false;
  this->ArrayComponent = 0;

  // by default process source cell scalars
  this->SetInputArrayToProcess(
    0, 1, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
svtkBinCellDataFilter::~svtkBinCellDataFilter()
{
  this->BinValues->Delete();
  this->SetCellLocator(nullptr);
  this->SetNumberOfNonzeroBinsArrayName(nullptr);
}

//----------------------------------------------------------------------------
void svtkBinCellDataFilter::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void svtkBinCellDataFilter::SetSourceData(svtkDataObject* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkBinCellDataFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
int svtkBinCellDataFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* source = svtkDataSet::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!source)
  {
    return 0;
  }

  // get the bins
  int numBins = this->GetNumberOfBins();
  double* values = this->GetValues();

  // is there data to process?
  svtkDataArray* sourceScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!sourceScalars)
  {
    return 1;
  }

  // Initialize cell count array.
  svtkNew<svtkIdTypeArray> binnedData;
  binnedData->SetNumberOfComponents(numBins + 1);
  binnedData->SetNumberOfTuples(input->GetNumberOfCells());
  {
    std::stringstream s;
    s << "binned_" << sourceScalars->GetName();
    binnedData->SetName(s.str().c_str());
  }

  for (int i = 0; i < numBins + 1; i++)
  {
    binnedData->FillComponent(i, 0);
  }

  // pass point and cell data
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  double tol2 = (this->ComputeTolerance ? SVTK_DOUBLE_MAX : (this->Tolerance * this->Tolerance));

  double weights[SVTK_CELL_SIZE];
  svtkIdType inputIds[SVTK_CELL_SIZE];

  if (!this->CellLocator)
  {
    this->CreateDefaultLocator();
  }
  this->CellLocator->SetDataSet(input);
  this->CellLocator->BuildLocator();

  svtkNew<svtkGenericCell> sourceCell, inputCell;
  svtkIdType cellId = 0;
  input->GetCell(cellId, inputCell);
  svtkCellIterator* srcIt = source->NewCellIterator();
  double pcoords[3], coords[3];
  int subId;
  // iterate over each cell in the source mesh
  for (srcIt->InitTraversal(); !srcIt->IsDoneWithTraversal(); srcIt->GoToNextCell())
  {
    if (this->CellOverlapMethod == svtkBinCellDataFilter::CELL_CENTROID)
    {
      // identify the centroid of the source cell
      srcIt->GetCell(sourceCell);
      sourceCell->GetParametricCenter(pcoords);
      sourceCell->EvaluateLocation(subId, pcoords, coords, weights);

      // find the cell that contains xyz and get it
      cellId = this->CellLocator->FindCell(coords, tol2, inputCell, pcoords, weights);

      if (this->ComputeTolerance && cellId >= 0)
      {
        // compute a tolerance proportional to the cell length.
        double dist2;
        double closestPoint[3];
        inputCell->EvaluatePosition(coords, closestPoint, subId, pcoords, dist2, weights);
        if (dist2 > (inputCell->GetLength2() * CELL_TOLERANCE_FACTOR_SQR))
        {
          cellId = -1;
        }
      }
    }
    else
    {
      svtkPoints* points = srcIt->GetPoints();
      for (svtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
      {
        points->GetPoint(i, coords);
        inputIds[i] = this->CellLocator->FindCell(coords, tol2, inputCell, pcoords, weights);
      }
      cellId = MostFrequentId(inputIds, points->GetNumberOfPoints());
    }

    // if the source cell centroid is within an input cell, bin the source
    // cell's value and increment the associated bin count.
    if (cellId >= 0)
    {
      double value = sourceScalars->GetComponent(srcIt->GetCellId(), this->ArrayComponent);
      int bin = GetBinId(value, values, numBins);
      binnedData->SetTypedComponent(cellId, bin, binnedData->GetTypedComponent(cellId, bin) + 1);
    }
  }
  srcIt->Delete();

  // add binned data to the output mesh
  output->GetCellData()->AddArray(binnedData);

  if (this->StoreNumberOfNonzeroBins)
  {
    // Initialize # of nonzero bins array.
    svtkNew<svtkIdTypeArray> numNonzeroBins;
    numNonzeroBins->SetNumberOfComponents(1);
    numNonzeroBins->SetNumberOfTuples(input->GetNumberOfCells());
    numNonzeroBins->SetName(this->NumberOfNonzeroBinsArrayName ? this->NumberOfNonzeroBinsArrayName
                                                               : "NumberOfNonzeroBins");

    for (svtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
    {
      svtkIdType nBins = 0;
      for (svtkIdType j = 0; j < binnedData->GetNumberOfComponents(); j++)
      {
        if (binnedData->GetTypedComponent(i, j) > 0)
        {
          nBins++;
        }
        numNonzeroBins->SetTypedComponent(i, 0, nBins);
      }
    }
    output->GetCellData()->AddArray(numNonzeroBins);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkBinCellDataFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  // A variation of the bug fix from John Biddiscombe.
  // Make sure that the scalar type and number of components
  // are propagated from the source not the input.
  if (svtkImageData::HasScalarType(sourceInfo))
  {
    svtkImageData::SetScalarType(svtkImageData::GetScalarType(sourceInfo), outInfo);
  }
  if (svtkImageData::HasNumberOfScalarComponents(sourceInfo))
  {
    svtkImageData::SetNumberOfScalarComponents(
      svtkImageData::GetNumberOfScalarComponents(sourceInfo), outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkBinCellDataFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happened to CopyUpdateExtent in svtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (output &&
    (!strcmp(output->GetClassName(), "svtkUnstructuredGrid") ||
      !strcmp(output->GetClassName(), "svtkPolyData")))
  {
    usePiece = 1;
  }

  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  sourceInfo->Remove(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (sourceInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      sourceInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  if (!this->SpatialMatch)
  {
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  else if (this->SpatialMatch == 1)
  {
    if (usePiece)
    {
      // Request an extra ghost level because the probe
      // gets external values with computation prescision problems.
      // I think the probe should be changed to have an epsilon ...
      sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
        outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
      sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
      sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
        outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()) + 1);
    }
    else
    {
      sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
    }
  }

  if (usePiece)
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  else
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
  }

  // Use the whole input in all processes, and use the requested update
  // extent of the output to divide up the source.
  if (this->SpatialMatch == 2)
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  return 1;
}

//--------------------------------------------------------------------------
// Method manages creation of locators.
void svtkBinCellDataFilter::CreateDefaultLocator()
{
  this->SetCellLocator(nullptr);
  this->CellLocator = svtkStaticCellLocator::New();
}

//----------------------------------------------------------------------------
void svtkBinCellDataFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkDataObject* source = this->GetSource();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Source: " << source << "\n";

  os << indent << "Spatial Match: " << (this->SpatialMatch ? "On" : "Off") << "\n";

  os << indent
     << "Store Number Of Nonzero Bins: " << (this->StoreNumberOfNonzeroBins ? "On" : "Off") << "\n";

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Compute Tolerance: " << (this->ComputeTolerance ? "On" : "Off") << "\n";

  os << indent << "Array Component: " << this->ArrayComponent << "\n";

  os << indent << "Cell Overlap Method: " << this->CellOverlapMethod << "\n";

  os << indent << "Cell Locator: " << this->CellLocator << "\n";
}
