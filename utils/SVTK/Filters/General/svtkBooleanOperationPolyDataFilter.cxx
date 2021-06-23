/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBooleanOperationPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBooleanOperationPolyDataFilter.h"

#include "svtkCellData.h"
#include "svtkDistancePolyDataFilter.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkGenericCell.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntersectionPolyDataFilter.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkBooleanOperationPolyDataFilter);

//-----------------------------------------------------------------------------
svtkBooleanOperationPolyDataFilter::svtkBooleanOperationPolyDataFilter()
  : svtkPolyDataAlgorithm()
{
  this->Tolerance = 1e-6;
  this->Operation = SVTK_UNION;
  this->ReorientDifferenceCells = 1;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
svtkBooleanOperationPolyDataFilter::~svtkBooleanOperationPolyDataFilter() = default;

//-----------------------------------------------------------------------------
void svtkBooleanOperationPolyDataFilter::SortPolyData(
  svtkPolyData* input, svtkIdList* interList, svtkIdList* unionList)
{
  int numCells = input->GetNumberOfCells();

  svtkDoubleArray* distArray =
    svtkArrayDownCast<svtkDoubleArray>(input->GetCellData()->GetArray("Distance"));

  for (int cid = 0; cid < numCells; cid++)
  {

    if (distArray->GetValue(cid) > this->Tolerance)
    {
      unionList->InsertNextId(cid);
    }
    else
    {
      interList->InsertNextId(cid);
    }
  }
}

//-----------------------------------------------------------------------------
int svtkBooleanOperationPolyDataFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo0 = inputVector[0]->GetInformationObject(0);
  svtkInformation* inInfo1 = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  svtkInformation* outInfo1 = outputVector->GetInformationObject(1);

  if (!inInfo0 || !inInfo1 || !outInfo0 || !outInfo1)
  {
    return 0;
  }

  svtkPolyData* input0 = svtkPolyData::SafeDownCast(inInfo0->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* input1 = svtkPolyData::SafeDownCast(inInfo1->Get(svtkDataObject::DATA_OBJECT()));

  svtkPolyData* outputSurface =
    svtkPolyData::SafeDownCast(outInfo0->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* outputIntersection =
    svtkPolyData::SafeDownCast(outInfo1->Get(svtkDataObject::DATA_OBJECT()));

  if (!input0 || !input1 || !outputSurface || !outputIntersection)
  {
    return 0;
  }

  // Get intersected versions
  svtkSmartPointer<svtkIntersectionPolyDataFilter> PolyDataIntersection =
    svtkSmartPointer<svtkIntersectionPolyDataFilter>::New();
  PolyDataIntersection->SetInputConnection(0, this->GetInputConnection(0, 0));
  PolyDataIntersection->SetInputConnection(1, this->GetInputConnection(1, 0));
  PolyDataIntersection->SplitFirstOutputOn();
  PolyDataIntersection->SplitSecondOutputOn();
  PolyDataIntersection->Update();

  if (PolyDataIntersection->GetStatus() != 1)
  {
    return 0;
  }

  outputIntersection->CopyStructure(PolyDataIntersection->GetOutput());
  outputIntersection->GetPointData()->PassData(PolyDataIntersection->GetOutput()->GetPointData());
  outputIntersection->GetCellData()->PassData(PolyDataIntersection->GetOutput()->GetCellData());

  // Compute distances
  svtkSmartPointer<svtkDistancePolyDataFilter> PolyDataDistance =
    svtkSmartPointer<svtkDistancePolyDataFilter>::New();

  PolyDataDistance->SetInputConnection(0, PolyDataIntersection->GetOutputPort(1));
  PolyDataDistance->SetInputConnection(1, PolyDataIntersection->GetOutputPort(2));
  PolyDataDistance->ComputeSecondDistanceOn();
  PolyDataDistance->Update();

  svtkPolyData* pd0 = PolyDataDistance->GetOutput();
  svtkPolyData* pd1 = PolyDataDistance->GetSecondDistanceOutput();

  pd0->BuildCells();
  pd0->BuildLinks();
  pd1->BuildCells();
  pd1->BuildLinks();

  // Set up field lists of both points and cells that are shared by
  // the input data sets.
  svtkDataSetAttributes::FieldList pointFields(2);
  pointFields.InitializeFieldList(pd0->GetPointData());
  pointFields.IntersectFieldList(pd1->GetPointData());

  svtkDataSetAttributes::FieldList cellFields(2);
  cellFields.InitializeFieldList(pd0->GetCellData());
  cellFields.IntersectFieldList(pd1->GetCellData());

  // Sort union/intersection.
  svtkSmartPointer<svtkIdList> interList = svtkSmartPointer<svtkIdList>::New();
  svtkSmartPointer<svtkIdList> unionList = svtkSmartPointer<svtkIdList>::New();

  this->SortPolyData(pd0, interList, unionList);

  outputSurface->AllocateCopy(pd0);
  outputSurface->GetPointData()->CopyAllocate(pointFields);
  outputSurface->GetCellData()->CopyAllocate(cellFields);

  if (this->Operation == SVTK_UNION || this->Operation == SVTK_DIFFERENCE)
  {
    this->CopyCells(pd0, outputSurface, 0, pointFields, cellFields, unionList, false);
  }
  else if (this->Operation == SVTK_INTERSECTION)
  {
    this->CopyCells(pd0, outputSurface, 0, pointFields, cellFields, interList, false);
  }

  // Label sources for each point and cell.
  svtkSmartPointer<svtkIntArray> pointSourceLabel = svtkSmartPointer<svtkIntArray>::New();
  pointSourceLabel->SetNumberOfComponents(1);
  pointSourceLabel->SetName("PointSource");
  pointSourceLabel->SetNumberOfTuples(outputSurface->GetNumberOfPoints());
  for (svtkIdType ii = 0; ii < outputSurface->GetNumberOfPoints(); ii++)
  {
    pointSourceLabel->InsertValue(ii, 0);
  }

  svtkSmartPointer<svtkIntArray> cellSourceLabel = svtkSmartPointer<svtkIntArray>::New();
  cellSourceLabel->SetNumberOfComponents(1);
  cellSourceLabel->SetName("CellSource");
  cellSourceLabel->SetNumberOfValues(outputSurface->GetNumberOfCells());
  for (svtkIdType ii = 0; ii < outputSurface->GetNumberOfCells(); ii++)
  {
    cellSourceLabel->InsertValue(ii, 0);
  }

  interList->Reset();
  unionList->Reset();

  this->SortPolyData(pd1, interList, unionList);

  if (this->Operation == SVTK_UNION)
  {
    this->CopyCells(pd1, outputSurface, 1, pointFields, cellFields, unionList, false);
  }
  else if (this->Operation == SVTK_INTERSECTION || this->Operation == SVTK_DIFFERENCE)
  {
    this->CopyCells(pd1, outputSurface, 1, pointFields, cellFields, interList,
      (this->ReorientDifferenceCells == 1 && this->Operation == SVTK_DIFFERENCE));
  }

  svtkIdType i;
  i = pointSourceLabel->GetNumberOfTuples();
  pointSourceLabel->Resize(outputSurface->GetNumberOfPoints());
  for (; i < outputSurface->GetNumberOfPoints(); i++)
  {
    pointSourceLabel->InsertValue(i, 1);
  }

  i = cellSourceLabel->GetNumberOfTuples();
  cellSourceLabel->Resize(outputSurface->GetNumberOfCells());
  for (; i < outputSurface->GetNumberOfCells(); i++)
  {
    cellSourceLabel->InsertValue(i, 1);
  }

  outputSurface->GetPointData()->AddArray(pointSourceLabel);
  outputSurface->GetCellData()->AddArray(cellSourceLabel);

  outputSurface->Squeeze();
  outputSurface->GetPointData()->Squeeze();
  outputSurface->GetCellData()->Squeeze();

  return 1;
}

//-----------------------------------------------------------------------------
void svtkBooleanOperationPolyDataFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Operation: ";
  switch (this->Operation)
  {
    case SVTK_UNION:
      os << "UNION";
      break;

    case SVTK_INTERSECTION:
      os << "INTERSECTION";
      break;

    case SVTK_DIFFERENCE:
      os << "DIFFERENCE";
      break;
  }
  os << "\n";
  os << indent << "ReorientDifferenceCells: " << this->ReorientDifferenceCells << "\n";
}

//-----------------------------------------------------------------------------
int svtkBooleanOperationPolyDataFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  return 1;
}

//-----------------------------------------------------------------------------
void svtkBooleanOperationPolyDataFilter ::CopyCells(svtkPolyData* in, svtkPolyData* out, int idx,
  svtkDataSetAttributes::FieldList& pointFieldList, svtkDataSetAttributes::FieldList& cellFieldList,
  svtkIdList* cellIds, bool reverseCells)
{
  // Largely copied from svtkPolyData::CopyCells, but modified to
  // use the special form of CopyData that uses a field list to
  // determine which data values to copy over.

  svtkPointData* outPD = out->GetPointData();
  svtkCellData* outCD = out->GetCellData();

  svtkFloatArray* outNormals = nullptr;
  if (reverseCells)
  {
    outNormals = svtkArrayDownCast<svtkFloatArray>(outPD->GetArray("Normals"));
  }

  svtkIdType numPts = in->GetNumberOfPoints();

  if (out->GetPoints() == nullptr)
  {
    svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
    out->SetPoints(points);
  }

  svtkPoints* newPoints = out->GetPoints();

  svtkSmartPointer<svtkIdList> pointMap = svtkSmartPointer<svtkIdList>::New();
  pointMap->SetNumberOfIds(numPts);
  for (svtkIdType i = 0; i < numPts; i++)
  {
    pointMap->SetId(i, -1);
  }

  // Filter the cells
  svtkSmartPointer<svtkGenericCell> cell = svtkSmartPointer<svtkGenericCell>::New();
  svtkSmartPointer<svtkIdList> newCellPts = svtkSmartPointer<svtkIdList>::New();
  for (svtkIdType cellId = 0; cellId < cellIds->GetNumberOfIds(); cellId++)
  {
    in->GetCell(cellIds->GetId(cellId), cell);
    svtkIdList* cellPts = cell->GetPointIds();
    svtkIdType numCellPts = cell->GetNumberOfPoints();

    for (svtkIdType i = 0; i < numCellPts; i++)
    {
      svtkIdType ptId = cellPts->GetId(i);
      svtkIdType newId = pointMap->GetId(ptId);
      if (newId < 0)
      {
        double x[3];
        in->GetPoint(ptId, x);
        newId = newPoints->InsertNextPoint(x);
        pointMap->SetId(ptId, newId);
        outPD->CopyData(pointFieldList, in->GetPointData(), idx, ptId, newId);

        if (reverseCells && outNormals)
        {
          float normal[3];
          outNormals->GetTypedTuple(newId, normal);
          normal[0] *= -1.0;
          normal[1] *= -1.0;
          normal[2] *= -1.0;
          outNormals->SetTypedTuple(newId, normal);
        }
      }
      newCellPts->InsertId(i, newId);
    }
    if (reverseCells)
    {
      for (svtkIdType i = 0; i < newCellPts->GetNumberOfIds() / 2; i++)
      {
        svtkIdType i1 = i;
        svtkIdType i2 = newCellPts->GetNumberOfIds() - i - 1;

        svtkIdType id = newCellPts->GetId(i1);
        newCellPts->SetId(i1, newCellPts->GetId(i2));
        newCellPts->SetId(i2, id);
      }
    }

    svtkIdType newCellId = out->InsertNextCell(cell->GetCellType(), newCellPts);
    outCD->CopyData(cellFieldList, in->GetCellData(), idx, cellIds->GetId(cellId), newCellId);

    newCellPts->Reset();
  } // for all cells
}
