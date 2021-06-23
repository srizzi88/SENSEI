/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRotationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRotationFilter.h"

#include "svtkCellData.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTransform.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkRotationFilter);

//---------------------------------------------------------------------------
svtkRotationFilter::svtkRotationFilter()
{
  this->Axis = 2;
  this->CopyInput = 0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0;
  this->NumberOfCopies = 0;
  this->Angle = 0;
}

//---------------------------------------------------------------------------
svtkRotationFilter::~svtkRotationFilter() = default;

//---------------------------------------------------------------------------
void svtkRotationFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Axis: " << this->Axis << endl;
  os << indent << "CopyInput: " << this->CopyInput << endl;
  os << indent << "Center: (" << this->Center[0] << "," << this->Center[1] << "," << this->Center[2]
     << ")" << endl;
  os << indent << "NumberOfCopies: " << this->NumberOfCopies << endl;
  os << indent << "Angle: " << this->Angle << endl;
}

//---------------------------------------------------------------------------
int svtkRotationFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* inCD = input->GetCellData();
  svtkCellData* outCD = output->GetCellData();

  if (!this->GetNumberOfCopies())
  {
    svtkErrorMacro("No number of copy set!");
    return 1;
  }

  double tuple[3];
  svtkPoints* outPoints;
  double point[3], center[3], negativCenter[3];
  svtkIdType ptId, cellId;
  svtkGenericCell* cell = svtkGenericCell::New();
  svtkIdList* ptIds = svtkIdList::New();

  outPoints = svtkPoints::New();

  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();

  if (this->CopyInput)
  {
    outPoints->Allocate((this->CopyInput + this->GetNumberOfCopies()) * numPts);
    output->Allocate((this->CopyInput + this->GetNumberOfCopies()) * numPts);
  }
  else
  {
    outPoints->Allocate(this->GetNumberOfCopies() * numPts);
    output->Allocate(this->GetNumberOfCopies() * numPts);
  }

  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  svtkDataArray *inPtVectors, *outPtVectors, *inPtNormals;
  svtkDataArray *inCellVectors, *outCellVectors, *inCellNormals;

  inPtVectors = inPD->GetVectors();
  outPtVectors = outPD->GetVectors();
  inPtNormals = inPD->GetNormals();
  inCellVectors = inCD->GetVectors();
  outCellVectors = outCD->GetVectors();
  inCellNormals = inCD->GetNormals();

  // Copy first points.
  if (this->CopyInput)
  {
    for (svtkIdType i = 0; i < numPts; ++i)
    {
      input->GetPoint(i, point);
      ptId = outPoints->InsertNextPoint(point);
      outPD->CopyData(inPD, i, ptId);
    }
  }
  svtkTransform* localTransform = svtkTransform::New();
  // Rotate points.
  // double angle = svtkMath::RadiansFromDegrees( this->GetAngle() );
  this->GetCenter(center);
  negativCenter[0] = -center[0];
  negativCenter[1] = -center[1];
  negativCenter[2] = -center[2];

  for (int k = 0; k < this->GetNumberOfCopies(); ++k)
  {
    localTransform->Identity();
    localTransform->Translate(center);
    switch (this->Axis)
    {
      case USE_X:
        localTransform->RotateX((k + 1) * this->GetAngle());
        break;

      case USE_Y:
        localTransform->RotateY((k + 1) * this->GetAngle());
        break;

      case USE_Z:
        localTransform->RotateZ((k + 1) * this->GetAngle());
        break;
    }
    localTransform->Translate(negativCenter);
    for (svtkIdType i = 0; i < numPts; ++i)
    {
      input->GetPoint(i, point);
      localTransform->TransformPoint(point, point);
      ptId = outPoints->InsertNextPoint(point);
      outPD->CopyData(inPD, i, ptId);
      if (inPtVectors)
      {
        inPtVectors->GetTuple(i, tuple);
        outPtVectors->SetTuple(ptId, tuple);
      }
      if (inPtNormals)
      {
        // inPtNormals->GetTuple(i, tuple);
        // outPtNormals->SetTuple(ptId, tuple);
      }
    }
  }

  localTransform->Delete();

  svtkIdType* newCellPts;
  svtkIdList* cellPts;

  // Copy original cells.
  if (this->CopyInput)
  {
    for (svtkIdType i = 0; i < numCells; ++i)
    {
      input->GetCellPoints(i, ptIds);
      output->InsertNextCell(input->GetCellType(i), ptIds);
      outCD->CopyData(inCD, i, i);
    }
  }

  // Generate rotated cells.
  for (int k = 0; k < this->GetNumberOfCopies(); ++k)
  {
    for (svtkIdType i = 0; i < numCells; ++i)
    {
      input->GetCellPoints(i, ptIds);
      input->GetCell(i, cell);
      svtkIdType numCellPts = cell->GetNumberOfPoints();
      int cellType = cell->GetCellType();
      cellPts = cell->GetPointIds();
      // Triangle strips with even number of triangles have
      // to be handled specially. A degenerate triangle is
      // introduce to flip all the triangles properly.
      if (cellType == SVTK_TRIANGLE_STRIP && numCellPts % 2 == 0)
      {
        svtkErrorMacro(<< "Triangles with bad points");
        return 0;
      }
      else
      {
        svtkDebugMacro(<< "celltype " << cellType << " numCellPts " << numCellPts);
        newCellPts = new svtkIdType[numCellPts];
        // for (j = numCellPts-1; j >= 0; j--)
        for (svtkIdType j = 0; j < numCellPts; ++j)
        {
          // newCellPts[numCellPts-1-j] = cellPts->GetId(j) + numPts*k;
          newCellPts[j] = cellPts->GetId(j) + numPts * k;
          if (this->CopyInput)
          {
            // newCellPts[numCellPts-1-j] += numPts;
            newCellPts[j] += numPts;
          }
        }
      }
      cellId = output->InsertNextCell(cellType, numCellPts, newCellPts);
      delete[] newCellPts;
      outCD->CopyData(inCD, i, cellId);
      if (inCellVectors)
      {
        inCellVectors->GetTuple(i, tuple);
        outCellVectors->SetTuple(cellId, tuple);
      }
      if (inCellNormals)
      {
        // inCellNormals->GetTuple(i, tuple);
        // outCellNormals->SetTuple(cellId, tuple);
      }
    }
  }

  cell->Delete();
  ptIds->Delete();
  output->SetPoints(outPoints);
  outPoints->Delete();
  output->CheckAttributes();

  return 1;
}

int svtkRotationFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}
