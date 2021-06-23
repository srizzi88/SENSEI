/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPRectilinearGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPRectilinearGridReader.h"

#include "svtkDataArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLRectilinearGridReader.h"

svtkStandardNewMacro(svtkXMLPRectilinearGridReader);

//----------------------------------------------------------------------------
svtkXMLPRectilinearGridReader::svtkXMLPRectilinearGridReader() = default;

//----------------------------------------------------------------------------
svtkXMLPRectilinearGridReader::~svtkXMLPRectilinearGridReader() = default;

//----------------------------------------------------------------------------
void svtkXMLPRectilinearGridReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkXMLPRectilinearGridReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkXMLPRectilinearGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkXMLPRectilinearGridReader::GetOutput(int idx)
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkXMLPRectilinearGridReader::GetPieceInput(int index)
{
  svtkXMLRectilinearGridReader* reader =
    static_cast<svtkXMLRectilinearGridReader*>(this->PieceReaders[index]);
  return reader->GetOutput();
}

//----------------------------------------------------------------------------
const char* svtkXMLPRectilinearGridReader::GetDataSetName()
{
  return "PRectilinearGrid";
}

//----------------------------------------------------------------------------
void svtkXMLPRectilinearGridReader::SetOutputExtent(int* extent)
{
  svtkRectilinearGrid::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
void svtkXMLPRectilinearGridReader::GetPieceInputExtent(int index, int* extent)
{
  this->GetPieceInput(index)->GetExtent(extent);
}

//----------------------------------------------------------------------------
int svtkXMLPRectilinearGridReader::ReadPrimaryElement(svtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Find the PCoordinates element.
  this->PCoordinatesElement = nullptr;
  int i;
  int numNested = ePrimary->GetNumberOfNestedElements();
  for (i = 0; i < numNested; ++i)
  {
    svtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "PCoordinates") == 0) &&
      (eNested->GetNumberOfNestedElements() == 3))
    {
      this->PCoordinatesElement = eNested;
    }
  }

  // If there is any volume, we require a PCoordinates element.
  if (!this->PCoordinatesElement)
  {
    int extent[6];
    svtkInformation* outInfo = this->GetCurrentOutputInformation();
    outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
    if ((extent[0] <= extent[1]) && (extent[2] <= extent[3]) && (extent[4] <= extent[5]))
    {
      svtkErrorMacro("Could not find PCoordinates element with 3 arrays.");
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLPRectilinearGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  if (!this->PCoordinatesElement)
  {
    // Empty volume.
    return;
  }

  // Allocate the coordinate arrays.
  svtkRectilinearGrid* output = svtkRectilinearGrid::SafeDownCast(this->GetCurrentOutput());

  svtkXMLDataElement* xc = this->PCoordinatesElement->GetNestedElement(0);
  svtkXMLDataElement* yc = this->PCoordinatesElement->GetNestedElement(1);
  svtkXMLDataElement* zc = this->PCoordinatesElement->GetNestedElement(2);

  // Create the coordinate arrays (all are data arrays).
  svtkAbstractArray* ax = this->CreateArray(xc);
  svtkAbstractArray* ay = this->CreateArray(yc);
  svtkAbstractArray* az = this->CreateArray(zc);

  svtkDataArray* x = svtkArrayDownCast<svtkDataArray>(ax);
  svtkDataArray* y = svtkArrayDownCast<svtkDataArray>(ay);
  svtkDataArray* z = svtkArrayDownCast<svtkDataArray>(az);
  if (x && y && z)
  {
    x->SetNumberOfTuples(this->PointDimensions[0]);
    y->SetNumberOfTuples(this->PointDimensions[1]);
    z->SetNumberOfTuples(this->PointDimensions[2]);
    output->SetXCoordinates(x);
    output->SetYCoordinates(y);
    output->SetZCoordinates(z);
    x->Delete();
    y->Delete();
    z->Delete();
  }
  else
  {
    if (ax)
    {
      ax->Delete();
    }
    if (ay)
    {
      ay->Delete();
    }
    if (az)
    {
      az->Delete();
    }
    this->DataError = 1;
  }
}

//----------------------------------------------------------------------------
int svtkXMLPRectilinearGridReader::ReadPieceData()
{
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  // Copy the coordinates arrays from the input piece.
  svtkRectilinearGrid* input = this->GetPieceInput(this->Piece);
  svtkRectilinearGrid* output = svtkRectilinearGrid::SafeDownCast(this->GetCurrentOutput());
  this->CopySubCoordinates(this->SubPieceExtent, this->UpdateExtent, this->SubExtent,
    input->GetXCoordinates(), output->GetXCoordinates());
  this->CopySubCoordinates(this->SubPieceExtent + 2, this->UpdateExtent + 2, this->SubExtent + 2,
    input->GetYCoordinates(), output->GetYCoordinates());
  this->CopySubCoordinates(this->SubPieceExtent + 4, this->UpdateExtent + 4, this->SubExtent + 4,
    input->GetZCoordinates(), output->GetZCoordinates());

  return 1;
}

//----------------------------------------------------------------------------
svtkXMLDataReader* svtkXMLPRectilinearGridReader::CreatePieceReader()
{
  return svtkXMLRectilinearGridReader::New();
}

//----------------------------------------------------------------------------
void svtkXMLPRectilinearGridReader::CopySubCoordinates(
  int* inBounds, int* outBounds, int* subBounds, svtkDataArray* inArray, svtkDataArray* outArray)
{
  unsigned int components = inArray->GetNumberOfComponents();
  unsigned int tupleSize = inArray->GetDataTypeSize() * components;

  int destStartIndex = subBounds[0] - outBounds[0];
  int sourceStartIndex = subBounds[0] - inBounds[0];
  int length = subBounds[1] - subBounds[0] + 1;

  memcpy(outArray->GetVoidPointer(destStartIndex * components),
    inArray->GetVoidPointer(sourceStartIndex * components), length * tupleSize);
}

int svtkXMLPRectilinearGridReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkRectilinearGrid");
  return 1;
}
