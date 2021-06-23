/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUnstructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLUnstructuredGridWriter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#define svtkXMLOffsetsManager_DoNotInclude
#include "svtkXMLOffsetsManager.h"
#undef svtkXMLOffsetsManager_DoNotInclude

#include <cassert>

svtkStandardNewMacro(svtkXMLUnstructuredGridWriter);

//----------------------------------------------------------------------------
svtkXMLUnstructuredGridWriter::svtkXMLUnstructuredGridWriter()
{
  this->CellsOM = new OffsetsManagerArray;
}

//----------------------------------------------------------------------------
svtkXMLUnstructuredGridWriter::~svtkXMLUnstructuredGridWriter()
{
  delete this->CellsOM;
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkUnstructuredGridBase* svtkXMLUnstructuredGridWriter::GetInput()
{
  return static_cast<svtkUnstructuredGridBase*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLUnstructuredGridWriter::GetDataSetName()
{
  return "UnstructuredGrid";
}

//----------------------------------------------------------------------------
const char* svtkXMLUnstructuredGridWriter::GetDefaultFileExtension()
{
  return "vtu";
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridWriter::WriteInlinePieceAttributes()
{
  this->Superclass::WriteInlinePieceAttributes();
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  svtkUnstructuredGridBase* input = this->GetInput();
  this->WriteScalarAttribute("NumberOfCells", input->GetNumberOfCells());
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridWriter::WriteInlinePiece(svtkIndent indent)
{
  svtkUnstructuredGridBase* input = this->GetInput();

  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteInlinePiece(indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the cell specifications.
  if (svtkUnstructuredGrid* grid = svtkUnstructuredGrid::SafeDownCast(input))
  {
    // This is a bit more efficient and avoids iteration over all cells.
    this->WriteCellsInline("Cells", grid->GetCells(), grid->GetCellTypesArray(), grid->GetFaces(),
      grid->GetFaceLocations(), indent);
  }
  else
  {
    svtkCellIterator* cellIter = input->NewCellIterator();
    this->WriteCellsInline(
      "Cells", cellIter, input->GetNumberOfCells(), input->GetMaxCellSize(), indent);
    cellIter->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridWriter::AllocatePositionArrays()
{
  this->Superclass::AllocatePositionArrays();

  this->NumberOfCellsPositions = new svtkTypeInt64[this->NumberOfPieces];
  this->CellsOM->Allocate(this->NumberOfPieces, 5, this->NumberOfTimeSteps);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridWriter::DeletePositionArrays()
{
  this->Superclass::DeletePositionArrays();

  delete[] this->NumberOfCellsPositions;
  this->NumberOfCellsPositions = nullptr;
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridWriter::WriteAppendedPieceAttributes(int index)
{
  this->Superclass::WriteAppendedPieceAttributes(index);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->NumberOfCellsPositions[index] = this->ReserveAttributeSpace("NumberOfCells");
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridWriter::WriteAppendedPiece(int index, svtkIndent indent)
{
  svtkUnstructuredGridBase* input = this->GetInput();
  this->Superclass::WriteAppendedPiece(index, indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  if (svtkUnstructuredGrid* grid = svtkUnstructuredGrid::SafeDownCast(input))
  {
    this->ConvertCells(grid->GetCells());
    this->WriteCellsAppended("Cells", grid->GetCellTypesArray(), grid->GetFaces(),
      grid->GetFaceLocations(), indent, &this->CellsOM->GetPiece(index));
  }
  else
  {
    svtkCellIterator* cellIter = input->NewCellIterator();
    this->WriteCellsAppended(
      "Cells", cellIter, input->GetNumberOfCells(), indent, &this->CellsOM->GetPiece(index));
    cellIter->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  svtkUnstructuredGridBase* input = this->GetInput();

  std::streampos returnPosition = os.tellp();
  os.seekp(std::streampos(this->NumberOfCellsPositions[index]));
  this->WriteScalarAttribute("NumberOfCells", input->GetNumberOfCells());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  os.seekp(returnPosition);

  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);

  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass write its data.
  this->Superclass::WriteAppendedPieceData(index);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the cell specification arrays.
  if (svtkUnstructuredGrid* grid = svtkUnstructuredGrid::SafeDownCast(input))
  {
    this->WriteCellsAppendedData(grid->GetCells(), grid->GetCellTypesArray(), grid->GetFaces(),
      grid->GetFaceLocations(), this->CurrentTimeIndex, &this->CellsOM->GetPiece(index));
  }
  else
  {
    svtkCellIterator* cellIter = input->NewCellIterator();
    this->WriteCellsAppendedData(cellIter, input->GetNumberOfCells(), input->GetMaxCellSize(),
      this->CurrentTimeIndex, &this->CellsOM->GetPiece(index));
    cellIter->Delete();
  }
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLUnstructuredGridWriter::GetNumberOfInputCells()
{
  return this->GetInput()->GetNumberOfCells();
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridWriter::CalculateSuperclassFraction(float* fractions)
{
  svtkUnstructuredGridBase* input = this->GetInput();

  // The superclass will write point/cell data and point specifications.
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  svtkIdType pdSize = pdArrays * this->GetNumberOfInputPoints();
  svtkIdType cdSize = cdArrays * this->GetNumberOfInputCells();
  svtkIdType pointsSize = this->GetNumberOfInputPoints();

  // This class will write cell specifications.
  svtkIdType connectSize = 0;
  if (svtkUnstructuredGrid* grid = svtkUnstructuredGrid::SafeDownCast(input))
  {
    if (grid->GetCells() == nullptr)
    {
      connectSize = 0;
    }
    else
    {
      connectSize = grid->GetCells()->GetNumberOfConnectivityIds();
    }
  }
  else
  {
    svtkCellIterator* cellIter = input->NewCellIterator();
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      connectSize += cellIter->GetNumberOfPoints();
    }
    cellIter->Delete();
  }

  svtkIdType offsetSize = input->GetNumberOfCells();
  svtkIdType typesSize = input->GetNumberOfCells();

  int total = (pdSize + cdSize + pointsSize + connectSize + offsetSize + typesSize);
  if (total == 0)
  {
    total = 1;
  }
  fractions[0] = 0;
  fractions[1] = float(pdSize + cdSize + pointsSize) / total;
  fractions[2] = 1;
}

//----------------------------------------------------------------------------
int svtkXMLUnstructuredGridWriter::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGridBase");
  return 1;
}
