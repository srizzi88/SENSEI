/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPDataSetWriter.h"
#include "svtkDataSet.h"
#include "svtkErrorCode.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtksys/FStream.hxx"

#include <vector>

svtkStandardNewMacro(svtkPDataSetWriter);

svtkCxxSetObjectMacro(svtkPDataSetWriter, Controller, svtkMultiProcessController);

//----------------------------------------------------------------------------
svtkPDataSetWriter::svtkPDataSetWriter()
{
  this->StartPiece = 0;
  this->EndPiece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;

  this->FilePattern = nullptr;
  this->SetFilePattern("%s.%d.svtk");
  this->UseRelativeFileNames = 1;

  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkPDataSetWriter::~svtkPDataSetWriter()
{
  this->SetFilePattern(nullptr);
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
void svtkPDataSetWriter::SetNumberOfPieces(int num)
{
  if (num == this->NumberOfPieces)
  {
    return;
  }

  this->Modified();
  this->NumberOfPieces = num;

  // Default behavior is for the single process to stream the pieces.
  this->StartPiece = 0;
  this->EndPiece = num - 1;
}

//----------------------------------------------------------------------------
int svtkPDataSetWriter::Write()
{
  int i;
  int length;

  ostream* fptr;
  svtkDataSet* input = this->GetInput();
  int inputAlgPort;
  svtkAlgorithm* inputAlg = this->GetInputAlgorithm(0, 0, inputAlgPort);

  if (this->FileName == nullptr)
  {
    svtkErrorMacro("No file name.");
    return 0;
  }

  if (this->StartPiece < 0)
  {
    this->StartPiece = 0;
  }
  if (this->NumberOfPieces < 0 || this->EndPiece < this->StartPiece)
  {
    svtkWarningMacro("No pieces to write.");
    return 1;
  }

  // Only one piece? The just write one svtk file.
  if (this->StartPiece == 0 && this->NumberOfPieces == 1)
  {
    return this->svtkDataSetWriter::Write();
  }

  // Lets compute the file root from the file name supplied by the user.
  length = static_cast<int>(strlen(this->FileName));
  std::vector<char> fileRoot(length + 1);
  size_t fileNameSize = length + strlen(this->FilePattern) + 20;
  std::vector<char> fileName(fileNameSize);
  strncpy(fileRoot.data(), this->FileName, length);
  fileRoot[length] = '\0';
  // Trim off the psvtk extension.
  if (strncmp(fileRoot.data() + length - 5, ".psvtk", 5) == 0)
  {
    fileRoot[length - 5] = '\0';
  }
  if (strncmp(fileRoot.data() + length - 4, ".svtk", 4) == 0)
  {
    fileRoot[length - 4] = '\0';
  }
  // If we are using relative file names, trim off the directory path.
  if (this->UseRelativeFileNames)
  {
    char *tmp, *slash;
    // Find the last / or \ in the file name.
    slash = nullptr;
    tmp = fileRoot.data();
    while (*tmp != '\0')
    {
      if (*tmp == '/' || *tmp == '\\')
      {
        slash = tmp;
      }
      ++tmp;
    }
    // Copy just the filename into root.
    if (slash)
    {
      ++slash;
      tmp = fileRoot.data();
      while (*slash != '\0')
      {
        *tmp++ = *slash++;
      }
      *tmp = '\0';
    }
  }

  // Restore the fileRoot to the full path.
  strncpy(fileRoot.data(), this->FileName, length);
  fileRoot[length] = '\0';
  // Trim off the psvtk extension.
  if (strncmp(fileRoot.data() + length - 5, ".psvtk", 5) == 0)
  {
    fileRoot[length - 5] = '\0';
  }
  if (strncmp(fileRoot.data() + length - 4, ".svtk", 4) == 0)
  {
    fileRoot[length - 4] = '\0';
  }

  this->UpdateInformation();

  // Now write the pieces assigned to this writer.
  svtkDataSetWriter* writer = svtkDataSetWriter::New();
  writer->SetFileTypeToBinary();
  svtkDataObject* copy;
  for (i = this->StartPiece; i <= this->EndPiece; ++i)
  {
    snprintf(fileName.data(), fileNameSize, this->FilePattern, fileRoot.data(), i);
    writer->SetFileName(fileName.data());
    inputAlg->UpdatePiece(i, this->NumberOfPieces, this->GhostLevel);

    // Store the extent of this piece in Extents. This is later used
    // to write the extents in the psvtk file.
    svtkInformation* info = input->GetInformation();
    int* ext = nullptr;
    if (info->Has(svtkDataObject::DATA_EXTENT()))
    {
      ext = input->GetInformation()->Get(svtkDataObject::DATA_EXTENT());
    }
    if (ext)
    {
      this->Extents[i] = std::vector<int>(ext, ext + 6);
    }

    copy = input->NewInstance();
    copy->ShallowCopy(input);
    // I am putting this in here because shallow copy does not copy the
    // UpdateExtentInitializedFlag, and I do not want to touch ShallowCopy
    // in ParaViews release.
    // copy->Crop(svtkStreamingDemandDrivenPipeline::GetUpdateExtent(
    //              this->GetInputInformation()));
    writer->SetInputData(svtkDataSet::SafeDownCast(copy));
    writer->Write();
    copy->Delete();
    copy = nullptr;
    if (writer->GetErrorCode() == svtkErrorCode::OutOfDiskSpaceError)
    {
      this->DeleteFiles();
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      break;
    }
  }
  writer->Delete();
  writer = nullptr;

  // Lets write the toplevel file.
  if (this->StartPiece == 0 && (!this->Controller || this->Controller->GetLocalProcessId() == 0))
  {
    fptr = this->OpenFile();
    if (fptr == nullptr)
    {
      return 0;
    }
    // Write a tag so that we know this file type.
    *fptr << "<File version=\"psvtk-1.0\"\n";
    fptr->flush();
    if (fptr->fail())
    {
      svtkErrorMacro(<< "Unable to write to file: " << this->FileName);
      this->CloseSVTKFile(fptr);
      remove(this->FileName);
      delete fptr;
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      return 0;
    }

    switch (input->GetDataObjectType())
    {
      case SVTK_POLY_DATA:
      case SVTK_UNSTRUCTURED_GRID:
        if (!this->WriteUnstructuredMetaData(
              input, fileRoot.data(), fileName.data(), fileNameSize, fptr))
        {
          this->CloseSVTKFile(fptr);
          remove(this->FileName);
          delete fptr;
          this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
          return 0;
        }
        break;
      case SVTK_IMAGE_DATA:
      case SVTK_STRUCTURED_POINTS:
        if (!this->WriteImageMetaData(
              (svtkImageData*)input, fileRoot.data(), fileName.data(), fileNameSize, fptr))
        {
          this->CloseSVTKFile(fptr);
          remove(this->FileName);
          delete fptr;
          this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
          return 0;
        }
        break;
      case SVTK_RECTILINEAR_GRID:
        if (!this->WriteRectilinearGridMetaData(
              (svtkRectilinearGrid*)input, fileRoot.data(), fileName.data(), fileNameSize, fptr))
        {
          this->CloseSVTKFile(fptr);
          remove(this->FileName);
          delete fptr;
          this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
          return 0;
        }
        break;
      case SVTK_STRUCTURED_GRID:
        if (!this->WriteStructuredGridMetaData(
              (svtkStructuredGrid*)input, fileRoot.data(), fileName.data(), fileNameSize, fptr))
        {
          this->CloseSVTKFile(fptr);
          remove(this->FileName);
          delete fptr;
          this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
          return 0;
        }
        break;
    }

    // fptr->close();
    delete fptr;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkPDataSetWriter::WriteUnstructuredMetaData(
  svtkDataSet* input, char* root, char* str, size_t strSize, ostream* fptr)
{
  int i;

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;
  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    snprintf(str, strSize, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\" />" << endl;
  }
  *fptr << "</File>" << endl;
  fptr->flush();
  if (fptr->fail())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkPDataSetWriter::WriteImageMetaData(
  svtkImageData* input, char* root, char* str, size_t strSize, ostream* fptr)
{
  int* pi;
  double* pf;
  svtkInformation* inInfo = this->GetInputInformation();

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;
  // Image data has a buch of meta data.
  *fptr << "      scalarType=\"" << input->GetScalarType() << "\"" << endl;
  pf = inInfo->Get(svtkDataObject::ORIGIN());
  *fptr << "      origin=\"" << pf[0] << " " << pf[1] << " " << pf[2] << "\"" << endl;
  pf = inInfo->Get(svtkDataObject::SPACING());
  *fptr << "      spacing=\"" << pf[0] << " " << pf[1] << " " << pf[2] << "\"" << endl;
  pi = svtkStreamingDemandDrivenPipeline::GetWholeExtent(inInfo);
  *fptr << "      wholeExtent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " " << pi[3] << " "
        << pi[4] << " " << pi[5] << "\"" << endl;

  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;

  // The code below gathers extens from all processes to write in the
  // meta-file. Note that the extent of each piece was already stored by
  // each writer. This is gathering it all to root node.
  if (this->Controller)
  {
    // Even though the logic is pretty straightforward, we need to
    // do a fair amount of work to use GatherV. Each rank simply
    // serializes its extents to 7 int blocks - piece number and 6
    // extent values. Then we gather this all to root.
    int rank = this->Controller->GetLocalProcessId();
    int nRanks = this->Controller->GetNumberOfProcesses();

    int nPiecesTotal = 0;
    svtkIdType nPieces = static_cast<svtkIdType>(this->Extents.size());

    std::vector<svtkIdType> offsets;
    std::vector<svtkIdType> nPiecesAll;
    std::vector<svtkIdType> recvLengths;
    if (rank == 0)
    {
      offsets.resize(nRanks);
      nPiecesAll.resize(nRanks);
      recvLengths.resize(nRanks);
    }
    this->Controller->Gather(&nPieces, nPiecesAll.data(), 1, 0);
    if (rank == 0)
    {
      for (int i = 0; i < nRanks; i++)
      {
        offsets[i] = nPiecesTotal * 7;
        nPiecesTotal += nPiecesAll[i];
        recvLengths[i] = nPiecesAll[i] * 7;
      }
    }
    std::vector<int> sendBuffer;
    int sendSize = nPieces * 7;
    if (nPieces > 0)
    {
      sendBuffer.resize(sendSize);
      ExtentsType::iterator iter = this->Extents.begin();
      for (int count = 0; iter != this->Extents.end(); ++iter, ++count)
      {
        sendBuffer[count * 7] = iter->first;
        memcpy(sendBuffer.data() + count * 7 + 1, &iter->second[0], 6 * sizeof(int));
      }
    }
    std::vector<int> recvBuffer;
    if (rank == 0)
    {
      recvBuffer.resize(nPiecesTotal * 7);
    }
    this->Controller->GatherV(
      sendBuffer.data(), recvBuffer.data(), sendSize, recvLengths.data(), offsets.data(), 0);

    if (rank == 0)
    {
      // Add all received values to Extents.
      // These are later written in WritePPieceAttributes()
      for (int i = 1; i < nRanks; i++)
      {
        for (int j = 0; j < nPiecesAll[i]; j++)
        {
          int* buffer = recvBuffer.data() + offsets[i] + j * 7;
          this->Extents[*buffer] = std::vector<int>(buffer + 1, buffer + 7);
        }
      }
    }
  }

  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    pi = &this->Extents[i][0];
    snprintf(str, strSize, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\"" << endl
          << "      extent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " " << pi[3] << " "
          << pi[4] << " " << pi[5] << "\" />" << endl;
  }
  *fptr << "</File>" << endl;
  fptr->flush();
  if (fptr->fail())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkPDataSetWriter::WriteRectilinearGridMetaData(
  svtkRectilinearGrid* input, char* root, char* str, size_t strSize, ostream* fptr)
{
  int i;
  int* pi;

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;

  pi = svtkStreamingDemandDrivenPipeline::GetWholeExtent(this->GetInputInformation());
  *fptr << "      wholeExtent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " " << pi[3] << " "
        << pi[4] << " " << pi[5] << "\"" << endl;

  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    pi = &this->Extents[i][0];
    snprintf(str, strSize, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\"" << endl
          << "      extent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " " << pi[3] << " "
          << pi[4] << " " << pi[5] << "\" />" << endl;
  }
  *fptr << "</File>" << endl;

  fptr->flush();
  if (fptr->fail())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkPDataSetWriter::WriteStructuredGridMetaData(
  svtkStructuredGrid* input, char* root, char* str, size_t strSize, ostream* fptr)
{
  int i;
  int* pi;

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;

  pi = svtkStreamingDemandDrivenPipeline::GetWholeExtent(this->GetInputInformation());
  *fptr << "      wholeExtent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " " << pi[3] << " "
        << pi[4] << " " << pi[5] << "\"" << endl;

  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    pi = &this->Extents[i][0];
    snprintf(str, strSize, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\"" << endl
          << "      extent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " " << pi[3] << " "
          << pi[4] << " " << pi[5] << "\" />" << endl;
  }
  *fptr << "</File>" << endl;

  fptr->flush();
  if (fptr->fail())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
// Open a svtk data file. Returns nullptr if error.
ostream* svtkPDataSetWriter::OpenFile()
{
  ostream* fptr;

  fptr = new svtksys::ofstream(this->FileName, ios::out);

  if (fptr->fail())
  {
    svtkErrorMacro(<< "Unable to open file: " << this->FileName);
    delete fptr;
    return nullptr;
  }

  return fptr;
}

void svtkPDataSetWriter::DeleteFiles()
{
  int i;
  int length = static_cast<int>(strlen(this->FileName));
  std::vector<char> fileRoot(length + 1);
  size_t fileNameSize = length + strlen(this->FilePattern) + 20;
  std::vector<char> fileName(fileNameSize);

  strncpy(fileRoot.data(), this->FileName, length);
  fileRoot[length] = '\0';
  // Trim off the psvtk extension.
  if (strncmp(fileRoot.data() + length - 5, ".psvtk", 5) == 0)
  {
    fileRoot[length - 5] = '\0';
  }
  if (strncmp(fileRoot.data() + length - 4, ".svtk", 4) == 0)
  {
    fileRoot[length - 4] = '\0';
  }
  // If we are using relative file names, trim off the directory path.
  if (this->UseRelativeFileNames)
  {
    char *tmp, *slash;
    // Find the last / or \ in the file name.
    slash = nullptr;
    tmp = fileRoot.data();
    while (*tmp != '\0')
    {
      if (*tmp == '/' || *tmp == '\\')
      {
        slash = tmp;
      }
      ++tmp;
    }
    // Copy just the filename into root.
    if (slash)
    {
      ++slash;
      tmp = fileRoot.data();
      while (*slash != '\0')
      {
        *tmp++ = *slash++;
      }
      *tmp = '\0';
    }
  }

  for (i = this->StartPiece; i <= this->EndPiece; i++)
  {
    snprintf(fileName.data(), fileNameSize, this->FilePattern, fileRoot.data(), i);
    remove(fileName.data());
  }

  remove(this->FileName);
}

//----------------------------------------------------------------------------
void svtkPDataSetWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StartPiece: " << this->StartPiece << endl;
  os << indent << "EndPiece: " << this->EndPiece << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "FilePattern: " << this->FilePattern << endl;
  os << indent << "UseRelativeFileNames: " << this->UseRelativeFileNames << endl;
}
