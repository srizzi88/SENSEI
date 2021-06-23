/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPTableWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPTableWriter.h"

#include "svtkCallbackCommand.h"
#include "svtkDataSetAttributes.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"
#include "svtkXMLTableWriter.h"

#include <svtksys/SystemTools.hxx>

#include <cassert>

svtkStandardNewMacro(svtkXMLPTableWriter);

//----------------------------------------------------------------------------
svtkXMLPTableWriter::svtkXMLPTableWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPTableWriter::~svtkXMLPTableWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPTableWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkTable* svtkXMLPTableWriter::GetInput()
{
  return svtkTable::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLPTableWriter::GetDataSetName()
{
  return "PTable";
}

//----------------------------------------------------------------------------
const char* svtkXMLPTableWriter::GetDefaultFileExtension()
{
  return "pvtt";
}

//----------------------------------------------------------------------------
svtkXMLWriter* svtkXMLPTableWriter::CreatePieceWriter(int index)
{
  // Create the writer for the piece.
  svtkXMLTableWriter* pWriter = svtkXMLTableWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  pWriter->SetNumberOfPieces(this->NumberOfPieces);
  pWriter->SetWritePiece(index);

  return pWriter;
}

//----------------------------------------------------------------------------
void svtkXMLPTableWriter::WritePData(svtkIndent indent)
{
  svtkTable* input = this->GetInput();
  this->WritePRowData(input->GetRowData(), indent);
}

//----------------------------------------------------------------------------
int svtkXMLPTableWriter::WritePieceInternal()
{
  int piece = this->GetCurrentPiece();
  svtkTable* inputTable = this->GetInput();
  if (inputTable && inputTable->GetNumberOfRows() > 0)
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
int svtkXMLPTableWriter::WritePiece(int index)
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
void svtkXMLPTableWriter::WritePRowData(svtkDataSetAttributes* ds, svtkIndent indent)
{
  if (ds->GetNumberOfArrays() == 0)
  {
    return;
  }
  ostream& os = *this->Stream;
  char** names = this->CreateStringArray(ds->GetNumberOfArrays());

  os << indent << "<PRowData";
  this->WriteAttributeIndices(ds, names);
  if (this->ErrorCode != svtkErrorCode::NoError)
  {
    this->DestroyStringArray(ds->GetNumberOfArrays(), names);
    return;
  }
  os << ">\n";

  for (int i = 0; i < ds->GetNumberOfArrays(); ++i)
  {
    this->WritePArray(ds->GetAbstractArray(i), indent.GetNextIndent(), names[i]);
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      this->DestroyStringArray(ds->GetNumberOfArrays(), names);
      return;
    }
  }

  os << indent << "</PRowData>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }

  this->DestroyStringArray(ds->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void svtkXMLPTableWriter::SetupPieceFileNameExtension()
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
int svtkXMLPTableWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}
