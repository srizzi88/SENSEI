/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPDataObjectWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPDataObjectWriter.h"

#include "svtkCallbackCommand.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <svtksys/SystemTools.hxx>

#include <cassert>

svtkCxxSetObjectMacro(svtkXMLPDataObjectWriter, Controller, svtkMultiProcessController);
//----------------------------------------------------------------------------
svtkXMLPDataObjectWriter::svtkXMLPDataObjectWriter()
{
  this->StartPiece = 0;
  this->EndPiece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;
  this->WriteSummaryFile = 1;

  this->UseSubdirectory = false;

  this->PathName = nullptr;
  this->FileNameBase = nullptr;
  this->FileNameExtension = nullptr;
  this->PieceFileNameExtension = nullptr;

  // Setup a callback for the internal writer to report progress.
  this->InternalProgressObserver = svtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(&svtkXMLPDataObjectWriter::ProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);

  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());

  this->ContinuingExecution = false;
  this->CurrentPiece = -1;
  this->PieceWrittenFlags = nullptr;
}

//----------------------------------------------------------------------------
svtkXMLPDataObjectWriter::~svtkXMLPDataObjectWriter()
{
  delete[] this->PathName;
  delete[] this->FileNameBase;
  delete[] this->FileNameExtension;
  delete[] this->PieceFileNameExtension;
  delete[] this->PieceWrittenFlags;
  this->SetController(nullptr);
  this->InternalProgressObserver->Delete();
}

//----------------------------------------------------------------------------
void svtkXMLPDataObjectWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
  os << indent << "StartPiece: " << this->StartPiece << "\n";
  os << indent << "EndPiece: " << this->EndPiece << "\n";
  os << indent << "GhostLevel: " << this->GhostLevel << "\n";
  os << indent << "UseSubdirectory: " << this->UseSubdirectory << "\n";
  os << indent << "WriteSummaryFile: " << this->WriteSummaryFile << "\n";
}

//----------------------------------------------------------------------------
svtkTypeBool svtkXMLPDataObjectWriter::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  svtkTypeBool retVal = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    if (retVal && this->ContinuingExecution)
    {
      request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
    else
    {
      request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->ContinuingExecution = false;
    }
  }
  return retVal;
}

//----------------------------------------------------------------------------
void svtkXMLPDataObjectWriter::SetWriteSummaryFile(int flag)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting WriteSummaryFile to "
                << flag);
  if (this->WriteSummaryFile != flag)
  {
    this->WriteSummaryFile = flag;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkXMLPDataObjectWriter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  int piece = 0;
  if (this->ContinuingExecution)
  {
    assert(this->CurrentPiece >= this->StartPiece && this->CurrentPiece <= this->EndPiece &&
      this->CurrentPiece < this->NumberOfPieces);
    piece = this->CurrentPiece;
  }
  else
  {
    piece = this->StartPiece;
  }

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->GetNumberOfPieces());
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLPDataObjectWriter::WriteInternal()
{
  bool beginning = this->ContinuingExecution == false;
  bool end = true;

  this->ContinuingExecution = false;
  this->CurrentPiece = beginning ? this->StartPiece : this->CurrentPiece;

  assert(this->CurrentPiece >= this->StartPiece && this->CurrentPiece <= this->EndPiece);
  end = this->CurrentPiece == this->EndPiece;

  if (beginning)
  {
    // Prepare the file name.
    this->SplitFileName();
    delete[] this->PieceWrittenFlags;
    this->PieceWrittenFlags = new unsigned char[this->NumberOfPieces];
    memset(this->PieceWrittenFlags, 0, sizeof(unsigned char) * this->NumberOfPieces);

    // Prepare the extension.
    this->SetupPieceFileNameExtension();
  }

  // Write the current piece.

  // Split progress range by piece. Just assume all pieces are the
  // same size.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  this->SetProgressRange(
    progressRange, this->CurrentPiece - this->StartPiece, this->EndPiece - this->StartPiece + 1);

  if (!this->WritePieceInternal())
  {
    return 0;
  }

  // Write the summary file if requested.
  if (end && this->WriteSummaryFile)
  {
    // Decide whether to write the summary file.
    bool writeSummaryLocally =
      (this->Controller == nullptr || this->Controller->GetLocalProcessId() == 0);

    // Let subclasses collect information, if any to write the summary file.
    this->PrepareSummaryFile();

    if (writeSummaryLocally)
    {
      if (!this->Superclass::WriteInternal())
      {
        svtkErrorMacro("Ran out of disk space; deleting file(s) already written");
        this->DeleteFiles();
        return 0;
      }
    }
  }

  if (end == false)
  {
    this->CurrentPiece++;
    assert(this->CurrentPiece <= this->EndPiece);
    this->ContinuingExecution = true;
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLPDataObjectWriter::PrepareSummaryFile()
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    assert(this->PieceWrittenFlags != nullptr);
    // Reduce information about which pieces were written out to rank 0.
    int myId = this->Controller->GetLocalProcessId();
    unsigned char* recvBuffer = (myId == 0) ? new unsigned char[this->NumberOfPieces] : nullptr;
    this->Controller->Reduce(
      this->PieceWrittenFlags, recvBuffer, this->NumberOfPieces, svtkCommunicator::MAX_OP, 0);
    if (myId == 0)
    {
      std::swap(this->PieceWrittenFlags, recvBuffer);
    }
    delete[] recvBuffer;
  }
}

//----------------------------------------------------------------------------
int svtkXMLPDataObjectWriter::WriteData()
{
  // Write the summary file.
  ostream& os = *(this->Stream);
  svtkIndent indent = svtkIndent().GetNextIndent();
  svtkIndent nextIndent = indent.GetNextIndent();

  this->StartFile();
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }

  os << indent << "<" << this->GetDataSetName();
  this->WritePrimaryElementAttributes(os, indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }
  os << ">\n";

  // Write the information needed for a reader to produce the output's
  // information during UpdateInformation without reading a piece.
  this->WritePData(indent.GetNextIndent());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }

  // Write the elements referencing each piece and its file.
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    if (this->PieceWrittenFlags[i] == 0)
    {
      continue;
    }
    os << nextIndent << "<Piece";
    this->WritePPieceAttributes(i);
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      return 0;
    }
    os << "/>\n";
  }

  os << indent << "</" << this->GetDataSetName() << ">\n";

  this->EndFile();
  return (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError) ? 0 : 1;
}

//----------------------------------------------------------------------------
void svtkXMLPDataObjectWriter::WritePPieceAttributes(int index)
{
  char* fileName = this->CreatePieceFileName(index);
  this->WriteStringAttribute("Source", fileName);
  delete[] fileName;
}

//----------------------------------------------------------------------------
char* svtkXMLPDataObjectWriter::CreatePieceFileName(int index, const char* path)
{
  std::ostringstream s;
  if (path)
  {
    s << path;
  }
  s << this->FileNameBase;
  if (this->UseSubdirectory)
  {
    s << "/" << this->FileNameBase;
  }
  s << "_" << index;
  if (this->PieceFileNameExtension)
  {
    s << this->PieceFileNameExtension;
  }

  size_t len = s.str().length();
  char* buffer = new char[len + 1];
  strncpy(buffer, s.str().c_str(), len);
  buffer[len] = '\0';

  return buffer;
}

//----------------------------------------------------------------------------
void svtkXMLPDataObjectWriter::SplitFileName()
{
  // Split the FileName into its PathName, FileNameBase, and
  // FileNameExtension components.

  std::string pathname = svtksys::SystemTools::GetProgramPath(this->FileName);
  // Pathname may be empty if FileName is simply a filename without any leading
  // "/".
  if (!pathname.empty())
  {
    pathname += "/";
  }
  std::string filename_wo_ext = svtksys::SystemTools::GetFilenameWithoutExtension(this->FileName);
  std::string ext = svtksys::SystemTools::GetFilenameExtension(this->FileName);

  delete[] this->PathName;
  delete[] this->FileNameBase;
  delete[] this->FileNameExtension;

  this->PathName = svtksys::SystemTools::DuplicateString(pathname.c_str());
  this->FileNameBase = svtksys::SystemTools::DuplicateString(filename_wo_ext.c_str());
  this->FileNameExtension = svtksys::SystemTools::DuplicateString(ext.c_str());
}

//----------------------------------------------------------------------------
void svtkXMLPDataObjectWriter::ProgressCallbackFunction(
  svtkObject* caller, unsigned long, void* clientdata, void*)
{
  svtkAlgorithm* w = svtkAlgorithm::SafeDownCast(caller);
  if (w)
  {
    reinterpret_cast<svtkXMLPDataObjectWriter*>(clientdata)->ProgressCallback(w);
  }
}

//----------------------------------------------------------------------------
void svtkXMLPDataObjectWriter::ProgressCallback(svtkAlgorithm* w)
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  float internalProgress = w->GetProgress();
  float progress = this->ProgressRange[0] + internalProgress * width;
  this->UpdateProgressDiscrete(progress);
  if (this->AbortExecute)
  {
    w->SetAbortExecute(1);
  }
}

//----------------------------------------------------------------------------
void svtkXMLPDataObjectWriter::DeleteFiles()
{
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    char* fileName = this->CreatePieceFileName(i, this->PathName);
    this->DeleteAFile(fileName);
    delete[] fileName;
  }
}

//----------------------------------------------------------------------------
void svtkXMLPDataObjectWriter::SetupPieceFileNameExtension()
{
  delete[] this->PieceFileNameExtension;
}
