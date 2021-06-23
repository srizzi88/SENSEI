/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageAlgorithm.h"

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkErrorCode.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
svtkImageAlgorithm::svtkImageAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
svtkImageAlgorithm::~svtkImageAlgorithm() = default;

//----------------------------------------------------------------------------
void svtkImageAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int svtkImageAlgorithm::RequestData(svtkInformation* request,
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // the default implementation is to do what the old pipeline did find what
  // output is requesting the data, and pass that into ExecuteData

  // which output port did the request come from
  int outputPort = request->Get(svtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

  // if output port is negative then that means this filter is calling the
  // update directly, in that case just assume port 0
  if (outputPort == -1)
  {
    outputPort = 0;
  }

  // get the data object
  svtkInformation* outInfo = outputVector->GetInformationObject(outputPort);
  // call ExecuteData
  this->SetErrorCode(svtkErrorCode::NoError);
  if (outInfo)
  {
    this->ExecuteDataWithInformation(outInfo->Get(svtkDataObject::DATA_OBJECT()), outInfo);
  }
  else
  {
    this->ExecuteData(nullptr);
  }
  // Check for any error set by downstream filter (IO in most case)
  if (this->GetErrorCode())
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkImageAlgorithm::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  // propagate update extent
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

void svtkImageAlgorithm::ExecuteDataWithInformation(svtkDataObject* dobj, svtkInformation*)
{
  this->ExecuteData(dobj);
}

//----------------------------------------------------------------------------
// Assume that any source that implements ExecuteData
// can handle an empty extent.
void svtkImageAlgorithm::ExecuteData(svtkDataObject*)
{
  this->Execute();
}

//----------------------------------------------------------------------------
void svtkImageAlgorithm::Execute()
{
  svtkErrorMacro(<< "Definition of Execute() method should be in subclass and you should really use "
                   "the ExecuteData(svtkInformation *request,...) signature instead");
}

//----------------------------------------------------------------------------
void svtkImageAlgorithm::CopyInputArrayAttributesToOutput(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // for image data to image data
  if (this->GetNumberOfInputPorts() && this->GetNumberOfOutputPorts())
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    // if the input is image data
    if (svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT())))
    {
      svtkInformation* info = this->GetInputArrayFieldInformation(0, inputVector);
      if (info)
      {
        int scalarType = info->Get(svtkDataObject::FIELD_ARRAY_TYPE());
        int numComp = info->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
        for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
        {
          svtkInformation* outInfo = outputVector->GetInformationObject(i);
          // if the output is image data
          if (svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT())))
          {
            // copy scalar type and scalar number of components
            svtkDataObject::SetPointDataActiveScalarInfo(outInfo, scalarType, numComp);
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
int svtkImageAlgorithm::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // do nothing except copy scalar type info
  this->CopyInputArrayAttributesToOutput(request, inputVector, outputVector);
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageAlgorithm::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageAlgorithm::AllocateOutputData(
  svtkImageData* output, svtkInformation* outInfo, int* uExtent)
{
  // set the extent to be the update extent
  output->SetExtent(uExtent);
  int scalarType = svtkImageData::GetScalarType(outInfo);
  int numComponents = svtkImageData::GetNumberOfScalarComponents(outInfo);
  output->AllocateScalars(scalarType, numComponents);
}

//----------------------------------------------------------------------------
svtkImageData* svtkImageAlgorithm::AllocateOutputData(svtkDataObject* output, svtkInformation* outInfo)
{
  // set the extent to be the update extent
  svtkImageData* out = svtkImageData::SafeDownCast(output);
  if (out)
  {
    int* uExtent = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    this->AllocateOutputData(out, outInfo, uExtent);
  }
  return out;
}

//----------------------------------------------------------------------------
// by default copy the attr from the first input to the first output
void svtkImageAlgorithm::CopyAttributeData(
  svtkImageData* input, svtkImageData* output, svtkInformationVector** inputVector)
{
  if (!input || !output)
  {
    return;
  }

  int inExt[6];
  int outExt[6];
  svtkDataArray* inArray;
  svtkDataArray* outArray;

  input->GetExtent(inExt);
  output->GetExtent(outExt);

  // Do not copy the array we will be generating.
  inArray = this->GetInputArrayToProcess(0, inputVector);

  // Conditionally copy point and cell data.  Only copy if corresponding
  // indexes refer to identical points.
  double* oIn = input->GetOrigin();
  double* sIn = input->GetSpacing();
  double* oOut = output->GetOrigin();
  double* sOut = output->GetSpacing();
  if (oIn[0] == oOut[0] && oIn[1] == oOut[1] && oIn[2] == oOut[2] && sIn[0] == sOut[0] &&
    sIn[1] == sOut[1] && sIn[2] == sOut[2])
  {
    output->GetPointData()->CopyAllOn();
    output->GetCellData()->CopyAllOn();
    if (inArray && inArray->GetName())
    {
      output->GetPointData()->CopyFieldOff(inArray->GetName());
    }
    else if (inArray == input->GetPointData()->GetScalars())
    {
      output->GetPointData()->CopyScalarsOff();
    }

    // If the extents are the same, then pass the attribute data for
    // efficiency.
    if (inExt[0] == outExt[0] && inExt[1] == outExt[1] && inExt[2] == outExt[2] &&
      inExt[3] == outExt[3] && inExt[4] == outExt[4] && inExt[5] == outExt[5])
    { // Pass
      // set the name of the output to match the input name
      outArray = output->GetPointData()->GetScalars();
      if (inArray)
      {
        outArray->SetName(inArray->GetName());
      }
      // Cache the scalars otherwise it may get overwritten
      // during CopyAttributes()
      outArray->Register(this);
      output->GetPointData()->SetScalars(nullptr);
      output->CopyAttributes(input);
      // Restore the scalars
      int idx = output->GetPointData()->AddArray(outArray);
      output->GetPointData()->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
      outArray->UnRegister(this);
    }
    else
    { // Copy
      // Since this can be expensive to copy all of these values,
      // lets make sure there are arrays to copy (other than the scalars)
      if (input->GetPointData()->GetNumberOfArrays() > 1)
      {
        // Copy the point data.
        // CopyAllocate frees all arrays.
        // Cache the scalars otherwise it may get overwritten
        // during CopyAllocate()
        svtkDataArray* tmp = output->GetPointData()->GetScalars();
        // set the name of the output to match the input name
        if (inArray)
        {
          tmp->SetName(inArray->GetName());
        }
        tmp->Register(this);
        output->GetPointData()->SetScalars(nullptr);
        output->GetPointData()->CopyAllocate(input->GetPointData(), output->GetNumberOfPoints());
        // Restore the scalars
        int idx = output->GetPointData()->AddArray(tmp);
        output->GetPointData()->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
        tmp->UnRegister(this);
        // Now Copy The point data, but only if output is a subextent of the
        // input.
        if (outExt[0] >= inExt[0] && outExt[1] <= inExt[1] && outExt[2] >= inExt[2] &&
          outExt[3] <= inExt[3] && outExt[4] >= inExt[4] && outExt[5] <= inExt[5])
        {
          output->GetPointData()->CopyStructuredData(input->GetPointData(), inExt, outExt);
        }
      }
      else
      {
        if (inArray)
        {
          svtkDataArray* tmp = output->GetPointData()->GetScalars();
          tmp->SetName(inArray->GetName());
        }
      }

      if (input->GetCellData()->GetNumberOfArrays() > 0)
      {
        output->GetCellData()->CopyAllocate(input->GetCellData(), output->GetNumberOfCells());
        // Cell extent is one less than point extent.
        // Conditional to handle a colapsed axis (lower dimensional cells).
        if (inExt[0] < inExt[1])
        {
          --inExt[1];
        }
        if (inExt[2] < inExt[3])
        {
          --inExt[3];
        }
        if (inExt[4] < inExt[5])
        {
          --inExt[5];
        }
        // Cell extent is one less than point extent.
        if (outExt[0] < outExt[1])
        {
          --outExt[1];
        }
        if (outExt[2] < outExt[3])
        {
          --outExt[3];
        }
        if (outExt[4] < outExt[5])
        {
          --outExt[5];
        }
        // Now Copy The cell data, but only if output is a subextent of the input.
        if (outExt[0] >= inExt[0] && outExt[1] <= inExt[1] && outExt[2] >= inExt[2] &&
          outExt[3] <= inExt[3] && outExt[4] >= inExt[4] && outExt[5] <= inExt[5])
        {
          output->GetCellData()->CopyStructuredData(input->GetCellData(), inExt, outExt);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
svtkImageData* svtkImageAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkImageData* svtkImageAlgorithm::GetOutput(int port)
{
  return svtkImageData::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void svtkImageAlgorithm::SetOutput(svtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
int svtkImageAlgorithm::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageAlgorithm::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkImageAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkImageAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkImageData* svtkImageAlgorithm::GetImageDataInput(int port)
{
  return svtkImageData::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
void svtkImageAlgorithm::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkImageAlgorithm::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}
