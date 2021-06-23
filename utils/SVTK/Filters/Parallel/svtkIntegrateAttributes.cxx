/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIntegrateAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkIntegrateAttributes.h"

#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolygon.h"
#include "svtkTriangle.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <cassert>

svtkStandardNewMacro(svtkIntegrateAttributes);

class svtkIntegrateAttributes::svtkFieldList : public svtkDataSetAttributes::FieldList
{
  typedef svtkDataSetAttributes::FieldList Superclass;

public:
  svtkFieldList(int numInputs = 0)
    : Superclass(numInputs)
  {
  }

protected:
  // overridden to only create svtkDoubleArray for numeric arrays.
  svtkSmartPointer<svtkAbstractArray> CreateArray(int type) const override
  {
    if (auto array = this->Superclass::CreateArray(type))
    {
      const int is_numeric = (array->IsNumeric());
      if (is_numeric)
      {
        return svtkSmartPointer<svtkAbstractArray>::Take(svtkDoubleArray::New());
      }
    }
    return nullptr;
  }
};

//-----------------------------------------------------------------------------
svtkIntegrateAttributes::svtkIntegrateAttributes()
{
  this->IntegrationDimension = 0;
  this->Sum = 0.0;
  this->SumCenter[0] = this->SumCenter[1] = this->SumCenter[2] = 0.0;
  this->Controller = nullptr;

  this->PointFieldList = nullptr;
  this->CellFieldList = nullptr;
  this->FieldListIndex = 0;

  this->DivideAllCellDataByVolume = false;

  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
svtkIntegrateAttributes::~svtkIntegrateAttributes()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
void svtkIntegrateAttributes::SetController(svtkMultiProcessController* controller)
{
  if (this->Controller)
  {
    this->Controller->UnRegister(this);
  }

  this->Controller = controller;

  if (this->Controller)
  {
    this->Controller->Register(this);
  }
}

//----------------------------------------------------------------------------
svtkExecutive* svtkIntegrateAttributes::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int svtkIntegrateAttributes::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkIntegrateAttributes::CompareIntegrationDimension(svtkDataSet* output, int dim)
{
  // higher dimension prevails
  if (this->IntegrationDimension < dim)
  { // Throw out results from lower dimension.
    this->Sum = 0;
    this->SumCenter[0] = this->SumCenter[1] = this->SumCenter[2] = 0.0;
    this->ZeroAttributes(output->GetPointData());
    this->ZeroAttributes(output->GetCellData());
    this->IntegrationDimension = dim;
    return 1;
  }
  // Skip this cell if we are inetrgrting a higher dimension.
  return (this->IntegrationDimension == dim);
}

//----------------------------------------------------------------------------
void svtkIntegrateAttributes::ExecuteBlock(svtkDataSet* input, svtkUnstructuredGrid* output,
  int fieldset_index, svtkIntegrateAttributes::svtkFieldList& pdList,
  svtkIntegrateAttributes::svtkFieldList& cdList)
{
  svtkUnsignedCharArray* ghostArray = input->GetCellGhostArray();

  // This is sort of a hack since it's incredibly painful to change all the
  // signatures to take the pdList, cdList and fieldset_index.
  this->PointFieldList = &pdList;
  this->CellFieldList = &cdList;
  this->FieldListIndex = fieldset_index;

  svtkIdList* cellPtIds = svtkIdList::New();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkIdType cellId;
  svtkPoints* cellPoints = nullptr; // needed if we need to split 3D cells
  int cellType;
  for (cellId = 0; cellId < numCells; ++cellId)
  {
    cellType = input->GetCellType(cellId);
    // Make sure we are not integrating ghost/blanked cells.
    if (ghostArray &&
      (ghostArray->GetValue(cellId) &
        (svtkDataSetAttributes::DUPLICATECELL | svtkDataSetAttributes::HIDDENCELL)))
    {
      continue;
    }

    switch (cellType)
    {
      // skip empty or 0D Cells
      case SVTK_EMPTY_CELL:
      case SVTK_VERTEX:
      case SVTK_POLY_VERTEX:
        break;

      case SVTK_POLY_LINE:
      case SVTK_LINE:
      {
        if (this->CompareIntegrationDimension(output, 1))
        {
          input->GetCellPoints(cellId, cellPtIds);
          this->IntegratePolyLine(input, output, cellId, cellPtIds);
        }
      }
      break;

      case SVTK_TRIANGLE:
      {
        if (this->CompareIntegrationDimension(output, 2))
        {
          input->GetCellPoints(cellId, cellPtIds);
          this->IntegrateTriangle(
            input, output, cellId, cellPtIds->GetId(0), cellPtIds->GetId(1), cellPtIds->GetId(2));
        }
      }
      break;

      case SVTK_TRIANGLE_STRIP:
      {
        if (this->CompareIntegrationDimension(output, 2))
        {
          input->GetCellPoints(cellId, cellPtIds);
          this->IntegrateTriangleStrip(input, output, cellId, cellPtIds);
        }
      }
      break;

      case SVTK_POLYGON:
      {
        if (this->CompareIntegrationDimension(output, 2))
        {
          input->GetCellPoints(cellId, cellPtIds);
          this->IntegratePolygon(input, output, cellId, cellPtIds);
        }
      }
      break;

      case SVTK_PIXEL:
      {
        if (this->CompareIntegrationDimension(output, 2))
        {
          input->GetCellPoints(cellId, cellPtIds);
          this->IntegratePixel(input, output, cellId, cellPtIds);
        }
      }
      break;

      case SVTK_QUAD:
      {
        if (this->CompareIntegrationDimension(output, 2))
        {
          svtkIdType pt1Id, pt2Id, pt3Id;
          input->GetCellPoints(cellId, cellPtIds);
          pt1Id = cellPtIds->GetId(0);
          pt2Id = cellPtIds->GetId(1);
          pt3Id = cellPtIds->GetId(2);
          this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
          pt2Id = cellPtIds->GetId(3);
          this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
        }
      }
      break;

      case SVTK_VOXEL:
      {
        if (this->CompareIntegrationDimension(output, 3))
        {
          input->GetCellPoints(cellId, cellPtIds);
          this->IntegrateVoxel(input, output, cellId, cellPtIds);
        }
      }
      break;

      case SVTK_TETRA:
      {
        if (this->CompareIntegrationDimension(output, 3))
        {
          svtkIdType pt1Id, pt2Id, pt3Id, pt4Id;
          input->GetCellPoints(cellId, cellPtIds);
          pt1Id = cellPtIds->GetId(0);
          pt2Id = cellPtIds->GetId(1);
          pt3Id = cellPtIds->GetId(2);
          pt4Id = cellPtIds->GetId(3);
          this->IntegrateTetrahedron(input, output, cellId, pt1Id, pt2Id, pt3Id, pt4Id);
        }
      }
      break;

      default:
      {
        // We need to explicitly get the cell
        svtkCell* cell = input->GetCell(cellId);
        int cellDim = cell->GetCellDimension();
        if (cellDim == 0)
        {
          continue;
        }
        if (!this->CompareIntegrationDimension(output, cellDim))
        {
          continue;
        }

        // We will need a place to store points from the cell's
        // triangulate function
        if (!cellPoints)
        {
          cellPoints = svtkPoints::New();
        }

        cell->Triangulate(1, cellPtIds, cellPoints);
        switch (cellDim)
        {
          case 1:
            this->IntegrateGeneral1DCell(input, output, cellId, cellPtIds);
            break;
          case 2:
            this->IntegrateGeneral2DCell(input, output, cellId, cellPtIds);
            break;
          case 3:
            this->IntegrateGeneral3DCell(input, output, cellId, cellPtIds);
            break;
          default:
            svtkWarningMacro("Unsupported Cell Dimension = " << cellDim);
        }
      }
    }
  }
  cellPtIds->Delete();
  if (cellPoints)
  {
    cellPoints->Delete();
  }

  this->PointFieldList = nullptr;
  this->CellFieldList = nullptr;
  this->FieldListIndex = 0;
}

//-----------------------------------------------------------------------------
int svtkIntegrateAttributes::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Integration of imaginary attribute with constant value 1.
  this->Sum = 0;
  // For computation of point/vertext location.
  this->SumCenter[0] = this->SumCenter[1] = this->SumCenter[2] = 0.0;

  this->IntegrationDimension = 0;

  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkCompositeDataSet* compositeInput = svtkCompositeDataSet::SafeDownCast(input);
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(input);
  if (compositeInput)
  {
    svtkCompositeDataIterator* iter = compositeInput->NewIterator();

    // Create the intersection field list. This is list of arrays common
    // to all blocks in the input.
    svtkFieldList pdList;
    svtkFieldList cdList;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataObject* dobj = iter->GetCurrentDataObject();
      svtkDataSet* ds = svtkDataSet::SafeDownCast(dobj);
      if (ds)
      {
        if (ds->GetNumberOfPoints() == 0)
        {
          continue; // skip empty datasets.
        }
        pdList.IntersectFieldList(ds->GetPointData());
        cdList.IntersectFieldList(ds->GetCellData());
      }
      else
      {
        if (dobj)
        {
          svtkWarningMacro("This filter cannot handle sub-datasets of type : "
            << dobj->GetClassName() << ". Skipping block");
        }
      }
    }

    // Now initialize the output for the intersected set of arrays.
    this->AllocateAttributes(pdList, output->GetPointData());
    this->AllocateAttributes(cdList, output->GetCellData());

    int index = 0;
    // Now execute for each block.
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataObject* dobj = iter->GetCurrentDataObject();
      svtkDataSet* ds = svtkDataSet::SafeDownCast(dobj);
      if (ds && ds->GetNumberOfPoints() > 0)
      {
        this->ExecuteBlock(ds, output, index, pdList, cdList);
        index++;
      }
    }
    iter->Delete();
  }
  else if (dsInput)
  {
    // Output will have all the same attribute arrays as input, but
    // only 1 entry per array, and arrays are double.
    // Set all values to 0.  All output attributes are type double.
    svtkFieldList pdList(1);
    svtkFieldList cdList(1);
    pdList.InitializeFieldList(dsInput->GetPointData());
    cdList.InitializeFieldList(dsInput->GetCellData());
    this->AllocateAttributes(pdList, output->GetPointData());
    this->AllocateAttributes(cdList, output->GetCellData());
    this->ExecuteBlock(dsInput, output, 0, pdList, cdList);
  }
  else
  {
    if (input)
    {
      svtkErrorMacro("This filter cannot handle data of type : " << input->GetClassName());
    }
    return 0;
  }

  // Here is the trick:  The satellites need a point and vertex to
  // marshal the attributes.

  // Generate point and vertex.  Add extra attributes for area too.
  // Satellites do not need the area attribute, but it does not hurt.
  double pt[3];
  svtkPoints* newPoints = svtkPoints::New();
  newPoints->SetNumberOfPoints(1);
  // Get rid of the weight factors.
  if (this->Sum != 0.0)
  {
    pt[0] = this->SumCenter[0] / this->Sum;
    pt[1] = this->SumCenter[1] / this->Sum;
    pt[2] = this->SumCenter[2] / this->Sum;
  }
  else
  {
    pt[0] = this->SumCenter[0];
    pt[1] = this->SumCenter[1];
    pt[2] = this->SumCenter[2];
  }
  newPoints->InsertPoint(0, pt);
  output->SetPoints(newPoints);
  newPoints->Delete();
  newPoints = nullptr;

  output->Allocate(1);
  svtkIdType vertexPtIds[1];
  vertexPtIds[0] = 0;
  output->InsertNextCell(SVTK_VERTEX, 1, vertexPtIds);

  // Create a new cell array for the total length, area or volume.
  svtkDoubleArray* sumArray = svtkDoubleArray::New();
  switch (this->IntegrationDimension)
  {
    case 1:
      sumArray->SetName("Length");
      break;
    case 2:
      sumArray->SetName("Area");
      break;
    case 3:
      sumArray->SetName("Volume");
      break;
  }
  if (this->IntegrationDimension > 0)
  {
    sumArray->SetNumberOfTuples(1);
    sumArray->SetValue(0, this->Sum);
    output->GetCellData()->AddArray(sumArray);
  }
  sumArray->Delete();

  int globalMin = this->PieceNodeMinToNode0(output);
  int processId = this->Controller ? this->Controller->GetLocalProcessId() : 0;
  int numProcs = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
  if (globalMin == numProcs)
  {
    // there is no data in any of the processors
    if (this->Sum != 0.0 && this->DivideAllCellDataByVolume)
    {
      DivideDataArraysByConstant(output->GetCellData(), true, this->Sum);
    }
    return 1;
  }
  if (processId > 0)
  {
    if (processId != globalMin)
    {
      this->SendPiece(output);
    }
  }
  else
  {
    for (int id = 1; id < numProcs; ++id)
    {
      if (id != globalMin)
      {
        this->ReceivePiece(output, id);
      }
    }

    // now that we have all of the sums from each process
    // set the point location with the global value
    if (this->Sum != 0.0)
    {
      pt[0] = this->SumCenter[0] / this->Sum;
      pt[1] = this->SumCenter[1] / this->Sum;
      pt[2] = this->SumCenter[2] / this->Sum;
      if (this->DivideAllCellDataByVolume)
      {
        DivideDataArraysByConstant(output->GetCellData(), true, this->Sum);
      }
    }
    else
    {
      pt[0] = this->SumCenter[0];
      pt[1] = this->SumCenter[1];
      pt[2] = this->SumCenter[2];
    }
    output->GetPoints()->SetPoint(0, pt);
  }

  return 1;
}

int svtkIntegrateAttributes::PieceNodeMinToNode0(svtkUnstructuredGrid* data)
{
  int numProcs = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
  int processId = this->Controller ? this->Controller->GetLocalProcessId() : 0;
  int localMin = (data->GetNumberOfCells() == 0 ? numProcs : processId);
  int globalMin = numProcs;
  if (numProcs == 1)
  {
    return 0;
  }
  this->Controller->AllReduce(&localMin, &globalMin, 1, svtkCommunicator::MIN_OP);
  if (globalMin == 0 || globalMin == numProcs)
  {
    return globalMin;
  }
  if (processId == 0)
  {
    this->ReceivePiece(data, globalMin);
  }
  else if (processId == globalMin)
  {
    this->SendPiece(data);
  }
  return globalMin;
}

void svtkIntegrateAttributes::SendPiece(svtkUnstructuredGrid* src)
{
  assert(this->Controller);
  double msg[5];
  msg[0] = (double)(this->IntegrationDimension);
  msg[1] = this->Sum;
  msg[2] = this->SumCenter[0];
  msg[3] = this->SumCenter[1];
  msg[4] = this->SumCenter[2];
  this->Controller->Send(msg, 5, 0, svtkIntegrateAttributes::IntegrateAttrInfo);
  this->Controller->Send(src, 0, svtkIntegrateAttributes::IntegrateAttrData);
  // Done sending.  Reset src so satellites will have empty data.
  src->Initialize();
}

void svtkIntegrateAttributes::ReceivePiece(svtkUnstructuredGrid* mergeTo, int fromId)
{
  assert(this->Controller);
  double msg[5];
  this->Controller->Receive(msg, 5, fromId, svtkIntegrateAttributes::IntegrateAttrInfo);
  svtkUnstructuredGrid* tmp = svtkUnstructuredGrid::New();
  this->Controller->Receive(tmp, fromId, svtkIntegrateAttributes::IntegrateAttrData);
  if (this->CompareIntegrationDimension(mergeTo, (int)(msg[0])))
  {
    this->Sum += msg[1];
    this->SumCenter[0] += msg[2];
    this->SumCenter[1] += msg[3];
    this->SumCenter[2] += msg[4];
    this->IntegrateSatelliteData(tmp->GetPointData(), mergeTo->GetPointData());
    this->IntegrateSatelliteData(tmp->GetCellData(), mergeTo->GetCellData());
  }
  tmp->Delete();
  tmp = nullptr;
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::AllocateAttributes(
  svtkIntegrateAttributes::svtkFieldList& fieldList, svtkDataSetAttributes* outda)
{
  outda->CopyAllocate(fieldList);
  for (int cc = 0, max = outda->GetNumberOfArrays(); cc < max; ++cc)
  {
    auto array = svtkDoubleArray::SafeDownCast(outda->GetAbstractArray(cc));
    assert(array != nullptr);
    array->SetNumberOfTuples(1);
    // It cannot hurt to zero the arrays here.
    array->FillValue(0.0);
  }

  for (int cc = 0; cc < svtkDataSetAttributes::NUM_ATTRIBUTES; ++cc)
  {
    // this should not be necessary, however, the old version of
    // svtkIntegrateAttributes didn't mark active attributes for any arrays. We
    // preserve that behavior here. This is needed since filters like svtkGlyph3D
    // love to drop active attributes (incorrectly, in my opinion). Until we
    // resolve that, I am keeping this old behavior.
    outda->SetActiveAttribute(-1, cc);
  }
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::ZeroAttributes(svtkDataSetAttributes* outda)
{
  int numArrays, i, numComponents, j;
  svtkDataArray* outArray;
  numArrays = outda->GetNumberOfArrays();
  for (i = 0; i < numArrays; ++i)
  {
    outArray = outda->GetArray(i);
    numComponents = outArray->GetNumberOfComponents();
    for (j = 0; j < numComponents; ++j)
    {
      outArray->SetComponent(0, j, 0.0);
    }
  }
}
//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::IntegrateData1(svtkDataSetAttributes* inda, svtkDataSetAttributes* outda,
  svtkIdType pt1Id, double k, svtkIntegrateAttributes::svtkFieldList& fieldList, int index)
{
  auto f = [pt1Id, k](svtkAbstractArray* ainArray, svtkAbstractArray* aoutArray) {
    svtkDataArray* inArray = svtkDataArray::FastDownCast(ainArray);
    svtkDataArray* outArray = svtkDataArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double dv = vIn1;
        const double vOut = (dv * k) + outArray->GetComponent(0, j);
        outArray->SetComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, inda, outda, f);
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::IntegrateData2(svtkDataSetAttributes* inda, svtkDataSetAttributes* outda,
  svtkIdType pt1Id, svtkIdType pt2Id, double k, svtkIntegrateAttributes::svtkFieldList& fieldList,
  int index)
{
  auto f = [pt1Id, pt2Id, k](svtkAbstractArray* ainArray, svtkAbstractArray* aoutArray) {
    svtkDataArray* inArray = svtkDataArray::FastDownCast(ainArray);
    svtkDataArray* outArray = svtkDataArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double vIn2 = inArray->GetComponent(pt2Id, j);
        const double dv = 0.5 * (vIn1 + vIn2);
        const double vOut = (dv * k) + outArray->GetComponent(0, j);
        outArray->SetComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, inda, outda, f);
}

//-----------------------------------------------------------------------------
// Is the extra performance worth duplicating this code with IntergrateData2.
void svtkIntegrateAttributes::IntegrateData3(svtkDataSetAttributes* inda, svtkDataSetAttributes* outda,
  svtkIdType pt1Id, svtkIdType pt2Id, svtkIdType pt3Id, double k,
  svtkIntegrateAttributes::svtkFieldList& fieldList, int index)
{
  auto f = [pt1Id, pt2Id, pt3Id, k](svtkAbstractArray* ainArray, svtkAbstractArray* aoutArray) {
    svtkDataArray* inArray = svtkDataArray::FastDownCast(ainArray);
    svtkDataArray* outArray = svtkDataArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double vIn2 = inArray->GetComponent(pt2Id, j);
        const double vIn3 = inArray->GetComponent(pt3Id, j);
        const double dv = (vIn1 + vIn2 + vIn3) / 3.0;
        const double vOut = (dv * k) + outArray->GetComponent(0, j);
        outArray->SetComponent(0, j, vOut);
      }
    }
  };
  fieldList.TransformData(index, inda, outda, f);
}

//-----------------------------------------------------------------------------
// Is the extra performance worth duplicating this code with IntergrateData2.
void svtkIntegrateAttributes::IntegrateData4(svtkDataSetAttributes* inda, svtkDataSetAttributes* outda,
  svtkIdType pt1Id, svtkIdType pt2Id, svtkIdType pt3Id, svtkIdType pt4Id, double k,
  svtkIntegrateAttributes::svtkFieldList& fieldList, int index)
{
  auto f = [pt1Id, pt2Id, pt3Id, pt4Id, k](
             svtkAbstractArray* ainArray, svtkAbstractArray* aoutArray) {
    svtkDataArray* inArray = svtkDataArray::FastDownCast(ainArray);
    svtkDataArray* outArray = svtkDataArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double vIn2 = inArray->GetComponent(pt2Id, j);
        const double vIn3 = inArray->GetComponent(pt3Id, j);
        const double vIn4 = inArray->GetComponent(pt4Id, j);
        const double dv = (vIn1 + vIn2 + vIn3 + vIn4) * 0.25;
        const double vOut = (dv * k) + outArray->GetComponent(0, j);
        outArray->SetComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, inda, outda, f);
}

//-----------------------------------------------------------------------------
// Used to sum arrays from all processes.
void svtkIntegrateAttributes::IntegrateSatelliteData(
  svtkDataSetAttributes* sendingProcAttributes, svtkDataSetAttributes* proc0Attributes)
{
  // if the sending processor has no data
  if (sendingProcAttributes->GetNumberOfArrays() == 0)
  {
    return;
  }

  // when processor 0 that has no data, receives data from the min
  // processor that has data
  if (proc0Attributes->GetNumberOfArrays() == 0)
  {
    proc0Attributes->DeepCopy(sendingProcAttributes);
    return;
  }

  int numArrays, i, numComponents, j;
  svtkDataArray* inArray;
  svtkDataArray* outArray;
  numArrays = proc0Attributes->GetNumberOfArrays();
  double vIn, vOut;
  for (i = 0; i < numArrays; ++i)
  {
    outArray = proc0Attributes->GetArray(i);
    numComponents = outArray->GetNumberOfComponents();
    // Protect against arrays in a different order.
    const char* name = outArray->GetName();
    if (name && name[0] != '\0')
    {
      inArray = sendingProcAttributes->GetArray(name);
      if (inArray && inArray->GetNumberOfComponents() == numComponents)
      {
        // We could template for speed.
        for (j = 0; j < numComponents; ++j)
        {
          vIn = inArray->GetComponent(0, j);
          vOut = outArray->GetComponent(0, j);
          outArray->SetComponent(0, j, vOut + vIn);
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::IntegratePolyLine(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* ptIds)
{
  double length;
  double pt1[3], pt2[3], mid[3];
  svtkIdType numLines, lineIdx;
  svtkIdType pt1Id, pt2Id;

  numLines = ptIds->GetNumberOfIds() - 1;
  for (lineIdx = 0; lineIdx < numLines; ++lineIdx)
  {
    pt1Id = ptIds->GetId(lineIdx);
    pt2Id = ptIds->GetId(lineIdx + 1);
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    length = sqrt(svtkMath::Distance2BetweenPoints(pt1, pt2));
    this->Sum += length;

    // Compute the middle, which is really just another attribute.
    mid[0] = (pt1[0] + pt2[0]) * 0.5;
    mid[1] = (pt1[1] + pt2[1]) * 0.5;
    mid[2] = (pt1[2] + pt2[2]) * 0.5;
    // Add weighted to sumCenter.
    this->SumCenter[0] += mid[0] * length;
    this->SumCenter[1] += mid[1] * length;
    this->SumCenter[2] += mid[2] * length;

    // Now integrate the rest of the attributes.
    this->IntegrateData2(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, length,
      *this->PointFieldList, this->FieldListIndex);
    this->IntegrateData1(input->GetCellData(), output->GetCellData(), cellId, length,
      *this->CellFieldList, this->FieldListIndex);
  }
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::IntegrateGeneral1DCell(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* ptIds)
{
  // Determine the number of lines
  svtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be an even number of points from the triangulation
  if (nPnts % 2)
  {
    svtkWarningMacro("Odd number of points(" << nPnts << ")  encountered - skipping "
                                            << " 1D Cell: " << cellId);
    return;
  }

  double length;
  double pt1[3], pt2[3], mid[3];
  svtkIdType pid = 0;
  svtkIdType pt1Id, pt2Id;

  while (pid < nPnts)
  {
    pt1Id = ptIds->GetId(pid++);
    pt2Id = ptIds->GetId(pid++);
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    length = sqrt(svtkMath::Distance2BetweenPoints(pt1, pt2));
    this->Sum += length;

    // Compute the middle, which is really just another attribute.
    mid[0] = (pt1[0] + pt2[0]) * 0.5;
    mid[1] = (pt1[1] + pt2[1]) * 0.5;
    mid[2] = (pt1[2] + pt2[2]) * 0.5;
    // Add weighted to sumCenter.
    this->SumCenter[0] += mid[0] * length;
    this->SumCenter[1] += mid[1] * length;
    this->SumCenter[2] += mid[2] * length;

    // Now integrate the rest of the attributes.
    this->IntegrateData2(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, length,
      *this->PointFieldList, this->FieldListIndex);
    this->IntegrateData1(input->GetCellData(), output->GetCellData(), cellId, length,
      *this->CellFieldList, this->FieldListIndex);
  }
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::IntegrateTriangleStrip(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* ptIds)
{
  svtkIdType numTris, triIdx;
  svtkIdType pt1Id, pt2Id, pt3Id;

  numTris = ptIds->GetNumberOfIds() - 2;
  for (triIdx = 0; triIdx < numTris; ++triIdx)
  {
    pt1Id = ptIds->GetId(triIdx);
    pt2Id = ptIds->GetId(triIdx + 1);
    pt3Id = ptIds->GetId(triIdx + 2);
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
  }
}

//-----------------------------------------------------------------------------
// Works for convex polygons, and interpoaltion is not correct.
void svtkIntegrateAttributes::IntegratePolygon(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* ptIds)
{
  svtkIdType numTris, triIdx;
  svtkIdType pt1Id, pt2Id, pt3Id;

  numTris = ptIds->GetNumberOfIds() - 2;
  pt1Id = ptIds->GetId(0);
  for (triIdx = 0; triIdx < numTris; ++triIdx)
  {
    pt2Id = ptIds->GetId(triIdx + 1);
    pt3Id = ptIds->GetId(triIdx + 2);
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
  }
}

//-----------------------------------------------------------------------------
// For axis aligned rectangular cells
void svtkIntegrateAttributes::IntegratePixel(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds)
{
  svtkIdType pt1Id, pt2Id, pt3Id, pt4Id;
  double pts[4][3];
  pt1Id = cellPtIds->GetId(0);
  pt2Id = cellPtIds->GetId(1);
  pt3Id = cellPtIds->GetId(2);
  pt4Id = cellPtIds->GetId(3);
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);

  double l, w, a, mid[3];

  // get the lengths of its 2 orthogonal sides.  Since only 1 coordinate
  // can be different we can add the differences in all 3 directions
  l = (pts[0][0] - pts[1][0]) + (pts[0][1] - pts[1][1]) + (pts[0][2] - pts[1][2]);

  w = (pts[0][0] - pts[2][0]) + (pts[0][1] - pts[2][1]) + (pts[0][2] - pts[2][2]);

  a = fabs(l * w);
  this->Sum += a;
  // Compute the middle, which is really just another attribute.
  mid[0] = (pts[0][0] + pts[1][0] + pts[2][0] + pts[3][0]) * 0.25;
  mid[1] = (pts[0][1] + pts[1][1] + pts[2][1] + pts[3][1]) * 0.25;
  mid[2] = (pts[0][2] + pts[1][2] + pts[2][2] + pts[3][2]) * 0.25;
  // Add weighted to sumCenter.
  this->SumCenter[0] += mid[0] * a;
  this->SumCenter[1] += mid[1] * a;
  this->SumCenter[2] += mid[2] * a;

  // Now integrate the rest of the attributes.
  this->IntegrateData4(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, pt3Id, pt4Id, a,
    *this->PointFieldList, this->FieldListIndex);
  this->IntegrateData1(input->GetCellData(), output->GetCellData(), cellId, a, *this->CellFieldList,
    this->FieldListIndex);
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::IntegrateTriangle(svtkDataSet* input, svtkUnstructuredGrid* output,
  svtkIdType cellId, svtkIdType pt1Id, svtkIdType pt2Id, svtkIdType pt3Id)
{
  double pt1[3], pt2[3], pt3[3];
  double mid[3], v1[3], v2[3];
  double cross[3];
  double k;

  input->GetPoint(pt1Id, pt1);
  input->GetPoint(pt2Id, pt2);
  input->GetPoint(pt3Id, pt3);

  // Compute two legs.
  v1[0] = pt2[0] - pt1[0];
  v1[1] = pt2[1] - pt1[1];
  v1[2] = pt2[2] - pt1[2];
  v2[0] = pt3[0] - pt1[0];
  v2[1] = pt3[1] - pt1[1];
  v2[2] = pt3[2] - pt1[2];

  // Use the cross product to compute the area of the parallelogram.
  svtkMath::Cross(v1, v2, cross);
  k = sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]) * 0.5;

  if (k == 0.0)
  {
    return;
  }
  this->Sum += k;

  // Compute the middle, which is really just another attribute.
  mid[0] = (pt1[0] + pt2[0] + pt3[0]) / 3.0;
  mid[1] = (pt1[1] + pt2[1] + pt3[1]) / 3.0;
  mid[2] = (pt1[2] + pt2[2] + pt3[2]) / 3.0;
  // Add weighted to sumCenter.
  this->SumCenter[0] += mid[0] * k;
  this->SumCenter[1] += mid[1] * k;
  this->SumCenter[2] += mid[2] * k;

  // Now integrate the rest of the attributes.
  this->IntegrateData3(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, pt3Id, k,
    *this->PointFieldList, this->FieldListIndex);
  this->IntegrateData1(input->GetCellData(), output->GetCellData(), cellId, k, *this->CellFieldList,
    this->FieldListIndex);
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::IntegrateGeneral2DCell(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* ptIds)
{
  svtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be a number of points that is a multiple of 3
  // from the triangulation
  if (nPnts % 3)
  {
    svtkWarningMacro("Number of points (" << nPnts << ") is not divisiable by 3 - skipping "
                                         << " 2D Cell: " << cellId);
    return;
  }

  svtkIdType triIdx = 0;
  svtkIdType pt1Id, pt2Id, pt3Id;

  while (triIdx < nPnts)
  {
    pt1Id = ptIds->GetId(triIdx++);
    pt2Id = ptIds->GetId(triIdx++);
    pt3Id = ptIds->GetId(triIdx++);
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id);
  }
}

//-----------------------------------------------------------------------------
// For Tetrahedral cells
void svtkIntegrateAttributes::IntegrateTetrahedron(svtkDataSet* input, svtkUnstructuredGrid* output,
  svtkIdType cellId, svtkIdType pt1Id, svtkIdType pt2Id, svtkIdType pt3Id, svtkIdType pt4Id)
{
  double pts[4][3];
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);

  double a[3], b[3], c[3], n[3], v, mid[3];
  int i;
  // Compute the principle vectors around pt0 and the
  // centroid
  for (i = 0; i < 3; i++)
  {
    a[i] = pts[1][i] - pts[0][i];
    b[i] = pts[2][i] - pts[0][i];
    c[i] = pts[3][i] - pts[0][i];
    mid[i] = (pts[0][i] + pts[1][i] + pts[2][i] + pts[3][i]) * 0.25;
  }

  // Calculate the volume of the tet which is 1/6 * the box product
  svtkMath::Cross(a, b, n);
  v = svtkMath::Dot(c, n) / 6.0;
  this->Sum += v;

  // Add weighted to sumCenter.
  this->SumCenter[0] += mid[0] * v;
  this->SumCenter[1] += mid[1] * v;
  this->SumCenter[2] += mid[2] * v;

  // Integrate the attributes on the cell itself
  this->IntegrateData1(input->GetCellData(), output->GetCellData(), cellId, v, *this->CellFieldList,
    this->FieldListIndex);

  // Integrate the attributes associated with the points
  this->IntegrateData4(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, pt3Id, pt4Id, v,
    *this->PointFieldList, this->FieldListIndex);
}

//-----------------------------------------------------------------------------
// For axis aligned hexahedral cells
void svtkIntegrateAttributes::IntegrateVoxel(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds)
{
  svtkIdType pt1Id, pt2Id, pt3Id, pt4Id, pt5Id;
  double pts[5][3];
  pt1Id = cellPtIds->GetId(0);
  pt2Id = cellPtIds->GetId(1);
  pt3Id = cellPtIds->GetId(2);
  pt4Id = cellPtIds->GetId(3);
  pt5Id = cellPtIds->GetId(4);
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);
  input->GetPoint(pt5Id, pts[4]);

  double l, w, h, v, mid[3];

  // Calculate the volume of the voxel
  l = pts[1][0] - pts[0][0];
  w = pts[2][1] - pts[0][1];
  h = pts[4][2] - pts[0][2];
  v = fabs(l * w * h);
  this->Sum += v;

  // Partially Compute the middle, which is really just another attribute.
  mid[0] = (pts[0][0] + pts[1][0] + pts[2][0] + pts[3][0]) * 0.125;
  mid[1] = (pts[0][1] + pts[1][1] + pts[2][1] + pts[3][1]) * 0.125;
  mid[2] = (pts[0][2] + pts[1][2] + pts[2][2] + pts[3][2]) * 0.125;

  // Integrate the attributes on the cell itself
  this->IntegrateData1(input->GetCellData(), output->GetCellData(), cellId, v, *this->CellFieldList,
    this->FieldListIndex);

  // Integrate the attributes associated with the points on the bottom face
  // note that since IntegrateData4 is going to weigh everything by 1/4
  // we need to pass down 1/2 the volume so they will be weighted by 1/8

  this->IntegrateData4(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, pt3Id, pt4Id,
    v * 0.5, *this->PointFieldList, this->FieldListIndex);

  // Now process the top face points
  pt1Id = cellPtIds->GetId(5);
  pt2Id = cellPtIds->GetId(6);
  pt3Id = cellPtIds->GetId(7);
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  // Finish Computing the middle, which is really just another attribute.
  mid[0] += (pts[0][0] + pts[1][0] + pts[2][0] + pts[4][0]) * 0.125;
  mid[1] += (pts[0][1] + pts[1][1] + pts[2][1] + pts[4][1]) * 0.125;
  mid[2] += (pts[0][2] + pts[1][2] + pts[2][2] + pts[4][2]) * 0.125;

  // Add weighted to sumCenter.
  this->SumCenter[0] += mid[0] * v;
  this->SumCenter[1] += mid[1] * v;
  this->SumCenter[2] += mid[2] * v;

  // Integrate the attributes associated with the points on the top face
  // note that since IntegrateData4 is going to weigh everything by 1/4
  // we need to pass down 1/2 the volume so they will be weighted by 1/8
  this->IntegrateData4(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, pt3Id, pt5Id,
    v * 0.5, *this->PointFieldList, this->FieldListIndex);
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::IntegrateGeneral3DCell(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* ptIds)
{

  svtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be a number of points that is a multiple of 4
  // from the triangulation
  if (nPnts % 4)
  {
    svtkWarningMacro("Number of points (" << nPnts << ") is not divisiable by 4 - skipping "
                                         << " 3D Cell: " << cellId);
    return;
  }

  svtkIdType tetIdx = 0;
  svtkIdType pt1Id, pt2Id, pt3Id, pt4Id;

  while (tetIdx < nPnts)
  {
    pt1Id = ptIds->GetId(tetIdx++);
    pt2Id = ptIds->GetId(tetIdx++);
    pt3Id = ptIds->GetId(tetIdx++);
    pt4Id = ptIds->GetId(tetIdx++);
    this->IntegrateTetrahedron(input, output, cellId, pt1Id, pt2Id, pt3Id, pt4Id);
  }
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::DivideDataArraysByConstant(
  svtkDataSetAttributes* data, bool skipLastArray, double sum)
{
  const int offset = skipLastArray ? -1 : 0;
  for (int i = 0; i < data->GetNumberOfArrays() + offset; ++i)
  {
    svtkDataArray* arr = data->GetArray(i);
    if (arr)
    {
      for (int j = 0; j < arr->GetNumberOfComponents(); ++j)
      {
        arr->SetComponent(0, j, arr->GetComponent(0, j) / sum);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void svtkIntegrateAttributes::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "IntegrationDimension: " << this->IntegrationDimension << endl;
}
