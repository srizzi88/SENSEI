/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCirclePackToPolyData.cxx

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
#include "svtkCirclePackToPolyData.h"
#include "svtkAppendPolyData.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSectorSource.h"
#include "svtkSmartPointer.h"
#include "svtkStripper.h"
#include "svtkTimerLog.h"
#include "svtkTree.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkStandardNewMacro(svtkCirclePackToPolyData);

svtkCirclePackToPolyData::svtkCirclePackToPolyData()
{
  this->SetCirclesArrayName("circles");
  this->Resolution = 100;
}

svtkCirclePackToPolyData::~svtkCirclePackToPolyData() = default;

int svtkCirclePackToPolyData::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
  return 1;
}

int svtkCirclePackToPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkTree* inputTree = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* outputPoly = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (inputTree->GetNumberOfVertices() == 0)
  {
    return 1;
  }

  svtkDataArray* circlesArray = this->GetInputArrayToProcess(0, inputTree);
  if (!circlesArray)
  {
    svtkErrorMacro("Circles array not found.");
    return 0;
  }

  double progress = 0.0;
  this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
  SVTK_CREATE(svtkAppendPolyData, appendFilter);

  for (int i = 0; i < inputTree->GetNumberOfVertices(); i++)
  {
    // Grab coords from the input
    double circle[3];
    circlesArray->GetTuple(i, circle);
    SVTK_CREATE(svtkPolyData, circlePData);
    this->CreateCircle(circle[0], circle[1], 0.0, circle[2], this->Resolution, circlePData);
    appendFilter->AddInputData(circlePData);

    if (i % 1000 == 0)
    {
      progress = static_cast<double>(i) / inputTree->GetNumberOfVertices() * 0.8;
      this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
    }
  }

  appendFilter->Update();
  outputPoly->ShallowCopy(appendFilter->GetOutput());

  // Pass the input vertex data to the output cell data :)
  svtkDataSetAttributes* const input_vertex_data = inputTree->GetVertexData();
  svtkDataSetAttributes* const output_cell_data = outputPoly->GetCellData();
  output_cell_data->PassData(input_vertex_data);

  return 1;
}

void svtkCirclePackToPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Resolution: " << this->Resolution << endl;
}

void svtkCirclePackToPolyData::CreateCircle(const double& x, const double& y, const double& z,
  const double& radius, const int& resolution, svtkPolyData* polyData)
{
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> cells = svtkSmartPointer<svtkCellArray>::New();

  points->SetNumberOfPoints(resolution);
  cells->AllocateEstimate(1, resolution);
  cells->InsertNextCell(resolution);

  for (int i = 0; i < resolution; ++i)
  {
    double theta = svtkMath::RadiansFromDegrees(360. * i / double(resolution));
    double xp = x + radius * cos(theta);
    double yp = y + radius * sin(theta);
    points->SetPoint(i, xp, yp, z);
    cells->InsertCellPoint(i);
  }

  polyData->Initialize();
  polyData->SetPolys(cells);
  polyData->SetPoints(points);
}
