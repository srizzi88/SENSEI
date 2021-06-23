/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEnSightMasterServerReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEnSightMasterServerReader.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtksys/FStream.hxx"

#include <string>

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkEnSightMasterServerReader);

static int svtkEnSightMasterServerReaderStartsWith(const char* str1, const char* str2)
{
  if (!str1 || !str2 || strlen(str1) < strlen(str2))
  {
    return 0;
  }
  return !strncmp(str1, str2, strlen(str2));
}

//----------------------------------------------------------------------------
svtkEnSightMasterServerReader::svtkEnSightMasterServerReader()
{
  this->PieceCaseFileName = nullptr;
  this->MaxNumberOfPieces = 0;
  this->CurrentPiece = -1;
}

//----------------------------------------------------------------------------
svtkEnSightMasterServerReader::~svtkEnSightMasterServerReader()
{
  this->SetPieceCaseFileName(nullptr);
}

//----------------------------------------------------------------------------
int svtkEnSightMasterServerReader::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!this->MaxNumberOfPieces)
  {
    svtkErrorMacro("No pieces to read");
    return 0;
  }

  if (this->CurrentPiece < 0 || this->CurrentPiece >= this->MaxNumberOfPieces)
  {
    svtkErrorMacro("Current piece has to be set before reading the file");
    return 0;
  }
  if (this->DetermineFileName(this->CurrentPiece) != SVTK_OK)
  {
    svtkErrorMacro("Cannot update piece: " << this->CurrentPiece);
    return 0;
  }
  if (!this->Reader)
  {
    this->Reader = svtkGenericEnSightReader::New();
  }
  this->Reader->SetCaseFileName(this->PieceCaseFileName);
  if (!this->Reader->GetFilePath())
  {
    this->Reader->SetFilePath(this->GetFilePath());
  }
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkEnSightMasterServerReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  if (this->DetermineFileName(-1) != SVTK_OK)
  {
    svtkErrorMacro("Problem parsing the case file");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkEnSightMasterServerReader::DetermineFileName(int piece)
{
  if (!this->CaseFileName)
  {
    svtkErrorMacro("A case file name must be specified.");
    return SVTK_ERROR;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += this->CaseFileName;
    svtkDebugMacro("full path to case file: " << sfilename.c_str());
  }
  else
  {
    sfilename = this->CaseFileName;
  }

  this->IS = new svtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    svtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  char result[1024];

  int servers = 0;
  int numberservers = 0;
  int currentserver = 0;

  while (this->ReadNextDataLine(result))
  {
    if (strcmp(result, "FORMAT") == 0)
    {
      // Format
    }
    else if (strcmp(result, "SERVERS") == 0)
    {
      servers = 1;
    }
    else if (servers && svtkEnSightMasterServerReaderStartsWith(result, "number of servers:"))
    {
      sscanf(result, "number of servers: %i", &numberservers);
      if (!numberservers)
      {
        svtkErrorMacro("The case file is corrupted");
        break;
      }
    }
    else if (servers && svtkEnSightMasterServerReaderStartsWith(result, "casefile:"))
    {
      if (currentserver == piece)
      {
        char filename[SVTK_MAXPATH] = "";
        sscanf(result, "casefile: %s", filename);
        if (filename[0] == 0)
        {
          svtkErrorMacro("Problem parsing file name from: " << result);
          return SVTK_ERROR;
        }
        this->SetPieceCaseFileName(filename);
        break;
      }
      currentserver++;
    }
  }
  if (piece == -1 && currentserver != numberservers)
  {
    // cout << "Number of servers (" << numberservers
    // << ") is not equal to the actual number of servers ("
    // << currentserver << ")" << endl;
    return SVTK_ERROR;
  }

  this->MaxNumberOfPieces = numberservers;
  delete this->IS;
  this->IS = nullptr;
  return SVTK_OK;
}

//----------------------------------------------------------------------------
int svtkEnSightMasterServerReader::CanReadFile(const char* fname)
{
  // We may have to read quite a few lines of the file to do this test
  // for real.  Just check the extension.
  size_t len = strlen(fname);
  if ((len >= 4) && (strcmp(fname + len - 4, ".sos") == 0))
  {
    return 1;
  }
  else if ((len >= 5) && (strcmp(fname + len - 5, ".case") == 0))
  {
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkEnSightMasterServerReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Current piece: " << this->CurrentPiece << endl;
  os << indent
     << "Piece Case File name: " << (this->PieceCaseFileName ? this->PieceCaseFileName : "<none>")
     << endl;
  os << indent << "Maximum numbe of pieces: " << this->MaxNumberOfPieces << endl;
}
