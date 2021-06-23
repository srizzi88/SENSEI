/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLRectilinearGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLRectilinearGridWriter.h"

#include "svtkCellData.h"
#include "svtkErrorCode.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#define svtkXMLOffsetsManager_DoNotInclude
#include "svtkXMLOffsetsManager.h"
#undef svtkXMLOffsetsManager_DoNotInclude

svtkStandardNewMacro(svtkXMLRectilinearGridWriter);

//----------------------------------------------------------------------------
svtkXMLRectilinearGridWriter::svtkXMLRectilinearGridWriter()
{
  this->CoordinateOM = new OffsetsManagerArray;
}

//----------------------------------------------------------------------------
svtkXMLRectilinearGridWriter::~svtkXMLRectilinearGridWriter()
{
  delete this->CoordinateOM;
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkXMLRectilinearGridWriter::GetInput()
{
  return static_cast<svtkRectilinearGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridWriter::GetInputExtent(int* extent)
{
  this->GetInput()->GetExtent(extent);
}

//----------------------------------------------------------------------------
const char* svtkXMLRectilinearGridWriter::GetDataSetName()
{
  return "RectilinearGrid";
}

//----------------------------------------------------------------------------
const char* svtkXMLRectilinearGridWriter::GetDefaultFileExtension()
{
  return "vtr";
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridWriter::AllocatePositionArrays()
{
  this->Superclass::AllocatePositionArrays();

  this->CoordinateOM->Allocate(this->NumberOfPieces);
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridWriter::DeletePositionArrays()
{
  this->Superclass::DeletePositionArrays();
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridWriter::WriteAppendedPiece(int index, svtkIndent indent)
{
  this->Superclass::WriteAppendedPiece(index, indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->WriteCoordinatesAppended(this->GetInput()->GetXCoordinates(),
    this->GetInput()->GetYCoordinates(), this->GetInput()->GetZCoordinates(), indent,
    &this->CoordinateOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridWriter::WriteAppendedPieceData(int index)
{
  // Split progress range by the approximate fractions of data written
  // by each step in this method.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  float fractions[3];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteAppendedPieceData(index);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the coordinates arrays.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the coordinates arrays.
  this->WriteCoordinatesAppendedData(this->GetInput()->GetXCoordinates(),
    this->GetInput()->GetYCoordinates(), this->GetInput()->GetZCoordinates(),
    this->CurrentTimeIndex, &this->CoordinateOM->GetPiece(index));
  this->CoordinateOM->GetPiece(index).Allocate(0); // mark it invalid
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridWriter::WriteInlinePiece(svtkIndent indent)
{
  // Split progress range by the approximate fractions of data written
  // by each step in this method.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteInlinePiece(indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the coordinates arrays.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the coordinates arrays.
  this->WriteCoordinatesInline(this->GetInput()->GetXCoordinates(),
    this->GetInput()->GetYCoordinates(), this->GetInput()->GetZCoordinates(), indent);
}

//----------------------------------------------------------------------------
void svtkXMLRectilinearGridWriter::CalculateSuperclassFraction(float* fractions)
{
  int extent[6];
  this->GetInputExtent(extent);
  int dims[3] = { extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1 };

  // The amount of data written by the superclass comes from the
  // point/cell data arrays.
  svtkIdType dimX = dims[0];
  svtkIdType dimY = dims[1];
  svtkIdType dimZ = dims[2];
  svtkIdType superclassPieceSize =
    (this->GetInput()->GetPointData()->GetNumberOfArrays() * dimX * dimY * dimZ +
      static_cast<svtkIdType>(this->GetInput()->GetCellData()->GetNumberOfArrays()) * (dimX - 1) *
        (dimY - 1) * (dimZ - 1));

  // The total data written includes the coordinate arrays.
  svtkIdType totalPieceSize = superclassPieceSize + dims[0] + dims[1] + dims[2];
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }
  fractions[0] = 0;
  fractions[1] = fractions[0] + static_cast<float>(superclassPieceSize) / totalPieceSize;
  fractions[2] = 1;
}

//----------------------------------------------------------------------------
int svtkXMLRectilinearGridWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}
