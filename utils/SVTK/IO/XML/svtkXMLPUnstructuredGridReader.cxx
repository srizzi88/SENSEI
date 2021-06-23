/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPUnstructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPUnstructuredGridReader.h"

#include "svtkCellArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLUnstructuredGridReader.h"

svtkStandardNewMacro(svtkXMLPUnstructuredGridReader);

//----------------------------------------------------------------------------
svtkXMLPUnstructuredGridReader::svtkXMLPUnstructuredGridReader() = default;

//----------------------------------------------------------------------------
svtkXMLPUnstructuredGridReader::~svtkXMLPUnstructuredGridReader() = default;

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredGridReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkXMLPUnstructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkXMLPUnstructuredGridReader::GetOutput(int idx)
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* svtkXMLPUnstructuredGridReader::GetDataSetName()
{
  return "PUnstructuredGrid";
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredGridReader::GetOutputUpdateExtent(
  int& piece, int& numberOfPieces, int& ghostLevel)
{
  svtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredGridReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  this->TotalNumberOfCells = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->TotalNumberOfCells += this->PieceReaders[i]->GetNumberOfCells();
    }
  }

  // Data reading will start at the beginning of the output.
  this->StartCell = 0;
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  svtkUnstructuredGrid* output = svtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Setup the output's cell arrays.
  svtkUnsignedCharArray* cellTypes = svtkUnsignedCharArray::New();
  cellTypes->SetNumberOfTuples(this->GetNumberOfCells());
  svtkCellArray* outCells = svtkCellArray::New();

  output->SetCells(cellTypes, outCells);

  outCells->Delete();
  cellTypes->Delete();
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredGridReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  if (this->PieceReaders[this->Piece])
  {
    this->StartCell += this->PieceReaders[this->Piece]->GetNumberOfCells();
  }
}

//----------------------------------------------------------------------------
int svtkXMLPUnstructuredGridReader::ReadPieceData()
{
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  svtkPointSet* ips = this->GetPieceInputAsPointSet(this->Piece);
  svtkUnstructuredGrid* input = static_cast<svtkUnstructuredGrid*>(ips);
  svtkUnstructuredGrid* output = svtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Copy the Cells.
  this->CopyCellArray(this->TotalNumberOfCells, input->GetCells(), output->GetCells());

  // Copy Faces and FaceLocations with offset adjustment if they exist
  if (svtkIdTypeArray* inputFaces = input->GetFaces())
  {
    svtkIdTypeArray* inputFaceLocations = input->GetFaceLocations();
    svtkIdTypeArray* outputFaces = output->GetFaces();
    if (!outputFaces)
    {
      output->InitializeFacesRepresentation(0);
      outputFaces = output->GetFaces();
    }
    svtkIdTypeArray* outputFaceLocations = output->GetFaceLocations();
    const svtkIdType numFaceLocs = inputFaceLocations->GetNumberOfValues();
    for (svtkIdType i = 0; i < numFaceLocs; ++i)
    {
      outputFaceLocations->InsertNextValue(outputFaces->GetMaxId() + 1);
      svtkIdType location = inputFaceLocations->GetValue(i);
      if (location < 0) // the face offsets array contains -1 for regular cells
      {
        continue;
      }

      svtkIdType numFaces = inputFaces->GetValue(location);
      location++;
      outputFaces->InsertNextValue(numFaces);
      for (svtkIdType f = 0; f < numFaces; f++)
      {
        svtkIdType numPoints = inputFaces->GetValue(location);
        outputFaces->InsertNextValue(numPoints);
        location++;
        for (svtkIdType p = 0; p < numPoints; p++)
        {
          // only the point ids get the offset
          outputFaces->InsertNextValue(inputFaces->GetValue(location) + this->StartPoint);
          location++;
        }
      }
    }
  }

  // Copy the corresponding cell types.
  svtkUnsignedCharArray* inTypes = input->GetCellTypesArray();
  svtkUnsignedCharArray* outTypes = output->GetCellTypesArray();
  svtkIdType components = outTypes->GetNumberOfComponents();
  memcpy(outTypes->GetVoidPointer(this->StartCell * components), inTypes->GetVoidPointer(0),
    inTypes->GetNumberOfTuples() * components * inTypes->GetDataTypeSize());

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredGridReader::CopyArrayForCells(svtkDataArray* inArray, svtkDataArray* outArray)
{
  if (!this->PieceReaders[this->Piece])
  {
    return;
  }
  if (!inArray || !outArray)
  {
    return;
  }

  svtkIdType numCells = this->PieceReaders[this->Piece]->GetNumberOfCells();
  svtkIdType components = outArray->GetNumberOfComponents();
  svtkIdType tupleSize = inArray->GetDataTypeSize() * components;
  memcpy(outArray->GetVoidPointer(this->StartCell * components), inArray->GetVoidPointer(0),
    numCells * tupleSize);
}

//----------------------------------------------------------------------------
svtkXMLDataReader* svtkXMLPUnstructuredGridReader::CreatePieceReader()
{
  return svtkXMLUnstructuredGridReader::New();
}

//----------------------------------------------------------------------------
int svtkXMLPUnstructuredGridReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredGridReader::SqueezeOutputArrays(svtkDataObject* output)
{
  svtkUnstructuredGrid* grid = svtkUnstructuredGrid::SafeDownCast(output);
  if (svtkIdTypeArray* outputFaces = grid->GetFaces())
  {
    outputFaces->Squeeze();
  }
  if (svtkIdTypeArray* outputFaceLocations = grid->GetFaceLocations())
  {
    outputFaceLocations->Squeeze();
  }
}
