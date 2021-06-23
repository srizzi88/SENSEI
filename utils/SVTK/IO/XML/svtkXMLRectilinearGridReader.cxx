/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLRectilinearGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLRectilinearGridReader.h"

#include "svtkDataArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLDataParser.h"

svtkStandardNewMacro(svtkXMLRectilinearGridReader);

//----------------------------------------------------------------------------
svtkXMLRectilinearGridReader::svtkXMLRectilinearGridReader()
{
  this->CoordinateElements = nullptr;
}

//----------------------------------------------------------------------------
svtkXMLRectilinearGridReader::~svtkXMLRectilinearGridReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkXMLRectilinearGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkXMLRectilinearGridReader::GetOutput(int idx)
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* svtkXMLRectilinearGridReader::GetDataSetName()
{
  return "RectilinearGrid";
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridReader::SetOutputExtent(int* extent)
{
  svtkRectilinearGrid::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->CoordinateElements = new svtkXMLDataElement*[numPieces];
  for (int i = 0; i < numPieces; ++i)
  {
    this->CoordinateElements[i] = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridReader::DestroyPieces()
{
  delete[] this->CoordinateElements;
  this->CoordinateElements = nullptr;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
int svtkXMLRectilinearGridReader::ReadPiece(svtkXMLDataElement* ePiece)
{
  if (!this->Superclass::ReadPiece(ePiece))
  {
    return 0;
  }

  // Find the Coordinates element in the piece.
  this->CoordinateElements[this->Piece] = nullptr;
  for (int i = 0; i < ePiece->GetNumberOfNestedElements(); ++i)
  {
    svtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "Coordinates") == 0) &&
      (eNested->GetNumberOfNestedElements() == 3))
    {
      this->CoordinateElements[this->Piece] = eNested;
    }
  }

  // If there is any volume, we require a Coordinates element.
  int* piecePointDimensions = this->PiecePointDimensions + this->Piece * 3;
  if (!this->CoordinateElements[this->Piece] && (piecePointDimensions[0] > 0) &&
    (piecePointDimensions[1] > 0) && (piecePointDimensions[2] > 0))
  {
    svtkErrorMacro("A piece is missing its Coordinates element.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  if (!this->CoordinateElements)
  {
    // Empty volume.
    return;
  }

  // Allocate the coordinate arrays.
  svtkRectilinearGrid* output = svtkRectilinearGrid::SafeDownCast(this->GetCurrentOutput());

  svtkXMLDataElement* xc = this->CoordinateElements[0]->GetNestedElement(0);
  svtkXMLDataElement* yc = this->CoordinateElements[0]->GetNestedElement(1);
  svtkXMLDataElement* zc = this->CoordinateElements[0]->GetNestedElement(2);

  // Create the coordinate arrays.
  svtkAbstractArray* xa = this->CreateArray(xc);
  svtkAbstractArray* ya = this->CreateArray(yc);
  svtkAbstractArray* za = this->CreateArray(zc);

  svtkDataArray* x = svtkArrayDownCast<svtkDataArray>(xa);
  svtkDataArray* y = svtkArrayDownCast<svtkDataArray>(ya);
  svtkDataArray* z = svtkArrayDownCast<svtkDataArray>(za);
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
    if (xa)
    {
      xa->Delete();
    }
    if (ya)
    {
      ya->Delete();
    }
    if (za)
    {
      za->Delete();
    }
    this->DataError = 1;
  }
}

//----------------------------------------------------------------------------
int svtkXMLRectilinearGridReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data (we read point specifications here).
  int dims[3] = { 0, 0, 0 };
  this->ComputePointDimensions(this->SubExtent, dims);
  svtkIdType superclassPieceSize = (this->NumberOfPointArrays * dims[0] * dims[1] * dims[2] +
    this->NumberOfCellArrays * (dims[0] - 1) * (dims[1] - 1) * (dims[2] - 1));

  // Total amount of data in this piece comes from point/cell data
  // arrays and the point specifications themselves.
  svtkIdType totalPieceSize = superclassPieceSize + dims[0] + dims[1] + dims[2];
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[5] = { 0, static_cast<float>(superclassPieceSize) / totalPieceSize,
    (static_cast<float>(superclassPieceSize) + dims[0]) / totalPieceSize,
    (static_cast<float>(superclassPieceSize) + dims[1] + dims[2]) / totalPieceSize, 1 };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  int index = this->Piece;
  svtkXMLDataElement* xc = this->CoordinateElements[index]->GetNestedElement(0);
  svtkXMLDataElement* yc = this->CoordinateElements[index]->GetNestedElement(1);
  svtkXMLDataElement* zc = this->CoordinateElements[index]->GetNestedElement(2);
  int* pieceExtent = this->PieceExtents + index * 6;
  svtkRectilinearGrid* output = svtkRectilinearGrid::SafeDownCast(this->GetCurrentOutput());

  // Set the range of progress for the X coordinates array.
  this->SetProgressRange(progressRange, 1, fractions);
  this->ReadSubCoordinates(
    pieceExtent, this->UpdateExtent, this->SubExtent, xc, output->GetXCoordinates());

  // Set the range of progress for the Y coordinates array.
  this->SetProgressRange(progressRange, 2, fractions);
  this->ReadSubCoordinates(
    pieceExtent + 2, this->UpdateExtent + 2, this->SubExtent + 2, yc, output->GetYCoordinates());

  // Set the range of progress for the Z coordinates array.
  this->SetProgressRange(progressRange, 3, fractions);
  this->ReadSubCoordinates(
    pieceExtent + 4, this->UpdateExtent + 4, this->SubExtent + 4, zc, output->GetZCoordinates());

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLRectilinearGridReader::ReadSubCoordinates(
  int* inBounds, int* outBounds, int* subBounds, svtkXMLDataElement* da, svtkDataArray* array)
{
  unsigned int components = array->GetNumberOfComponents();

  int destStartIndex = subBounds[0] - outBounds[0];
  int sourceStartIndex = subBounds[0] - inBounds[0];
  int length = subBounds[1] - subBounds[0] + 1;

  return this->ReadArrayValues(da, destStartIndex * components, array, sourceStartIndex, length);
}

//----------------------------------------------------------------------------
int svtkXMLRectilinearGridReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkRectilinearGrid");
  return 1;
}
