/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataStreamer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageDataStreamer.h"

#include "svtkCommand.h"
#include "svtkExtentTranslator.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageDataStreamer);
svtkCxxSetObjectMacro(svtkImageDataStreamer, ExtentTranslator, svtkExtentTranslator);

//----------------------------------------------------------------------------
svtkImageDataStreamer::svtkImageDataStreamer()
{
  // default to 10 divisions
  this->NumberOfStreamDivisions = 10;
  this->CurrentDivision = 0;

  // create default translator
  this->ExtentTranslator = svtkExtentTranslator::New();

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

svtkImageDataStreamer::~svtkImageDataStreamer()
{
  if (this->ExtentTranslator)
  {
    this->ExtentTranslator->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkImageDataStreamer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfStreamDivisions: " << this->NumberOfStreamDivisions << endl;
  if (this->ExtentTranslator)
  {
    os << indent << "ExtentTranslator:\n";
    this->ExtentTranslator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "ExtentTranslator: (none)\n";
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkImageDataStreamer::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    // we must set the extent on the input
    svtkInformation* outInfo = outputVector->GetInformationObject(0);

    // get the requested update extent
    int outExt[6];
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);

    // setup the inputs update extent
    int inExt[6] = { 0, -1, 0, -1, 0, -1 };
    svtkExtentTranslator* translator = this->GetExtentTranslator();

    translator->SetWholeExtent(outExt);
    translator->SetNumberOfPieces(this->NumberOfStreamDivisions);
    translator->SetPiece(this->CurrentDivision);
    if (translator->PieceToExtentByPoints())
    {
      translator->GetExtent(inExt);
    }

    inputVector[0]->GetInformationObject(0)->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

    return 1;
  }

  // generate the data
  else if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    // get the output data object
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

    // is this the first request
    if (!this->CurrentDivision)
    {
      // Tell the pipeline to start looping.
      request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      this->AllocateOutputData(output, outInfo);
    }

    // actually copy the data
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

    int inExt[6];
    inInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt);

    output->CopyAndCastFrom(input, inExt);

    // update the progress
    this->UpdateProgress(static_cast<float>(this->CurrentDivision + 1.0) /
      static_cast<float>(this->NumberOfStreamDivisions));

    this->CurrentDivision++;
    if (this->CurrentDivision == this->NumberOfStreamDivisions)
    {
      // Tell the pipeline to stop looping.
      request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentDivision = 0;
    }

    return 1;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
