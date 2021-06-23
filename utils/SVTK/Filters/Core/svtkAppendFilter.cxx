/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAppendFilter.h"

#include "svtkBoundingBox.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDataSetCollection.h"
#include "svtkExecutive.h"
#include "svtkIncrementalOctreePointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnstructuredGrid.h"

#include <string>

svtkStandardNewMacro(svtkAppendFilter);

//----------------------------------------------------------------------------
svtkAppendFilter::svtkAppendFilter()
{
  this->InputList = nullptr;
  this->MergePoints = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->Tolerance = 0.0;
  this->ToleranceIsAbsolute = true;
}

//----------------------------------------------------------------------------
svtkAppendFilter::~svtkAppendFilter()
{
  if (this->InputList != nullptr)
  {
    this->InputList->Delete();
    this->InputList = nullptr;
  }
}

//----------------------------------------------------------------------------
svtkDataSet* svtkAppendFilter::GetInput(int idx)
{
  if (idx >= this->GetNumberOfInputConnections(0) || idx < 0)
  {
    return nullptr;
  }

  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void svtkAppendFilter::RemoveInputData(svtkDataSet* ds)
{
  if (!ds)
  {
    return;
  }
  int numCons = this->GetNumberOfInputConnections(0);
  for (int i = 0; i < numCons; i++)
  {
    if (this->GetInput(i) == ds)
    {
      this->RemoveInputConnection(0, this->GetInputConnection(0, i));
    }
  }
}

//----------------------------------------------------------------------------
svtkDataSetCollection* svtkAppendFilter::GetInputList()
{
  if (this->InputList)
  {
    this->InputList->Delete();
  }
  this->InputList = svtkDataSetCollection::New();

  for (int idx = 0; idx < this->GetNumberOfInputConnections(0); ++idx)
  {
    if (this->GetInput(idx))
    {
      this->InputList->AddItem(this->GetInput(idx));
    }
  }

  return this->InputList;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int svtkAppendFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  bool reallyMergePoints = false;
  if (this->MergePoints == 1 && inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    reallyMergePoints = true;

    // ensure that none of the inputs has ghost-cells.
    // (originally the code was checking for ghost cells only on 1st input,
    // that's not sufficient).
    for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
    {
      svtkDataSet* tempData = svtkDataSet::GetData(inputVector[0], cc);
      if (tempData->HasAnyGhostCells())
      {
        svtkDebugMacro(<< "Ghost cells present, so points will not be merged");
        reallyMergePoints = false;
        break;
      }
    }
  }

  // get the output info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Appending data together");

  // Loop over all data sets, checking to see what data is common to
  // all inputs. Note that data is common if 1) it is the same attribute
  // type (scalar, vector, etc.), 2) it is the same native type (int,
  // float, etc.), and 3) if a data array in a field, if it has the same name.
  svtkIdType totalNumPts = 0;
  svtkIdType totalNumCells = 0;
  // If we only have a single dataset and it's an unstructured grid
  // we can just shallow copy that and exit quickly.
  int numDataSets = 0;
  svtkUnstructuredGrid* inputUG = nullptr;

  svtkSmartPointer<svtkDataSetCollection> inputs;
  inputs.TakeReference(this->GetNonEmptyInputs(inputVector));

  svtkCollectionSimpleIterator iter;
  inputs->InitTraversal(iter);
  svtkDataSet* dataSet = nullptr;
  while ((dataSet = inputs->GetNextDataSet(iter)))
  {
    totalNumPts += dataSet->GetNumberOfPoints();
    totalNumCells += dataSet->GetNumberOfCells();
    numDataSets++;
    inputUG = svtkUnstructuredGrid::SafeDownCast(dataSet);
  }

  if (totalNumPts < 1)
  {
    svtkDebugMacro(<< "No data to append!");
    return 1;
  }

  if (numDataSets == 1 && inputUG != nullptr)
  {
    svtkDebugMacro(
      << "Only a single unstructured grid in the composite dataset and we can shallow copy.");
    output->ShallowCopy(inputUG);
    return 1;
  }

  // Now we can allocate memory
  output->Allocate(totalNumCells);

  svtkSmartPointer<svtkPoints> newPts = svtkSmartPointer<svtkPoints>::New();

  // set precision for the points in the output
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    // take the precision of the first pointset
    int datatype = SVTK_FLOAT;
    const int numInputs = inputVector[0]->GetNumberOfInformationObjects();
    for (int inputIndex = 0; inputIndex < numInputs; ++inputIndex)
    {
      svtkInformation* inInfo = inputVector[0]->GetInformationObject(inputIndex);
      svtkPointSet* ps = nullptr;
      if (inInfo)
      {
        ps = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
      }
      if (ps != nullptr && ps->GetNumberOfPoints() > 0)
      {
        datatype = ps->GetPoints()->GetDataType();
        break;
      }
    }
    newPts->SetDataType(datatype);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  // If we aren't merging points, we need to allocate the points here.
  if (!reallyMergePoints)
  {
    newPts->SetNumberOfPoints(totalNumPts);
  }

  svtkSmartPointer<svtkIdList> ptIds = svtkSmartPointer<svtkIdList>::New();
  ptIds->Allocate(SVTK_CELL_SIZE);
  svtkSmartPointer<svtkIdList> newPtIds = svtkSmartPointer<svtkIdList>::New();
  newPtIds->Allocate(SVTK_CELL_SIZE);

  svtkIdType twentieth = (totalNumPts + totalNumCells) / 20 + 1;

  // For optionally merging duplicate points
  svtkIdType* globalIndices = new svtkIdType[totalNumPts];
  svtkSmartPointer<svtkIncrementalOctreePointLocator> ptInserter;
  if (reallyMergePoints)
  {
    svtkBoundingBox outputBB;

    inputs->InitTraversal(iter);
    while ((dataSet = inputs->GetNextDataSet(iter)))
    {

      // Union of bounding boxes
      double localBox[6];
      dataSet->GetBounds(localBox);
      outputBB.AddBounds(localBox);
    }

    double outputBounds[6];
    outputBB.GetBounds(outputBounds);

    ptInserter = svtkSmartPointer<svtkIncrementalOctreePointLocator>::New();
    if (this->ToleranceIsAbsolute)
    {
      ptInserter->SetTolerance(this->Tolerance);
    }
    else
    {
      ptInserter->SetTolerance(this->Tolerance * outputBB.GetDiagonalLength());
    }

    ptInserter->InitPointInsertion(newPts, outputBounds);
  }

  // append the blocks / pieces in terms of the geometry and topology
  svtkIdType count = 0;
  svtkIdType ptOffset = 0;
  float decimal = 0.0;
  inputs->InitTraversal(iter);
  int abort = 0;
  while (!abort && (dataSet = inputs->GetNextDataSet(iter)))
  {
    svtkIdType dataSetNumPts = dataSet->GetNumberOfPoints();
    svtkIdType dataSetNumCells = dataSet->GetNumberOfCells();

    // copy points
    for (svtkIdType ptId = 0; ptId < dataSetNumPts && !abort; ++ptId)
    {
      if (reallyMergePoints)
      {
        svtkIdType globalPtId = 0;
        ptInserter->InsertUniquePoint(dataSet->GetPoint(ptId), globalPtId);
        globalIndices[ptId + ptOffset] = globalPtId;
        // The point inserter puts the point into newPts, so we don't have to do that here.
      }
      else
      {
        globalIndices[ptId + ptOffset] = ptId + ptOffset;
        newPts->SetPoint(ptId + ptOffset, dataSet->GetPoint(ptId));
      }

      // Update progress
      count++;
      if (!(count % twentieth))
      {
        decimal += 0.05;
        this->UpdateProgress(decimal);
        abort = this->GetAbortExecute();
      }
    }

    // copy cell
    svtkUnstructuredGrid* ug = svtkUnstructuredGrid::SafeDownCast(dataSet);
    for (svtkIdType cellId = 0; cellId < dataSetNumCells && !abort; ++cellId)
    {
      newPtIds->Reset();
      if (ug && dataSet->GetCellType(cellId) == SVTK_POLYHEDRON)
      {
        svtkIdType nfaces;
        const svtkIdType* facePtIds;
        ug->GetFaceStream(cellId, nfaces, facePtIds);
        for (svtkIdType id = 0; id < nfaces; ++id)
        {
          svtkIdType nPoints = facePtIds[0];
          newPtIds->InsertNextId(nPoints);
          for (svtkIdType j = 1; j <= nPoints; ++j)
          {
            newPtIds->InsertNextId(globalIndices[facePtIds[j] + ptOffset]);
          }
          facePtIds += nPoints + 1;
        }
        output->InsertNextCell(SVTK_POLYHEDRON, nfaces, newPtIds->GetPointer(0));
      }
      else
      {
        dataSet->GetCellPoints(cellId, ptIds);
        for (svtkIdType id = 0; id < ptIds->GetNumberOfIds(); ++id)
        {
          newPtIds->InsertId(id, globalIndices[ptIds->GetId(id) + ptOffset]);
        }
        output->InsertNextCell(dataSet->GetCellType(cellId), newPtIds);
      }

      // Update progress
      count++;
      if (!(count % twentieth))
      {
        decimal += 0.05;
        this->UpdateProgress(decimal);
        abort = this->GetAbortExecute();
      }
    }
    ptOffset += dataSetNumPts;
  }

  // this filter can copy global ids except for global point ids when merging
  // points (see paraview/paraview#18666).
  // Note, not copying global ids is the default behavior.
  if (reallyMergePoints == false)
  {
    output->GetPointData()->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
  }
  output->GetCellData()->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);

  // Now copy the array data
  this->AppendArrays(
    svtkDataObject::POINT, inputVector, globalIndices, output, newPts->GetNumberOfPoints());
  this->UpdateProgress(0.75);
  this->AppendArrays(svtkDataObject::CELL, inputVector, nullptr, output, output->GetNumberOfCells());
  this->UpdateProgress(1.0);

  // Update ourselves and release memory
  output->SetPoints(newPts);
  output->Squeeze();

  delete[] globalIndices;

  return 1;
}

//----------------------------------------------------------------------------
svtkDataSetCollection* svtkAppendFilter::GetNonEmptyInputs(svtkInformationVector** inputVector)
{
  svtkDataSetCollection* collection = svtkDataSetCollection::New();
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  for (int inputIndex = 0; inputIndex < numInputs; ++inputIndex)
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(inputIndex);
    svtkDataSet* dataSet = nullptr;
    if (inInfo)
    {
      dataSet = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    }
    if (dataSet != nullptr)
    {
      if (dataSet->GetNumberOfPoints() <= 0 && dataSet->GetNumberOfCells() <= 0)
      {
        continue; // no input, just skip
      }
      collection->AddItem(dataSet);
    }
  }

  return collection;
}

//----------------------------------------------------------------------------
void svtkAppendFilter::AppendArrays(int attributesType, svtkInformationVector** inputVector,
  svtkIdType* globalIds, svtkUnstructuredGrid* output, svtkIdType totalNumberOfElements)
{
  // Check if attributesType is supported
  if (attributesType != svtkDataObject::POINT && attributesType != svtkDataObject::CELL)
  {
    svtkErrorMacro(<< "Unhandled attributes type " << attributesType << ", must be either "
                  << "svtkDataObject::POINT or svtkDataObject::CELL");
    return;
  }

  svtkDataSetAttributes::FieldList fieldList;
  auto inputs = svtkSmartPointer<svtkDataSetCollection>::Take(this->GetNonEmptyInputs(inputVector));
  svtkCollectionSimpleIterator iter;
  svtkDataSet* dataSet = nullptr;
  for (dataSet = nullptr, inputs->InitTraversal(iter); (dataSet = inputs->GetNextDataSet(iter));)
  {
    if (auto inputData = dataSet->GetAttributes(attributesType))
    {
      fieldList.IntersectFieldList(inputData);
    }
  }

  svtkDataSetAttributes* outputData = output->GetAttributes(attributesType);
  outputData->CopyAllocate(fieldList, totalNumberOfElements);

  // copy arrays.
  int inputIndex;
  svtkIdType offset = 0;
  for (inputIndex = 0, dataSet = nullptr, inputs->InitTraversal(iter);
       (dataSet = inputs->GetNextDataSet(iter));)
  {
    if (auto inputData = dataSet->GetAttributes(attributesType))
    {
      const auto numberOfInputTuples = inputData->GetNumberOfTuples();
      if (globalIds != nullptr)
      {
        for (svtkIdType id = 0; id < numberOfInputTuples; ++id)
        {
          fieldList.CopyData(inputIndex, inputData, id, outputData, globalIds[offset + id]);
        }
      }
      else
      {
        fieldList.CopyData(inputIndex, inputData, 0, numberOfInputTuples, outputData, offset);
      }
      offset += numberOfInputTuples;
      ++inputIndex;
    }
  }
}

//----------------------------------------------------------------------------
int svtkAppendFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  int numInputConnections = this->GetNumberOfInputConnections(0);

  // Let downstream request a subset of connection 0, for connections >= 1
  // send their WHOLE_EXTENT as UPDATE_EXTENT.
  for (int idx = 1; idx < numInputConnections; ++idx)
  {
    svtkInformation* inputInfo = inputVector[0]->GetInformationObject(idx);
    if (inputInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int ext[6];
      inputInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      inputInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkAppendFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void svtkAppendFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MergePoints:" << (this->MergePoints ? "On" : "Off") << "\n";
  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
