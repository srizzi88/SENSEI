/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReverseSense.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkReverseSense.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkReverseSense);

// Construct object so that behavior is to reverse cell ordering and
// leave normal orientation as is.
svtkReverseSense::svtkReverseSense()
{
  this->ReverseCells = 1;
  this->ReverseNormals = 0;
}

int svtkReverseSense::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataArray* normals = input->GetPointData()->GetNormals();
  svtkDataArray* cellNormals = input->GetCellData()->GetNormals();

  svtkDebugMacro(<< "Reversing sense of poly data");

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // If specified, traverse all cells and reverse them
  int abort = 0;
  svtkIdType progressInterval;

  if (this->ReverseCells)
  {
    svtkIdType numCells = input->GetNumberOfCells();
    svtkCellArray *verts, *lines, *polys, *strips;

    // Instantiate necessary topology arrays
    verts = svtkCellArray::New();
    verts->DeepCopy(input->GetVerts());
    lines = svtkCellArray::New();
    lines->DeepCopy(input->GetLines());
    polys = svtkCellArray::New();
    polys->DeepCopy(input->GetPolys());
    strips = svtkCellArray::New();
    strips->DeepCopy(input->GetStrips());

    output->SetVerts(verts);
    verts->Delete();
    output->SetLines(lines);
    lines->Delete();
    output->SetPolys(polys);
    polys->Delete();
    output->SetStrips(strips);
    strips->Delete();

    progressInterval = numCells / 10 + 1;
    for (svtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
    {
      if (!(cellId % progressInterval)) // manage progress / early abort
      {
        this->UpdateProgress(0.6 * cellId / numCells);
        abort = this->GetAbortExecute();
      }
      output->ReverseCell(cellId);
    }
  }

  // If specified and normals available, reverse orientation of normals.
  // Using NewInstance() creates normals of the same data type.
  if (this->ReverseNormals && normals)
  {
    // first do point normals
    svtkIdType numPoints = input->GetNumberOfPoints();
    svtkDataArray* outNormals = normals->NewInstance();
    outNormals->SetNumberOfComponents(normals->GetNumberOfComponents());
    outNormals->SetNumberOfTuples(numPoints);
    outNormals->SetName(normals->GetName());
    double n[3];

    progressInterval = numPoints / 5 + 1;
    for (svtkIdType ptId = 0; ptId < numPoints; ++ptId)
    {
      if (!(ptId % progressInterval)) // manage progress / early abort
      {
        this->UpdateProgress(0.6 + 0.2 * ptId / numPoints);
        abort = this->GetAbortExecute();
      }
      normals->GetTuple(ptId, n);
      n[0] = -n[0];
      n[1] = -n[1];
      n[2] = -n[2];
      outNormals->SetTuple(ptId, n);
    }

    output->GetPointData()->SetNormals(outNormals);
    outNormals->Delete();
  }

  // now do cell normals
  if (this->ReverseNormals && cellNormals)
  {
    svtkIdType numCells = input->GetNumberOfCells();
    svtkDataArray* outNormals = cellNormals->NewInstance();
    outNormals->SetNumberOfComponents(cellNormals->GetNumberOfComponents());
    outNormals->SetNumberOfTuples(numCells);
    outNormals->SetName(cellNormals->GetName());
    double n[3];

    progressInterval = numCells / 5 + 1;
    for (svtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
    {
      if (!(cellId % progressInterval)) // manage progress / early abort
      {
        this->UpdateProgress(0.8 + 0.2 * cellId / numCells);
        abort = this->GetAbortExecute();
      }

      cellNormals->GetTuple(cellId, n);
      n[0] = -n[0];
      n[1] = -n[1];
      n[2] = -n[2];
      outNormals->SetTuple(cellId, n);
    }

    output->GetCellData()->SetNormals(outNormals);
    outNormals->Delete();
  }

  return 1;
}

void svtkReverseSense::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Reverse Cells: " << (this->ReverseCells ? "On\n" : "Off\n");
  os << indent << "Reverse Normals: " << (this->ReverseNormals ? "On\n" : "Off\n");
}
