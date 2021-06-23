/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUnstructuredDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLUnstructuredDataWriter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkDataArray.h"
#include "svtkDataCompressor.h"
#include "svtkDataSetAttributes.h"
#include "svtkErrorCode.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkPolyhedron.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUpdateCellsV8toV9.h"
#define svtkXMLOffsetsManager_DoNotInclude
#include "svtkXMLOffsetsManager.h"
#undef svtkXMLOffsetsManager_DoNotInclude

#include <cassert>
#include <utility>

//----------------------------------------------------------------------------
svtkXMLUnstructuredDataWriter::svtkXMLUnstructuredDataWriter()
{
  this->NumberOfPieces = 1;
  this->WritePiece = -1;
  this->GhostLevel = 0;

  this->CurrentPiece = 0;
  this->FieldDataOM->Allocate(0);
  this->PointsOM = new OffsetsManagerGroup;
  this->PointDataOM = new OffsetsManagerArray;
  this->CellDataOM = new OffsetsManagerArray;

  this->Faces = svtkIdTypeArray::New();
  this->FaceOffsets = svtkIdTypeArray::New();
  this->Faces->SetName("faces");
  this->FaceOffsets->SetName("faceoffsets");
}

//----------------------------------------------------------------------------
svtkXMLUnstructuredDataWriter::~svtkXMLUnstructuredDataWriter()
{
  this->Faces->Delete();
  this->FaceOffsets->Delete();

  delete this->PointsOM;
  delete this->PointDataOM;
  delete this->CellDataOM;
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
  os << indent << "WritePiece: " << this->WritePiece << "\n";
  os << indent << "GhostLevel: " << this->GhostLevel << "\n";
}

//----------------------------------------------------------------------------
svtkPointSet* svtkXMLUnstructuredDataWriter::GetInputAsPointSet()
{
  return static_cast<svtkPointSet*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
svtkTypeBool svtkXMLUnstructuredDataWriter::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    if ((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      this->SetInputUpdateExtent(this->CurrentPiece, this->NumberOfPieces, this->GhostLevel);
    }
    else
    {
      this->SetInputUpdateExtent(this->WritePiece, this->NumberOfPieces, this->GhostLevel);
    }
    return 1;
  }

  // generate the data
  else if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->SetErrorCode(svtkErrorCode::NoError);

    if (!this->Stream && !this->FileName && !this->WriteToOutputString)
    {
      this->SetErrorCode(svtkErrorCode::NoFileNameError);
      svtkErrorMacro("The FileName or Stream must be set first or "
                    "the output must be written to a string.");
      return 0;
    }

    int numPieces = this->NumberOfPieces;

    if (this->WritePiece >= 0)
    {
      this->CurrentPiece = this->WritePiece;
    }
    else
    {
      float wholeProgressRange[2] = { 0, 1 };
      this->SetProgressRange(wholeProgressRange, this->CurrentPiece, this->NumberOfPieces);
    }

    int result = 1;
    if ((this->CurrentPiece == 0 && this->CurrentTimeIndex == 0) || this->WritePiece >= 0)
    {
      // We are just starting to write.  Do not call
      // UpdateProgressDiscrete because we want a 0 progress callback the
      // first time.
      this->UpdateProgress(0);

      // Initialize progress range to entire 0..1 range.
      if (this->WritePiece >= 0)
      {
        float wholeProgressRange[2] = { 0, 1 };
        this->SetProgressRange(wholeProgressRange, 0, 1);
      }

      if (!this->OpenStream())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      if (svtkDataSet* dataSet = this->GetInputAsDataSet())
      {
        if (dataSet->GetPointGhostArray() != nullptr && dataSet->GetCellGhostArray() != nullptr)
        {
          // use the current version for the file.
          this->UsePreviousVersion = false;
        }
        else
        {
          svtkNew<svtkCellTypes> cellTypes;
          dataSet->GetCellTypes(cellTypes);
          if (svtkNeedsNewFileVersionV8toV9(cellTypes))
          {
            this->UsePreviousVersion = false;
          }
        }
      }

      // Write the file.
      if (!this->StartFile())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      if (!this->WriteHeader())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      this->CurrentTimeIndex = 0;
      if (this->DataMode == svtkXMLWriter::Appended && this->FieldDataOM->GetNumberOfElements())
      {
        svtkNew<svtkFieldData> fieldDataCopy;
        this->UpdateFieldData(fieldDataCopy);

        // Write the field data arrays.
        this->WriteFieldDataAppendedData(fieldDataCopy, this->CurrentTimeIndex, this->FieldDataOM);
        if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }
      }
    }

    if (!(this->UserContinueExecuting == 0)) // if user ask to stop do not try to write a piece
    {
      result = this->WriteAPiece();
    }

    if ((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      // Tell the pipeline to start looping.
      if (this->CurrentPiece == 0)
      {
        request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      }
      this->CurrentPiece++;
    }

    if (this->CurrentPiece == this->NumberOfPieces || this->WritePiece >= 0)
    {
      request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentPiece = 0;
      // We are done writing all the pieces, lets loop over time now:
      this->CurrentTimeIndex++;

      if (this->UserContinueExecuting != 1)
      {
        if (!this->WriteFooter())
        {
          this->NumberOfPieces = numPieces;
          return 0;
        }

        if (!this->EndFile())
        {
          this->NumberOfPieces = numPieces;
          return 0;
        }

        this->CloseStream();
        this->CurrentTimeIndex = 0; // Reset
      }
    }
    this->NumberOfPieces = numPieces;

    // We have finished writing (at least this piece)
    this->SetProgressPartial(1);
    return result;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::AllocatePositionArrays()
{
  this->NumberOfPointsPositions = new svtkTypeInt64[this->NumberOfPieces];

  this->PointsOM->Allocate(this->NumberOfPieces, this->NumberOfTimeSteps);
  this->PointDataOM->Allocate(this->NumberOfPieces);
  this->CellDataOM->Allocate(this->NumberOfPieces);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::DeletePositionArrays()
{
  delete[] this->NumberOfPointsPositions;
  this->NumberOfPointsPositions = nullptr;
}

//----------------------------------------------------------------------------
int svtkXMLUnstructuredDataWriter::WriteHeader()
{
  svtkIndent indent = svtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if (!this->WritePrimaryElement(os, indent))
  {
    return 0;
  }

  this->WriteFieldData(indent.GetNextIndent());

  if (this->DataMode == svtkXMLWriter::Appended)
  {
    svtkIndent nextIndent = indent.GetNextIndent();

    this->AllocatePositionArrays();

    if ((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      // Loop over each piece and write its structure.
      int i;
      for (i = 0; i < this->NumberOfPieces; ++i)
      {
        // Open the piece's element.
        os << nextIndent << "<Piece";
        this->WriteAppendedPieceAttributes(i);
        if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }
        os << ">\n";

        this->WriteAppendedPiece(i, nextIndent.GetNextIndent());
        if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }

        // Close the piece's element.
        os << nextIndent << "</Piece>\n";
      }
    }
    else
    {
      // Write just the requested piece.
      // Open the piece's element.
      os << nextIndent << "<Piece";
      this->WriteAppendedPieceAttributes(this->WritePiece);
      if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }
      os << ">\n";

      this->WriteAppendedPiece(this->WritePiece, nextIndent.GetNextIndent());
      if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }

      // Close the piece's element.
      os << nextIndent << "</Piece>\n";
    }

    // Close the primary element.
    os << indent << "</" << this->GetDataSetName() << ">\n";
    os.flush();
    if (os.fail())
    {
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      this->DeletePositionArrays();
      return 0;
    }

    this->StartAppendedData();
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      this->DeletePositionArrays();
      return 0;
    }
  }

  return 1;
}

void CreateFaceStream(
  svtkCellIterator* cellIter, svtkIdTypeArray* faceStream, svtkIdTypeArray* faceOffsets)
{
  svtkNew<svtkGenericCell> cell;

  faceStream->Reset();
  faceOffsets->Reset();

  svtkIdType offset(0);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    svtkIdType ct = cellIter->GetCellType();
    if (ct != SVTK_POLYHEDRON)
    {
      faceOffsets->InsertNextValue(-1);
      continue;
    }
    cellIter->GetCell(cell.GetPointer());
    svtkCell* theCell = cell->GetRepresentativeCell();
    svtkPolyhedron* poly = svtkPolyhedron::SafeDownCast(theCell);
    if (!poly || !poly->GetNumberOfFaces())
    {
      continue;
    }

    svtkIdType n(0);
    svtkIdType* faces = poly->GetFaces();
    svtkIdType nFaces = faces[n++];

    // create offset in svtkUnstructuredGrid fashion, this will later be converted using ConvertFaces
    faceOffsets->InsertNextValue(offset);

    faceStream->InsertNextValue(nFaces);
    for (svtkIdType i = 0; i < nFaces; ++i)
    {
      svtkIdType nFaceVerts = faces[n++];
      faceStream->InsertNextValue(nFaceVerts);
      for (svtkIdType j = 0; j < nFaceVerts; ++j)
      {
        svtkIdType vi = faces[n++];
        faceStream->InsertNextValue(vi);
      }
    }
    offset += n;
  }
}

//----------------------------------------------------------------------------
int svtkXMLUnstructuredDataWriter::WriteAPiece()
{
  svtkIndent indent = svtkIndent().GetNextIndent();

  int result = 1;

  if (this->DataMode == svtkXMLWriter::Appended)
  {
    this->WriteAppendedPieceData(this->CurrentPiece);
  }
  else
  {
    result = this->WriteInlineMode(indent);
  }

  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    this->DeletePositionArrays();
    result = 0;
  }
  return result;
}

//----------------------------------------------------------------------------
int svtkXMLUnstructuredDataWriter::WriteFooter()
{
  svtkIndent indent = svtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if (this->DataMode == svtkXMLWriter::Appended)
  {
    this->DeletePositionArrays();
    this->EndAppendedData();
  }
  else
  {
    // Close the primary element.
    os << indent << "</" << this->GetDataSetName() << ">\n";
    os.flush();
    if (os.fail())
    {
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLUnstructuredDataWriter::WriteInlineMode(svtkIndent indent)
{
  ostream& os = *(this->Stream);
  svtkIndent nextIndent = indent.GetNextIndent();

  // Open the piece's element.
  os << nextIndent << "<Piece";
  this->WriteInlinePieceAttributes();
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }
  os << ">\n";

  this->WriteInlinePiece(nextIndent.GetNextIndent());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }

  // Close the piece's element.
  os << nextIndent << "</Piece>\n";

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteInlinePieceAttributes()
{
  svtkPointSet* input = this->GetInputAsPointSet();
  this->WriteScalarAttribute("NumberOfPoints", input->GetNumberOfPoints());
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteInlinePiece(svtkIndent indent)
{
  svtkPointSet* input = this->GetInputAsPointSet();

  // Split progress among point data, cell data, and point arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateDataFractions(fractions);

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);

  // Write the point data arrays.
  this->WritePointDataInline(input->GetPointData(), indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the cell data arrays.
  this->WriteCellDataInline(input->GetCellData(), indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the point specification array.
  this->SetProgressRange(progressRange, 2, fractions);

  // Write the point specification array.
  this->WritePointsInline(input->GetPoints(), indent);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteAppendedPieceAttributes(int index)
{
  this->NumberOfPointsPositions[index] = this->ReserveAttributeSpace("NumberOfPoints");
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteAppendedPiece(int index, svtkIndent indent)
{
  svtkPointSet* input = this->GetInputAsPointSet();

  this->WritePointDataAppended(input->GetPointData(), indent, &this->PointDataOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->WriteCellDataAppended(input->GetCellData(), indent, &this->CellDataOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->WritePointsAppended(input->GetPoints(), indent, &this->PointsOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  svtkPointSet* input = this->GetInputAsPointSet();

  std::streampos returnPosition = os.tellp();
  os.seekp(std::streampos(this->NumberOfPointsPositions[index]));
  svtkPoints* points = input->GetPoints();
  this->WriteScalarAttribute("NumberOfPoints", (points ? points->GetNumberOfPoints() : 0));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  os.seekp(returnPosition);

  // Split progress among point data, cell data, and point arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateDataFractions(fractions);

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);

  // Write the point data arrays.
  this->WritePointDataAppendedData(
    input->GetPointData(), this->CurrentTimeIndex, &this->PointDataOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the cell data arrays.
  this->WriteCellDataAppendedData(
    input->GetCellData(), this->CurrentTimeIndex, &this->CellDataOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the point specification array.
  this->SetProgressRange(progressRange, 2, fractions);

  // Write the point specification array.
  // Since we are writing the point let save the Modified Time of svtkPoints:
  this->WritePointsAppendedData(
    input->GetPoints(), this->CurrentTimeIndex, &this->PointsOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsInline(const char* name, svtkCellIterator* cellIter,
  svtkIdType numCells, svtkIdType cellSizeEstimate, svtkIndent indent)
{
  this->ConvertCells(cellIter, numCells, cellSizeEstimate);

  svtkNew<svtkUnsignedCharArray> types;
  types->Allocate(numCells);
  svtkIdType nPolyhedra(0);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    svtkIdType ct = cellIter->GetCellType();
    if (ct == SVTK_POLYHEDRON)
    {
      ++nPolyhedra;
    }
    types->InsertNextValue(static_cast<unsigned char>(ct));
  }

  if (nPolyhedra > 0)
  {
    svtkNew<svtkIdTypeArray> faces, offsets;
    CreateFaceStream(cellIter, faces.GetPointer(), offsets.GetPointer());
    this->ConvertFaces(faces.GetPointer(), offsets.GetPointer());
  }
  else
  {
    this->Faces->SetNumberOfTuples(0);
    this->FaceOffsets->SetNumberOfTuples(0);
  }

  this->WriteCellsInlineWorker(name, types.GetPointer(), indent);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsInline(
  const char* name, svtkCellArray* cells, svtkDataArray* types, svtkIndent indent)
{
  this->WriteCellsInline(name, cells, types, nullptr, nullptr, indent);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsInline(const char* name, svtkCellArray* cells,
  svtkDataArray* types, svtkIdTypeArray* faces, svtkIdTypeArray* faceOffsets, svtkIndent indent)
{
  if (cells)
  {
    this->ConvertCells(cells);
  }
  this->ConvertFaces(faces, faceOffsets);

  this->WriteCellsInlineWorker(name, types, indent);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsInlineWorker(
  const char* name, svtkDataArray* types, svtkIndent indent)
{
  ostream& os = *(this->Stream);
  os << indent << "<" << name << ">\n";

  // Split progress by cell connectivity, offset, and type arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[6];
  this->CalculateCellFractions(fractions, types ? types->GetNumberOfTuples() : 0);

  // Set the range of progress for the connectivity array.
  this->SetProgressRange(progressRange, 0, fractions);

  // Write the connectivity array.
  this->WriteArrayInline(this->CellPoints, indent.GetNextIndent());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the offsets array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Write the offsets array.
  this->WriteArrayInline(this->CellOffsets, indent.GetNextIndent());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  if (types)
  {
    // Set the range of progress for the types array.
    this->SetProgressRange(progressRange, 2, fractions);

    // Write the types array.
    this->WriteArrayInline(types, indent.GetNextIndent(), "types");
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }
  }

  if (this->Faces->GetNumberOfTuples())
  {
    // Set the range of progress for the faces array.
    this->SetProgressRange(progressRange, 3, fractions);

    // Write the connectivity array.
    this->WriteArrayInline(this->Faces, indent.GetNextIndent(), "faces");
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }
  }

  if (this->FaceOffsets->GetNumberOfTuples())
  {
    // Set the range of progress for the face offset array.
    this->SetProgressRange(progressRange, 4, fractions);

    // Write the face offsets array.
    this->WriteArrayInline(this->FaceOffsets, indent.GetNextIndent(), "faceoffsets");
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }
  }

  os << indent << "</" << name << ">\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
  }
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsAppended(
  const char* name, svtkDataArray* types, svtkIndent indent, OffsetsManagerGroup* cellsManager)
{
  this->WriteCellsAppended(name, types, nullptr, nullptr, indent, cellsManager);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsAppended(const char* name, svtkDataArray* types,
  svtkIdTypeArray* faces, svtkIdTypeArray* faceOffsets, svtkIndent indent,
  OffsetsManagerGroup* cellsManager)
{
  this->ConvertFaces(faces, faceOffsets);
  ostream& os = *(this->Stream);
  os << indent << "<" << name << ">\n";

  // Helper for the 'for' loop
  svtkDataArray* allcells[5];
  allcells[0] = this->CellPoints;
  allcells[1] = this->CellOffsets;
  allcells[2] = types;
  allcells[3] = this->Faces->GetNumberOfTuples() ? this->Faces : nullptr;
  allcells[4] = this->FaceOffsets->GetNumberOfTuples() ? this->FaceOffsets : nullptr;
  const char* names[] = { nullptr, nullptr, "types", nullptr, nullptr };

  for (int t = 0; t < this->NumberOfTimeSteps; t++)
  {
    for (int i = 0; i < 5; i++)
    {
      if (allcells[i])
      {
        this->WriteArrayAppended(
          allcells[i], indent.GetNextIndent(), cellsManager->GetElement(i), names[i], 0, t);
        if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
        {
          return;
        }
      }
    }
  }
  os << indent << "</" << name << ">\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return;
  }
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsAppended(const char* name, svtkCellIterator* cellIter,
  svtkIdType numCells, svtkIndent indent, OffsetsManagerGroup* cellsManager)
{
  this->ConvertCells(cellIter, numCells, 3);

  svtkNew<svtkUnsignedCharArray> types;
  types->Allocate(numCells);
  svtkIdType nPolyhedra(0);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    svtkIdType ct = cellIter->GetCellType();
    if (ct == SVTK_POLYHEDRON)
    {
      ++nPolyhedra;
    }
    types->InsertNextValue(static_cast<unsigned char>(ct));
  }
  if (nPolyhedra > 0)
  {
    svtkNew<svtkIdTypeArray> faces, offsets;
    CreateFaceStream(cellIter, faces.GetPointer(), offsets.GetPointer());
    this->WriteCellsAppended(
      name, types.GetPointer(), faces.GetPointer(), offsets.GetPointer(), indent, cellsManager);
  }
  else
  {
    this->WriteCellsAppended(name, types.GetPointer(), nullptr, nullptr, indent, cellsManager);
  }
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsAppendedData(
  svtkCellArray* cells, svtkDataArray* types, int timestep, OffsetsManagerGroup* cellsManager)
{
  this->WriteCellsAppendedData(cells, types, nullptr, nullptr, timestep, cellsManager);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsAppendedData(svtkCellIterator* cellIter,
  svtkIdType numCells, svtkIdType cellSizeEstimate, int timestep, OffsetsManagerGroup* cellsManager)
{
  this->ConvertCells(cellIter, numCells, cellSizeEstimate);

  svtkNew<svtkUnsignedCharArray> types;
  types->Allocate(this->CellOffsets->GetNumberOfTuples() + 1);
  int nPolyhedra(0);
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    svtkIdType ct = cellIter->GetCellType();
    if (ct == SVTK_POLYHEDRON)
    {
      ++nPolyhedra;
    }
    types->InsertNextValue(static_cast<unsigned char>(ct));
  }

  if (nPolyhedra > 0)
  {
    // even though it looks like we do this for the second time
    // the test points out that it is needed here.
    svtkNew<svtkIdTypeArray> faces, offsets;
    CreateFaceStream(cellIter, faces.GetPointer(), offsets.GetPointer());
    this->ConvertFaces(faces.GetPointer(), offsets.GetPointer());
  }
  else
  {
    this->Faces->SetNumberOfTuples(0);
    this->FaceOffsets->SetNumberOfTuples(0);
  }

  this->WriteCellsAppendedDataWorker(types.GetPointer(), timestep, cellsManager);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsAppendedData(svtkCellArray* cells, svtkDataArray* types,
  svtkIdTypeArray* faces, svtkIdTypeArray* faceOffsets, int timestep,
  OffsetsManagerGroup* cellsManager)
{
  if (cells)
  {
    this->ConvertCells(cells);
  }

  this->ConvertFaces(faces, faceOffsets);
  this->WriteCellsAppendedDataWorker(types, timestep, cellsManager);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::WriteCellsAppendedDataWorker(
  svtkDataArray* types, int timestep, OffsetsManagerGroup* cellsManager)
{
  // Split progress by cell connectivity, offset, and type arrays.
  float progressRange[5] = { 0, 0, 0, 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[6];
  this->CalculateCellFractions(fractions, types ? types->GetNumberOfTuples() : 0);

  // Helper for the 'for' loop
  svtkDataArray* allcells[5];
  allcells[0] = this->CellPoints;
  allcells[1] = this->CellOffsets;
  allcells[2] = types;
  allcells[3] = this->Faces->GetNumberOfTuples() ? this->Faces : nullptr;
  allcells[4] = this->FaceOffsets->GetNumberOfTuples() ? this->FaceOffsets : nullptr;

  for (int i = 0; i < 5; i++)
  {
    if (allcells[i])
    {
      // Set the range of progress for the connectivity array.
      this->SetProgressRange(progressRange, i, fractions);

      svtkMTimeType mtime = allcells[i]->GetMTime();
      svtkMTimeType& cellsMTime = cellsManager->GetElement(i).GetLastMTime();
      // Only write cells if MTime has changed
      if (cellsMTime != mtime)
      {
        cellsMTime = mtime;
        // Write the connectivity array.
        this->WriteArrayAppendedData(allcells[i], cellsManager->GetElement(i).GetPosition(timestep),
          cellsManager->GetElement(i).GetOffsetValue(timestep));
        if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
        {
          return;
        }
      }
      else
      {
        // One timestep must have already been written or the
        // mtime would have changed and we would not be here.
        assert(timestep > 0);
        cellsManager->GetElement(i).GetOffsetValue(timestep) =
          cellsManager->GetElement(i).GetOffsetValue(timestep - 1);
        this->ForwardAppendedDataOffset(cellsManager->GetElement(i).GetPosition(timestep),
          cellsManager->GetElement(i).GetOffsetValue(timestep), "offset");
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::ConvertCells(
  svtkCellIterator* cellIter, svtkIdType numCells, svtkIdType cellSizeEstimate)
{
  svtkNew<svtkAOSDataArrayTemplate<svtkIdType> > conn;
  svtkNew<svtkAOSDataArrayTemplate<svtkIdType> > offsets;

  conn->SetName("connectivity");
  offsets->SetName("offsets");

  conn->Allocate(numCells * cellSizeEstimate);
  offsets->Allocate(numCells);

  // Offsets array skips the leading 0 and includes the connectivity array size
  // at the end.

  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    svtkIdType* begin = cellIter->GetPointIds()->GetPointer(0);
    svtkIdType* end = begin + cellIter->GetNumberOfPoints();
    while (begin != end)
    {
      conn->InsertNextValue(*begin++);
    }

    offsets->InsertNextValue(conn->GetNumberOfTuples());
  }

  conn->Squeeze();
  offsets->Squeeze();

  this->CellPoints = std::move(conn);
  this->CellOffsets = std::move(offsets);
}

namespace
{

struct ConvertCellsVisitor
{
  svtkSmartPointer<svtkDataArray> Offsets;
  svtkSmartPointer<svtkDataArray> Connectivity;

  template <typename CellStateT>
  void operator()(CellStateT& state)
  {
    using ArrayT = typename CellStateT::ArrayType;

    svtkNew<ArrayT> offsets;
    svtkNew<ArrayT> conn;

    // Shallow copy will let us change the name of the array to what the
    // writer expects without actually copying the array data:
    conn->ShallowCopy(state.GetConnectivity());
    conn->SetName("connectivity");
    this->Connectivity = std::move(conn);

    // The file format for offsets always skips the first offset, because
    // it's always zero. Use SetArray and GetPointer to create a view
    // of the offsets array that starts at index=1:
    auto* offsetsIn = state.GetOffsets();
    const svtkIdType numOffsets = offsetsIn->GetNumberOfValues();
    if (numOffsets >= 2)
    {
      offsets->SetArray(offsetsIn->GetPointer(1), numOffsets - 1, 1 /*save*/);
    }
    offsets->SetName("offsets");

    this->Offsets = std::move(offsets);
  }
};

} // end anon namespace

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::ConvertCells(svtkCellArray* cells)
{
  ConvertCellsVisitor visitor;
  cells->Visit(visitor);
  this->CellPoints = std::move(visitor.Connectivity);
  this->CellOffsets = std::move(visitor.Offsets);
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::ConvertFaces(svtkIdTypeArray* faces, svtkIdTypeArray* faceOffsets)
{
  if (!faces || !faces->GetNumberOfTuples() || !faceOffsets || !faceOffsets->GetNumberOfTuples())
  {
    this->Faces->SetNumberOfTuples(0);
    this->FaceOffsets->SetNumberOfTuples(0);
    return;
  }

  // copy faces stream.
  this->Faces->SetNumberOfTuples(faces->GetNumberOfTuples());
  svtkIdType* fromPtr = faces->GetPointer(0);
  svtkIdType* toPtr = this->Faces->GetPointer(0);
  for (svtkIdType i = 0; i < faces->GetNumberOfTuples(); i++)
  {
    *toPtr++ = *fromPtr++;
  }

  // this->FaceOffsets point to the face arrays of cells. Specifically
  // FaceOffsets[i] points to the end of the i-th cell's faces + 1. While
  // input faceOffsets[i] points to the beginning of the i-th cell. Note
  // that for both arrays, a non-polyhedron cell has an offset of -1.
  svtkIdType numberOfCells = faceOffsets->GetNumberOfTuples();
  this->FaceOffsets->SetNumberOfTuples(numberOfCells);
  svtkIdType* newOffsetPtr = this->FaceOffsets->GetPointer(0);
  svtkIdType* oldOffsetPtr = faceOffsets->GetPointer(0);
  svtkIdType* facesPtr = this->Faces->GetPointer(0);
  bool foundPolyhedronCell = false;
  for (svtkIdType i = 0; i < numberOfCells; i++)
  {
    if (oldOffsetPtr[i] < 0) // non-polyhedron cell
    {
      newOffsetPtr[i] = -1;
    }
    else // polyhedron cell
    {
      foundPolyhedronCell = true;
      // read numberOfFaces in a cell
      svtkIdType currLoc = oldOffsetPtr[i];
      svtkIdType numberOfCellFaces = facesPtr[currLoc];
      currLoc += 1;
      for (svtkIdType j = 0; j < numberOfCellFaces; j++)
      {
        // read numberOfPoints in a face
        svtkIdType numberOfFacePoints = facesPtr[currLoc];
        currLoc += numberOfFacePoints + 1;
      }
      newOffsetPtr[i] = currLoc;
    }
  }

  if (!foundPolyhedronCell)
  {
    this->Faces->SetNumberOfTuples(0);
    this->FaceOffsets->SetNumberOfTuples(0);
  }
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLUnstructuredDataWriter::GetNumberOfInputPoints()
{
  svtkPointSet* input = this->GetInputAsPointSet();
  svtkPoints* points = input->GetPoints();
  return points ? points->GetNumberOfPoints() : 0;
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::CalculateDataFractions(float* fractions)
{
  // Calculate the fraction of point/cell data and point
  // specifications contributed by each component.
  svtkPointSet* input = this->GetInputAsPointSet();
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  svtkIdType pdSize = pdArrays * this->GetNumberOfInputPoints();
  svtkIdType cdSize = cdArrays * this->GetNumberOfInputCells();
  int total = (pdSize + cdSize + this->GetNumberOfInputPoints());
  if (total == 0)
  {
    total = 1;
  }
  fractions[0] = 0;
  fractions[1] = float(pdSize) / total;
  fractions[2] = float(pdSize + cdSize) / total;
  fractions[3] = 1;
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::CalculateCellFractions(float* fractions, svtkIdType typesSize)
{
  // Calculate the fraction of cell specification data contributed by
  // each of the connectivity, offset, and type arrays.
  svtkIdType connectSize = this->CellPoints ? this->CellPoints->GetNumberOfTuples() : 0;
  svtkIdType offsetSize = this->CellOffsets ? this->CellOffsets->GetNumberOfTuples() : 0;
  svtkIdType faceSize = this->Faces ? this->Faces->GetNumberOfTuples() : 0;
  svtkIdType faceoffsetSize = this->FaceOffsets ? this->FaceOffsets->GetNumberOfTuples() : 0;
  svtkIdType total = connectSize + offsetSize + faceSize + faceoffsetSize + typesSize;
  if (total == 0)
  {
    total = 1;
  }
  fractions[0] = 0;
  fractions[1] = float(connectSize) / total;
  fractions[2] = float(connectSize + offsetSize) / total;
  fractions[3] = float(connectSize + offsetSize + faceSize) / total;
  fractions[4] = float(connectSize + offsetSize + faceSize + faceoffsetSize) / total;
  fractions[5] = 1;
}

//----------------------------------------------------------------------------
void svtkXMLUnstructuredDataWriter::SetInputUpdateExtent(int piece, int numPieces, int ghostLevel)
{
  svtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
}
