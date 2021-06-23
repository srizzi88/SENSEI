/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearCellExtrusionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLinearCellExtrusionFilter.h"

#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPolygon.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <array>
#include <vector>

svtkStandardNewMacro(svtkLinearCellExtrusionFilter);

//----------------------------------------------------------------------------
svtkLinearCellExtrusionFilter::svtkLinearCellExtrusionFilter()
{
  // set default array
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
int svtkLinearCellExtrusionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkPolyData* input = svtkPolyData::GetData(inputVector[0]);
  svtkUnstructuredGrid* output = svtkUnstructuredGrid::GetData(outputVector);
  svtkDataArray* array = this->GetInputArrayToProcess(0, inputVector);

  svtkCellArray* polys = input->GetPolys();
  svtkNew<svtkPoints> outputPoints;
  outputPoints->DeepCopy(input->GetPoints());

  output->SetPoints(outputPoints);
  output->GetCellData()->ShallowCopy(input->GetCellData());

  if (this->MergeDuplicatePoints)
  {
    this->CreateDefaultLocator();
    this->Locator->SetDataSet(output);
    this->Locator->InitPointInsertion(outputPoints, outputPoints->GetBounds());

    for (svtkIdType i = 0; i < outputPoints->GetNumberOfPoints(); i++)
    {
      svtkIdType dummy;
      this->Locator->InsertUniquePoint(outputPoints->GetPoint(i), dummy);
    }
  }

  svtkDataArray* inputNormals = input->GetCellData()->GetNormals();

  output->Allocate(polys->GetSize() * 2); // estimation

  std::vector<std::array<double, 3> > topPoints;
  std::vector<svtkIdType> polyhedronIds; // used for polyhedrons

  svtkIdType cellId = 0;
  auto iter = svtk::TakeSmartPointer(polys->NewIterator());
  for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell(), cellId++)
  {
    svtkIdType cellSize;
    const svtkIdType* cellPoints;
    iter->GetCurrentCell(cellSize, cellPoints);

    topPoints.resize(cellSize);
    for (svtkIdType i = 0; i < cellSize; i++)
    {
      outputPoints->GetPoint(cellPoints[i], topPoints[i].data());
    }

    double normal[3];
    if (this->UseUserVector)
    {
      normal[0] = this->UserVector[0];
      normal[1] = this->UserVector[1];
      normal[2] = this->UserVector[2];
    }
    else
    {
      if (inputNormals)
      {
        inputNormals->GetTuple(cellId, normal);
      }
      else
      {
        svtkPolygon::ComputeNormal(cellSize, topPoints[0].data(), normal);
      }
    }

    // extrude
    double currentValue = (array ? array->GetComponent(cellId, 0) : 1.0);
    double scale = currentValue * this->ScaleFactor;
    for (svtkIdType i = 0; i < cellSize; i++)
    {
      auto& p = topPoints[i];
      p[0] += scale * normal[0];
      p[1] += scale * normal[1];
      p[2] += scale * normal[2];
    }

    if (cellSize == 3) // triangle => wedge
    {
      svtkIdType newPts[3];

      if (this->MergeDuplicatePoints)
      {
        this->Locator->InsertUniquePoint(topPoints[0].data(), newPts[0]);
        this->Locator->InsertUniquePoint(topPoints[1].data(), newPts[1]);
        this->Locator->InsertUniquePoint(topPoints[2].data(), newPts[2]);
      }
      else
      {
        newPts[0] = outputPoints->InsertNextPoint(topPoints[0].data());
        newPts[1] = outputPoints->InsertNextPoint(topPoints[1].data());
        newPts[2] = outputPoints->InsertNextPoint(topPoints[2].data());
      }

      svtkIdType ptsId[6] = { cellPoints[2], cellPoints[1], cellPoints[0], newPts[2], newPts[1],
        newPts[0] };

      output->InsertNextCell(SVTK_WEDGE, 6, ptsId);
    }
    else if (cellSize == 4) // quad => hexahedron
    {
      svtkIdType newPts[4];

      if (this->MergeDuplicatePoints)
      {
        this->Locator->InsertUniquePoint(topPoints[0].data(), newPts[0]);
        this->Locator->InsertUniquePoint(topPoints[1].data(), newPts[1]);
        this->Locator->InsertUniquePoint(topPoints[2].data(), newPts[2]);
        this->Locator->InsertUniquePoint(topPoints[3].data(), newPts[3]);
      }
      else
      {
        newPts[0] = outputPoints->InsertNextPoint(topPoints[0].data());
        newPts[1] = outputPoints->InsertNextPoint(topPoints[1].data());
        newPts[2] = outputPoints->InsertNextPoint(topPoints[2].data());
        newPts[3] = outputPoints->InsertNextPoint(topPoints[3].data());
      }

      svtkIdType ptsId[8] = { cellPoints[3], cellPoints[2], cellPoints[1], cellPoints[0], newPts[3],
        newPts[2], newPts[1], newPts[0] };

      output->InsertNextCell(SVTK_HEXAHEDRON, 8, ptsId);
    }
    else // generic case => polyhedron
    {
      polyhedronIds.resize(2 * (cellSize + 1) + cellSize * 5);

      svtkIdType* topFace = polyhedronIds.data();
      svtkIdType* baseFace = topFace + cellSize + 1;

      topFace[0] = cellSize;
      baseFace[0] = cellSize;
      for (svtkIdType i = 0; i < cellSize; i++)
      {
        if (this->MergeDuplicatePoints)
        {
          this->Locator->InsertUniquePoint(topPoints[i].data(), topFace[i + 1]);
        }
        else
        {
          topFace[i + 1] = outputPoints->InsertNextPoint(topPoints[i].data());
        }
        baseFace[i + 1] = cellPoints[cellSize - i - 1];
      }

      for (svtkIdType i = 0; i < cellSize; i++)
      {
        svtkIdType* currentSide = polyhedronIds.data() + 2 * (cellSize + 1) + 5 * i;
        currentSide[0] = 4;
        currentSide[1] = topFace[1 + (i + 1) % cellSize];
        currentSide[2] = topFace[1 + i];
        currentSide[3] = cellPoints[i];
        currentSide[4] = cellPoints[(i + 1) % cellSize];
      }

      output->InsertNextCell(SVTK_POLYHEDRON, cellSize + 2, polyhedronIds.data());

      if (cellId % 1000 == 0)
      {
        this->UpdateProgress(cellId / static_cast<double>(polys->GetNumberOfCells()));
      }
    }
  }

  output->Squeeze();

  this->UpdateProgress(1.0);

  return 1;
}

//----------------------------------------------------------------------------
void svtkLinearCellExtrusionFilter::CreateDefaultLocator()
{
  if (!this->Locator)
  {
    this->Locator = svtkSmartPointer<svtkMergePoints>::New();
  }
}

//----------------------------------------------------------------------------
void svtkLinearCellExtrusionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScaleFactor: " << this->ScaleFactor << "\n"
     << indent << "UserVector: " << this->UserVector[0] << " " << this->UserVector[1] << " "
     << this->UserVector[2] << "\n"
     << indent << "UseUserVector: " << (this->UseUserVector ? "ON" : "OFF") << "\n"
     << indent << "MergeDuplicatePoints: " << (this->MergeDuplicatePoints ? "ON" : "OFF") << endl;
}

//----------------------------------------------------------------------------
int svtkLinearCellExtrusionFilter::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUnstructuredGrid");
  return 1;
}
