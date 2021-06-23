/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeMapToPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkTreeMapToPolyData.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTree.h"

svtkStandardNewMacro(svtkTreeMapToPolyData);

svtkTreeMapToPolyData::svtkTreeMapToPolyData()
{
  this->SetRectanglesArrayName("area");
  this->SetLevelArrayName("level");
  this->LevelDeltaZ = 0.001;
  this->AddNormals = true;
}

svtkTreeMapToPolyData::~svtkTreeMapToPolyData() = default;

int svtkTreeMapToPolyData::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
  return 1;
}

int svtkTreeMapToPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkTree* inputTree = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* outputPoly = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // For each input vertex create 4 points and 1 cell (quad)
  svtkPoints* outputPoints = svtkPoints::New();
  outputPoints->SetNumberOfPoints(inputTree->GetNumberOfVertices() * 4);
  svtkCellArray* outputCells = svtkCellArray::New();

  // Create an array for the point normals
  svtkFloatArray* normals = svtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(inputTree->GetNumberOfVertices() * 4);
  normals->SetName("normals");

  svtkDataArray* coordArray = this->GetInputArrayToProcess(0, inputTree);
  if (!coordArray)
  {
    svtkErrorMacro("Area array not found.");
    return 0;
  }
  svtkDataArray* levelArray = this->GetInputArrayToProcess(1, inputTree);

  // Now set the point coordinates, normals, and insert the cell
  for (int i = 0; i < inputTree->GetNumberOfVertices(); i++)
  {
    // Grab coords from the input
    double coords[4];
    coordArray->GetTuple(i, coords);

    double z = 0;
    if (levelArray)
    {
      z = this->LevelDeltaZ * levelArray->GetTuple1(i);
    }
    else
    {
      z = this->LevelDeltaZ * inputTree->GetLevel(i);
    }

    int index = i * 4;
    outputPoints->SetPoint(index, coords[0], coords[2], z);
    outputPoints->SetPoint(index + 1, coords[1], coords[2], z);
    outputPoints->SetPoint(index + 2, coords[1], coords[3], z);
    outputPoints->SetPoint(index + 3, coords[0], coords[3], z);

    // Create an asymmetric gradient on the cells
    // this gradient helps differentiate same colored
    // cells from their neighbors. The asymmetric
    // nature of the gradient is required.
    normals->SetComponent(index, 0, 0);
    normals->SetComponent(index, 1, .707);
    normals->SetComponent(index, 2, .707);

    normals->SetComponent(index + 1, 0, 0);
    normals->SetComponent(index + 1, 1, .866);
    normals->SetComponent(index + 1, 2, .5);

    normals->SetComponent(index + 2, 0, 0);
    normals->SetComponent(index + 2, 1, .707);
    normals->SetComponent(index + 2, 2, .707);

    normals->SetComponent(index + 3, 0, 0);
    normals->SetComponent(index + 3, 1, 0);
    normals->SetComponent(index + 3, 2, 1);

    // Create the cell that uses these points
    svtkIdType cellConn[] = { index, index + 1, index + 2, index + 3 };
    outputCells->InsertNextCell(4, cellConn);
  }

  // Pass the input point data to the output cell data :)
  outputPoly->GetCellData()->PassData(inputTree->GetVertexData());

  // Set the output points and cells
  outputPoly->SetPoints(outputPoints);
  outputPoly->SetPolys(outputCells);

  if (this->AddNormals)
  {
    // Set the point normals
    outputPoly->GetPointData()->AddArray(normals);
    outputPoly->GetPointData()->SetActiveNormals("normals");
  }

  // Clean up.
  normals->Delete();
  outputPoints->Delete();
  outputCells->Delete();

  return 1;
}

void svtkTreeMapToPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LevelDeltaZ: " << this->LevelDeltaZ << endl;
  os << indent << "AddNormals: " << this->AddNormals << endl;
}
