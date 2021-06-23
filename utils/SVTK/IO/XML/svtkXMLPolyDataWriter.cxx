/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPolyDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPolyDataWriter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkErrorCode.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#define svtkXMLOffsetsManager_DoNotInclude
#include "svtkXMLOffsetsManager.h"
#undef svtkXMLOffsetsManager_DoNotInclude

svtkStandardNewMacro(svtkXMLPolyDataWriter);

//----------------------------------------------------------------------------
svtkXMLPolyDataWriter::svtkXMLPolyDataWriter()
{
  this->VertsOM = new OffsetsManagerArray;
  this->LinesOM = new OffsetsManagerArray;
  this->StripsOM = new OffsetsManagerArray;
  this->PolysOM = new OffsetsManagerArray;
}

//----------------------------------------------------------------------------
svtkXMLPolyDataWriter::~svtkXMLPolyDataWriter()
{
  delete this->VertsOM;
  delete this->LinesOM;
  delete this->StripsOM;
  delete this->PolysOM;
}

//----------------------------------------------------------------------------
void svtkXMLPolyDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkXMLPolyDataWriter::GetInput()
{
  return static_cast<svtkPolyData*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLPolyDataWriter::GetDataSetName()
{
  return "PolyData";
}

//----------------------------------------------------------------------------
const char* svtkXMLPolyDataWriter::GetDefaultFileExtension()
{
  return "vtp";
}

//----------------------------------------------------------------------------
void svtkXMLPolyDataWriter::AllocatePositionArrays()
{
  this->Superclass::AllocatePositionArrays();

  this->NumberOfVertsPositions = new unsigned long[this->NumberOfPieces];
  this->NumberOfLinesPositions = new unsigned long[this->NumberOfPieces];
  this->NumberOfStripsPositions = new unsigned long[this->NumberOfPieces];
  this->NumberOfPolysPositions = new unsigned long[this->NumberOfPieces];

  this->VertsOM->Allocate(this->NumberOfPieces, 2, this->NumberOfTimeSteps);
  this->LinesOM->Allocate(this->NumberOfPieces, 2, this->NumberOfTimeSteps);
  this->StripsOM->Allocate(this->NumberOfPieces, 2, this->NumberOfTimeSteps);
  this->PolysOM->Allocate(this->NumberOfPieces, 2, this->NumberOfTimeSteps);
}

//----------------------------------------------------------------------------
void svtkXMLPolyDataWriter::DeletePositionArrays()
{
  this->Superclass::DeletePositionArrays();

  delete[] this->NumberOfVertsPositions;
  delete[] this->NumberOfLinesPositions;
  delete[] this->NumberOfStripsPositions;
  delete[] this->NumberOfPolysPositions;
}

//----------------------------------------------------------------------------
void svtkXMLPolyDataWriter::WriteInlinePieceAttributes()
{
  this->Superclass::WriteInlinePieceAttributes();
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  svtkPolyData* input = this->GetInput();
  this->WriteScalarAttribute("NumberOfVerts", input->GetVerts()->GetNumberOfCells());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->WriteScalarAttribute("NumberOfLines", input->GetLines()->GetNumberOfCells());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->WriteScalarAttribute("NumberOfStrips", input->GetStrips()->GetNumberOfCells());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->WriteScalarAttribute("NumberOfPolys", input->GetPolys()->GetNumberOfCells());
}

//----------------------------------------------------------------------------
void svtkXMLPolyDataWriter::WriteInlinePiece(svtkIndent indent)
{
  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  float fractions[6];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteInlinePiece(indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  svtkPolyData* input = this->GetInput();

  // Set the range of progress for Verts.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the Verts.
  this->WriteCellsInline("Verts", input->GetVerts(), nullptr, indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for Lines.
  this->SetProgressRange(progressRange, 2, fractions);

  // Write the Lines.
  this->WriteCellsInline("Lines", input->GetLines(), nullptr, indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for Strips.
  this->SetProgressRange(progressRange, 3, fractions);

  // Write the Strips.
  this->WriteCellsInline("Strips", input->GetStrips(), nullptr, indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for Polys.
  this->SetProgressRange(progressRange, 4, fractions);

  // Write the Polys.
  this->WriteCellsInline("Polys", input->GetPolys(), nullptr, indent);
}

//----------------------------------------------------------------------------
void svtkXMLPolyDataWriter::WriteAppendedPieceAttributes(int index)
{
  this->Superclass::WriteAppendedPieceAttributes(index);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->NumberOfVertsPositions[index] = this->ReserveAttributeSpace("NumberOfVerts");
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->NumberOfLinesPositions[index] = this->ReserveAttributeSpace("NumberOfLines");
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->NumberOfStripsPositions[index] = this->ReserveAttributeSpace("NumberOfStrips");
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->NumberOfPolysPositions[index] = this->ReserveAttributeSpace("NumberOfPolys");
}

//----------------------------------------------------------------------------
void svtkXMLPolyDataWriter::WriteAppendedPiece(int index, svtkIndent indent)
{
  this->Superclass::WriteAppendedPiece(index, indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->ConvertCells(this->GetInput()->GetVerts());
  this->WriteCellsAppended("Verts", nullptr, indent, &this->VertsOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->ConvertCells(this->GetInput()->GetLines());
  this->WriteCellsAppended("Lines", nullptr, indent, &this->LinesOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->ConvertCells(this->GetInput()->GetStrips());
  this->WriteCellsAppended("Strips", nullptr, indent, &this->StripsOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->ConvertCells(this->GetInput()->GetPolys());
  this->WriteCellsAppended("Polys", nullptr, indent, &this->PolysOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void svtkXMLPolyDataWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  svtkPolyData* input = this->GetInput();

  unsigned long returnPosition = os.tellp();
  os.seekp(this->NumberOfVertsPositions[index]);
  this->WriteScalarAttribute("NumberOfVerts", input->GetVerts()->GetNumberOfCells());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  os.seekp(this->NumberOfLinesPositions[index]);
  this->WriteScalarAttribute("NumberOfLines", input->GetLines()->GetNumberOfCells());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  os.seekp(this->NumberOfStripsPositions[index]);
  this->WriteScalarAttribute("NumberOfStrips", input->GetStrips()->GetNumberOfCells());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  os.seekp(this->NumberOfPolysPositions[index]);
  this->WriteScalarAttribute("NumberOfPolys", input->GetPolys()->GetNumberOfCells());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  os.seekp(returnPosition);

  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  float fractions[6];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteAppendedPieceData(index);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for Verts.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the Verts.
  this->WriteCellsAppendedData(
    input->GetVerts(), nullptr, this->CurrentTimeIndex, &this->VertsOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for Lines.
  this->SetProgressRange(progressRange, 2, fractions);

  // Write the Lines.
  this->WriteCellsAppendedData(
    input->GetLines(), nullptr, this->CurrentTimeIndex, &this->LinesOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for Strips.
  this->SetProgressRange(progressRange, 3, fractions);

  // Write the Strips.
  this->WriteCellsAppendedData(
    input->GetStrips(), nullptr, this->CurrentTimeIndex, &this->StripsOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for Polys.
  this->SetProgressRange(progressRange, 4, fractions);

  // Write the Polys.
  this->WriteCellsAppendedData(
    input->GetPolys(), nullptr, this->CurrentTimeIndex, &this->PolysOM->GetPiece(index));
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPolyDataWriter::GetNumberOfInputCells()
{
  svtkPolyData* input = this->GetInput();
  return (input->GetVerts()->GetNumberOfCells() + input->GetLines()->GetNumberOfCells() +
    input->GetStrips()->GetNumberOfCells() + input->GetPolys()->GetNumberOfCells());
}

//----------------------------------------------------------------------------
void svtkXMLPolyDataWriter::CalculateSuperclassFraction(float* fractions)
{
  svtkPolyData* input = this->GetInput();

  // The superclass will write point/cell data and point specifications.
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  svtkIdType pdSize = pdArrays * this->GetNumberOfInputPoints();
  svtkIdType cdSize = cdArrays * this->GetNumberOfInputCells();
  svtkIdType pointsSize = this->GetNumberOfInputPoints();

  // This class will write cell specifications.
  svtkIdType connectSizeV = input->GetVerts()->GetNumberOfConnectivityIds();
  svtkIdType connectSizeL = input->GetLines()->GetNumberOfConnectivityIds();
  svtkIdType connectSizeS = input->GetStrips()->GetNumberOfConnectivityIds();
  svtkIdType connectSizeP = input->GetPolys()->GetNumberOfConnectivityIds();
  svtkIdType offsetSizeV = input->GetVerts()->GetNumberOfCells();
  svtkIdType offsetSizeL = input->GetLines()->GetNumberOfCells();
  svtkIdType offsetSizeS = input->GetStrips()->GetNumberOfCells();
  svtkIdType offsetSizeP = input->GetPolys()->GetNumberOfCells();
  fractions[0] = 0;
  fractions[1] = fractions[0] + pdSize + cdSize + pointsSize;
  fractions[2] = fractions[1] + connectSizeV + offsetSizeV;
  fractions[3] = fractions[2] + connectSizeL + offsetSizeL;
  fractions[4] = fractions[3] + connectSizeS + offsetSizeS;
  fractions[5] = fractions[4] + connectSizeP + offsetSizeP;
  if (fractions[5] == 0)
  {
    fractions[5] = 1;
  }
  for (int i = 0; i < 5; ++i)
  {
    fractions[i + 1] = fractions[i + 1] / fractions[5];
  }
}

//----------------------------------------------------------------------------
int svtkXMLPolyDataWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}
