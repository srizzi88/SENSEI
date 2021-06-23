/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPPolyDataReader.h"
#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLPolyDataReader.h"

svtkStandardNewMacro(svtkXMLPPolyDataReader);

//----------------------------------------------------------------------------
svtkXMLPPolyDataReader::svtkXMLPPolyDataReader() = default;

//----------------------------------------------------------------------------
svtkXMLPPolyDataReader::~svtkXMLPPolyDataReader() = default;

//----------------------------------------------------------------------------
void svtkXMLPPolyDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkXMLPPolyDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkXMLPPolyDataReader::GetOutput(int idx)
{
  return svtkPolyData::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* svtkXMLPPolyDataReader::GetDataSetName()
{
  return "PPolyData";
}

//----------------------------------------------------------------------------
void svtkXMLPPolyDataReader::GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel)
{
  svtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPPolyDataReader::GetNumberOfCellsInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    return this->PieceReaders[piece]->GetNumberOfCells();
  }
  return 0;
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPPolyDataReader::GetNumberOfVertsInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    svtkXMLPolyDataReader* pReader = static_cast<svtkXMLPolyDataReader*>(this->PieceReaders[piece]);
    return pReader->GetNumberOfVerts();
  }
  return 0;
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPPolyDataReader::GetNumberOfLinesInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    svtkXMLPolyDataReader* pReader = static_cast<svtkXMLPolyDataReader*>(this->PieceReaders[piece]);
    return pReader->GetNumberOfLines();
  }
  return 0;
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPPolyDataReader::GetNumberOfStripsInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    svtkXMLPolyDataReader* pReader = static_cast<svtkXMLPolyDataReader*>(this->PieceReaders[piece]);
    return pReader->GetNumberOfStrips();
  }
  return 0;
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLPPolyDataReader::GetNumberOfPolysInPiece(int piece)
{
  if (this->PieceReaders[piece])
  {
    svtkXMLPolyDataReader* pReader = static_cast<svtkXMLPolyDataReader*>(this->PieceReaders[piece]);
    return pReader->GetNumberOfPolys();
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkXMLPPolyDataReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  this->TotalNumberOfCells = 0;
  this->TotalNumberOfVerts = 0;
  this->TotalNumberOfLines = 0;
  this->TotalNumberOfStrips = 0;
  this->TotalNumberOfPolys = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    this->TotalNumberOfCells += this->GetNumberOfCellsInPiece(i);
    this->TotalNumberOfVerts += this->GetNumberOfVertsInPiece(i);
    this->TotalNumberOfLines += this->GetNumberOfLinesInPiece(i);
    this->TotalNumberOfStrips += this->GetNumberOfStripsInPiece(i);
    this->TotalNumberOfPolys += this->GetNumberOfPolysInPiece(i);
  }

  // Data reading will start at the beginning of the output.
  this->StartVert = 0;
  this->StartLine = 0;
  this->StartStrip = 0;
  this->StartPoly = 0;
}

//----------------------------------------------------------------------------
void svtkXMLPPolyDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  svtkPolyData* output = svtkPolyData::SafeDownCast(this->GetCurrentOutput());

  // Setup the output's cell arrays.
  svtkCellArray* outVerts = svtkCellArray::New();
  svtkCellArray* outLines = svtkCellArray::New();
  svtkCellArray* outStrips = svtkCellArray::New();
  svtkCellArray* outPolys = svtkCellArray::New();

  output->SetVerts(outVerts);
  output->SetLines(outLines);
  output->SetStrips(outStrips);
  output->SetPolys(outPolys);

  outPolys->Delete();
  outStrips->Delete();
  outLines->Delete();
  outVerts->Delete();
}

//----------------------------------------------------------------------------
void svtkXMLPPolyDataReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  this->StartVert += this->GetNumberOfVertsInPiece(this->Piece);
  this->StartLine += this->GetNumberOfLinesInPiece(this->Piece);
  this->StartStrip += this->GetNumberOfStripsInPiece(this->Piece);
  this->StartPoly += this->GetNumberOfPolysInPiece(this->Piece);
}

//----------------------------------------------------------------------------
int svtkXMLPPolyDataReader::ReadPieceData()
{
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  svtkPointSet* ips = this->GetPieceInputAsPointSet(this->Piece);
  svtkPolyData* input = static_cast<svtkPolyData*>(ips);
  svtkPolyData* output = svtkPolyData::SafeDownCast(this->GetCurrentOutput());

  // Copy the Verts.
  this->CopyCellArray(this->TotalNumberOfVerts, input->GetVerts(), output->GetVerts());

  // Copy the Lines.
  this->CopyCellArray(this->TotalNumberOfLines, input->GetLines(), output->GetLines());

  // Copy the Strips.
  this->CopyCellArray(this->TotalNumberOfStrips, input->GetStrips(), output->GetStrips());

  // Copy the Polys.
  this->CopyCellArray(this->TotalNumberOfPolys, input->GetPolys(), output->GetPolys());

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLPPolyDataReader::CopyArrayForCells(svtkDataArray* inArray, svtkDataArray* outArray)
{
  if (!this->PieceReaders[this->Piece])
  {
    return;
  }
  if (inArray == nullptr || outArray == nullptr)
  {
    return;
  }

  svtkIdType components = outArray->GetNumberOfComponents();
  svtkIdType tupleSize = inArray->GetDataTypeSize() * components;

  // Copy the cell data for the Verts in the piece.
  svtkIdType inStartCell = 0;
  svtkIdType outStartCell = this->StartVert;
  svtkIdType numCells = this->GetNumberOfVertsInPiece(this->Piece);
  memcpy(outArray->GetVoidPointer(outStartCell * components),
    inArray->GetVoidPointer(inStartCell * components), numCells * tupleSize);

  // Copy the cell data for the Lines in the piece.
  inStartCell += numCells;
  outStartCell = this->TotalNumberOfVerts + this->StartLine;
  numCells = this->GetNumberOfLinesInPiece(this->Piece);
  memcpy(outArray->GetVoidPointer(outStartCell * components),
    inArray->GetVoidPointer(inStartCell * components), numCells * tupleSize);

  // Copy the cell data for the Strips in the piece.
  inStartCell += numCells;
  outStartCell = (this->TotalNumberOfVerts + this->TotalNumberOfLines + this->StartStrip);
  numCells = this->GetNumberOfStripsInPiece(this->Piece);
  memcpy(outArray->GetVoidPointer(outStartCell * components),
    inArray->GetVoidPointer(inStartCell * components), numCells * tupleSize);

  // Copy the cell data for the Polys in the piece.
  inStartCell += numCells;
  outStartCell = (this->TotalNumberOfVerts + this->TotalNumberOfLines + this->TotalNumberOfStrips +
    this->StartPoly);
  numCells = this->GetNumberOfPolysInPiece(this->Piece);
  memcpy(outArray->GetVoidPointer(outStartCell * components),
    inArray->GetVoidPointer(inStartCell * components), numCells * tupleSize);
}

//----------------------------------------------------------------------------
svtkXMLDataReader* svtkXMLPPolyDataReader::CreatePieceReader()
{
  return svtkXMLPolyDataReader::New();
}

//----------------------------------------------------------------------------
int svtkXMLPPolyDataReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
  return 1;
}
