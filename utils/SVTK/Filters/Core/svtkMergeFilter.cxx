/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMergeFilter.h"

#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkMergeFilter);

class svtkFieldNode
{
public:
  svtkFieldNode(const char* name, svtkDataSet* ptr = nullptr)
  {
    size_t length = strlen(name);
    if (length > 0)
    {
      this->Name = new char[length + 1];
      strcpy(this->Name, name);
    }
    else
    {
      this->Name = nullptr;
    }
    this->Ptr = ptr;
    this->Next = nullptr;
  }
  ~svtkFieldNode() { delete[] this->Name; }

  const char* GetName() { return Name; }
  svtkDataSet* Ptr;
  svtkFieldNode* Next;

private:
  svtkFieldNode(const svtkFieldNode&) = delete;
  void operator=(const svtkFieldNode&) = delete;
  char* Name;
};

class svtkFieldList
{
public:
  svtkFieldList()
  {
    this->First = nullptr;
    this->Last = nullptr;
  }
  ~svtkFieldList()
  {
    svtkFieldNode* node = this->First;
    svtkFieldNode* next;
    while (node)
    {
      next = node->Next;
      delete node;
      node = next;
    }
  }

  void Add(const char* name, svtkDataSet* ptr)
  {
    svtkFieldNode* newNode = new svtkFieldNode(name, ptr);
    if (!this->First)
    {
      this->First = newNode;
      this->Last = newNode;
    }
    else
    {
      this->Last->Next = newNode;
      this->Last = newNode;
    }
  }

  friend class svtkFieldListIterator;

private:
  svtkFieldNode* First;
  svtkFieldNode* Last;
};

class svtkFieldListIterator
{
public:
  svtkFieldListIterator(svtkFieldList* list)
  {
    this->List = list;
    this->Position = nullptr;
  }
  void Begin() { this->Position = this->List->First; }
  void Next()
  {
    if (this->Position)
    {
      this->Position = this->Position->Next;
    }
  }
  int End() { return this->Position ? 0 : 1; }
  svtkFieldNode* Get() { return this->Position; }

private:
  svtkFieldNode* Position;
  svtkFieldList* List;
};

//------------------------------------------------------------------------------

// Create object with no input or output.
svtkMergeFilter::svtkMergeFilter()
{
  this->FieldList = new svtkFieldList;
  this->SetNumberOfInputPorts(6);
}

svtkMergeFilter::~svtkMergeFilter()
{
  delete this->FieldList;
}

svtkDataSet* svtkMergeFilter::GetGeometry()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

void svtkMergeFilter::SetScalarsData(svtkDataSet* input)
{
  this->SetInputData(1, input);
}
svtkDataSet* svtkMergeFilter::GetScalars()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

void svtkMergeFilter::SetVectorsData(svtkDataSet* input)
{
  this->SetInputData(2, input);
}
svtkDataSet* svtkMergeFilter::GetVectors()
{
  if (this->GetNumberOfInputConnections(2) < 1)
  {
    return nullptr;
  }
  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(2, 0));
}

void svtkMergeFilter::SetNormalsData(svtkDataSet* input)
{
  this->SetInputData(3, input);
}
svtkDataSet* svtkMergeFilter::GetNormals()
{
  if (this->GetNumberOfInputConnections(3) < 1)
  {
    return nullptr;
  }
  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(3, 0));
}

void svtkMergeFilter::SetTCoordsData(svtkDataSet* input)
{
  this->SetInputData(4, input);
}
svtkDataSet* svtkMergeFilter::GetTCoords()
{
  if (this->GetNumberOfInputConnections(4) < 1)
  {
    return nullptr;
  }
  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(4, 0));
}

void svtkMergeFilter::SetTensorsData(svtkDataSet* input)
{
  this->SetInputData(5, input);
}
svtkDataSet* svtkMergeFilter::GetTensors()
{
  if (this->GetNumberOfInputConnections(5) < 1)
  {
    return nullptr;
  }
  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(5, 0));
}

void svtkMergeFilter::AddField(const char* name, svtkDataSet* input)
{
  this->FieldList->Add(name, input);
}

int svtkMergeFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* scalarsInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* vectorsInfo = inputVector[2]->GetInformationObject(0);
  svtkInformation* normalsInfo = inputVector[3]->GetInformationObject(0);
  svtkInformation* tCoordsInfo = inputVector[4]->GetInformationObject(0);
  svtkInformation* tensorsInfo = inputVector[5]->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* scalarsData = nullptr;
  svtkDataSet* vectorsData = nullptr;
  svtkDataSet* normalsData = nullptr;
  svtkDataSet* tCoordsData = nullptr;
  svtkDataSet* tensorsData = nullptr;
  if (scalarsInfo)
  {
    scalarsData = svtkDataSet::SafeDownCast(scalarsInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  if (vectorsInfo)
  {
    vectorsData = svtkDataSet::SafeDownCast(vectorsInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  if (normalsInfo)
  {
    normalsData = svtkDataSet::SafeDownCast(normalsInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  if (tCoordsInfo)
  {
    tCoordsData = svtkDataSet::SafeDownCast(tCoordsInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  if (tensorsInfo)
  {
    tensorsData = svtkDataSet::SafeDownCast(tensorsInfo->Get(svtkDataObject::DATA_OBJECT()));
  }

  svtkIdType numPts, numScalars = 0, numVectors = 0, numNormals = 0, numTCoords = 0;
  svtkIdType numTensors = 0;
  svtkIdType numCells, numCellScalars = 0, numCellVectors = 0, numCellNormals = 0;
  svtkIdType numCellTCoords = 0, numCellTensors = 0;
  svtkPointData* pd;
  svtkDataArray* scalars = nullptr;
  svtkDataArray* vectors = nullptr;
  svtkDataArray* normals = nullptr;
  svtkDataArray* tcoords = nullptr;
  svtkDataArray* tensors = nullptr;
  svtkCellData* cd;
  svtkDataArray* cellScalars = nullptr;
  svtkDataArray* cellVectors = nullptr;
  svtkDataArray* cellNormals = nullptr;
  svtkDataArray* cellTCoords = nullptr;
  svtkDataArray* cellTensors = nullptr;
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();

  svtkDebugMacro(<< "Merging data!");

  // geometry needs to be copied
  output->CopyStructure(input);
  if ((numPts = input->GetNumberOfPoints()) < 1)
  {
    svtkWarningMacro(<< "Nothing to merge!");
  }
  numCells = input->GetNumberOfCells();

  if (scalarsData)
  {
    pd = scalarsData->GetPointData();
    scalars = pd->GetScalars();
    if (scalars != nullptr)
    {
      numScalars = scalars->GetNumberOfTuples();
    }
    cd = scalarsData->GetCellData();
    cellScalars = cd->GetScalars();
    if (cellScalars != nullptr)
    {
      numCellScalars = cellScalars->GetNumberOfTuples();
    }
  }

  if (vectorsData)
  {
    pd = vectorsData->GetPointData();
    vectors = pd->GetVectors();
    if (vectors != nullptr)
    {
      numVectors = vectors->GetNumberOfTuples();
    }
    cd = vectorsData->GetCellData();
    cellVectors = cd->GetVectors();
    if (cellVectors != nullptr)
    {
      numCellVectors = cellVectors->GetNumberOfTuples();
    }
  }

  if (normalsData)
  {
    pd = normalsData->GetPointData();
    normals = pd->GetNormals();
    if (normals != nullptr)
    {
      numNormals = normals->GetNumberOfTuples();
    }
    cd = normalsData->GetCellData();
    cellNormals = cd->GetNormals();
    if (cellNormals != nullptr)
    {
      numCellNormals = cellNormals->GetNumberOfTuples();
    }
  }

  if (tCoordsData)
  {
    pd = tCoordsData->GetPointData();
    tcoords = pd->GetTCoords();
    if (tcoords != nullptr)
    {
      numTCoords = tcoords->GetNumberOfTuples();
    }
    cd = tCoordsData->GetCellData();
    cellTCoords = cd->GetTCoords();
    if (cellTCoords != nullptr)
    {
      numCellTCoords = cellTCoords->GetNumberOfTuples();
    }
  }

  if (tensorsData)
  {
    pd = tensorsData->GetPointData();
    tensors = pd->GetTensors();
    if (tensors != nullptr)
    {
      numTensors = tensors->GetNumberOfTuples();
    }
    cd = tensorsData->GetCellData();
    cellTensors = cd->GetTensors();
    if (cellTensors != nullptr)
    {
      numCellTensors = cellTensors->GetNumberOfTuples();
    }
  }

  // merge data only if it is consistent
  if (numPts == numScalars)
  {
    outputPD->SetScalars(scalars);
  }
  else
  {
    svtkWarningMacro("Scalars for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point scalars "
      << numPts << " != " << numScalars);
  }
  if (numCells == numCellScalars)
  {
    outputCD->SetScalars(cellScalars);
  }
  else
  {
    svtkWarningMacro("Scalars for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell scalars "
      << numCells << " != " << numCellScalars);
  }

  if (numPts == numVectors)
  {
    outputPD->SetVectors(vectors);
  }
  else
  {
    svtkWarningMacro("Vectors for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point vectors "
      << numPts << " != " << numVectors);
  }
  if (numCells == numCellVectors)
  {
    outputCD->SetVectors(cellVectors);
  }
  else
  {
    svtkWarningMacro("Vectors for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell vectors "
      << numCells << " != " << numCellVectors);
  }

  if (numPts == numNormals)
  {
    outputPD->SetNormals(normals);
  }
  else
  {
    svtkWarningMacro("Normals for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point normals "
      << numPts << " != " << numNormals);
  }
  if (numCells == numCellNormals)
  {
    outputCD->SetNormals(cellNormals);
  }
  else
  {
    svtkWarningMacro("Normals for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell normals "
      << numCells << " != " << numCellNormals);
  }

  if (numPts == numTCoords)
  {
    outputPD->SetTCoords(tcoords);
  }
  else
  {
    svtkWarningMacro("TCoords for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point tcoords "
      << numPts << " != " << numTCoords);
  }
  if (numCells == numCellTCoords)
  {
    outputCD->SetTCoords(cellTCoords);
  }
  else
  {
    svtkWarningMacro("TCoords for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell tcoords "
      << numCells << " != " << numCellTCoords);
  }

  if (numPts == numTensors)
  {
    outputPD->SetTensors(tensors);
  }
  else
  {
    svtkWarningMacro("Tensors for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point tensors "
      << numPts << " != " << numTensors);
  }

  if (numCells == numCellTensors)
  {
    outputCD->SetTensors(cellTensors);
  }
  else
  {
    svtkWarningMacro("Tensors for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell tcoords "
      << numCells << " != " << numTCoords);
  }

  svtkFieldListIterator it(this->FieldList);
  svtkDataArray* da;
  const char* name;
  svtkIdType num;
  for (it.Begin(); !it.End(); it.Next())
  {
    pd = it.Get()->Ptr->GetPointData();
    cd = it.Get()->Ptr->GetCellData();
    name = it.Get()->GetName();
    if ((da = pd->GetArray(name)))
    {
      num = da->GetNumberOfTuples();
      if (num == numPts)
      {
        outputPD->AddArray(da);
      }
    }
    if ((da = cd->GetArray(name)))
    {
      num = da->GetNumberOfTuples();
      if (num == numCells)
      {
        outputCD->AddArray(da);
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
//  Trick:  Abstract data types that may or may not be the same type
// (structured/unstructured), but the points/cells match up.
// Output/Geometry may be structured while ScalarInput may be
// unstructured (but really have same triangulation/topology as geometry).
// Just request all the input. Always generate all of the output (todo).
int svtkMergeFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inputInfo;
  int idx;

  for (idx = 0; idx < 6; ++idx)
  {
    inputInfo = inputVector[idx]->GetInformationObject(0);
    if (inputInfo)
    {
      inputInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inputInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
      inputInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
      inputInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
    }
  }
  return 1;
}

int svtkMergeFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  int retval = this->Superclass::FillInputPortInformation(port, info);
  if (port > 0)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return retval;
}

void svtkMergeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
