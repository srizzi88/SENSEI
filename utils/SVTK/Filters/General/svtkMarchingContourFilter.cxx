/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMarchingContourFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMarchingContourFilter.h"

#include "svtkCell.h"
#include "svtkContourFilter.h"
#include "svtkContourValues.h"
#include "svtkImageMarchingCubes.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMarchingCubes.h"
#include "svtkMarchingSquares.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkScalarTree.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredPoints.h"

#include <cmath>

svtkStandardNewMacro(svtkMarchingContourFilter);

// Construct object with initial range (0,1) and single contour value
// of 0.0.
svtkMarchingContourFilter::svtkMarchingContourFilter()
{
  this->ContourValues = svtkContourValues::New();

  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->Locator = nullptr;

  this->UseScalarTree = 0;
  this->ScalarTree = nullptr;
}

svtkMarchingContourFilter::~svtkMarchingContourFilter()
{
  this->ContourValues->Delete();
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (this->ScalarTree)
  {
    this->ScalarTree->Delete();
  }
}

// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
svtkMTimeType svtkMarchingContourFilter::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->ContourValues)
  {
    time = this->ContourValues->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->Locator)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//
// General contouring filter.  Handles arbitrary input.
//
int svtkMarchingContourFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataArray* inScalars;
  svtkIdType numCells;

  svtkDebugMacro(<< "Executing marching contour filter");

  numCells = input->GetNumberOfCells();
  inScalars = input->GetPointData()->GetScalars();
  if (!inScalars || numCells < 1)
  {
    svtkErrorMacro(<< "No data to contour");
    return 1;
  }

  // If structured points, use more efficient algorithms
  if ((input->GetDataObjectType() == SVTK_STRUCTURED_POINTS))
  {
    if (inScalars->GetDataType() != SVTK_BIT)
    {
      int dim = input->GetCell(0)->GetCellDimension();

      if (input->GetCell(0)->GetCellDimension() >= 2)
      {
        svtkDebugMacro(<< "Structured Points");
        this->StructuredPointsContour(dim, input, output);
        return 1;
      }
    }
  }

  if ((input->GetDataObjectType() == SVTK_IMAGE_DATA))
  {
    if (inScalars->GetDataType() != SVTK_BIT)
    {
      int dim = input->GetCell(0)->GetCellDimension();

      if (input->GetCell(0)->GetCellDimension() >= 2)
      {
        svtkDebugMacro(<< "Image");
        this->ImageContour(dim, input, output);
        return 1;
      }
    }
  }

  svtkDebugMacro(<< "Unoptimized");
  this->DataSetContour(input, output);

  return 1;
}

void svtkMarchingContourFilter::StructuredPointsContour(
  int dim, svtkDataSet* input, svtkPolyData* thisOutput)
{
  svtkPolyData* output;
  svtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double* values = this->ContourValues->GetValues();

  if (dim == 2) // marching squares
  {
    svtkMarchingSquares* msquares;
    int i;

    msquares = svtkMarchingSquares::New();
    msquares->SetInputData((svtkImageData*)input);
    msquares->SetDebug(this->Debug);
    msquares->SetNumberOfContours(numContours);
    for (i = 0; i < numContours; i++)
    {
      msquares->SetValue(i, values[i]);
    }

    msquares->Update();
    output = msquares->GetOutput();
    output->Register(this);
    msquares->Delete();
  }

  else // marching cubes
  {
    svtkMarchingCubes* mcubes;
    int i;

    mcubes = svtkMarchingCubes::New();
    mcubes->SetInputData((svtkImageData*)input);
    mcubes->SetComputeNormals(this->ComputeNormals);
    mcubes->SetComputeGradients(this->ComputeGradients);
    mcubes->SetComputeScalars(this->ComputeScalars);
    mcubes->SetDebug(this->Debug);
    mcubes->SetNumberOfContours(numContours);
    for (i = 0; i < numContours; i++)
    {
      mcubes->SetValue(i, values[i]);
    }

    mcubes->Update();
    output = mcubes->GetOutput();
    output->Register(this);
    mcubes->Delete();
  }

  thisOutput->CopyStructure(output);
  thisOutput->GetPointData()->ShallowCopy(output->GetPointData());
  output->UnRegister(this);
}

void svtkMarchingContourFilter::DataSetContour(svtkDataSet* input, svtkPolyData* output)
{
  svtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double* values = this->ContourValues->GetValues();

  svtkContourFilter* contour = svtkContourFilter::New();
  contour->SetInputData((svtkImageData*)input);
  contour->SetComputeNormals(this->ComputeNormals);
  contour->SetComputeGradients(this->ComputeGradients);
  contour->SetComputeScalars(this->ComputeScalars);
  contour->SetDebug(this->Debug);
  contour->SetNumberOfContours(numContours);
  for (int i = 0; i < numContours; i++)
  {
    contour->SetValue(i, values[i]);
  }

  contour->Update();
  output->ShallowCopy(contour->GetOutput());
  this->SetOutput(output);
  contour->Delete();
}

void svtkMarchingContourFilter::ImageContour(int dim, svtkDataSet* input, svtkPolyData* output)
{
  svtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double* values = this->ContourValues->GetValues();
  svtkPolyData* contourOutput;

  if (dim == 2) // marching squares
  {
    svtkMarchingSquares* msquares;
    int i;

    msquares = svtkMarchingSquares::New();
    msquares->SetInputData((svtkImageData*)input);
    msquares->SetDebug(this->Debug);
    msquares->SetNumberOfContours(numContours);
    for (i = 0; i < numContours; i++)
    {
      msquares->SetValue(i, values[i]);
    }

    contourOutput = msquares->GetOutput();
    msquares->Update();
    output->ShallowCopy(contourOutput);
    msquares->Delete();
  }

  else // image marching cubes
  {
    svtkImageMarchingCubes* mcubes;
    int i;

    mcubes = svtkImageMarchingCubes::New();
    mcubes->SetInputData((svtkImageData*)input);
    mcubes->SetComputeNormals(this->ComputeNormals);
    mcubes->SetComputeGradients(this->ComputeGradients);
    mcubes->SetComputeScalars(this->ComputeScalars);
    mcubes->SetDebug(this->Debug);
    mcubes->SetNumberOfContours(numContours);
    for (i = 0; i < numContours; i++)
    {
      mcubes->SetValue(i, values[i]);
    }

    contourOutput = mcubes->GetOutput();
    mcubes->Update();
    output->ShallowCopy(contourOutput);
    mcubes->Delete();
  }
}

// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkMarchingContourFilter::SetLocator(svtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (locator)
  {
    locator->Register(this);
  }
  this->Locator = locator;
  this->Modified();
}

void svtkMarchingContourFilter::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

int svtkMarchingContourFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

void svtkMarchingContourFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "Use Scalar Tree: " << (this->UseScalarTree ? "On\n" : "Off\n");

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}
