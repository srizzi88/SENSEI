/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPStructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPStructuredGridReader.h"

#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLStructuredGridReader.h"

svtkStandardNewMacro(svtkXMLPStructuredGridReader);

//----------------------------------------------------------------------------
svtkXMLPStructuredGridReader::svtkXMLPStructuredGridReader() = default;

//----------------------------------------------------------------------------
svtkXMLPStructuredGridReader::~svtkXMLPStructuredGridReader() = default;

//----------------------------------------------------------------------------
void svtkXMLPStructuredGridReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkXMLPStructuredGridReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkXMLPStructuredGridReader::GetOutput(int idx)
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkXMLPStructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkXMLPStructuredGridReader::GetPieceInput(int index)
{
  svtkXMLStructuredGridReader* reader =
    static_cast<svtkXMLStructuredGridReader*>(this->PieceReaders[index]);
  return reader->GetOutput();
}

//----------------------------------------------------------------------------
const char* svtkXMLPStructuredGridReader::GetDataSetName()
{
  return "PStructuredGrid";
}

//----------------------------------------------------------------------------
void svtkXMLPStructuredGridReader::SetOutputExtent(int* extent)
{
  svtkStructuredGrid::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
void svtkXMLPStructuredGridReader::GetPieceInputExtent(int index, int* extent)
{
  this->GetPieceInput(index)->GetExtent(extent);
}

//----------------------------------------------------------------------------
int svtkXMLPStructuredGridReader::ReadPrimaryElement(svtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Find the PPoints element.
  this->PPointsElement = nullptr;
  int numNested = ePrimary->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
  {
    svtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "PPoints") == 0) && (eNested->GetNumberOfNestedElements() == 1))
    {
      this->PPointsElement = eNested;
    }
  }

  if (!this->PPointsElement)
  {
    int extent[6];
    this->GetCurrentOutputInformation()->Get(
      svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
    if ((extent[0] <= extent[1]) && (extent[2] <= extent[3]) && (extent[4] <= extent[5]))
    {
      svtkErrorMacro("Could not find PPoints element with 1 array.");
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLPStructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  // Create the points array.
  svtkPoints* points = svtkPoints::New();
  if (this->PPointsElement)
  {
    // Non-zero volume.
    svtkAbstractArray* aa = this->CreateArray(this->PPointsElement->GetNestedElement(0));
    svtkDataArray* a = svtkArrayDownCast<svtkDataArray>(aa);
    if (a)
    {
      a->SetNumberOfTuples(this->GetNumberOfPoints());
      points->SetData(a);
      a->Delete();
    }
    else
    {
      if (aa)
      {
        aa->Delete();
      }
      this->DataError = 1;
    }
  }
  svtkStructuredGrid::SafeDownCast(this->GetCurrentOutput())->SetPoints(points);
  points->Delete();
}

//----------------------------------------------------------------------------
int svtkXMLPStructuredGridReader::ReadPieceData()
{
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  // Copy the points.
  svtkStructuredGrid* input = this->GetPieceInput(this->Piece);
  svtkStructuredGrid* output = svtkStructuredGrid::SafeDownCast(this->GetCurrentOutput());

  this->CopyArrayForPoints(input->GetPoints()->GetData(), output->GetPoints()->GetData());

  return 1;
}

//---------------------------------------------------------------------------
svtkXMLDataReader* svtkXMLPStructuredGridReader::CreatePieceReader()
{
  return svtkXMLStructuredGridReader::New();
}

//---------------------------------------------------------------------------
int svtkXMLPStructuredGridReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkStructuredGrid");
  return 1;
}
