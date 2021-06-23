/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPDataWriter.h"

#include "svtkCallbackCommand.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkErrorCode.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <svtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
svtkXMLPDataWriter::svtkXMLPDataWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPDataWriter::~svtkXMLPDataWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkXMLPDataWriter::WritePData(svtkIndent indent)
{
  svtkDataSet* input = this->GetInputAsDataSet();

  // We want to avoid using appended data mode as it
  // is not supported in meta formats.
  int dataMode = this->DataMode;
  if (dataMode == svtkXMLWriter::Appended)
  {
    this->DataMode = svtkXMLWriter::Binary;
  }

  svtkFieldData* fieldData = input->GetFieldData();

  svtkInformation* meta = input->GetInformation();
  bool hasTime = meta->Has(svtkDataObject::DATA_TIME_STEP()) ? true : false;
  if ((fieldData && fieldData->GetNumberOfArrays()) || hasTime)
  {
    svtkNew<svtkFieldData> fieldDataCopy;
    fieldDataCopy->ShallowCopy(fieldData);
    if (hasTime)
    {
      svtkNew<svtkDoubleArray> time;
      time->SetNumberOfTuples(1);
      time->SetTypedComponent(0, 0, meta->Get(svtkDataObject::DATA_TIME_STEP()));
      time->SetName("TimeValue");
      fieldDataCopy->AddArray(time);
    }
    this->WriteFieldDataInline(fieldDataCopy, indent);
  }

  this->DataMode = dataMode;

  this->WritePPointData(input->GetPointData(), indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->WritePCellData(input->GetCellData(), indent);
}

//----------------------------------------------------------------------------
int svtkXMLPDataWriter::WritePieceInternal()
{
  int piece = this->GetCurrentPiece();

  svtkDataSet* inputDS = this->GetInputAsDataSet();
  if (inputDS && (inputDS->GetNumberOfPoints() > 0 || inputDS->GetNumberOfCells() > 0))
  {
    if (!this->WritePiece(piece))
    {
      svtkErrorMacro("Ran out of disk space; deleting file(s) already written");
      this->DeleteFiles();
      return 0;
    }
    this->PieceWrittenFlags[piece] = static_cast<unsigned char>(0x1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLPDataWriter::WritePiece(int index)
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
void svtkXMLPDataWriter::WritePrimaryElementAttributes(
  std::ostream& svtkNotUsed(os), svtkIndent svtkNotUsed(indent))
{
  this->WriteScalarAttribute("GhostLevel", this->GhostLevel);
}

//----------------------------------------------------------------------------
void svtkXMLPDataWriter::SetupPieceFileNameExtension()
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
