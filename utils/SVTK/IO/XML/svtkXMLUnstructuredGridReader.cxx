/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUnstructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLUnstructuredGridReader.h"

#include "svtkCellArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUpdateCellsV8toV9.h"
#include "svtkXMLDataElement.h"

#include <cassert>

svtkStandardNewMacro(svtkXMLUnstructuredGridReader);

//----------------------------------------------------------------------------
svtkXMLUnstructuredGridReader::svtkXMLUnstructuredGridReader()
{
  this->CellElements = nullptr;
  this->NumberOfCells = nullptr;
  this->CellsTimeStep = -1;
  this->CellsOffset = static_cast<unsigned long>(-1); // almost invalid state
}

//----------------------------------------------------------------------------
svtkXMLUnstructuredGridReader::~svtkXMLUnstructuredGridReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkXMLUnstructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkXMLUnstructuredGridReader::GetOutput(int idx)
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* svtkXMLUnstructuredGridReader::GetDataSetName()
{
  return "UnstructuredGrid";
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridReader::GetOutputUpdateExtent(
  int& piece, int& numberOfPieces, int& ghostLevel)
{
  svtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  int i;
  this->TotalNumberOfCells = 0;
  for (i = this->StartPiece; i < this->EndPiece; ++i)
  {
    this->TotalNumberOfCells += this->NumberOfCells[i];
  }

  // Data reading will start at the beginning of the output.
  this->StartCell = 0;
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->NumberOfCells = new svtkIdType[numPieces];
  this->CellElements = new svtkXMLDataElement*[numPieces];
  for (int i = 0; i < numPieces; ++i)
  {
    this->CellElements[i] = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridReader::DestroyPieces()
{
  delete[] this->CellElements;
  delete[] this->NumberOfCells;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLUnstructuredGridReader::GetNumberOfCellsInPiece(int piece)
{
  return this->NumberOfCells[piece];
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  svtkUnstructuredGrid* output = svtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Setup the output's cell arrays.
  svtkNew<svtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfTuples(this->GetNumberOfCells());
  cellTypes->FillValue(SVTK_EMPTY_CELL);
  svtkNew<svtkCellArray> outCells;

  output->SetCells(cellTypes, outCells);
}

//----------------------------------------------------------------------------
int svtkXMLUnstructuredGridReader::ReadPiece(svtkXMLDataElement* ePiece)
{
  if (!this->Superclass::ReadPiece(ePiece))
  {
    return 0;
  }
  int i;

  if (!ePiece->GetScalarAttribute("NumberOfCells", this->NumberOfCells[this->Piece]))
  {
    svtkErrorMacro("Piece " << this->Piece << " is missing its NumberOfCells attribute.");
    this->NumberOfCells[this->Piece] = 0;
    return 0;
  }

  // Find the Cells element in the piece.
  this->CellElements[this->Piece] = nullptr;
  for (i = 0; i < ePiece->GetNumberOfNestedElements(); ++i)
  {
    svtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "Cells") == 0) && (eNested->GetNumberOfNestedElements() > 0))
    {
      this->CellElements[this->Piece] = eNested;
    }
  }

  if (!this->CellElements[this->Piece])
  {
    svtkErrorMacro("A piece is missing its Cells element.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredGridReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  this->StartCell += this->NumberOfCells[this->Piece];
}

//----------------------------------------------------------------------------
int svtkXMLUnstructuredGridReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data and point specifications (we read cell
  // specifications here).
  svtkIdType superclassPieceSize =
    ((this->NumberOfPointArrays + 1) * this->GetNumberOfPointsInPiece(this->Piece) +
      this->NumberOfCellArrays * this->GetNumberOfCellsInPiece(this->Piece));

  // Total amount of data in this piece comes from cell/face data arrays.
  // Three of them are for standard svtkUnstructuredGrid cell specification:
  // connectivities, offsets and types. Two optional arrays are for face
  // specification of polyhedron cells: faces and face offsets.
  // Note: We don't know exactly the array size of cell connectivities and
  // faces until we actually read the file. The following progress computation
  // assumes that each array cost the same time to read.
  svtkIdType totalPieceSize = superclassPieceSize + 5 * this->GetNumberOfCellsInPiece(this->Piece);
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.  The cell
  // specification reads two arrays, and then the cell types array is
  // one more.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[5] = { 0, float(superclassPieceSize) / totalPieceSize,
    ((float(superclassPieceSize) + 2 * this->GetNumberOfCellsInPiece(this->Piece)) /
      totalPieceSize),
    ((float(superclassPieceSize) + 3 * this->GetNumberOfCellsInPiece(this->Piece)) /
      totalPieceSize),
    1 };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  svtkUnstructuredGrid* output = svtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Set the range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the Cells.
  svtkXMLDataElement* eCells = this->CellElements[this->Piece];
  if (!eCells)
  {
    svtkErrorMacro("Cannot find cell arrays in piece " << this->Piece);
    return 0;
  }

  //  int needToRead = this->CellsNeedToReadTimeStep(eNested,
  //    this->CellsTimeStep, this->CellsOffset);
  //  if( needToRead )
  {
    // Read the array.
    if (!this->ReadCellArray(
          this->NumberOfCells[this->Piece], this->TotalNumberOfCells, eCells, output->GetCells()))
    {
      return 0;
    }
  }

  // Set the range of progress for the cell types.
  this->SetProgressRange(progressRange, 2, fractions);

  // Read the corresponding cell types.
  svtkIdType numberOfCells = this->NumberOfCells[this->Piece];
  if (numberOfCells > 0)
  {
    svtkXMLDataElement* eTypes = this->FindDataArrayWithName(eCells, "types");
    if (!eTypes)
    {
      svtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array could not be found.");
      return 0;
    }
    svtkAbstractArray* ac2 = this->CreateArray(eTypes);
    svtkDataArray* c2 = svtkArrayDownCast<svtkDataArray>(ac2);
    if (!c2 || (c2->GetNumberOfComponents() != 1))
    {
      svtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array could not be created"
        << " with one component.");
      if (ac2)
      {
        ac2->Delete();
      }
      return 0;
    }
    c2->SetNumberOfTuples(numberOfCells);
    if (!this->ReadArrayValues(eTypes, 0, c2, 0, numberOfCells))
    {
      svtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array is not long enough.");
      return 0;
    }
    svtkUnsignedCharArray* cellTypes = this->ConvertToUnsignedCharArray(c2);
    if (!cellTypes)
    {
      svtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array could not be converted"
        << " to a svtkUnsignedCharArray.");
      return 0;
    }

    // Copy the cell type data.
    memcpy(output->GetCellTypesArray()->GetPointer(this->StartCell), cellTypes->GetPointer(0),
      numberOfCells);

    // Permute node numbering on higher order hexahedra for legacy files (see
    // https://gitlab.kitware.com/svtk/svtk/-/merge_requests/6678 )
    if (this->GetFileMajorVersion() < 2 ||
      (this->GetFileMajorVersion() == 2 && this->GetFileMinorVersion() < 1))
    {
      svtkUpdateCellsV8toV9(output);
    }

    cellTypes->Delete();
  }

  // Set the range of progress for the faces.
  this->SetProgressRange(progressRange, 3, fractions);

  //
  // Read face array. Used for polyhedron mesh support. First need to
  // check if faces and faceoffsets arrays are available in this piece.
  if (!this->FindDataArrayWithName(eCells, "faces") ||
    !this->FindDataArrayWithName(eCells, "faceoffsets"))
  {
    if (output->GetFaces())
    {
      // This piece doesn't have any polyhedron but other pieces that
      // we've already processed do so we need to add in face information
      // for cells that don't have that by marking -1.
      for (svtkIdType c = 0; c < numberOfCells; c++)
      {
        output->GetFaceLocations()->InsertNextValue(-1);
      }
    }
    return 1;
  }

  // By default svtkUnstructuredGrid does not contain face information, which is
  // only used by polyhedron cells. If so far no polyhedron cells have been
  // added, the pointers to the arrays will be nullptr. In this case, we need to
  // initialize the arrays and assign values to the previous non-polyhedron cells.
  if (!output->GetFaces() || !output->GetFaceLocations())
  {
    output->InitializeFacesRepresentation(this->StartCell);
  }

  // Read face arrays.
  if (!this->ReadFaceArray(
        this->NumberOfCells[this->Piece], eCells, output->GetFaces(), output->GetFaceLocations()))
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLUnstructuredGridReader::ReadArrayForCells(
  svtkXMLDataElement* da, svtkAbstractArray* outArray)
{
  svtkIdType startCell = this->StartCell;
  svtkIdType numCells = this->NumberOfCells[this->Piece];
  svtkIdType components = outArray->GetNumberOfComponents();
  return this->ReadArrayValues(da, startCell * components, outArray, 0, numCells * components);
}

//----------------------------------------------------------------------------
int svtkXMLUnstructuredGridReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUnstructuredGrid");
  return 1;
}
