/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPHyperTreeGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPHyperTreeGridWriter.h"

#include "svtkCallbackCommand.h"
#include "svtkErrorCode.h"
#include "svtkHyperTreeGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkXMLHyperTreeGridWriter.h"

#include <svtksys/SystemTools.hxx>

#include <cassert>

svtkStandardNewMacro(svtkXMLPHyperTreeGridWriter);

//----------------------------------------------------------------------------
svtkXMLPHyperTreeGridWriter::svtkXMLPHyperTreeGridWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPHyperTreeGridWriter::~svtkXMLPHyperTreeGridWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPHyperTreeGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkHyperTreeGrid* svtkXMLPHyperTreeGridWriter::GetInput()
{
  return svtkHyperTreeGrid::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLPHyperTreeGridWriter::GetDataSetName()
{
  return "PHyperTreeGrid";
}

//----------------------------------------------------------------------------
const char* svtkXMLPHyperTreeGridWriter::GetDefaultFileExtension()
{
  return "phtg";
}

//----------------------------------------------------------------------------
svtkXMLHyperTreeGridWriter* svtkXMLPHyperTreeGridWriter::CreateHyperTreeGridPieceWriter(
  int svtkNotUsed(index))
{
  // Create the writer for the piece.
  svtkXMLHyperTreeGridWriter* pWriter = svtkXMLHyperTreeGridWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//----------------------------------------------------------------------------
svtkXMLWriter* svtkXMLPHyperTreeGridWriter::CreatePieceWriter(int index)
{
  // Create the writer for the piece.
  svtkXMLHyperTreeGridWriter* pWriter = this->CreateHyperTreeGridPieceWriter(index);
  return pWriter;
}

//----------------------------------------------------------------------------
void svtkXMLPHyperTreeGridWriter::WritePData(svtkIndent svtkNotUsed(indent)) {}

//----------------------------------------------------------------------------
int svtkXMLPHyperTreeGridWriter::WritePieceInternal()
{
  int piece = this->GetCurrentPiece();
  svtkHyperTreeGrid* inputHyperTreeGrid = this->GetInput();

  if (inputHyperTreeGrid)
  {
    if (!this->WritePiece(piece))
    {
      svtkErrorMacro("Could not write the current piece.");
      this->DeleteFiles();
      return 0;
    }
    this->PieceWrittenFlags[piece] = static_cast<unsigned char>(0x1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLPHyperTreeGridWriter::WritePiece(int index)
{
  // Create the writer for the piece.  Its configuration should match
  // our own writer.
  svtkXMLWriter* pWriter = this->CreatePieceWriter(index);
  pWriter->AddObserver(svtkCommand::ProgressEvent, this->InternalProgressObserver);

  char* fileName = this->CreatePieceFileName(index, this->PathName);
  std::string path = svtksys::SystemTools::GetParentDirectory(fileName);
  if (!path.empty() && !svtksys::SystemTools::PathExists(path))
  {
    svtksys::SystemTools::MakeDirectory(path);
  }
  pWriter->SetFileName(fileName);
  delete[] fileName;

  // Copy the writer settings.
  pWriter->SetDebug(this->Debug);
  pWriter->SetCompressor(this->Compressor);
  pWriter->SetDataMode(this->DataMode);
  pWriter->SetByteOrder(this->ByteOrder);
  pWriter->SetEncodeAppendedData(this->EncodeAppendedData);
  pWriter->SetHeaderType(this->HeaderType);
  pWriter->SetBlockSize(this->BlockSize);

  // Write the piece.
  int result = pWriter->Write();
  this->SetErrorCode(pWriter->GetErrorCode());

  // Cleanup.
  pWriter->RemoveObserver(this->InternalProgressObserver);
  pWriter->Delete();

  return result;
}

//----------------------------------------------------------------------------
void svtkXMLPHyperTreeGridWriter::SetupPieceFileNameExtension()
{
  this->Superclass::SetupPieceFileNameExtension();

  // Create a temporary piece writer and then initialize the extension.
  svtkXMLWriter* writer = this->CreatePieceWriter(0);
  const char* ext = writer->GetDefaultFileExtension();
  this->PieceFileNameExtension = new char[strlen(ext) + 2];
  this->PieceFileNameExtension[0] = '.';
  strcpy(this->PieceFileNameExtension + 1, ext);
  writer->Delete();
}

//----------------------------------------------------------------------------
int svtkXMLPHyperTreeGridWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkHyperTreeGrid");
  return 1;
}
