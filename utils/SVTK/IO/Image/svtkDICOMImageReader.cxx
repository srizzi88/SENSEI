/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDICOMImageReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDICOMImageReader.h"

#include "svtkDataArray.h"
#include "svtkDirectory.h"
#include "svtkErrorCode.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

#include <svtksys/SystemTools.hxx>

#include <string>
#include <vector>

#include "DICOMAppHelper.h"
#include "DICOMParser.h"

svtkStandardNewMacro(svtkDICOMImageReader);

class svtkDICOMImageReaderVector : public std::vector<std::string>
{
};

//----------------------------------------------------------------------------
svtkDICOMImageReader::svtkDICOMImageReader()
{
  this->Parser = new DICOMParser();
  this->AppHelper = new DICOMAppHelper();
  this->DirectoryName = nullptr;
  this->PatientName = nullptr;
  this->StudyUID = nullptr;
  this->StudyID = nullptr;
  this->TransferSyntaxUID = nullptr;
  this->DICOMFileNames = new svtkDICOMImageReaderVector();
}

//----------------------------------------------------------------------------
svtkDICOMImageReader::~svtkDICOMImageReader()
{
  delete this->Parser;
  delete this->AppHelper;
  delete this->DICOMFileNames;

  delete[] this->DirectoryName;
  delete[] this->PatientName;
  delete[] this->StudyUID;
  delete[] this->StudyID;
  delete[] this->TransferSyntaxUID;
}

//----------------------------------------------------------------------------
void svtkDICOMImageReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->DirectoryName)
  {
    os << "DirectoryName : " << this->DirectoryName << "\n";
  }
  else
  {
    os << "DirectoryName : (nullptr)"
       << "\n";
  }
  if (this->FileName)
  {
    os << "FileName : " << this->FileName << "\n";
  }
  else
  {
    os << "FileName : (nullptr)"
       << "\n";
  }
}

//----------------------------------------------------------------------------
int svtkDICOMImageReader::CanReadFile(const char* fname)
{
  bool canOpen = this->Parser->OpenFile((const char*)fname);
  if (!canOpen)
  {
    svtkErrorMacro("DICOMParser couldn't open : " << fname);
    return 0;
  }
  bool canRead = this->Parser->IsDICOMFile();
  if (canRead)
  {
    return 1;
  }
  else
  {
    svtkWarningMacro("DICOMParser couldn't parse : " << fname);
    return 0;
  }
}

//----------------------------------------------------------------------------
void svtkDICOMImageReader::ExecuteInformation()
{
  if (this->FileName == nullptr && this->DirectoryName == nullptr)
  {
    return;
  }

  if (this->FileName)
  {
    svtksys::SystemTools::Stat_t fs;
    if (svtksys::SystemTools::Stat(this->FileName, &fs))
    {
      svtkErrorMacro("Unable to open file " << this->FileName);
      return;
    }

    this->DICOMFileNames->clear();
    this->AppHelper->Clear();
    this->Parser->ClearAllDICOMTagCallbacks();

    this->Parser->OpenFile(this->FileName);
    this->AppHelper->RegisterCallbacks(this->Parser);

    this->Parser->ReadHeader();
    this->SetupOutputInformation(1);
  }
  else if (this->DirectoryName)
  {
    svtkDirectory* dir = svtkDirectory::New();
    int opened = dir->Open(this->DirectoryName);
    if (!opened)
    {
      svtkErrorMacro("Couldn't open " << this->DirectoryName);
      dir->Delete();
      return;
    }
    svtkIdType numFiles = dir->GetNumberOfFiles();

    svtkDebugMacro(<< "There are " << numFiles << " files in the directory.");

    this->DICOMFileNames->clear();
    this->AppHelper->Clear();

    for (svtkIdType i = 0; i < numFiles; i++)
    {
      if (strcmp(dir->GetFile(i), ".") == 0 || strcmp(dir->GetFile(i), "..") == 0)
      {
        continue;
      }

      std::string fileString = this->DirectoryName;
      fileString += "/";
      fileString += dir->GetFile(i);

      int val = this->CanReadFile(fileString.c_str());

      if (val == 1)
      {
        svtkDebugMacro(<< "Adding " << fileString.c_str() << " to DICOMFileNames.");
        this->DICOMFileNames->push_back(fileString);
      }
      else
      {
        svtkDebugMacro(<< fileString.c_str() << " - DICOMParser CanReadFile returned : " << val);
      }
    }
    std::vector<std::string>::iterator iter;

    for (iter = this->DICOMFileNames->begin(); iter != this->DICOMFileNames->end(); ++iter)
    {
      const char* fn = iter->c_str();
      svtkDebugMacro(<< "Trying : " << fn);

      bool couldOpen = this->Parser->OpenFile(fn);
      if (!couldOpen)
      {
        dir->Delete();
        return;
      }

      //
      this->Parser->ClearAllDICOMTagCallbacks();
      this->AppHelper->RegisterCallbacks(this->Parser);

      this->Parser->ReadHeader();
      this->Parser->CloseFile();

      svtkDebugMacro(<< "File name : " << fn);
      svtkDebugMacro(<< "Slice number : " << this->AppHelper->GetSliceNumber());
    }

    std::vector<std::pair<float, std::string> > sortedFiles;

    this->AppHelper->GetImagePositionPatientFilenamePairs(sortedFiles, false);
    this->SetupOutputInformation(static_cast<int>(sortedFiles.size()));

    // this->AppHelper->OutputSeries();

    if (!sortedFiles.empty())
    {
      this->DICOMFileNames->clear();
      std::vector<std::pair<float, std::string> >::iterator siter;
      for (siter = sortedFiles.begin(); siter != sortedFiles.end(); ++siter)
      {
        svtkDebugMacro(<< "Sorted filename : " << (*siter).second.c_str());
        svtkDebugMacro(<< "Adding file " << (*siter).second.c_str()
                      << " at slice : " << (*siter).first);
        this->DICOMFileNames->push_back((*siter).second);
      }
    }
    else
    {
      svtkErrorMacro(<< "Couldn't get sorted files. Slices may be in wrong order!");
    }
    dir->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkDICOMImageReader::ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo)
{
  svtkImageData* data = this->AllocateOutputData(output, outInfo);

  if (!this->FileName && this->DICOMFileNames->empty())
  {
    svtkErrorMacro(<< "Either a filename was not specified or the specified directory does not "
                     "contain any DICOM images.");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return;
  }

  data->GetPointData()->GetScalars()->SetName("DICOMImage");

  this->ComputeDataIncrements();

  if (this->FileName)
  {
    svtkDebugMacro(<< "Single file : " << this->FileName);
    this->Parser->ClearAllDICOMTagCallbacks();
    this->Parser->OpenFile(this->FileName);
    this->AppHelper->Clear();
    this->AppHelper->RegisterCallbacks(this->Parser);
    this->AppHelper->RegisterPixelDataCallback(this->Parser);

    this->Parser->ReadHeader();

    void* imgData = nullptr;
    DICOMParser::VRTypes dataType;
    unsigned long imageDataLength;

    this->AppHelper->GetImageData(imgData, dataType, imageDataLength);
    if (!imageDataLength)
    {
      svtkErrorMacro(<< "There was a problem retrieving data from: " << this->FileName);
      this->SetErrorCode(svtkErrorCode::FileFormatError);
      return;
    }

    void* buffer = data->GetScalarPointer();
    if (buffer == nullptr)
    {
      svtkErrorMacro(<< "No memory allocated for image data!");
      return;
    }
    // DICOM stores the upper left pixel as the first pixel in an
    // image. SVTK stores the lower left pixel as the first pixel in
    // an image.  Need to flip the data.
    svtkIdType rowLength;
    rowLength = this->DataIncrements[1];
    unsigned char* b = (unsigned char*)buffer;
    unsigned char* iData = (unsigned char*)imgData;
    iData += (imageDataLength - rowLength); // beginning of last row
    for (int i = 0; i < this->AppHelper->GetHeight(); ++i)
    {
      memcpy(b, iData, rowLength);
      b += rowLength;
      iData -= rowLength;
    }
  }
  else if (!this->DICOMFileNames->empty())
  {
    svtkDebugMacro(<< "Multiple files (" << static_cast<int>(this->DICOMFileNames->size()) << ")");
    this->Parser->ClearAllDICOMTagCallbacks();
    this->AppHelper->Clear();
    this->AppHelper->RegisterCallbacks(this->Parser);
    this->AppHelper->RegisterPixelDataCallback(this->Parser);

    void* buffer = data->GetScalarPointer();
    if (buffer == nullptr)
    {
      svtkErrorMacro(<< "No memory allocated for image data!");
      return;
    }

    std::vector<std::string>::iterator fiter;

    int count = 0;
    svtkIdType numFiles = static_cast<int>(this->DICOMFileNames->size());

    for (fiter = this->DICOMFileNames->begin(); fiter != this->DICOMFileNames->end(); ++fiter)
    {
      count++;
      const char* file = fiter->c_str();
      svtkDebugMacro(<< "File : " << file);
      this->Parser->OpenFile(file);
      this->Parser->ReadHeader();

      void* imgData = nullptr;
      DICOMParser::VRTypes dataType;
      unsigned long imageDataLengthInBytes;

      this->AppHelper->GetImageData(imgData, dataType, imageDataLengthInBytes);
      if (!imageDataLengthInBytes)
      {
        svtkErrorMacro(<< "There was a problem retrieving data from: " << file);
        this->SetErrorCode(svtkErrorCode::FileFormatError);
        return;
      }

      // DICOM stores the upper left pixel as the first pixel in an
      // image. SVTK stores the lower left pixel as the first pixel in
      // an image.  Need to flip the data.
      svtkIdType rowLength;
      rowLength = this->DataIncrements[1];
      unsigned char* b = (unsigned char*)buffer;
      unsigned char* iData = (unsigned char*)imgData;
      iData += (imageDataLengthInBytes - rowLength); // beginning of last row
      for (int i = 0; i < this->AppHelper->GetHeight(); ++i)
      {
        memcpy(b, iData, rowLength);
        b += rowLength;
        iData -= rowLength;
      }
      buffer = ((char*)buffer) + imageDataLengthInBytes;

      this->UpdateProgress(float(count) / float(numFiles));
      int len = static_cast<int>(strlen((const char*)(*fiter).c_str()));
      char* filename = new char[len + 1];
      strcpy(filename, (const char*)(*fiter).c_str());
      this->SetProgressText(filename);
      delete[] filename;
    }
  }
}

//----------------------------------------------------------------------------
void svtkDICOMImageReader::SetupOutputInformation(int num_slices)
{
  int width = this->AppHelper->GetWidth();
  int height = this->AppHelper->GetHeight();
  int bit_depth = this->AppHelper->GetBitsAllocated();
  int num_comp = this->AppHelper->GetNumberOfComponents();

  this->DataExtent[0] = 0;
  this->DataExtent[1] = width - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = height - 1;
  this->DataExtent[4] = 0;
  this->DataExtent[5] = num_slices - 1;

  bool isFloat = this->AppHelper->RescaledImageDataIsFloat();

  bool sign = this->AppHelper->RescaledImageDataIsSigned();

  if (isFloat)
  {
    this->SetDataScalarTypeToFloat();
  }
  else if (bit_depth <= 8)
  {
    this->SetDataScalarTypeToUnsignedChar();
  }
  else
  {
    if (sign)
    {
      this->SetDataScalarTypeToShort();
    }
    else
    {
      this->SetDataScalarTypeToUnsignedShort();
    }
  }
  this->SetNumberOfScalarComponents(num_comp);

  this->GetPixelSpacing();

  this->svtkImageReader2::ExecuteInformation();
}

//----------------------------------------------------------------------------
void svtkDICOMImageReader::SetDirectoryName(const char* dn)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting DirectoryName to "
                << (dn ? dn : "(null)"));
  if (this->DirectoryName == nullptr && dn == nullptr)
  {
    return;
  }
  delete[] this->FileName;
  this->FileName = nullptr;
  if (this->DirectoryName && dn && (!strcmp(this->DirectoryName, dn)))
  {
    return;
  }
  delete[] this->DirectoryName;
  if (dn)
  {
    this->DirectoryName = new char[strlen(dn) + 1];
    strcpy(this->DirectoryName, dn);
  }
  else
  {
    this->DirectoryName = nullptr;
  }
  this->Modified();
}

//----------------------------------------------------------------------------
double* svtkDICOMImageReader::GetPixelSpacing()
{
  std::vector<std::pair<float, std::string> > sortedFiles;

  this->AppHelper->GetImagePositionPatientFilenamePairs(sortedFiles, false);

  float* spacing = this->AppHelper->GetPixelSpacing();
  this->DataSpacing[0] = spacing[0];
  this->DataSpacing[1] = spacing[1];

  if (sortedFiles.size() > 1)
  {
    std::pair<float, std::string> p1 = sortedFiles[0];
    std::pair<float, std::string> p2 = sortedFiles[1];
    this->DataSpacing[2] = fabs(p1.first - p2.first);
  }
  else
  {
    this->DataSpacing[2] = spacing[2];
  }

  return this->DataSpacing;
}

//----------------------------------------------------------------------------
int svtkDICOMImageReader::GetWidth()
{
  return this->AppHelper->GetWidth();
}

//----------------------------------------------------------------------------
int svtkDICOMImageReader::GetHeight()
{
  return this->AppHelper->GetHeight();
}

//----------------------------------------------------------------------------
float* svtkDICOMImageReader::GetImagePositionPatient()
{
  return this->AppHelper->GetImagePositionPatient();
}

//----------------------------------------------------------------------------
float* svtkDICOMImageReader::GetImageOrientationPatient()
{
  return this->AppHelper->GetImageOrientationPatient();
}

//----------------------------------------------------------------------------
int svtkDICOMImageReader::GetBitsAllocated()
{
  return this->AppHelper->GetBitsAllocated();
}

//----------------------------------------------------------------------------
int svtkDICOMImageReader::GetPixelRepresentation()
{
  return this->AppHelper->GetPixelRepresentation();
}

//----------------------------------------------------------------------------
int svtkDICOMImageReader::GetNumberOfComponents()
{
  return this->AppHelper->GetNumberOfComponents();
}

//----------------------------------------------------------------------------
const char* svtkDICOMImageReader::GetTransferSyntaxUID()
{
  std::string tmp = this->AppHelper->GetTransferSyntaxUID();

  delete[] this->TransferSyntaxUID;
  this->TransferSyntaxUID = new char[tmp.length() + 1];
  strcpy(this->TransferSyntaxUID, tmp.c_str());
  this->TransferSyntaxUID[tmp.length()] = '\0';

  return this->TransferSyntaxUID;
}

//----------------------------------------------------------------------------
float svtkDICOMImageReader::GetRescaleSlope()
{
  return this->AppHelper->GetRescaleSlope();
}

//----------------------------------------------------------------------------
float svtkDICOMImageReader::GetRescaleOffset()
{
  return this->AppHelper->GetRescaleOffset();
}

//----------------------------------------------------------------------------
const char* svtkDICOMImageReader::GetPatientName()
{
  std::string tmp = this->AppHelper->GetPatientName();

  delete[] this->PatientName;
  this->PatientName = new char[tmp.length() + 1];
  strcpy(this->PatientName, tmp.c_str());
  this->PatientName[tmp.length()] = '\0';

  return this->PatientName;
}

//----------------------------------------------------------------------------
const char* svtkDICOMImageReader::GetStudyUID()
{
  std::string tmp = this->AppHelper->GetStudyUID();

  delete[] this->StudyUID;
  this->StudyUID = new char[tmp.length() + 1];
  strcpy(this->StudyUID, tmp.c_str());
  this->StudyUID[tmp.length()] = '\0';

  return this->StudyUID;
}

//----------------------------------------------------------------------------
const char* svtkDICOMImageReader::GetStudyID()
{
  std::string tmp = this->AppHelper->GetStudyID();

  delete[] this->StudyID;
  this->StudyID = new char[tmp.length() + 1];
  strcpy(this->StudyID, tmp.c_str());
  this->StudyID[tmp.length()] = '\0';

  return this->StudyID;
}

//----------------------------------------------------------------------------
float svtkDICOMImageReader::GetGantryAngle()
{
  return this->AppHelper->GetGantryAngle();
}

//----------------------------------------------------------------------------
int svtkDICOMImageReader::GetNumberOfDICOMFileNames()
{
  return static_cast<int>(this->DICOMFileNames->size());
}

//----------------------------------------------------------------------------
const char* svtkDICOMImageReader::GetDICOMFileName(int index)
{
  if (index >= 0 && index < this->GetNumberOfDICOMFileNames())
  {
    return (*this->DICOMFileNames)[index].c_str();
  }
  return nullptr;
}
