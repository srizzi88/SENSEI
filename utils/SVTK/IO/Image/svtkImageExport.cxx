/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageExport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageExport.h"

#include "svtkAlgorithmOutput.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkMatrix3x3.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cctype>
#include <cstring>

svtkStandardNewMacro(svtkImageExport);

//----------------------------------------------------------------------------
svtkImageExport::svtkImageExport()
{
  this->ImageLowerLeft = 1;
  this->ExportVoidPointer = nullptr;
  this->DataDimensions[0] = this->DataDimensions[1] = this->DataDimensions[2] = 0;
  this->LastPipelineMTime = 0;

  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
svtkImageExport::~svtkImageExport() = default;

//----------------------------------------------------------------------------
svtkAlgorithm* svtkImageExport::GetInputAlgorithm()
{
  return this->GetInputConnection(0, 0) ? this->GetInputConnection(0, 0)->GetProducer() : nullptr;
}

//----------------------------------------------------------------------------
svtkInformation* svtkImageExport::GetInputInformation()
{
  return this->GetExecutive()->GetInputInformation(0, 0);
}

//----------------------------------------------------------------------------
void svtkImageExport::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ImageLowerLeft: " << (this->ImageLowerLeft ? "On\n" : "Off\n");
}

svtkImageData* svtkImageExport::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
svtkIdType svtkImageExport::GetDataMemorySize()
{
  svtkImageData* input = this->GetInput();
  if (input == nullptr)
  {
    return 0;
  }

  this->GetInputAlgorithm()->UpdateInformation();
  svtkInformation* inInfo = this->GetInputInformation();
  int* extent = inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int size = input->GetScalarSize();
  size *= input->GetNumberOfScalarComponents();
  size *= (extent[1] - extent[0] + 1);
  size *= (extent[3] - extent[2] + 1);
  size *= (extent[5] - extent[4] + 1);

  return size;
}

//----------------------------------------------------------------------------
void svtkImageExport::GetDataDimensions(int* dims)
{
  svtkImageData* input = this->GetInput();
  if (input == nullptr)
  {
    dims[0] = dims[1] = dims[2] = 0;
    return;
  }

  this->GetInputAlgorithm()->UpdateInformation();
  svtkInformation* inInfo = this->GetInputInformation();
  int* extent = inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
}

//----------------------------------------------------------------------------
void svtkImageExport::SetExportVoidPointer(void* ptr)
{
  if (this->ExportVoidPointer == ptr)
  {
    return;
  }
  this->ExportVoidPointer = ptr;
  this->Modified();
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int svtkImageExport::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  // we are the end of the pipeline, we do nothing
  return 1;
}

//----------------------------------------------------------------------------
// Exports all the data from the input.
void svtkImageExport::Export(void* output)
{
  void* ptr = this->GetPointerToData();
  if (!ptr)
  {
    // GetPointerToData() outputs an error message.
    return;
  }

  if (this->ImageLowerLeft)
  {
    memcpy(output, ptr, this->GetDataMemorySize());
  }
  else
  { // flip the image when it is output
    int* extent =
      this->GetInputInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    int xsize = extent[1] - extent[0] + 1;
    int ysize = extent[3] - extent[2] + 1;
    int zsize = extent[5] - extent[4] + 1;
    int csize = this->GetInput()->GetScalarSize() * this->GetInput()->GetNumberOfScalarComponents();

    for (svtkIdType i = 0; i < zsize; i++)
    {
      ptr = static_cast<void*>(static_cast<char*>(ptr) + ysize * xsize * csize);
      for (svtkIdType j = 0; j < ysize; j++)
      {
        ptr = static_cast<void*>(static_cast<char*>(ptr) - xsize * csize);
        memcpy(output, ptr, xsize * csize);
        output = static_cast<void*>(static_cast<char*>(output) + xsize * csize);
      }
      ptr = static_cast<void*>(static_cast<char*>(ptr) + ysize * xsize * csize);
    }
  }
}

//----------------------------------------------------------------------------
// Provides a valid pointer to the data (only valid until the next
// update, though)

void* svtkImageExport::GetPointerToData()
{
  // Error checking
  if (this->GetInput() == nullptr)
  {
    svtkErrorMacro(<< "Export: Please specify an input!");
    return nullptr;
  }

  svtkImageData* input = this->GetInput();
  svtkAlgorithm* inpAlgorithm = this->GetInputAlgorithm();
  inpAlgorithm->UpdateInformation();
  inpAlgorithm->ReleaseDataFlagOff();

  inpAlgorithm->UpdateWholeExtent();
  this->UpdateProgress(0.0);
  this->UpdateProgress(1.0);

  return input->GetScalarPointer();
}

//----------------------------------------------------------------------------
void* svtkImageExport::GetCallbackUserData()
{
  return this;
}

svtkImageExport::UpdateInformationCallbackType svtkImageExport::GetUpdateInformationCallback() const
{
  return &svtkImageExport::UpdateInformationCallbackFunction;
}

svtkImageExport::PipelineModifiedCallbackType svtkImageExport::GetPipelineModifiedCallback() const
{
  return &svtkImageExport::PipelineModifiedCallbackFunction;
}

svtkImageExport::WholeExtentCallbackType svtkImageExport::GetWholeExtentCallback() const
{
  return &svtkImageExport::WholeExtentCallbackFunction;
}

svtkImageExport::SpacingCallbackType svtkImageExport::GetSpacingCallback() const
{
  return &svtkImageExport::SpacingCallbackFunction;
}

svtkImageExport::OriginCallbackType svtkImageExport::GetOriginCallback() const
{
  return &svtkImageExport::OriginCallbackFunction;
}

svtkImageExport::DirectionCallbackType svtkImageExport::GetDirectionCallback() const
{
  return &svtkImageExport::DirectionCallbackFunction;
}

svtkImageExport::ScalarTypeCallbackType svtkImageExport::GetScalarTypeCallback() const
{
  return &svtkImageExport::ScalarTypeCallbackFunction;
}

svtkImageExport::NumberOfComponentsCallbackType svtkImageExport::GetNumberOfComponentsCallback() const
{
  return &svtkImageExport::NumberOfComponentsCallbackFunction;
}

svtkImageExport::PropagateUpdateExtentCallbackType svtkImageExport::GetPropagateUpdateExtentCallback()
  const
{
  return &svtkImageExport::PropagateUpdateExtentCallbackFunction;
}

svtkImageExport::UpdateDataCallbackType svtkImageExport::GetUpdateDataCallback() const
{
  return &svtkImageExport::UpdateDataCallbackFunction;
}

svtkImageExport::DataExtentCallbackType svtkImageExport::GetDataExtentCallback() const
{
  return &svtkImageExport::DataExtentCallbackFunction;
}

svtkImageExport::BufferPointerCallbackType svtkImageExport::GetBufferPointerCallback() const
{
  return &svtkImageExport::BufferPointerCallbackFunction;
}

//----------------------------------------------------------------------------
void svtkImageExport::UpdateInformationCallbackFunction(void* userData)
{
  static_cast<svtkImageExport*>(userData)->UpdateInformationCallback();
}

int svtkImageExport::PipelineModifiedCallbackFunction(void* userData)
{
  return static_cast<svtkImageExport*>(userData)->PipelineModifiedCallback();
}

int* svtkImageExport::WholeExtentCallbackFunction(void* userData)
{
  return static_cast<svtkImageExport*>(userData)->WholeExtentCallback();
}

double* svtkImageExport::SpacingCallbackFunction(void* userData)
{
  return static_cast<svtkImageExport*>(userData)->SpacingCallback();
}

double* svtkImageExport::OriginCallbackFunction(void* userData)
{
  return static_cast<svtkImageExport*>(userData)->OriginCallback();
}

double* svtkImageExport::DirectionCallbackFunction(void* userData)
{
  return static_cast<svtkImageExport*>(userData)->DirectionCallback();
}

const char* svtkImageExport::ScalarTypeCallbackFunction(void* userData)
{
  return static_cast<svtkImageExport*>(userData)->ScalarTypeCallback();
}

int svtkImageExport::NumberOfComponentsCallbackFunction(void* userData)
{
  return static_cast<svtkImageExport*>(userData)->NumberOfComponentsCallback();
}

void svtkImageExport::PropagateUpdateExtentCallbackFunction(void* userData, int* extent)
{
  static_cast<svtkImageExport*>(userData)->PropagateUpdateExtentCallback(extent);
}

void svtkImageExport::UpdateDataCallbackFunction(void* userData)
{
  static_cast<svtkImageExport*>(userData)->UpdateDataCallback();
}

int* svtkImageExport::DataExtentCallbackFunction(void* userData)
{
  return static_cast<svtkImageExport*>(userData)->DataExtentCallback();
}

void* svtkImageExport::BufferPointerCallbackFunction(void* userData)
{
  return static_cast<svtkImageExport*>(userData)->BufferPointerCallback();
}

//----------------------------------------------------------------------------
void svtkImageExport::UpdateInformationCallback()
{
  if (this->GetInputAlgorithm())
  {
    this->GetInputAlgorithm()->UpdateInformation();
  }
}

int svtkImageExport::PipelineModifiedCallback()
{
  if (!this->GetInput())
  {
    return 0;
  }

  svtkMTimeType mtime = 0;
  if (this->GetInputAlgorithm())
  {
    svtkExecutive* e = this->GetInputAlgorithm()->GetExecutive();
    if (e)
    {
      e->ComputePipelineMTime(
        nullptr, e->GetInputInformation(), e->GetOutputInformation(), -1, &mtime);
    }
  }

  if (mtime > this->LastPipelineMTime)
  {
    this->LastPipelineMTime = mtime;
    return 1;
  }
  return 0;
}

int* svtkImageExport::WholeExtentCallback()
{
  static int defaultextent[6] = { 0 };
  if (!this->GetInputAlgorithm())
  {
    return defaultextent;
  }
  else
  {
    return this->GetInputInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  }
}

double* svtkImageExport::SpacingCallback()
{
  static double defaultspacing[3] = { 0.0 };
  if (this->GetInputAlgorithm())
  {
    return this->GetDataSpacing();
  }
  else if (!this->GetInput())
  {
    return defaultspacing;
  }
  else
  {
    return this->GetInput()->GetSpacing();
  }
}

double* svtkImageExport::OriginCallback()
{
  static double defaultorigin[3] = { 0.0 };
  if (this->GetInputAlgorithm())
  {
    return this->GetDataOrigin();
  }
  else if (!this->GetInput())
  {
    return defaultorigin;
  }
  else
  {
    return this->GetInput()->GetOrigin();
  }
}

double* svtkImageExport::DirectionCallback()
{
  static double defaultdirection[3] = { 0.0 };
  if (this->GetInputAlgorithm())
  {
    return this->GetDataDirection();
  }
  else if (!this->GetInput())
  {
    return defaultdirection;
  }
  else
  {
    return this->GetInput()->GetDirectionMatrix()->GetData();
  }
}

const char* svtkImageExport::ScalarTypeCallback()
{
  if (!this->GetInput())
  {
    return "unsigned char";
  }

  int scalarType;
  if (this->GetInputAlgorithm())
  {
    scalarType = this->GetDataScalarType();
  }
  else
  {
    scalarType = this->GetInput()->GetScalarType();
  }

  switch (scalarType)
  {
    case SVTK_DOUBLE:
    {
      return "double";
    }
    case SVTK_FLOAT:
    {
      return "float";
    }
    case SVTK_LONG:
    {
      return "long";
    }
    case SVTK_UNSIGNED_LONG:
    {
      return "unsigned long";
    }
    case SVTK_INT:
    {
      return "int";
    }
    case SVTK_UNSIGNED_INT:
    {
      return "unsigned int";
    }
    case SVTK_SHORT:
    {
      return "short";
    }
    case SVTK_UNSIGNED_SHORT:
    {
      return "unsigned short";
    }
    case SVTK_CHAR:
    {
      return "char";
    }
    case SVTK_UNSIGNED_CHAR:
    {
      return "unsigned char";
    }
    case SVTK_SIGNED_CHAR:
    {
      return "signed char";
    }
    default:
    {
      return "<unsupported>";
    }
  }
}

int svtkImageExport::NumberOfComponentsCallback()
{
  if (!this->GetInput())
  {
    return 1;
  }
  else if (this->GetInputAlgorithm())
  {
    return this->GetDataNumberOfScalarComponents();
  }
  else
  {
    return this->GetInput()->GetNumberOfScalarComponents();
  }
}

void svtkImageExport::PropagateUpdateExtentCallback(int* extent)
{
  if (this->GetInputAlgorithm())
  {
    int port = this->GetInputConnection(0, 0)->GetIndex();
    svtkInformation* info = this->GetInputAlgorithm()->GetOutputInformation(port);
    info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
  }
}

void svtkImageExport::UpdateDataCallback()
{
  if (this->GetInputAlgorithm())
  {
    this->GetInputAlgorithm()->Update();
  }
}

int* svtkImageExport::DataExtentCallback()
{
  static int defaultextent[6] = { 0, 0, 0, 0, 0, 0 };
  if (this->GetInputAlgorithm())
  {
    return this->GetDataExtent();
  }
  if (!this->GetInput())
  {
    return defaultextent;
  }
  else
  {
    return this->GetInput()->GetExtent();
  }
}

void* svtkImageExport::BufferPointerCallback()
{
  if (!this->GetInput())
  {
    return static_cast<void*>(nullptr);
  }
  else
  {
    return this->GetInput()->GetScalarPointer();
  }
}

int svtkImageExport::GetDataNumberOfScalarComponents()
{
  if (this->GetInputAlgorithm() == nullptr)
  {
    return 1;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  return svtkImageData::GetNumberOfScalarComponents(this->GetExecutive()->GetInputInformation(0, 0));
}

int svtkImageExport::GetDataScalarType()
{
  if (this->GetInputAlgorithm() == nullptr)
  {
    return SVTK_UNSIGNED_CHAR;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  return svtkImageData::GetScalarType(this->GetExecutive()->GetInputInformation(0, 0));
}

int* svtkImageExport::GetDataExtent()
{
  static int defaultextent[6] = { 0, 0, 0, 0, 0, 0 };
  if (this->GetInputAlgorithm() == nullptr)
  {
    return defaultextent;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  return this->GetInputInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
}

void svtkImageExport::GetDataExtent(int* ptr)
{
  if (this->GetInputAlgorithm() == nullptr)
  {
    ptr[0] = ptr[1] = ptr[2] = ptr[3] = ptr[4] = ptr[5] = 0;
    return;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  this->GetInputInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ptr);
}

double* svtkImageExport::GetDataSpacing()
{
  static double defaultspacing[3] = { 1, 1, 1 };
  if (this->GetInput() == nullptr)
  {
    return defaultspacing;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  return this->GetInputInformation()->Get(svtkDataObject::SPACING());
}

void svtkImageExport::GetDataSpacing(double* ptr)
{
  if (this->GetInputAlgorithm() == nullptr)
  {
    ptr[0] = ptr[1] = ptr[2] = 0.0;
    return;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  this->GetInputInformation()->Get(svtkDataObject::SPACING(), ptr);
}

double* svtkImageExport::GetDataOrigin()
{
  static double defaultorigin[3] = { 0, 0, 0 };
  if (this->GetInputAlgorithm() == nullptr)
  {
    return defaultorigin;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  return this->GetInputInformation()->Get(svtkDataObject::ORIGIN());
}

void svtkImageExport::GetDataOrigin(double* ptr)
{
  if (this->GetInputAlgorithm() == nullptr)
  {
    ptr[0] = ptr[1] = ptr[2] = 0.0;
    return;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  this->GetInputInformation()->Get(svtkDataObject::ORIGIN(), ptr);
}

double* svtkImageExport::GetDataDirection()
{
  static double defaultdirection[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
  if (this->GetInputAlgorithm() == nullptr)
  {
    return defaultdirection;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  return this->GetInputInformation()->Get(svtkDataObject::DIRECTION());
}

void svtkImageExport::GetDataDirection(double* ptr)
{
  if (this->GetInputAlgorithm() == nullptr)
  {
    ptr[0] = ptr[1] = ptr[2] = ptr[3] = ptr[4] = ptr[5] = ptr[6] = ptr[7] = ptr[8] = 0.0;
    return;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  this->GetInputInformation()->Get(svtkDataObject::DIRECTION(), ptr);
}
