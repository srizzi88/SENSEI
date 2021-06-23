/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedLocations.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelectedLocations.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointLocator.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkSmartPointer.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkExtractSelectedLocations);

//----------------------------------------------------------------------------
svtkExtractSelectedLocations::svtkExtractSelectedLocations()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkExtractSelectedLocations::~svtkExtractSelectedLocations() = default;

//----------------------------------------------------------------------------
int svtkExtractSelectedLocations::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // verify the input, selection and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    svtkErrorMacro(<< "No input specified");
    return 0;
  }

  if (!selInfo)
  {
    // When not given a selection, quietly select nothing.
    return 1;
  }

  svtkSelection* sel = svtkSelection::SafeDownCast(selInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkSelectionNode* node = nullptr;
  if (sel->GetNumberOfNodes() == 1)
  {
    node = sel->GetNode(0);
  }
  if (!node)
  {
    svtkErrorMacro("Selection must have a single node.");
    return 0;
  }
  if (node->GetContentType() != svtkSelectionNode::LOCATIONS)
  {
    svtkErrorMacro("Incompatible CONTENT_TYPE.");
    return 0;
  }

  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Extracting from dataset");

  int fieldType = svtkSelectionNode::CELL;
  if (node->GetProperties()->Has(svtkSelectionNode::FIELD_TYPE()))
  {
    fieldType = node->GetProperties()->Get(svtkSelectionNode::FIELD_TYPE());
  }
  switch (fieldType)
  {
    case svtkSelectionNode::CELL:
      return this->ExtractCells(node, input, output);
    case svtkSelectionNode::POINT:
      return this->ExtractPoints(node, input, output);
  }
  return 1;
}

// Copy the points marked as "in" and build a pointmap
static void svtkExtractSelectedLocationsCopyPoints(
  svtkDataSet* input, svtkDataSet* output, signed char* inArray, svtkIdType* pointMap)
{
  svtkPoints* newPts = svtkPoints::New();

  svtkIdType i, numPts = input->GetNumberOfPoints();

  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  outPD->SetCopyGlobalIds(1);
  outPD->CopyAllocate(inPD);

  svtkIdTypeArray* originalPtIds = svtkIdTypeArray::New();
  originalPtIds->SetName("svtkOriginalPointIds");
  originalPtIds->SetNumberOfComponents(1);

  for (i = 0; i < numPts; i++)
  {
    if (inArray[i] > 0)
    {
      pointMap[i] = newPts->InsertNextPoint(input->GetPoint(i));
      outPD->CopyData(inPD, i, pointMap[i]);
      originalPtIds->InsertNextValue(i);
    }
    else
    {
      pointMap[i] = -1;
    }
  }

  // outputDS must be either svtkPolyData or svtkUnstructuredGrid
  svtkPointSet::SafeDownCast(output)->SetPoints(newPts);
  newPts->Delete();

  outPD->AddArray(originalPtIds);
  originalPtIds->Delete();
}

// Copy the cells marked as "in" using the given pointmap
template <class T>
void svtkExtractSelectedLocationsCopyCells(
  svtkDataSet* input, T* output, signed char* inArray, svtkIdType* pointMap)
{
  svtkIdType numCells = input->GetNumberOfCells();
  output->AllocateEstimate(numCells / 4, 1);

  svtkCellData* inCD = input->GetCellData();
  svtkCellData* outCD = output->GetCellData();
  outCD->SetCopyGlobalIds(1);
  outCD->CopyAllocate(inCD);

  svtkIdTypeArray* originalIds = svtkIdTypeArray::New();
  originalIds->SetNumberOfComponents(1);
  originalIds->SetName("svtkOriginalCellIds");

  svtkIdType i, j, newId = 0;
  svtkIdList* ptIds = svtkIdList::New();
  for (i = 0; i < numCells; i++)
  {
    if (inArray[i] > 0)
    {
      // special handling for polyhedron cells
      if (svtkUnstructuredGrid::SafeDownCast(input) && svtkUnstructuredGrid::SafeDownCast(output) &&
        input->GetCellType(i) == SVTK_POLYHEDRON)
      {
        ptIds->Reset();
        svtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(i, ptIds);
        svtkUnstructuredGrid::ConvertFaceStreamPointIds(ptIds, pointMap);
      }
      else
      {
        input->GetCellPoints(i, ptIds);
        for (j = 0; j < ptIds->GetNumberOfIds(); j++)
        {
          ptIds->SetId(j, pointMap[ptIds->GetId(j)]);
        }
      }
      output->InsertNextCell(input->GetCellType(i), ptIds);
      outCD->CopyData(inCD, i, newId++);
      originalIds->InsertNextValue(i);
    }
  }

  outCD->AddArray(originalIds);
  originalIds->Delete();
  ptIds->Delete();
}

//----------------------------------------------------------------------------
int svtkExtractSelectedLocations::ExtractCells(
  svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output)
{
  // get a hold of input data structures and allocate output data structures
  svtkDoubleArray* locArray = svtkArrayDownCast<svtkDoubleArray>(sel->GetSelectionList());

  if (!locArray)
  {
    return 1;
  }

  int passThrough = 0;
  if (this->PreserveTopology)
  {
    passThrough = 1;
  }

  int invert = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::INVERSE()))
  {
    invert = sel->GetProperties()->Get(svtkSelectionNode::INVERSE());
  }

  svtkIdType i, numPts = input->GetNumberOfPoints();
  svtkSmartPointer<svtkSignedCharArray> pointInArray = svtkSmartPointer<svtkSignedCharArray>::New();
  pointInArray->SetNumberOfComponents(1);
  pointInArray->SetNumberOfTuples(numPts);
  signed char flag = invert ? 1 : -1;
  for (i = 0; i < numPts; i++)
  {
    pointInArray->SetValue(i, flag);
  }

  svtkIdType numCells = input->GetNumberOfCells();
  svtkSmartPointer<svtkSignedCharArray> cellInArray = svtkSmartPointer<svtkSignedCharArray>::New();
  cellInArray->SetNumberOfComponents(1);
  cellInArray->SetNumberOfTuples(numCells);
  for (i = 0; i < numCells; i++)
  {
    cellInArray->SetValue(i, flag);
  }

  if (passThrough)
  {
    output->ShallowCopy(input);
    pointInArray->SetName("svtkInsidedness");
    svtkPointData* outPD = output->GetPointData();
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
    cellInArray->SetName("svtkInsidedness");
    svtkCellData* outCD = output->GetCellData();
    outCD->AddArray(cellInArray);
    outCD->SetScalars(cellInArray);
  }

  // Reverse the "in" flag
  flag = -flag;

  svtkIdList* ptIds = nullptr;
  char* cellCounter = nullptr;
  if (invert)
  {
    ptIds = svtkIdList::New();
    cellCounter = new char[numPts];
    for (i = 0; i < numPts; ++i)
    {
      cellCounter[i] = 0;
    }
  }

  svtkGenericCell* cell = svtkGenericCell::New();
  svtkIdList* idList = svtkIdList::New();
  svtkIdType numLocs = locArray->GetNumberOfTuples();

  int subId;
  double pcoords[3];
  double* weights = new double[input->GetMaxCellSize()];

  svtkIdType ptId, cellId, locArrayIndex;
  for (locArrayIndex = 0; locArrayIndex < numLocs; locArrayIndex++)
  {
    cellId = input->FindCell(
      locArray->GetTuple(locArrayIndex), nullptr, cell, 0, 0.0, subId, pcoords, weights);
    if ((cellId >= 0) && (cellInArray->GetValue(cellId) != flag))
    {
      cellInArray->SetValue(cellId, flag);
      input->GetCellPoints(cellId, idList);
      if (!invert)
      {
        for (i = 0; i < idList->GetNumberOfIds(); ++i)
        {
          pointInArray->SetValue(idList->GetId(i), flag);
        }
      }
      else
      {
        for (i = 0; i < idList->GetNumberOfIds(); ++i)
        {
          ptId = idList->GetId(i);
          ptIds->InsertUniqueId(ptId);
          cellCounter[ptId]++;
        }
      }
    }
  }

  delete[] weights;
  cell->Delete();

  if (invert)
  {
    for (i = 0; i < ptIds->GetNumberOfIds(); ++i)
    {
      ptId = ptIds->GetId(i);
      input->GetPointCells(ptId, idList);
      if (cellCounter[ptId] == idList->GetNumberOfIds())
      {
        pointInArray->SetValue(ptId, flag);
      }
    }
    ptIds->Delete();
    delete[] cellCounter;
  }

  idList->Delete();

  if (!passThrough)
  {
    svtkIdType* pointMap = new svtkIdType[numPts]; // maps old point ids into new
    svtkExtractSelectedLocationsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (output->GetDataObjectType() == SVTK_POLY_DATA)
    {
      svtkExtractSelectedLocationsCopyCells<svtkPolyData>(
        input, svtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
    }
    else
    {
      svtkExtractSelectedLocationsCopyCells<svtkUnstructuredGrid>(
        input, svtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
    }
    delete[] pointMap;
    this->UpdateProgress(1.0);
  }

  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedLocations::ExtractPoints(
  svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output)
{
  // get a hold of input data structures and allocate output data structures
  svtkDoubleArray* locArray = svtkArrayDownCast<svtkDoubleArray>(sel->GetSelectionList());
  if (!locArray)
  {
    return 1;
  }

  int passThrough = 0;
  if (this->PreserveTopology)
  {
    passThrough = 1;
  }

  int invert = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::INVERSE()))
  {
    invert = sel->GetProperties()->Get(svtkSelectionNode::INVERSE());
  }

  int containingCells = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::CONTAINING_CELLS()))
  {
    containingCells = sel->GetProperties()->Get(svtkSelectionNode::CONTAINING_CELLS());
  }

  double epsilon = 0.1;
  if (sel->GetProperties()->Has(svtkSelectionNode::EPSILON()))
  {
    epsilon = sel->GetProperties()->Get(svtkSelectionNode::EPSILON());
  }

  svtkIdType i, numPts = input->GetNumberOfPoints();
  svtkSmartPointer<svtkSignedCharArray> pointInArray = svtkSmartPointer<svtkSignedCharArray>::New();
  pointInArray->SetNumberOfComponents(1);
  pointInArray->SetNumberOfTuples(numPts);
  signed char flag = invert ? 1 : -1;
  for (i = 0; i < numPts; i++)
  {
    pointInArray->SetValue(i, flag);
  }

  svtkIdType numCells = input->GetNumberOfCells();
  svtkSmartPointer<svtkSignedCharArray> cellInArray;
  if (containingCells)
  {
    cellInArray = svtkSmartPointer<svtkSignedCharArray>::New();
    cellInArray->SetNumberOfComponents(1);
    cellInArray->SetNumberOfTuples(numCells);
    for (i = 0; i < numCells; i++)
    {
      cellInArray->SetValue(i, flag);
    }
  }

  if (passThrough)
  {
    output->ShallowCopy(input);
    pointInArray->SetName("svtkInsidedness");
    svtkPointData* outPD = output->GetPointData();
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
    if (containingCells)
    {
      cellInArray->SetName("svtkInsidedness");
      svtkCellData* outCD = output->GetCellData();
      outCD->AddArray(cellInArray);
      outCD->SetScalars(cellInArray);
    }
  }

  // Reverse the "in" flag
  flag = -flag;

  svtkPointLocator* locator = nullptr;

  if (input->IsA("svtkPointSet"))
  {
    locator = svtkPointLocator::New();
    locator->SetDataSet(input);
  }

  svtkIdList* ptCells = svtkIdList::New();
  svtkIdList* cellPts = svtkIdList::New();
  svtkIdType numLocs = locArray->GetNumberOfTuples();
  double dist2;
  svtkIdType j, ptId, cellId, locArrayIndex;
  double epsSquared = epsilon * epsilon;
  if (numPts > 0)
  {
    for (locArrayIndex = 0; locArrayIndex < numLocs; locArrayIndex++)
    {
      if (locator != nullptr)
      {
        ptId =
          locator->FindClosestPointWithinRadius(epsilon, locArray->GetTuple(locArrayIndex), dist2);
      }
      else
      {
        double* L = locArray->GetTuple(locArrayIndex);
        ptId = input->FindPoint(locArray->GetTuple(locArrayIndex));
        if (ptId >= 0)
        {
          double* X = input->GetPoint(ptId);
          double dx = X[0] - L[0];
          dx = dx * dx;
          double dy = X[1] - L[1];
          dy = dy * dy;
          double dz = X[2] - L[2];
          dz = dz * dz;
          if (dx + dy + dz > epsSquared)
          {
            ptId = -1;
          }
        }
      }

      if ((ptId >= 0) && (pointInArray->GetValue(ptId) != flag))
      {
        pointInArray->SetValue(ptId, flag);
        if (containingCells)
        {
          input->GetPointCells(ptId, ptCells);
          for (i = 0; i < ptCells->GetNumberOfIds(); ++i)
          {
            cellId = ptCells->GetId(i);
            if (!passThrough && !invert && cellInArray->GetValue(cellId) != flag)
            {
              input->GetCellPoints(cellId, cellPts);
              for (j = 0; j < cellPts->GetNumberOfIds(); ++j)
              {
                pointInArray->SetValue(cellPts->GetId(j), flag);
              }
            }
            cellInArray->SetValue(cellId, flag);
          }
        }
      }
    }
  }
  else
  {
    ptId = -1;
  }

  ptCells->Delete();
  cellPts->Delete();
  if (locator)
  {
    locator->SetDataSet(nullptr);
    locator->Delete();
  }

  if (!passThrough)
  {
    svtkIdType* pointMap = new svtkIdType[numPts]; // maps old point ids into new
    svtkExtractSelectedLocationsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (containingCells)
    {
      if (output->GetDataObjectType() == SVTK_POLY_DATA)
      {
        svtkExtractSelectedLocationsCopyCells<svtkPolyData>(
          input, svtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
      else
      {
        svtkExtractSelectedLocationsCopyCells<svtkUnstructuredGrid>(
          input, svtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
    }
    else
    {
      numPts = output->GetNumberOfPoints();
      svtkUnstructuredGrid* outputUG = svtkUnstructuredGrid::SafeDownCast(output);
      outputUG->Allocate(numPts);
      for (i = 0; i < numPts; ++i)
      {
        outputUG->InsertNextCell(SVTK_VERTEX, 1, &i);
      }
    }
    delete[] pointMap;
    this->UpdateProgress(1.0);
  }

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedLocations::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
