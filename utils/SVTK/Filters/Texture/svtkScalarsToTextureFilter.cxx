/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarsToTextureFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkScalarsToTextureFilter.h"

#include "svtkDataObject.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkResampleToImage.h"
#include "svtkScalarsToColors.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTextureMapToPlane.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkScalarsToTextureFilter);

//-----------------------------------------------------------------------------
svtkScalarsToTextureFilter::svtkScalarsToTextureFilter()
{
  this->SetNumberOfOutputPorts(2);
  this->TextureDimensions[0] = this->TextureDimensions[1] = 128;
}

//-----------------------------------------------------------------------------
void svtkScalarsToTextureFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Texture dimensions: " << this->TextureDimensions[0] << "x"
     << this->TextureDimensions[1] << '\n';

  if (this->TransferFunction)
  {
    os << indent << "Transfer function:\n";
    this->TransferFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Transfer function: (none)" << endl;
  }
}

//-----------------------------------------------------------------------------
int svtkScalarsToTextureFilter::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 1)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
    return 1;
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

//-----------------------------------------------------------------------------
int svtkScalarsToTextureFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  svtkInformation* outInfo1 = outputVector->GetInformationObject(1);

  // get and check input
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    svtkErrorMacro("Input polydata is null.");
    return 0;
  }

  svtkDataArray* array = this->GetInputArrayToProcess(0, inputVector);
  if (!array)
  {
    svtkErrorMacro("No array to process.");
    return 0;
  }

  // get the name of array to process
  const char* arrayName = array->GetName();

  // get the output
  svtkPolyData* outputGeometry =
    svtkPolyData::SafeDownCast(outInfo0->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* outputTexture =
    svtkImageData::SafeDownCast(outInfo1->Get(svtkDataObject::DATA_OBJECT()));

  // generate texture coords
  svtkNew<svtkTextureMapToPlane> texMap;
  texMap->SetInputData(input);
  texMap->Update();
  svtkPolyData* pdTex = svtkPolyData::SafeDownCast(texMap->GetOutput());

  // Deep copy the poly data to first output, as it will be modified just after
  outputGeometry->DeepCopy(pdTex);

  // overwrite position with texture coordinates
  svtkDataArray* tcoords = pdTex->GetPointData()->GetTCoords();
  svtkPoints* pts = pdTex->GetPoints();
  for (svtkIdType i = 0; i < pts->GetNumberOfPoints(); i++)
  {
    double p[3];
    tcoords->GetTuple(i, p);
    p[2] = 0.0;
    pts->SetPoint(i, p);
  }
  pts->Modified();

  // generate texture image
  svtkNew<svtkResampleToImage> resample;
  resample->UseInputBoundsOff();
  resample->SetSamplingBounds(0.0, 1.0, 0.0, 1.0, 0.0, 0.0);
  resample->SetSamplingDimensions(
    std::max(1, this->TextureDimensions[0]), std::max(1, this->TextureDimensions[1]), 1);
  resample->SetInputDataObject(pdTex);
  resample->Update();

  outputTexture->ShallowCopy(resample->GetOutput());

  // compute RGBA through lookup table
  if (this->UseTransferFunction)
  {
    svtkDataArray* scalars = outputTexture->GetPointData()->GetArray(arrayName);
    svtkSmartPointer<svtkScalarsToColors> stc = this->TransferFunction;
    if (stc.Get() == nullptr)
    {
      // use a default lookup table
      double* range = scalars->GetRange();
      svtkNew<svtkLookupTable> lut;
      lut->SetTableRange(range[0], range[1]);
      lut->Build();
      stc = lut.Get();
    }

    svtkUnsignedCharArray* colors = stc->MapScalars(scalars, SVTK_COLOR_MODE_DEFAULT, -1);
    colors->SetName("RGBA");
    outputTexture->GetPointData()->SetScalars(colors);
    colors->Delete();
  }

  return 1;
}

// ----------------------------------------------------------------------------
int svtkScalarsToTextureFilter::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(1);

  int extent[6] = { 0, this->TextureDimensions[0] - 1, 0, this->TextureDimensions[1] - 1, 0, 0 };

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  outInfo->Set(svtkDataObject::ORIGIN(), 0.0, 0.0, 0.0);
  outInfo->Set(svtkDataObject::SPACING(), 1.0 / extent[1], 1.0 / extent[3], 0.0);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_FLOAT, 1);

  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
svtkScalarsToColors* svtkScalarsToTextureFilter::GetTransferFunction()
{
  return this->TransferFunction;
}

//-----------------------------------------------------------------------------
void svtkScalarsToTextureFilter::SetTransferFunction(svtkScalarsToColors* stc)
{
  if (this->TransferFunction.Get() != stc)
  {
    this->TransferFunction = stc;
    this->Modified();
  }
}
