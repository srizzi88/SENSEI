/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractRectilinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractRectilinearGrid.h"

#include "svtkCellData.h"
#include "svtkExtractStructuredGridHelper.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredData.h"

svtkStandardNewMacro(svtkExtractRectilinearGrid);

// Construct object to extract all of the input data.
svtkExtractRectilinearGrid::svtkExtractRectilinearGrid()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = SVTK_INT_MAX;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;

  this->IncludeBoundary = 0;
  this->Internal = svtkExtractStructuredGridHelper::New();
}

//----------------------------------------------------------------------------
svtkExtractRectilinearGrid::~svtkExtractRectilinearGrid()
{
  if (this->Internal != nullptr)
  {
    this->Internal->Delete();
  }
}

//----------------------------------------------------------------------------
int svtkExtractRectilinearGrid::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!this->Internal->IsValid())
  {
    return 0;
  }

  int i;
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  bool emptyExtent = false;
  int uExt[6];
  for (i = 0; i < 3; i++)
  {
    if (this->Internal->GetSize(i) < 1)
    {
      uExt[0] = uExt[2] = uExt[4] = 0;
      uExt[1] = uExt[3] = uExt[5] = -1;
      emptyExtent = true;
      break;
    }
  }

  if (!emptyExtent)
  {
    // Find input update extent based on requested output
    // extent
    int oUExt[6];
    outputVector->GetInformationObject(0)->Get(
      svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), oUExt);
    int oWExt[6]; // For parallel parititon this will be different.
    this->Internal->GetOutputWholeExtent(oWExt);
    for (i = 0; i < 3; i++)
    {
      int idx = oUExt[2 * i] - oWExt[2 * i]; // Extent value to index
      if (idx < 0 || idx >= (int)this->Internal->GetSize(i))
      {
        svtkWarningMacro("Requested extent outside whole extent.");
        idx = 0;
      }
      uExt[2 * i] = this->Internal->GetMappedExtentValueFromIndex(i, idx);
      int jdx = oUExt[2 * i + 1] - oWExt[2 * i]; // Extent value to index
      if (jdx < idx || jdx >= (int)this->Internal->GetSize(i))
      {
        svtkWarningMacro("Requested extent outside whole extent.");
        jdx = 0;
      }
      uExt[2 * i + 1] = this->Internal->GetMappedExtentValueFromIndex(i, jdx);
    }
  }
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt, 6);
  // We can handle anything.
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);

  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractRectilinearGrid::RequestInformation(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int wholeExtent[6], outWholeExt[6];

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  this->Internal->Initialize(
    this->VOI, wholeExtent, this->SampleRate, (this->IncludeBoundary == 1));
  this->Internal->GetOutputWholeExtent(outWholeExt);

  if (!this->Internal->IsValid())
  {
    svtkWarningMacro("Error while initializing filter.");
    return 0;
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outWholeExt, 6);
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractRectilinearGrid::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Reset internal helper to the actual extents of the piece we're working on:
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkRectilinearGrid* inGrid = svtkRectilinearGrid::GetData(inInfo);
  this->Internal->Initialize(
    this->VOI, inGrid->GetExtent(), this->SampleRate, (this->IncludeBoundary != 0));

  if (!this->Internal->IsValid())
  {
    return 0;
  }

  // Set the output extent -- this is how RequestDataImpl knows what to copy.
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkRectilinearGrid* output =
    svtkRectilinearGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  output->SetExtent(this->Internal->GetOutputWholeExtent());

  return this->RequestDataImpl(inputVector, outputVector) ? 1 : 0;
}

//----------------------------------------------------------------------------
bool svtkExtractRectilinearGrid::RequestDataImpl(
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if ((this->SampleRate[0] < 1) || (this->SampleRate[1] < 1) || (this->SampleRate[2] < 1))
  {
    svtkErrorMacro("SampleRate must be >= 1 in all 3 dimensions!");
    return false;
  }

  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkRectilinearGrid* input =
    svtkRectilinearGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkRectilinearGrid* output =
    svtkRectilinearGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfPoints() == 0)
  {
    return true;
  }

  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* outCD = output->GetCellData();

  int* inExt = input->GetExtent();
  int* outExt = output->GetExtent();

  int outDims[3];
  svtkStructuredData::GetDimensionsFromExtent(outExt, outDims);

  svtkDebugMacro(<< "Extracting Grid");
  this->Internal->CopyPointsAndPointData(inExt, outExt, pd, nullptr, outPD, nullptr);
  this->Internal->CopyCellData(inExt, outExt, cd, outCD);

  // copy coordinates
  svtkDataArray* in_coords[3];
  in_coords[0] = input->GetXCoordinates();
  in_coords[1] = input->GetYCoordinates();
  in_coords[2] = input->GetZCoordinates();

  svtkDataArray* out_coords[3];

  for (int dim = 0; dim < 3; ++dim)
  {
    // Allocate coordinates array for this dimension
    out_coords[dim] = svtkDataArray::CreateDataArray(in_coords[dim]->GetDataType());
    out_coords[dim]->SetNumberOfTuples(outDims[dim]);

    for (int oExtVal = outExt[2 * dim]; oExtVal <= outExt[2 * dim + 1]; ++oExtVal)
    {
      int outExtIdx = oExtVal - outExt[2 * dim];
      int inExtIdx = this->Internal->GetMappedIndex(dim, outExtIdx);
      out_coords[dim]->SetTuple(outExtIdx, inExtIdx, in_coords[dim]);
    } // END for all points along this dimension in the output
  }   // END for all dimensions

  output->SetXCoordinates(out_coords[0]);
  output->SetYCoordinates(out_coords[1]);
  output->SetZCoordinates(out_coords[2]);
  out_coords[0]->Delete();
  out_coords[1]->Delete();
  out_coords[2]->Delete();

  return true;
}

//----------------------------------------------------------------------------
void svtkExtractRectilinearGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "VOI: \n";
  os << indent << "  Imin,Imax: (" << this->VOI[0] << ", " << this->VOI[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" << this->VOI[2] << ", " << this->VOI[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" << this->VOI[4] << ", " << this->VOI[5] << ")\n";

  os << indent << "Sample Rate: (" << this->SampleRate[0] << ", " << this->SampleRate[1] << ", "
     << this->SampleRate[2] << ")\n";

  os << indent << "Include Boundary: " << (this->IncludeBoundary ? "On\n" : "Off\n");
}
