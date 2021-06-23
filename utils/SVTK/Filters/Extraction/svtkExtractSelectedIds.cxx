/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelectedIds.h"

#include "svtkArrayDispatch.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkDataArrayRange.h"
#include "svtkExtractCells.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkSmartPointer.h"
#include "svtkSortDataArray.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkExtractSelectedIds);

//----------------------------------------------------------------------------
svtkExtractSelectedIds::svtkExtractSelectedIds()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkExtractSelectedIds::~svtkExtractSelectedIds() = default;

//----------------------------------------------------------------------------
int svtkExtractSelectedIds::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 0)
  {
    // this filter can only work with datasets.
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedIds::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // verify the input selection and output
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
  if (node->GetContentType() != svtkSelectionNode::GLOBALIDS &&
    node->GetContentType() != svtkSelectionNode::PEDIGREEIDS &&
    node->GetContentType() != svtkSelectionNode::VALUES &&
    node->GetContentType() != svtkSelectionNode::INDICES)
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
static void svtkExtractSelectedIdsCopyPoints(
  svtkDataSet* input, svtkDataSet* output, signed char* inArray, svtkIdType* pointMap)
{
  svtkPoints* newPts = svtkPoints::New();

  svtkIdType i, numPts = input->GetNumberOfPoints();

  svtkIdTypeArray* originalPtIds = svtkIdTypeArray::New();
  originalPtIds->SetNumberOfComponents(1);
  originalPtIds->SetName("svtkOriginalPointIds");

  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  outPD->SetCopyGlobalIds(1);
  outPD->CopyAllocate(inPD);

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

  outPD->AddArray(originalPtIds);
  originalPtIds->Delete();

  // outputDS must be either svtkPolyData or svtkUnstructuredGrid
  svtkPointSet::SafeDownCast(output)->SetPoints(newPts);
  newPts->Delete();
}

// Copy the cells marked as "in" using the given pointmap
template <class T>
void svtkExtractSelectedIdsCopyCells(
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

namespace
{
struct svtkESIDeepCopyImpl
{

  template <class ArrayOut, class ArrayIn>
  void operator()(ArrayOut* outArray, ArrayIn* inArray, int compno) const
  {
    using T = svtk::GetAPIType<ArrayOut>;
    const auto inRange = svtk::DataArrayTupleRange(inArray);
    auto outRange = svtk::DataArrayValueRange(outArray);

    auto out = outRange.begin();

    if (compno < 0)
    {
      for (auto tuple : inRange)
      {
        double mag = 0;
        for (auto comp : tuple)
        {
          mag += static_cast<double>(comp) * static_cast<double>(comp);
        }
        mag = sqrt(mag);
        *out = static_cast<T>(mag);
        out++;
      }
    }
    else
    {
      for (auto tuple : inRange)
      {
        *out = tuple[compno];
        out++;
      }
    }
  }

  void operator()(
    svtkStdString* out, svtkStdString* in, int compno, int num_comps, svtkIdType numTuples) const
  {
    if (compno < 0)
    {
      // we cannot compute magnitudes for string arrays!
      compno = 0;
    }
    for (svtkIdType cc = 0; cc < numTuples; cc++)
    {
      *out = in[compno];
      out++;
      in += num_comps;
    }
  }
};

// Deep copies a specified component (or magnitude of compno < 0).
static void svtkESIDeepCopy(svtkAbstractArray* out, svtkAbstractArray* in, int compno)
{
  if (in->GetNumberOfComponents() == 1)
  {
    // trivial case.
    out->DeepCopy(in);
    return;
  }

  svtkIdType numTuples = in->GetNumberOfTuples();
  out->SetNumberOfComponents(1);
  out->SetNumberOfTuples(numTuples);

  svtkDataArray* dataArrayIn = svtkDataArray::SafeDownCast(in);
  svtkDataArray* dataArrayOut = svtkDataArray::SafeDownCast(out);
  svtkESIDeepCopyImpl copy_worklet;
  if (dataArrayIn && dataArrayOut)
  {
    const bool executed = svtkArrayDispatch::Dispatch2SameValueType::Execute(
      dataArrayIn, dataArrayOut, copy_worklet, compno);
    if (!executed)
    { // fallback to svtkDataArray dispatch access
      copy_worklet(dataArrayIn, dataArrayOut, compno);
    }
  }
  else
  {
    svtkStringArray* strArrayIn = svtkStringArray::SafeDownCast(in);
    svtkStringArray* strArrayOut = svtkStringArray::SafeDownCast(out);
    if (strArrayIn && strArrayOut)
    {
      copy_worklet(strArrayIn->GetPointer(0), strArrayOut->GetPointer(0), compno,
        in->GetNumberOfComponents(), numTuples);
    }
  }
}

//---------------------
struct svtkExtractSelectedIdsExtractCells
{
  template <typename... Args>
  void operator()(svtkStringArray* id, svtkStringArray* label, Args&&... args) const
  {
    this->execute(id->GetPointer(0), label->GetPointer(0), std::forward<Args>(args)...);
  }
  template <typename ArrayT, typename ArrayU, typename... Args>
  void operator()(ArrayT id, ArrayU label, Args&&... args) const
  {
    const auto idRange = svtk::DataArrayValueRange(id);
    const auto labelRange = svtk::DataArrayValueRange(label);
    this->execute(idRange.begin(), labelRange.begin(), std::forward<Args>(args)...);
  }

  template <typename IdIter, typename LabelIter>
  void execute(IdIter id, LabelIter label, svtkExtractSelectedIds* self, int passThrough, int invert,
    svtkDataSet* input, svtkIdTypeArray* idxArray, svtkSignedCharArray* cellInArray,
    svtkSignedCharArray* pointInArray, svtkIdType numIds) const
  {
    using T1 = typename std::iterator_traits<IdIter>::value_type;
    using T2 = typename std::iterator_traits<LabelIter>::value_type;

    // Reverse the "in" flag
    signed char flag = invert ? 1 : -1;
    flag = -flag;

    svtkIdType numCells = input->GetNumberOfCells();
    svtkIdType numPts = input->GetNumberOfPoints();
    svtkIdList* idList = svtkIdList::New();
    svtkIdList* ptIds = nullptr;
    char* cellCounter = nullptr;
    if (invert)
    {
      ptIds = svtkIdList::New();
      cellCounter = new char[numPts];
      for (svtkIdType i = 0; i < numPts; ++i)
      {
        cellCounter[i] = 0;
      }
    }

    svtkIdType idArrayIndex = 0, labelArrayIndex = 0;

    // Check each cell to see if it's selected
    while (labelArrayIndex < numCells)
    {
      // Advance through the selection ids until we find
      // one that's NOT LESS THAN the current cell label.
      bool idLessThanLabel = false;
      if (idArrayIndex < numIds)
      {
        idLessThanLabel = (id[idArrayIndex] < static_cast<T1>(label[labelArrayIndex]));
      }
      while ((idArrayIndex < numIds) && idLessThanLabel)
      {
        ++idArrayIndex;
        if (idArrayIndex >= numIds)
        {
          break;
        }
        idLessThanLabel = (id[idArrayIndex] < static_cast<T1>(label[labelArrayIndex]));
      }

      if (idArrayIndex >= numIds)
      {
        // We're out of selection ids, so we're done.
        break;
      }
      self->UpdateProgress(static_cast<double>(idArrayIndex) / (numIds * (passThrough + 1)));

      // Advance through and mark all cells with a label EQUAL TO the
      // current selection id, as well as their points.
      bool idEqualToLabel = (id[idArrayIndex] == static_cast<T1>(label[labelArrayIndex]));
      while (idEqualToLabel)
      {
        svtkIdType cellId = idxArray->GetValue(labelArrayIndex);
        cellInArray->SetValue(cellId, flag);
        input->GetCellPoints(cellId, idList);
        if (!invert)
        {
          for (svtkIdType i = 0; i < idList->GetNumberOfIds(); ++i)
          {
            pointInArray->SetValue(idList->GetId(i), flag);
          }
        }
        else
        {
          for (svtkIdType i = 0; i < idList->GetNumberOfIds(); ++i)
          {
            svtkIdType ptId = idList->GetId(i);
            ptIds->InsertUniqueId(ptId);
            cellCounter[ptId]++;
          }
        }
        ++labelArrayIndex;
        if (labelArrayIndex >= numCells)
        {
          break;
        }
        idEqualToLabel = (id[idArrayIndex] == static_cast<T1>(label[labelArrayIndex]));
      }

      // Advance through cell labels until we find
      // one that's NOT LESS THAN the current selection id.
      bool labelLessThanId = false;
      if (labelArrayIndex < numCells)
      {
        labelLessThanId = (label[labelArrayIndex] < static_cast<T2>(id[idArrayIndex]));
      }
      while ((labelArrayIndex < numCells) && labelLessThanId)
      {
        ++labelArrayIndex;
        if (labelArrayIndex >= numCells)
        {
          break;
        }
        labelLessThanId = (label[labelArrayIndex] < static_cast<T2>(id[idArrayIndex]));
      }
    }

    if (invert)
    {
      for (svtkIdType i = 0; i < ptIds->GetNumberOfIds(); ++i)
      {
        svtkIdType ptId = ptIds->GetId(i);
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
  }
};

//---------------------
struct svtkExtractSelectedIdsExtractPoints
{
  template <typename... Args>
  void operator()(svtkStringArray* id, svtkStringArray* label, Args&&... args) const
  {
    this->execute(id->GetPointer(0), label->GetPointer(0), std::forward<Args>(args)...);
  }
  template <typename ArrayT, typename ArrayU, typename... Args>
  void operator()(ArrayT id, ArrayU label, Args&&... args) const
  {
    const auto idRange = svtk::DataArrayValueRange(id);
    const auto labelRange = svtk::DataArrayValueRange(label);
    this->execute(idRange.cbegin(), labelRange.cbegin(), std::forward<Args>(args)...);
  }

  template <typename IdIter, typename LabelIter>
  void execute(IdIter id, LabelIter label, svtkExtractSelectedIds* self, int passThrough, int invert,
    int containingCells, svtkDataSet* input, svtkIdTypeArray* idxArray,
    svtkSignedCharArray* cellInArray, svtkSignedCharArray* pointInArray, svtkIdType numIds) const
  {
    using T1 = typename std::iterator_traits<IdIter>::value_type;
    using T2 = typename std::iterator_traits<LabelIter>::value_type;

    // Reverse the "in" flag
    signed char flag = invert ? 1 : -1;
    flag = -flag;

    svtkIdList* ptCells = nullptr;
    svtkIdList* cellPts = nullptr;
    if (containingCells)
    {
      ptCells = svtkIdList::New();
      cellPts = svtkIdList::New();
    }

    svtkIdType numPts = input->GetNumberOfPoints();
    svtkIdType idArrayIndex = 0, labelArrayIndex = 0;

    // Check each point to see if it's selected
    while (labelArrayIndex < numPts)
    {
      // Advance through the selection ids until we find
      // one that's NOT LESS THAN the current point label.
      bool idLessThanLabel = false;
      if (idArrayIndex < numIds)
      {
        idLessThanLabel = (id[idArrayIndex] < static_cast<T1>(label[labelArrayIndex]));
      }
      while ((idArrayIndex < numIds) && idLessThanLabel)
      {
        ++idArrayIndex;
        if (idArrayIndex >= numIds)
        {
          break;
        }
        idLessThanLabel = (id[idArrayIndex] < static_cast<T1>(label[labelArrayIndex]));
      }

      self->UpdateProgress(static_cast<double>(idArrayIndex) / (numIds * (passThrough + 1)));
      if (idArrayIndex >= numIds)
      {
        // We're out of selection ids, so we're done.
        break;
      }

      // Advance through and mark all points with a label EQUAL TO the
      // current selection id, as well as their cells.
      bool idEqualToLabel = (id[idArrayIndex] == static_cast<T1>(label[labelArrayIndex]));
      while (idEqualToLabel)
      {
        svtkIdType ptId = idxArray->GetValue(labelArrayIndex);
        pointInArray->SetValue(ptId, flag);
        if (containingCells)
        {
          input->GetPointCells(ptId, ptCells);
          for (svtkIdType i = 0; i < ptCells->GetNumberOfIds(); ++i)
          {
            svtkIdType cellId = ptCells->GetId(i);
            if (!passThrough && !invert && cellInArray->GetValue(cellId) != flag)
            {
              input->GetCellPoints(cellId, cellPts);
              for (svtkIdType j = 0; j < cellPts->GetNumberOfIds(); ++j)
              {
                pointInArray->SetValue(cellPts->GetId(j), flag);
              }
            }
            cellInArray->SetValue(cellId, flag);
          }
        }
        ++labelArrayIndex;
        if (labelArrayIndex >= numPts)
        {
          break;
        }
        idEqualToLabel = (id[idArrayIndex] == static_cast<T1>(label[labelArrayIndex]));
      }

      // Advance through point labels until we find
      // one that's NOT LESS THAN the current selection id.
      bool labelLessThanId = false;
      if (labelArrayIndex < numPts)
      {
        labelLessThanId = (label[labelArrayIndex] < static_cast<T2>(id[idArrayIndex]));
      }
      while ((labelArrayIndex < numPts) && labelLessThanId)
      {
        ++labelArrayIndex;
        if (labelArrayIndex >= numPts)
        {
          break;
        }
        labelLessThanId = (label[labelArrayIndex] < static_cast<T2>(id[idArrayIndex]));
      }
    }

    if (containingCells)
    {
      ptCells->Delete();
      cellPts->Delete();
    }
  }
};

} // end anonymous namespace

//----------------------------------------------------------------------------
int svtkExtractSelectedIds::ExtractCells(
  svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output)
{
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

  // decide what the IDS mean
  svtkAbstractArray* labelArray = nullptr;
  int selType = sel->GetProperties()->Get(svtkSelectionNode::CONTENT_TYPE());
  if (selType == svtkSelectionNode::GLOBALIDS)
  {
    labelArray = svtkArrayDownCast<svtkIdTypeArray>(input->GetCellData()->GetGlobalIds());
  }
  else if (selType == svtkSelectionNode::PEDIGREEIDS)
  {
    labelArray = input->GetCellData()->GetPedigreeIds();
  }
  else if (selType == svtkSelectionNode::VALUES && sel->GetSelectionList()->GetName())
  {
    // user chose a specific label array
    labelArray = input->GetCellData()->GetAbstractArray(sel->GetSelectionList()->GetName());
  }

  if (labelArray == nullptr && selType != svtkSelectionNode::INDICES)
  {
    return 1;
  }

  svtkIdTypeArray* idxArray = svtkIdTypeArray::New();
  idxArray->SetNumberOfComponents(1);
  idxArray->SetNumberOfTuples(numCells);
  for (i = 0; i < numCells; i++)
  {
    idxArray->SetValue(i, i);
  }

  if (labelArray)
  {
    int component_no = 0;
    if (sel->GetProperties()->Has(svtkSelectionNode::COMPONENT_NUMBER()))
    {
      component_no = sel->GetProperties()->Get(svtkSelectionNode::COMPONENT_NUMBER());
      if (component_no >= labelArray->GetNumberOfComponents())
      {
        component_no = 0;
      }
    }

    svtkAbstractArray* sortedArray = svtkAbstractArray::CreateArray(labelArray->GetDataType());
    svtkESIDeepCopy(sortedArray, labelArray, component_no);
    svtkSortDataArray::Sort(sortedArray, idxArray);
    labelArray = sortedArray;
  }
  else
  {
    // no global array, so just use the input cell index
    labelArray = idxArray;
    labelArray->Register(nullptr);
  }

  svtkIdType numIds = 0;
  svtkAbstractArray* idArray = sel->GetSelectionList();
  if (idArray)
  {
    numIds = idArray->GetNumberOfTuples();
    svtkAbstractArray* sortedArray = svtkAbstractArray::CreateArray(idArray->GetDataType());
    sortedArray->DeepCopy(idArray);
    svtkSortDataArray::SortArrayByComponent(sortedArray, 0);
    idArray = sortedArray;
  }

  if (idArray == nullptr)
  {
    labelArray->Delete();
    idxArray->Delete();
    return 1;
  }

  // Array types must match if they are string arrays.
  svtkExtractSelectedIdsExtractCells worker;
  if (svtkArrayDownCast<svtkStringArray>(labelArray))
  {
    svtkStringArray* labels = svtkArrayDownCast<svtkStringArray>(labelArray);
    svtkStringArray* ids = svtkArrayDownCast<svtkStringArray>(idArray);
    if (ids == nullptr)
    {
      labelArray->Delete();
      idxArray->Delete();
      idArray->Delete();
      svtkWarningMacro("Array types don't match. They must match for svtkStringArray.");
      return 0;
    }
    worker(
      ids, labels, this, passThrough, invert, input, idxArray, cellInArray, pointInArray, numIds);
  }
  else
  {
    svtkDataArray* labels = svtkDataArray::SafeDownCast(labelArray);
    svtkDataArray* ids = svtkDataArray::SafeDownCast(idArray);

    const bool executed = svtkArrayDispatch::Dispatch2::Execute(ids, labels, worker, this,
      passThrough, invert, input, idxArray, cellInArray, pointInArray, numIds);
    if (!executed)
    { // fallback to svtkDataArray dispatch access
      worker(
        ids, labels, this, passThrough, invert, input, idxArray, cellInArray, pointInArray, numIds);
    }
  }

  idArray->Delete();
  idxArray->Delete();
  labelArray->Delete();

  if (!passThrough)
  {
    svtkIdType* pointMap = new svtkIdType[numPts]; // maps old point ids into new
    svtkExtractSelectedIdsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (output->GetDataObjectType() == SVTK_POLY_DATA)
    {
      svtkExtractSelectedIdsCopyCells<svtkPolyData>(
        input, svtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
    }
    else
    {
      svtkExtractSelectedIdsCopyCells<svtkUnstructuredGrid>(
        input, svtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
    }
    delete[] pointMap;
    this->UpdateProgress(1.0);
  }

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedIds::ExtractPoints(
  svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output)
{
  int passThrough = 0;
  if (this->PreserveTopology)
  {
    passThrough = 1;
  }

  int containingCells = 0;
  if (sel->GetProperties()->Has(svtkSelectionNode::CONTAINING_CELLS()))
  {
    containingCells = sel->GetProperties()->Get(svtkSelectionNode::CONTAINING_CELLS());
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

  // decide what the IDS mean
  svtkAbstractArray* labelArray = nullptr;
  int selType = sel->GetProperties()->Get(svtkSelectionNode::CONTENT_TYPE());
  if (selType == svtkSelectionNode::GLOBALIDS)
  {
    labelArray = svtkArrayDownCast<svtkIdTypeArray>(input->GetPointData()->GetGlobalIds());
  }
  else if (selType == svtkSelectionNode::PEDIGREEIDS)
  {
    labelArray = input->GetPointData()->GetPedigreeIds();
  }
  else if (selType == svtkSelectionNode::VALUES && sel->GetSelectionList()->GetName())
  {
    // user chose a specific label array
    labelArray = input->GetPointData()->GetAbstractArray(sel->GetSelectionList()->GetName());
  }
  if (labelArray == nullptr && selType != svtkSelectionNode::INDICES)
  {
    return 1;
  }

  svtkIdTypeArray* idxArray = svtkIdTypeArray::New();
  idxArray->SetNumberOfComponents(1);
  idxArray->SetNumberOfTuples(numPts);
  for (i = 0; i < numPts; i++)
  {
    idxArray->SetValue(i, i);
  }

  if (labelArray)
  {
    int component_no = 0;
    if (sel->GetProperties()->Has(svtkSelectionNode::COMPONENT_NUMBER()))
    {
      component_no = sel->GetProperties()->Get(svtkSelectionNode::COMPONENT_NUMBER());
      if (component_no >= labelArray->GetNumberOfComponents())
      {
        component_no = 0;
      }
    }

    svtkAbstractArray* sortedArray = svtkAbstractArray::CreateArray(labelArray->GetDataType());
    svtkESIDeepCopy(sortedArray, labelArray, component_no);
    svtkSortDataArray::Sort(sortedArray, idxArray);
    labelArray = sortedArray;
  }
  else
  {
    // no global array, so just use the input cell index
    labelArray = idxArray;
    labelArray->Register(nullptr);
  }

  svtkIdType numIds = 0;
  svtkAbstractArray* idArray = sel->GetSelectionList();
  if (idArray == nullptr)
  {
    labelArray->Delete();
    idxArray->Delete();
    return 1;
  }

  // Array types must match if they are string arrays.
  if (svtkArrayDownCast<svtkStringArray>(labelArray) &&
    svtkArrayDownCast<svtkStringArray>(idArray) == nullptr)
  {
    svtkWarningMacro("Array types don't match. They must match for svtkStringArray.");
    labelArray->Delete();
    idxArray->Delete();
    return 0;
  }

  numIds = idArray->GetNumberOfTuples();
  svtkAbstractArray* sortedArray = svtkAbstractArray::CreateArray(idArray->GetDataType());
  sortedArray->DeepCopy(idArray);
  svtkSortDataArray::SortArrayByComponent(sortedArray, 0);
  idArray = sortedArray;

  svtkExtractSelectedIdsExtractPoints worker;
  if (svtkArrayDownCast<svtkStringArray>(labelArray))
  {
    svtkStringArray* labels = svtkArrayDownCast<svtkStringArray>(labelArray);
    svtkStringArray* ids = svtkArrayDownCast<svtkStringArray>(idArray);
    worker(ids, labels, this, passThrough, invert, containingCells, input, idxArray, cellInArray,
      pointInArray, numIds);
  }
  else
  {
    svtkDataArray* labels = svtkDataArray::SafeDownCast(labelArray);
    svtkDataArray* ids = svtkDataArray::SafeDownCast(idArray);

    bool executed = svtkArrayDispatch::Dispatch2::Execute(ids, labels, worker, this, passThrough,
      invert, containingCells, input, idxArray, cellInArray, pointInArray, numIds);
    if (!executed)
    { // fallback to svtkDataArray dispatch access
      worker(ids, labels, this, passThrough, invert, containingCells, input, idxArray, cellInArray,
        pointInArray, numIds);
    }
  }

  idArray->Delete();
  idxArray->Delete();
  labelArray->Delete();

  if (!passThrough)
  {
    svtkIdType* pointMap = new svtkIdType[numPts]; // maps old point ids into new
    svtkExtractSelectedIdsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (containingCells)
    {
      if (output->GetDataObjectType() == SVTK_POLY_DATA)
      {
        svtkExtractSelectedIdsCopyCells<svtkPolyData>(
          input, svtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
      else
      {
        svtkExtractSelectedIdsCopyCells<svtkUnstructuredGrid>(
          input, svtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
    }
    else
    {
      numPts = output->GetNumberOfPoints();
      if (output->GetDataObjectType() == SVTK_POLY_DATA)
      {
        svtkPolyData* outputPD = svtkPolyData::SafeDownCast(output);
        svtkCellArray* newVerts = svtkCellArray::New();
        newVerts->AllocateEstimate(numPts, 1);
        for (i = 0; i < numPts; ++i)
        {
          newVerts->InsertNextCell(1, &i);
        }
        outputPD->SetVerts(newVerts);
        newVerts->Delete();
      }
      else
      {
        svtkUnstructuredGrid* outputUG = svtkUnstructuredGrid::SafeDownCast(output);
        outputUG->Allocate(numPts);
        for (i = 0; i < numPts; ++i)
        {
          outputUG->InsertNextCell(SVTK_VERTEX, 1, &i);
        }
      }
    }
    this->UpdateProgress(1.0);
    delete[] pointMap;
  }
  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedIds::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
