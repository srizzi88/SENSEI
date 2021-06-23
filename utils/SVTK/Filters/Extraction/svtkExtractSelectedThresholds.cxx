/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedThresholds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelectedThresholds.h"

#include "svtkArrayDispatch.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkThreshold.h"
#include "svtkUnstructuredGrid.h"

#include <algorithm>

svtkStandardNewMacro(svtkExtractSelectedThresholds);

//----------------------------------------------------------------------------
svtkExtractSelectedThresholds::svtkExtractSelectedThresholds()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkExtractSelectedThresholds::~svtkExtractSelectedThresholds() = default;

//----------------------------------------------------------------------------
int svtkExtractSelectedThresholds::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDataObject* inputDO = svtkDataObject::GetData(inInfo);

  // verify the input, selection and output
  if (!selInfo)
  {
    // When not given a selection, quietly select nothing.
    return 1;
  }

  svtkSelection* sel = svtkSelection::GetData(selInfo);
  svtkSelectionNode* node = nullptr;
  if (sel->GetNumberOfNodes() == 1)
  {
    node = sel->GetNode(0);
  }
  if (!node)
  {
    svtkErrorMacro("Selection must have a single node.");
    return 1;
  }
  if (!node->GetProperties()->Has(svtkSelectionNode::CONTENT_TYPE()) ||
    node->GetProperties()->Get(svtkSelectionNode::CONTENT_TYPE()) != svtkSelectionNode::THRESHOLDS)
  {
    svtkErrorMacro("Missing or invalid CONTENT_TYPE.");
    return 1;
  }

  if (svtkDataSet* input = svtkDataSet::SafeDownCast(inputDO))
  {
    if (input->GetNumberOfCells() == 0 && input->GetNumberOfPoints() == 0)
    {
      // empty input, nothing to do..
      return 1;
    }

    svtkDataSet* output = svtkDataSet::GetData(outInfo);
    svtkDebugMacro(<< "Extracting from dataset");

    int thresholdByPointVals = 0;
    int fieldType = svtkSelectionNode::CELL;
    if (node->GetProperties()->Has(svtkSelectionNode::FIELD_TYPE()))
    {
      fieldType = node->GetProperties()->Get(svtkSelectionNode::FIELD_TYPE());
      if (fieldType == svtkSelectionNode::POINT)
      {
        if (node->GetProperties()->Has(svtkSelectionNode::CONTAINING_CELLS()))
        {
          thresholdByPointVals = node->GetProperties()->Get(svtkSelectionNode::CONTAINING_CELLS());
        }
      }
    }

    if (thresholdByPointVals || fieldType == svtkSelectionNode::CELL)
    {
      return this->ExtractCells(node, input, output, thresholdByPointVals);
    }
    if (fieldType == svtkSelectionNode::POINT)
    {
      return this->ExtractPoints(node, input, output);
    }
  }
  else if (svtkTable* inputTable = svtkTable::SafeDownCast(inputDO))
  {
    if (inputTable->GetNumberOfRows() == 0)
    {
      return 1;
    }
    svtkTable* output = svtkTable::GetData(outInfo);
    return this->ExtractRows(node, inputTable, output);
  }

  return 0;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedThresholds::ExtractCells(
  svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output, int usePointScalars)
{
  // find the values to threshold within
  svtkDataArray* lims = svtkArrayDownCast<svtkDataArray>(sel->GetSelectionList());
  if (lims == nullptr)
  {
    svtkErrorMacro(<< "No values to threshold with");
    return 1;
  }

  // find out what array we are supposed to threshold in
  svtkDataArray* inScalars = nullptr;
  bool use_ids = false;
  if (usePointScalars)
  {
    if (sel->GetSelectionList()->GetName())
    {
      if (strcmp(sel->GetSelectionList()->GetName(), "svtkGlobalIds") == 0)
      {
        inScalars = input->GetPointData()->GetGlobalIds();
      }
      else if (strcmp(sel->GetSelectionList()->GetName(), "svtkIndices") == 0)
      {
        use_ids = true;
      }
      else
      {
        inScalars = input->GetPointData()->GetArray(sel->GetSelectionList()->GetName());
      }
    }
    else
    {
      inScalars = input->GetPointData()->GetScalars();
    }
  }
  else
  {
    if (sel->GetSelectionList()->GetName())
    {
      if (strcmp(sel->GetSelectionList()->GetName(), "svtkGlobalIds") == 0)
      {
        inScalars = input->GetCellData()->GetGlobalIds();
      }
      else if (strcmp(sel->GetSelectionList()->GetName(), "svtkIndices") == 0)
      {
        use_ids = true;
      }
      else
      {
        inScalars = input->GetCellData()->GetArray(sel->GetSelectionList()->GetName());
      }
    }
    else
    {
      inScalars = input->GetCellData()->GetScalars();
    }
  }
  if (inScalars == nullptr && !use_ids)
  {
    svtkErrorMacro("Could not figure out what array to threshold in.");
    return 1;
  }

  int inverse = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::INVERSE()))
  {
    inverse = sel->GetProperties()->Get(svtkSelectionNode::INVERSE());
  }

  int passThrough = 0;
  if (this->PreserveTopology)
  {
    passThrough = 1;
  }

  int comp_no = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::COMPONENT_NUMBER()))
  {
    comp_no = sel->GetProperties()->Get(svtkSelectionNode::COMPONENT_NUMBER());
  }

  svtkIdType cellId, newCellId;
  svtkIdList *cellPts, *pointMap = nullptr;
  svtkIdList* newCellPts = nullptr;
  svtkCell* cell = nullptr;
  svtkPoints* newPoints = nullptr;
  svtkIdType i, ptId, newId, numPts, numCells;
  svtkIdType numCellPts;
  double x[3];

  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();
  int keepCell;

  outPD->CopyGlobalIdsOn();
  outPD->CopyAllocate(pd);
  outCD->CopyGlobalIdsOn();
  outCD->CopyAllocate(cd);

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  svtkDataSet* outputDS = output;
  svtkSignedCharArray* pointInArray = nullptr;
  svtkSignedCharArray* cellInArray = nullptr;

  svtkUnstructuredGrid* outputUG = nullptr;
  svtkIdTypeArray* originalCellIds = nullptr;
  svtkIdTypeArray* originalPointIds = nullptr;

  signed char flag = inverse ? 1 : -1;

  if (passThrough)
  {
    outputDS->ShallowCopy(input);

    pointInArray = svtkSignedCharArray::New();
    pointInArray->SetNumberOfComponents(1);
    pointInArray->SetNumberOfTuples(numPts);
    for (i = 0; i < numPts; i++)
    {
      pointInArray->SetValue(i, flag);
    }
    pointInArray->SetName("svtkInsidedness");
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);

    cellInArray = svtkSignedCharArray::New();
    cellInArray->SetNumberOfComponents(1);
    cellInArray->SetNumberOfTuples(numCells);
    for (i = 0; i < numCells; i++)
    {
      cellInArray->SetValue(i, flag);
    }
    cellInArray->SetName("svtkInsidedness");
    outCD->AddArray(cellInArray);
    outCD->SetScalars(cellInArray);
  }
  else
  {
    outputUG = svtkUnstructuredGrid::SafeDownCast(output);
    outputUG->Allocate(input->GetNumberOfCells());
    newPoints = svtkPoints::New();
    newPoints->Allocate(numPts);

    pointMap = svtkIdList::New(); // maps old point ids into new
    pointMap->SetNumberOfIds(numPts);
    for (i = 0; i < numPts; i++)
    {
      pointMap->SetId(i, -1);
    }

    newCellPts = svtkIdList::New();

    originalCellIds = svtkIdTypeArray::New();
    originalCellIds->SetName("svtkOriginalCellIds");
    originalCellIds->SetNumberOfComponents(1);
    outCD->AddArray(originalCellIds);

    originalPointIds = svtkIdTypeArray::New();
    originalPointIds->SetName("svtkOriginalPointIds");
    originalPointIds->SetNumberOfComponents(1);
    outPD->AddArray(originalPointIds);
    originalPointIds->Delete();
  }

  flag = -flag;

  // Check that the scalars of each cell satisfy the threshold criterion
  for (cellId = 0; cellId < input->GetNumberOfCells(); cellId++)
  {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    // BUG: This code misses the case where the threshold is contained
    // completely within the cell but none of its points are inside
    // the range.  Consider as an example the threshold range [1, 2]
    // with a cell [0, 3].
    if (usePointScalars)
    {
      keepCell = 0;
      int totalAbove = 0;
      int totalBelow = 0;
      for (i = 0; (i < numCellPts) && (passThrough || !keepCell); i++)
      {
        int above = 0;
        int below = 0;
        ptId = cellPts->GetId(i);
        int inside = this->EvaluateValue(inScalars, comp_no, ptId, lims, &above, &below, nullptr);
        totalAbove += above;
        totalBelow += below;
        // Have we detected a cell that straddles the threshold?
        if ((!inside) && (totalAbove && totalBelow))
        {
          inside = 1;
        }
        if (passThrough && (inside ^ inverse))
        {
          pointInArray->SetValue(ptId, flag);
          cellInArray->SetValue(cellId, flag);
        }
        keepCell |= inside;
      }
    }
    else // use cell scalars
    {
      keepCell = this->EvaluateValue(inScalars, comp_no, cellId, lims);
      if (passThrough && (keepCell ^ inverse))
      {
        cellInArray->SetValue(cellId, flag);
      }
    }

    if (!passThrough && (numCellPts > 0) && (keepCell + inverse == 1)) // Poor man's XOR
    {
      // satisfied thresholding (also non-empty cell, i.e. not SVTK_EMPTY_CELL)
      originalCellIds->InsertNextValue(cellId);

      for (i = 0; i < numCellPts; i++)
      {
        ptId = cellPts->GetId(i);
        if ((newId = pointMap->GetId(ptId)) < 0)
        {
          input->GetPoint(ptId, x);
          newId = newPoints->InsertNextPoint(x);
          pointMap->SetId(ptId, newId);
          outPD->CopyData(pd, ptId, newId);
          originalPointIds->InsertNextValue(ptId);
        }
        newCellPts->InsertId(i, newId);
      }
      newCellId = outputUG->InsertNextCell(cell->GetCellType(), newCellPts);
      outCD->CopyData(cd, cellId, newCellId);
      newCellPts->Reset();
    } // satisfied thresholding
  }   // for all cells

  // now clean up / update ourselves
  if (passThrough)
  {
    pointInArray->Delete();
    cellInArray->Delete();
  }
  else
  {
    outputUG->SetPoints(newPoints);
    newPoints->Delete();
    pointMap->Delete();
    newCellPts->Delete();
    originalCellIds->Delete();
  }

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedThresholds::ExtractPoints(
  svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output)
{
  // find the values to threshold within
  svtkDataArray* lims = svtkArrayDownCast<svtkDataArray>(sel->GetSelectionList());
  if (lims == nullptr)
  {
    svtkErrorMacro(<< "No values to threshold with");
    return 1;
  }

  // find out what array we are supposed to threshold in
  svtkDataArray* inScalars = nullptr;
  bool use_ids = false;
  if (sel->GetSelectionList()->GetName())
  {
    if (strcmp(sel->GetSelectionList()->GetName(), "svtkGlobalIds") == 0)
    {
      inScalars = input->GetPointData()->GetGlobalIds();
    }
    else if (strcmp(sel->GetSelectionList()->GetName(), "svtkIndices") == 0)
    {
      use_ids = true;
    }
    else
    {
      inScalars = input->GetPointData()->GetArray(sel->GetSelectionList()->GetName());
    }
  }
  else
  {
    inScalars = input->GetPointData()->GetScalars();
  }
  if (inScalars == nullptr && !use_ids)
  {
    svtkErrorMacro("Could not figure out what array to threshold in.");
    return 1;
  }

  int inverse = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::INVERSE()))
  {
    inverse = sel->GetProperties()->Get(svtkSelectionNode::INVERSE());
  }

  int passThrough = 0;
  if (this->PreserveTopology)
  {
    passThrough = 1;
  }

  int comp_no = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::COMPONENT_NUMBER()))
  {
    comp_no = sel->GetProperties()->Get(svtkSelectionNode::COMPONENT_NUMBER());
  }

  svtkIdType numPts = input->GetNumberOfPoints();
  svtkPointData* inputPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();

  svtkDataSet* outputDS = output;
  svtkSignedCharArray* pointInArray = nullptr;

  svtkUnstructuredGrid* outputUG = nullptr;
  svtkPoints* newPts = svtkPoints::New();

  svtkIdTypeArray* originalPointIds = nullptr;

  signed char flag = inverse ? 1 : -1;

  if (passThrough)
  {
    outputDS->ShallowCopy(input);

    pointInArray = svtkSignedCharArray::New();
    pointInArray->SetNumberOfComponents(1);
    pointInArray->SetNumberOfTuples(numPts);
    for (svtkIdType i = 0; i < numPts; i++)
    {
      pointInArray->SetValue(i, flag);
    }
    pointInArray->SetName("svtkInsidedness");
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
  }
  else
  {
    outputUG = svtkUnstructuredGrid::SafeDownCast(output);
    outputUG->Allocate(numPts);

    newPts->Allocate(numPts);
    outputUG->SetPoints(newPts);

    outPD->CopyGlobalIdsOn();
    outPD->CopyAllocate(inputPD);

    originalPointIds = svtkIdTypeArray::New();
    originalPointIds->SetNumberOfComponents(1);
    originalPointIds->SetName("svtkOriginalPointIds");
    outPD->AddArray(originalPointIds);
    originalPointIds->Delete();
  }

  flag = -flag;

  svtkIdType outPtCnt = 0;
  for (svtkIdType ptId = 0; ptId < numPts; ptId++)
  {
    int keepPoint = this->EvaluateValue(inScalars, comp_no, ptId, lims);
    if (keepPoint ^ inverse)
    {
      if (passThrough)
      {
        pointInArray->SetValue(ptId, flag);
      }
      else
      {
        double X[4];
        input->GetPoint(ptId, X);
        newPts->InsertNextPoint(X);
        outPD->CopyData(inputPD, ptId, outPtCnt);
        originalPointIds->InsertNextValue(ptId);
        outputUG->InsertNextCell(SVTK_VERTEX, 1, &outPtCnt);
        outPtCnt++;
      }
    }
  }

  if (passThrough)
  {
    pointInArray->Delete();
  }
  newPts->Delete();
  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedThresholds::ExtractRows(
  svtkSelectionNode* sel, svtkTable* input, svtkTable* output)
{
  // find the values to threshold within
  svtkDataArray* lims = svtkArrayDownCast<svtkDataArray>(sel->GetSelectionList());
  if (lims == nullptr)
  {
    svtkErrorMacro(<< "No values to threshold with");
    return 1;
  }

  // Determine the array to threshold.
  svtkDataArray* inScalars = nullptr;
  bool use_ids = false;
  if (sel->GetSelectionList()->GetName())
  {
    if (strcmp(sel->GetSelectionList()->GetName(), "svtkGlobalIds") == 0)
    {
      inScalars = input->GetRowData()->GetGlobalIds();
    }
    else if (strcmp(sel->GetSelectionList()->GetName(), "svtkIndices") == 0)
    {
      use_ids = true;
    }
    else
    {
      inScalars = input->GetRowData()->GetArray(sel->GetSelectionList()->GetName());
    }
  }

  if (inScalars == nullptr && !use_ids)
  {
    svtkErrorMacro("Could not figure out what array to threshold in.");
    return 1;
  }

  int inverse = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::INVERSE()))
  {
    inverse = sel->GetProperties()->Get(svtkSelectionNode::INVERSE());
  }

  int passThrough = 0;
  if (this->PreserveTopology)
  {
    passThrough = 1;
  }

  int comp_no = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::COMPONENT_NUMBER()))
  {
    comp_no = sel->GetProperties()->Get(svtkSelectionNode::COMPONENT_NUMBER());
  }

  svtkDataSetAttributes* inRD = input->GetRowData();
  svtkDataSetAttributes* outRD = output->GetRowData();
  svtkSmartPointer<svtkSignedCharArray> rowInArray;
  svtkSmartPointer<svtkIdTypeArray> originalRowIds;
  svtkIdType numRows = input->GetNumberOfRows();

  signed char flag = inverse ? 1 : -1;

  if (passThrough)
  {
    output->ShallowCopy(input);

    rowInArray = svtkSmartPointer<svtkSignedCharArray>::New();
    rowInArray->SetNumberOfComponents(1);
    rowInArray->SetNumberOfTuples(numRows);
    std::fill(rowInArray->GetPointer(0), rowInArray->GetPointer(0) + numRows, flag);
    rowInArray->SetName("svtkInsidedness");
    outRD->AddArray(rowInArray);
  }
  else
  {
    outRD->CopyGlobalIdsOn();
    outRD->CopyAllocate(inRD);

    originalRowIds = svtkSmartPointer<svtkIdTypeArray>::New();
    originalRowIds->SetNumberOfComponents(1);
    originalRowIds->SetName("svtkOriginalRowIds");
    originalRowIds->Allocate(numRows);
    outRD->AddArray(originalRowIds);
  }

  flag = -flag;

  svtkIdType outRCnt = 0;
  for (svtkIdType rowId = 0; rowId < numRows; rowId++)
  {
    int keepRow = this->EvaluateValue(inScalars, comp_no, rowId, lims);
    if (keepRow ^ inverse)
    {
      if (passThrough)
      {
        rowInArray->SetValue(rowId, flag);
      }
      else
      {
        outRD->CopyData(inRD, rowId, outRCnt);
        originalRowIds->InsertNextValue(rowId);
        outRCnt++;
      }
    }
  }
  outRD->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedThresholds::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

namespace
{
struct TestItem
{
  template <class ArrayT>
  void operator()(ArrayT* lims, double value, int& keep) const
  {
    auto limsRange = svtk::DataArrayValueRange(lims);
    assert(limsRange.size() % 2 == 0);
    for (auto i = limsRange.begin(); i < limsRange.end(); i += 2)
    {
      if (value >= *i && value <= *(i + 1))
      {
        keep = true;
        return;
      }
    }
    keep = false;
  }

  template <class ArrayT>
  void operator()(
    ArrayT* lims, double value, int& keepCell, int& above, int& below, int& inside) const
  {
    auto limsRange = svtk::DataArrayValueRange(lims);
    assert(limsRange.size() % 2 == 0);
    for (auto i = limsRange.begin(); i < limsRange.end(); i += 2)
    {
      const auto& low = *i;
      const auto& high = *(i + 1);
      if (value >= low && value <= high)
      {
        keepCell = true;
        ++inside;
      }
      else if (value < low)
      {
        ++below;
      }
      else if (value > high)
      {
        ++above;
      }
    }
    keepCell = false;
  }
};
} // namespace

//----------------------------------------------------------------------------
int svtkExtractSelectedThresholds::EvaluateValue(
  svtkDataArray* scalars, int comp_no, svtkIdType id, svtkDataArray* lims)
{
  int keepCell = 0;
  // check the value in the array against all of the thresholds in lims
  // if it is inside any, return true
  double value = 0.0;
  if (comp_no < 0 && scalars)
  {
    // use magnitude.
    int numComps = scalars->GetNumberOfComponents();
    const double* tuple = scalars->GetTuple(id);
    for (int cc = 0; cc < numComps; cc++)
    {
      value += tuple[cc] * tuple[cc];
    }
    value = sqrt(value);
  }
  else
  {
    value = scalars ? scalars->GetComponent(id, comp_no)
                    : static_cast<double>(id); /// <=== precision loss when using id.
  }

  if (!svtkArrayDispatch::Dispatch::Execute(lims, TestItem{}, value, keepCell))
  {
    TestItem{}(lims, value, keepCell);
  }
  return keepCell;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedThresholds::EvaluateValue(svtkDataArray* scalars, int comp_no, svtkIdType id,
  svtkDataArray* lims, int* AboveCount, int* BelowCount, int* InsideCount)
{
  double value = 0.0;
  if (comp_no < 0 && scalars)
  {
    // use magnitude.
    int numComps = scalars->GetNumberOfComponents();
    const double* tuple = scalars->GetTuple(id);
    for (int cc = 0; cc < numComps; cc++)
    {
      value += tuple[cc] * tuple[cc];
    }
    value = sqrt(value);
  }
  else
  {
    value = scalars ? scalars->GetComponent(id, comp_no)
                    : static_cast<double>(id); /// <=== precision loss when using id.
  }

  int keepCell = 0;
  // check the value in the array against all of the thresholds in lims
  // if it is inside any, return true
  int above = 0;
  int below = 0;
  int inside = 0;

  if (!svtkArrayDispatch::Dispatch::Execute(lims, TestItem{}, value, keepCell, above, below, inside))
  {
    TestItem{}(lims, value, keepCell, above, below, inside);
  }

  if (AboveCount)
    *AboveCount = above;
  if (BelowCount)
    *BelowCount = below;
  if (InsideCount)
    *InsideCount = inside;
  return keepCell;
}
