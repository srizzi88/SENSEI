/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTrivialProducer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTrivialProducer.h"

#include "svtkDataObject.h"
#include "svtkGarbageCollector.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkTrivialProducer);

// This compile-time switch determines whether the update extent is
// checked.  If so this algorithm will produce an error message when
// the update extent is smaller than the whole extent which will
// result in lost data.  There are real cases in which this is a valid
// thing so an error message should normally not be produced.  However
// there are hard-to-find bugs that can be revealed quickly if this
// option is enabled.  This should be enabled only for debugging
// purposes in a working checkout of SVTK.  Do not commit a change that
// turns on this switch!
#define SVTK_TRIVIAL_PRODUCER_CHECK_UPDATE_EXTENT 0

//----------------------------------------------------------------------------
svtkTrivialProducer::svtkTrivialProducer()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Output = nullptr;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;
}

//----------------------------------------------------------------------------
svtkTrivialProducer::~svtkTrivialProducer()
{
  this->SetOutput(nullptr);
}

//----------------------------------------------------------------------------
void svtkTrivialProducer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkTrivialProducer::SetOutput(svtkDataObject* newOutput)
{
  svtkDataObject* oldOutput = this->Output;
  if (newOutput != oldOutput)
  {
    if (newOutput)
    {
      newOutput->Register(this);
    }
    this->Output = newOutput;
    this->GetExecutive()->SetOutputData(0, newOutput);
    if (oldOutput)
    {
      oldOutput->UnRegister(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkTrivialProducer::GetMTime()
{
  svtkMTimeType mtime = this->Superclass::GetMTime();
  if (this->Output)
  {
    svtkMTimeType omtime = this->Output->GetMTime();
    if (omtime > mtime)
    {
      mtime = omtime;
    }
  }
  return mtime;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkTrivialProducer::CreateDefaultExecutive()
{
  return svtkStreamingDemandDrivenPipeline::New();
}

//----------------------------------------------------------------------------
int svtkTrivialProducer::FillInputPortInformation(int, svtkInformation*)
{
  return 1;
}

//----------------------------------------------------------------------------
int svtkTrivialProducer::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void svtkTrivialProducer::FillOutputDataInformation(svtkDataObject* output, svtkInformation* outInfo)
{
  svtkInformation* dataInfo = output->GetInformation();
  if (dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_3D_EXTENT)
  {
    int extent[6];
    dataInfo->Get(svtkDataObject::DATA_EXTENT(), extent);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  }

  // Let the data object copy information to the pipeline
  output->CopyInformationToPipeline(outInfo);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkTrivialProducer::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()) && this->Output)
  {
    svtkInformation* outputInfo = outputVector->GetInformationObject(0);

    svtkTrivialProducer::FillOutputDataInformation(this->Output, outputInfo);

    // Overwrite the whole extent if WholeExtent is set. This is needed
    // for distributed structured data.
    if (this->WholeExtent[0] <= this->WholeExtent[1] &&
      this->WholeExtent[2] <= this->WholeExtent[3] && this->WholeExtent[4] <= this->WholeExtent[5])
    {
      outputInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);
    }

    // We assume that whoever sets up the trivial producer handles
    // partitioned data properly. For structured data, this means setting
    // up WHOLE_EXTENT as above. For unstructured data, nothing special is
    // required
    outputInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }

#if SVTK_TRIVIAL_PRODUCER_CHECK_UPDATE_EXTENT
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    // If an exact extent smaller than the whole extent has been
    // requested then warn.
    svtkInformation* outputInfo = outputVector->GetInformationObject(0);
    if (outputInfo->Get(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT()))
    {
      svtkInformation* dataInfo = this->Output->GetInformation();
      if (dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_3D_EXTENT)
      {
        // Compare the update extent to the whole extent.
        int updateExtent[6] = { 0, -1, 0, -1, 0, -1 };
        int wholeExtent[6] = { 0, -1, 0, -1, 0, -1 };
        outputInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
        outputInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent);
        if (updateExtent[0] != wholeExtent[0] || updateExtent[1] != wholeExtent[1] ||
          updateExtent[2] != wholeExtent[2] || updateExtent[3] != wholeExtent[3] ||
          updateExtent[4] != wholeExtent[4] || updateExtent[5] != wholeExtent[5])
        {
          svtkErrorMacro("Request for exact extent "
            << updateExtent[0] << " " << updateExtent[1] << " " << updateExtent[2] << " "
            << updateExtent[3] << " " << updateExtent[4] << " " << updateExtent[5]
            << " will lose data because it is not the whole extent " << wholeExtent[0] << " "
            << wholeExtent[1] << " " << wholeExtent[2] << " " << wholeExtent[3] << " "
            << wholeExtent[4] << " " << wholeExtent[5] << ".");
        }
      }
    }
  }
#endif
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_NOT_GENERATED()))
  {
    // We do not really generate the output.  Do not let the executive
    // initialize it.
    svtkInformation* outputInfo = outputVector->GetInformationObject(0);
    outputInfo->Set(svtkDemandDrivenPipeline::DATA_NOT_GENERATED(), 1);
  }
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()) && this->Output)
  {
    svtkInformation* outputInfo = outputVector->GetInformationObject(0);

    // If downstream wants exact structured extent that is less than
    // whole, we need make a copy of the original dataset and crop it
    // - if EXACT_EXTENT() is specified.
    svtkInformation* dataInfo = this->Output->GetInformation();
    if (dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_3D_EXTENT)
    {
      int wholeExt[6];
      outputInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
      int updateExt[6];
      outputInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExt);

      if (outputInfo->Has(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT()) &&
        outputInfo->Get(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT()))
      {

        if (updateExt[0] != wholeExt[0] || updateExt[1] != wholeExt[1] ||
          updateExt[2] != wholeExt[2] || updateExt[3] != wholeExt[3] ||
          updateExt[4] != wholeExt[4] || updateExt[5] != wholeExt[5])
        {
          svtkDataObject* newOutput = this->Output->NewInstance();
          newOutput->ShallowCopy(this->Output);
          newOutput->Crop(outputInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
          outputInfo->Set(svtkDataObject::DATA_OBJECT(), newOutput);
          newOutput->Delete();
        }
        else
        {
          // If we didn't replace the output, it should be same as original
          // dataset. If not, fix it.
          svtkDataObject* output = outputInfo->Get(svtkDataObject::DATA_OBJECT());
          if (output != this->Output)
          {
            outputInfo->Set(svtkDataObject::DATA_OBJECT(), this->Output);
          }
        }
      }
      else
      {
        // If EXACT_EXTENT() is not there,
        // make sure that we provide requested extent or more
        svtkDataObject* output = outputInfo->Get(svtkDataObject::DATA_OBJECT());
        if (updateExt[0] < wholeExt[0] || updateExt[1] > wholeExt[1] ||
          updateExt[2] < wholeExt[2] || updateExt[3] > wholeExt[3] || updateExt[4] < wholeExt[4] ||
          updateExt[5] > wholeExt[5])
        {
          svtkErrorMacro("This data object does not contain the requested extent.");
        }
        // This means that we used a previously cropped output, replace it
        // with current
        else if (output != this->Output)
        {
          outputInfo->Set(svtkDataObject::DATA_OBJECT(), this->Output);
        }
      }
    }

    // Pretend we generated the output.
    outputInfo->Remove(svtkDemandDrivenPipeline::DATA_NOT_GENERATED());
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void svtkTrivialProducer::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->Output, "Output");
}
