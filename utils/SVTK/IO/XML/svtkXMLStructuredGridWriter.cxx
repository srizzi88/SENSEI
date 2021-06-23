/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLStructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLStructuredGridWriter.h"

#include "svtkCellData.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStructuredGrid.h"
#define svtkXMLOffsetsManager_DoNotInclude
#include "svtkXMLOffsetsManager.h"
#undef svtkXMLOffsetsManager_DoNotInclude

svtkStandardNewMacro(svtkXMLStructuredGridWriter);

//----------------------------------------------------------------------------
svtkXMLStructuredGridWriter::svtkXMLStructuredGridWriter()
{
  this->PointsOM = new OffsetsManagerGroup;
}

//----------------------------------------------------------------------------
svtkXMLStructuredGridWriter::~svtkXMLStructuredGridWriter()
{
  delete this->PointsOM;
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkXMLStructuredGridWriter::GetInput()
{
  return static_cast<svtkStructuredGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridWriter::GetInputExtent(int* extent)
{
  this->GetInput()->GetExtent(extent);
}

//----------------------------------------------------------------------------
const char* svtkXMLStructuredGridWriter::GetDataSetName()
{
  return "StructuredGrid";
}

//----------------------------------------------------------------------------
const char* svtkXMLStructuredGridWriter::GetDefaultFileExtension()
{
  return "vts";
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridWriter::AllocatePositionArrays()
{
  this->Superclass::AllocatePositionArrays();
  this->PointsOM->Allocate(this->NumberOfPieces, this->NumberOfTimeSteps);
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridWriter::DeletePositionArrays()
{
  this->Superclass::DeletePositionArrays();
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridWriter::WriteAppendedPiece(int index, svtkIndent indent)
{
  this->Superclass::WriteAppendedPiece(index, indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->WritePointsAppended(
    this->GetInput()->GetPoints(), indent, &this->PointsOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridWriter::WriteAppendedPieceData(int index)
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

  // Set the range of progress for the points array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the points array.
  this->WritePointsAppendedData(
    this->GetInput()->GetPoints(), this->CurrentTimeIndex, &this->PointsOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridWriter::WriteInlinePiece(svtkIndent indent)
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

  // Set the range of progress for the points array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the points array.
  this->WritePointsInline(this->GetInput()->GetPoints(), indent);
}

//----------------------------------------------------------------------------
void svtkXMLStructuredGridWriter::CalculateSuperclassFraction(float* fractions)
{
  // The amount of data written by the superclass comes from the
  // point/cell data arrays.
  svtkIdType superclassPieceSize = GetNumberOfValues(this->GetInput());
  // The total data written includes the points array.
  svtkIdType totalPieceSize = superclassPieceSize + this->GetInput()->GetNumberOfPoints() * 3;
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }
  fractions[0] = 0.0f;
  fractions[1] = static_cast<float>(superclassPieceSize) / totalPieceSize;
  fractions[2] = 1.0f;
}

//----------------------------------------------------------------------------
int svtkXMLStructuredGridWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
  return 1;
}
