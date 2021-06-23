/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRendererSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRendererSource.h"

#include "svtkCommand.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMapper.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkRendererSource);

svtkCxxSetObjectMacro(svtkRendererSource, Input, svtkRenderer);

//----------------------------------------------------------------------------
svtkRendererSource::svtkRendererSource()
{
  this->Input = nullptr;
  this->WholeWindow = 0;
  this->RenderFlag = 0;
  this->DepthValues = 0;
  this->DepthValuesInScalars = 0;
  this->DepthValuesOnly = 0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkRendererSource::~svtkRendererSource()
{
  if (this->Input)
  {
    this->Input->UnRegister(this);
    this->Input = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkRendererSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkIdType numOutPts;
  float x1, y1, x2, y2;
  int dims[3];

  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkImageData* output = svtkImageData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  int uExtent[6];
  info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExtent);
  output->SetExtent(uExtent);

  svtkDebugMacro(<< "Extracting image");

  // Make sure there is proper input
  if (this->Input == nullptr)
  {
    svtkErrorMacro(<< "Please specify a renderer as input!");
    return;
  }

  svtkRenderWindow* renWin = this->Input->GetRenderWindow();
  if (renWin == nullptr)
  {
    svtkErrorMacro(<< "Renderer needs to be associated with renderin window!");
    return;
  }

  // We're okay to go. There are two paths to proceed. Simply a depth image,
  // or some combination of depth image and color scalars.
  // calc the pixel range for the renderer
  if (this->RenderFlag)
  {
    renWin->Render();
  }

  x1 = this->Input->GetViewport()[0] * (renWin->GetSize()[0] - 1);
  y1 = this->Input->GetViewport()[1] * (renWin->GetSize()[1] - 1);
  x2 = this->Input->GetViewport()[2] * (renWin->GetSize()[0] - 1);
  y2 = this->Input->GetViewport()[3] * (renWin->GetSize()[1] - 1);

  if (this->WholeWindow)
  {
    x1 = 0;
    y1 = 0;
    x2 = renWin->GetSize()[0] - 1;
    y2 = renWin->GetSize()[1] - 1;
  }

  // Get origin, aspect ratio and dimensions from the input
  dims[0] = static_cast<int>(x2 - x1 + 1);
  dims[1] = static_cast<int>(y2 - y1 + 1);
  dims[2] = 1;
  output->SetDimensions(dims);
  numOutPts = dims[0] * dims[1];

  // If simply requesting depth values (no colors), do the following
  // and then return.
  if (this->DepthValuesOnly)
  {
    float *zBuf, *ptr;
    output->AllocateScalars(info);
    svtkFloatArray* outScalars =
      svtkArrayDownCast<svtkFloatArray>(output->GetPointData()->GetScalars());
    outScalars->SetName("ZValues");
    ptr = outScalars->WritePointer(0, numOutPts);

    zBuf = renWin->GetZbufferData(
      static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2));

    memcpy(ptr, zBuf, numOutPts * sizeof(float));

    delete[] zBuf;
    return;
  }

  // Okay requesting color scalars plus possibly depth values.
  unsigned char *pixels, *ptr;
  output->AllocateScalars(info);
  svtkUnsignedCharArray* outScalars =
    svtkArrayDownCast<svtkUnsignedCharArray>(output->GetPointData()->GetScalars());

  if (this->DepthValuesInScalars)
  {
    outScalars->SetName("RGBValues");
  }
  else
  {
    outScalars->SetName("RGBZValues");
  }

  // Allocate data.  Scalar type is FloatScalars.
  pixels = renWin->GetPixelData(
    static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2), 1);

  // allocate scalars
  int nb_comp = output->GetNumberOfScalarComponents();
  ptr = outScalars->WritePointer(0, numOutPts * nb_comp);

  // copy scalars over (if only RGB is requested, use the pixels directly)
  if (!this->DepthValuesInScalars)
  {
    memcpy(ptr, pixels, numOutPts * nb_comp);
  }

  // Lets get the ZBuffer also, if requested.
  if (this->DepthValues || this->DepthValuesInScalars)
  {
    float* zBuf = renWin->GetZbufferData(
      static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2));

    // If RGBZ is requested, intermix RGB with shift/scaled Z
    if (this->DepthValuesInScalars)
    {
      float *zptr = zBuf, *zptr_end = zBuf + numOutPts;
      float min = *zBuf, max = *zBuf;
      while (zptr < zptr_end)
      {
        if (min < *zptr)
        {
          min = *zptr;
        }
        if (max > *zptr)
        {
          max = *zptr;
        }
        zptr++;
      }
      float scale = 255.0 / (max - min);

      zptr = zBuf;
      unsigned char* ppixels = pixels;
      while (zptr < zptr_end)
      {
        *ptr++ = *ppixels++;
        *ptr++ = *ppixels++;
        *ptr++ = *ppixels++;
        *ptr++ = static_cast<unsigned char>((*zptr++ - min) * scale);
      }
    }

    // If Z is requested as independent array, create it
    if (this->DepthValues)
    {
      svtkFloatArray* zArray = svtkFloatArray::New();
      zArray->Allocate(numOutPts);
      zArray->SetNumberOfTuples(numOutPts);
      float* zPtr = zArray->WritePointer(0, numOutPts);
      memcpy(zPtr, zBuf, numOutPts * sizeof(float));
      zArray->SetName("ZBuffer");
      output->GetPointData()->AddArray(zArray);
      zArray->Delete();
    }

    delete[] zBuf;
  }

  delete[] pixels;
}

//----------------------------------------------------------------------------
void svtkRendererSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RenderFlag: " << (this->RenderFlag ? "On\n" : "Off\n");

  if (this->Input)
  {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Input: (none)\n";
  }

  os << indent << "Whole Window: " << (this->WholeWindow ? "On\n" : "Off\n");
  os << indent << "Depth Values: " << (this->DepthValues ? "On\n" : "Off\n");
  os << indent << "Depth Values In Scalars: " << (this->DepthValuesInScalars ? "On\n" : "Off\n");
  os << indent << "Depth Values Only: " << (this->DepthValuesOnly ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
svtkMTimeType svtkRendererSource::GetMTime()
{
  svtkRenderer* ren = this->GetInput();
  svtkMTimeType t1 = this->MTime.GetMTime();
  svtkMTimeType t2;

  if (!ren)
  {
    return t1;
  }

  // Update information on the input and
  // compute information that is general to svtkDataObject.
  t2 = ren->GetMTime();
  if (t2 > t1)
  {
    t1 = t2;
  }
  svtkActorCollection* actors = ren->GetActors();
  svtkCollectionSimpleIterator ait;
  actors->InitTraversal(ait);
  svtkActor* actor;
  svtkMapper* mapper;
  svtkDataSet* data;
  while ((actor = actors->GetNextActor(ait)))
  {
    t2 = actor->GetMTime();
    if (t2 > t1)
    {
      t1 = t2;
    }
    mapper = actor->GetMapper();
    if (mapper)
    {
      t2 = mapper->GetMTime();
      if (t2 > t1)
      {
        t1 = t2;
      }
      data = mapper->GetInput();
      if (data)
      {
        mapper->GetInputAlgorithm()->UpdateInformation();
        t2 = data->GetMTime();
        if (t2 > t1)
        {
          t1 = t2;
        }
      }
      t2 = svtkDemandDrivenPipeline::SafeDownCast(mapper->GetInputExecutive())->GetPipelineMTime();
      if (t2 > t1)
      {
        t1 = t2;
      }
    }
  }

  return t1;
}

//----------------------------------------------------------------------------
void svtkRendererSource::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkRenderer* ren = this->GetInput();
  if (ren == nullptr || ren->GetRenderWindow() == nullptr)
  {
    svtkErrorMacro("The input renderer has not been set yet!!!");
    return;
  }

  // calc the pixel range for the renderer
  float x1, y1, x2, y2;
  x1 = ren->GetViewport()[0] * ((ren->GetRenderWindow())->GetSize()[0] - 1);
  y1 = ren->GetViewport()[1] * ((ren->GetRenderWindow())->GetSize()[1] - 1);
  x2 = ren->GetViewport()[2] * ((ren->GetRenderWindow())->GetSize()[0] - 1);
  y2 = ren->GetViewport()[3] * ((ren->GetRenderWindow())->GetSize()[1] - 1);
  if (this->WholeWindow)
  {
    x1 = 0;
    y1 = 0;
    x2 = (this->Input->GetRenderWindow())->GetSize()[0] - 1;
    y2 = (this->Input->GetRenderWindow())->GetSize()[1] - 1;
  }
  int extent[6] = { 0, static_cast<int>(x2 - x1), 0, static_cast<int>(y2 - y1), 0, 0 };

  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  if (this->DepthValuesOnly)
  {
    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_FLOAT, 1);
  }
  else
  {
    svtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, SVTK_UNSIGNED_CHAR, 3 + (this->DepthValuesInScalars ? 1 : 0));
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkRendererSource::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->RequestData(request, inputVector, outputVector);
    return 1;
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    this->RequestInformation(request, inputVector, outputVector);
    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
svtkImageData* svtkRendererSource::GetOutput()
{
  return svtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int svtkRendererSource::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  return 1;
}
