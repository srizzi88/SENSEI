/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPUnstructuredDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPUnstructuredDataReader.h"
#include "svtkCellArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointSet.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLUnstructuredDataReader.h"

//----------------------------------------------------------------------------
svtkXMLPUnstructuredDataReader::svtkXMLPUnstructuredDataReader()
{
  this->TotalNumberOfPoints = 0;
  this->TotalNumberOfCells = 0;
}

//----------------------------------------------------------------------------
svtkXMLPUnstructuredDataReader::~svtkXMLPUnstructuredDataReader() = default;

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkPointSet* svtkXMLPUnstructuredDataReader::GetOutputAsPointSet()
{
  return svtkPointSet::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
svtkPointSet* svtkXMLPUnstructuredDataReader::GetPieceInputAsPointSet(int piece)
{
  svtkXMLDataReader* reader = this->PieceReaders[piece];
  if (!reader)
  {
    return nullptr;
  }
  if (reader->GetNumberOfOutputPorts() < 1)
  {
    return nullptr;
  }
  return static_cast<svtkPointSet*>(reader->GetExecutive()->GetOutputData(0));
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::SetupOutputTotals()
{
  this->TotalNumberOfPoints = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->TotalNumberOfPoints += this->PieceReaders[i]->GetNumberOfPoints();
    }
  }
  this->StartPoint = 0;
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::SetupNextPiece()
{
  if (this->PieceReaders[this->Piece])
  {
    this->StartPoint += this->PieceReaders[this->Piece]->GetNumberOfPoints();
  }
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPUnstructuredDataReader::GetNumberOfPoints()
{
  return this->TotalNumberOfPoints;
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPUnstructuredDataReader::GetNumberOfCells()
{
  return this->TotalNumberOfCells;
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPUnstructuredDataReader::GetNumberOfPointsInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    return this->PieceReaders[piece]->GetNumberOfPoints();
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPUnstructuredDataReader::GetNumberOfCellsInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    return this->PieceReaders[piece]->GetNumberOfCells();
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void svtkXMLPUnstructuredDataReader::SetupOutputInformation(svtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::CopyOutputInformation(svtkInformation* outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);

  svtkInformation* localInfo = this->GetExecutive()->GetOutputInformation(port);
  if (localInfo->Has(CAN_HANDLE_PIECE_REQUEST()))
  {
    outInfo->CopyEntry(localInfo, CAN_HANDLE_PIECE_REQUEST());
  }
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  // Create the points array.
  svtkPoints* points = svtkPoints::New();
  if (this->PPointsElement)
  {
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
  svtkPointSet::SafeDownCast(this->GetCurrentOutput())->SetPoints(points);
  points->Delete();
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel)
{
  this->UpdatePieceId = piece;
  this->UpdateNumberOfPieces = numberOfPieces;
  this->UpdateGhostLevel = ghostLevel;

  // If more pieces are requested than available, just return empty
  // pieces for the extra ones.
  if (this->UpdateNumberOfPieces > this->NumberOfPieces)
  {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
  }

  // Find the range of pieces to read.
  if (this->UpdatePieceId < this->UpdateNumberOfPieces)
  {
    this->StartPiece = ((this->UpdatePieceId * this->NumberOfPieces) / this->UpdateNumberOfPieces);
    this->EndPiece =
      (((this->UpdatePieceId + 1) * this->NumberOfPieces) / this->UpdateNumberOfPieces);
  }
  else
  {
    this->StartPiece = 0;
    this->EndPiece = 0;
  }

  // Update the information of the pieces we need.
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    if (this->CanReadPiece(i))
    {
      this->PieceReaders[i]->UpdateInformation();
      svtkXMLUnstructuredDataReader* pReader =
        static_cast<svtkXMLUnstructuredDataReader*>(this->PieceReaders[i]);
      pReader->SetupUpdateExtent(0, 1, this->UpdateGhostLevel);
    }
  }

  // Find the total size of the output.
  this->SetupOutputTotals();
}

//----------------------------------------------------------------------------
int svtkXMLPUnstructuredDataReader::ReadPrimaryElement(svtkXMLDataElement* ePri)
{
  if (!this->Superclass::ReadPrimaryElement(ePri))
  {
    return 0;
  }

  // Find the PPoints element.
  this->PPointsElement = nullptr;
  int numNested = ePri->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
  {
    svtkXMLDataElement* eNested = ePri->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "PPoints") == 0) && (eNested->GetNumberOfNestedElements() == 1))
    {
      this->PPointsElement = eNested;
    }
  }

  // If PPoints element was not found, we must assume there are 0
  // points.  If there are found to be points later, the error will be
  // reported by ReadPieceData.

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::ReadXMLData()
{
  // Get the update request.
  svtkInformation* outInfo = this->GetCurrentOutputInformation();
  int piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numberOfPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  svtkDebugMacro(
    "Updating piece " << piece << " of " << numberOfPieces << " with ghost level " << ghostLevel);

  // Setup the range of pieces that will be read.
  this->SetupUpdateExtent(piece, numberOfPieces, ghostLevel);

  // If there are no data to read, stop now.
  if (this->StartPiece == this->EndPiece)
  {
    return;
  }

  svtkDebugMacro(
    "Reading piece range [" << this->StartPiece << ", " << this->EndPiece << ") from file.");

  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();

  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  float* fractions = new float[this->EndPiece - this->StartPiece + 1];
  fractions[0] = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    int index = i - this->StartPiece;
    fractions[index + 1] =
      (fractions[index] + this->GetNumberOfPointsInPiece(i) + this->GetNumberOfCellsInPiece(i));
  }
  if (fractions[this->EndPiece - this->StartPiece] == 0)
  {
    fractions[this->EndPiece - this->StartPiece] = 1;
  }
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    int index = i - this->StartPiece;
    fractions[index + 1] = fractions[index + 1] / fractions[this->EndPiece - this->StartPiece];
  }

  // Read the data needed from each piece.
  for (int i = this->StartPiece; (i < this->EndPiece && !this->AbortExecute && !this->DataError);
       ++i)
  {
    // Set the range of progress for this piece.
    this->SetProgressRange(progressRange, i - this->StartPiece, fractions);

    if (!this->Superclass::ReadPieceData(i))
    {
      // An error occurred while reading the piece.
      this->DataError = 1;
    }
    this->SetupNextPiece();
  }

  delete[] fractions;
}

//----------------------------------------------------------------------------
int svtkXMLPUnstructuredDataReader::ReadPieceData()
{
  // Use the internal reader to read the piece.
  this->PieceReaders[this->Piece]->UpdatePiece(0, 1, this->UpdateGhostLevel);

  svtkPointSet* input = this->GetPieceInputAsPointSet(this->Piece);
  svtkPointSet* output = svtkPointSet::SafeDownCast(this->GetCurrentOutput());

  // If there are some points, but no PPoints element, report the
  // error.
  if (!this->PPointsElement && (this->GetNumberOfPoints() > 0))
  {
    svtkErrorMacro("Could not find PPoints element with 1 array.");
    return 0;
  }

  if (!input->GetPoints())
  {
    return 0;
  }

  // Copy the points array.
  this->CopyArrayForPoints(input->GetPoints()->GetData(), output->GetPoints()->GetData());

  // Let the superclass read the data it wants.
  return this->Superclass::ReadPieceData();
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::CopyArrayForPoints(
  svtkDataArray* inArray, svtkDataArray* outArray)
{
  if (!this->PieceReaders[this->Piece])
  {
    return;
  }
  if (!inArray || !outArray)
  {
    return;
  }

  svtkIdType numPoints = this->PieceReaders[this->Piece]->GetNumberOfPoints();
  svtkIdType components = outArray->GetNumberOfComponents();
  svtkIdType tupleSize = inArray->GetDataTypeSize() * components;
  memcpy(outArray->GetVoidPointer(this->StartPoint * components), inArray->GetVoidPointer(0),
    numPoints * tupleSize);
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataReader::CopyCellArray(
  svtkIdType /*totalNumberOfCells*/, svtkCellArray* inCells, svtkCellArray* outCells)
{
  outCells->Append(inCells, this->StartPoint);
}

//----------------------------------------------------------------------------
int svtkXMLPUnstructuredDataReader::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}
