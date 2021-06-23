/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLStructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLStructuredGridReader.h"

#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLDataParser.h"

svtkStandardNewMacro(svtkXMLStructuredGridReader);

//----------------------------------------------------------------------------
svtkXMLStructuredGridReader::svtkXMLStructuredGridReader()
{
  this->PointElements = nullptr;
}

//----------------------------------------------------------------------------
svtkXMLStructuredGridReader::~svtkXMLStructuredGridReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkXMLStructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkXMLStructuredGridReader::GetOutput(int idx)
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* svtkXMLStructuredGridReader::GetDataSetName()
{
  return "StructuredGrid";
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridReader::SetOutputExtent(int* extent)
{
  svtkStructuredGrid::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->PointElements = new svtkXMLDataElement*[numPieces];
  int i;
  for (i = 0; i < numPieces; ++i)
  {
    this->PointElements[i] = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridReader::DestroyPieces()
{
  delete[] this->PointElements;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
int svtkXMLStructuredGridReader::ReadPiece(svtkXMLDataElement* ePiece)
{
  if (!this->Superclass::ReadPiece(ePiece))
  {
    return 0;
  }

  // Find the Points element in the piece.
  int i;
  this->PointElements[this->Piece] = nullptr;
  for (i = 0; i < ePiece->GetNumberOfNestedElements(); ++i)
  {
    svtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "Points") == 0) && (eNested->GetNumberOfNestedElements() == 1))
    {
      this->PointElements[this->Piece] = eNested;
    }
  }

  // If there is any volume, we require a Points element.
  int* piecePointDimensions = this->PiecePointDimensions + this->Piece * 3;
  if (!this->PointElements[this->Piece] && (piecePointDimensions[0] > 0) &&
    (piecePointDimensions[1] > 0) && (piecePointDimensions[2] > 0))
  {
    svtkErrorMacro("A piece is missing its Points element "
                  "or element does not have exactly 1 array.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  // Create the points array.
  svtkPoints* points = svtkPoints::New();

  // Use the configuration of the first piece since all are the same.
  svtkXMLDataElement* ePoints = this->PointElements[0];
  if (ePoints)
  {
    // Non-zero volume.
    svtkAbstractArray* aa = this->CreateArray(ePoints->GetNestedElement(0));
    svtkDataArray* a = svtkArrayDownCast<svtkDataArray>(aa);
    if (a)
    {
      // Allocate the points array.
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
int svtkXMLStructuredGridReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data (we read point specifications here).
  int dims[3] = { 0, 0, 0 };
  this->ComputePointDimensions(this->SubExtent, dims);
  svtkIdType superclassPieceSize = (this->NumberOfPointArrays * dims[0] * dims[1] * dims[2] +
    this->NumberOfCellArrays * (dims[0] - 1) * (dims[1] - 1) * (dims[2] - 1));

  // Total amount of data in this piece comes from point/cell data
  // arrays and the point specifications themselves.
  svtkIdType totalPieceSize = superclassPieceSize + dims[0] * dims[1] * dims[2];
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[3] = { 0, float(superclassPieceSize) / totalPieceSize, 1 };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  if (!this->PointElements[this->Piece])
  {
    // Empty volume.
    return 1;
  }

  // Set the range of progress for the points array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the points array.
  svtkStructuredGrid* output = svtkStructuredGrid::SafeDownCast(this->GetCurrentOutput());
  svtkXMLDataElement* ePoints = this->PointElements[this->Piece];
  return this->ReadArrayForPoints(ePoints->GetNestedElement(0), output->GetPoints()->GetData());
}

int svtkXMLStructuredGridReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkStructuredGrid");

  return 1;
}
