/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageReader2.h"

#include "svtkByteSwap.h"
#include "svtkDataArray.h"
#include "svtkErrorCode.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"

#include "svtksys/Encoding.hxx"
#include "svtksys/FStream.hxx"
#include "svtksys/SystemTools.hxx"

svtkStandardNewMacro(svtkImageReader2);

#ifdef read
#undef read
#endif

#ifdef close
#undef close
#endif

//----------------------------------------------------------------------------
svtkImageReader2::svtkImageReader2()
{
  this->FilePrefix = nullptr;
  this->FilePattern = new char[strlen("%s.%d") + 1];
  strcpy(this->FilePattern, "%s.%d");
  this->File = nullptr;

  this->DataScalarType = SVTK_SHORT;
  this->NumberOfScalarComponents = 1;

  this->DataOrigin[0] = this->DataOrigin[1] = this->DataOrigin[2] = 0.0;

  this->DataSpacing[0] = this->DataSpacing[1] = this->DataSpacing[2] = 1.0;

  this->DataDirection[0] = this->DataDirection[4] = this->DataDirection[8] = 1.0;
  this->DataDirection[1] = this->DataDirection[2] = this->DataDirection[3] =
    this->DataDirection[5] = this->DataDirection[6] = this->DataDirection[7] = 0.0;

  this->DataExtent[0] = this->DataExtent[2] = this->DataExtent[4] = 0;
  this->DataExtent[1] = this->DataExtent[3] = this->DataExtent[5] = 0;

  this->DataIncrements[0] = this->DataIncrements[1] = this->DataIncrements[2] =
    this->DataIncrements[3] = 1;

  this->FileNames = nullptr;

  this->FileName = nullptr;
  this->InternalFileName = nullptr;

  this->MemoryBuffer = nullptr;
  this->MemoryBufferLength = 0;

  this->HeaderSize = 0;
  this->ManualHeaderSize = 0;

  this->FileNameSliceOffset = 0;
  this->FileNameSliceSpacing = 1;

  // Left over from short reader
  this->SwapBytes = 0;
  this->FileLowerLeft = 0;
  this->FileDimensionality = 2;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkImageReader2::~svtkImageReader2()
{
  this->CloseFile();

  if (this->FileNames)
  {
    this->FileNames->Delete();
    this->FileNames = nullptr;
  }
  delete[] this->FileName;
  this->FileName = nullptr;
  delete[] this->FilePrefix;
  this->FilePrefix = nullptr;
  delete[] this->FilePattern;
  this->FilePattern = nullptr;
  delete[] this->InternalFileName;
  this->InternalFileName = nullptr;
}

//----------------------------------------------------------------------------
// This function sets the name of the file.
void svtkImageReader2::ComputeInternalFileName(int slice)
{
  // delete any old filename
  delete[] this->InternalFileName;
  this->InternalFileName = nullptr;

  if (!this->FileName && !this->FilePattern && !this->FileNames)
  {
    svtkErrorMacro(<< "Either a FileName, FileNames, or FilePattern"
                  << " must be specified.");
    return;
  }

  // make sure we figure out a filename to open
  if (this->FileNames)
  {
    const char* filename = this->FileNames->GetValue(slice);
    size_t size = strlen(filename) + 10;
    this->InternalFileName = new char[size];
    snprintf(this->InternalFileName, size, "%s", filename);
  }
  else if (this->FileName)
  {
    size_t size = strlen(this->FileName) + 10;
    this->InternalFileName = new char[size];
    snprintf(this->InternalFileName, size, "%s", this->FileName);
  }
  else
  {
    int slicenum = slice * this->FileNameSliceSpacing + this->FileNameSliceOffset;
    if (this->FilePrefix && this->FilePattern)
    {
      size_t size = strlen(this->FilePrefix) + strlen(this->FilePattern) + 10;
      this->InternalFileName = new char[size];
      snprintf(this->InternalFileName, size, this->FilePattern, this->FilePrefix, slicenum);
    }
    else if (this->FilePattern)
    {
      size_t size = strlen(this->FilePattern) + 10;
      this->InternalFileName = new char[size];
      int len = static_cast<int>(strlen(this->FilePattern));
      int hasPercentS = 0;
      for (int i = 0; i < len - 1; ++i)
      {
        if (this->FilePattern[i] == '%' && this->FilePattern[i + 1] == 's')
        {
          hasPercentS = 1;
          break;
        }
      }
      if (hasPercentS)
      {
        snprintf(this->InternalFileName, size, this->FilePattern, "", slicenum);
      }
      else
      {
        snprintf(this->InternalFileName, size, this->FilePattern, slicenum);
      }
    }
    else
    {
      delete[] this->InternalFileName;
      this->InternalFileName = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
// This function sets the name of the file.
void svtkImageReader2::SetFileName(const char* name)
{
  if (this->FileName && name && (!strcmp(this->FileName, name)))
  {
    return;
  }
  if (!name && !this->FileName)
  {
    return;
  }
  delete[] this->FileName;
  this->FileName = nullptr;
  if (name)
  {
    this->FileName = new char[strlen(name) + 1];
    strcpy(this->FileName, name);

    delete[] this->FilePrefix;
    this->FilePrefix = nullptr;
    if (this->FileNames)
    {
      this->FileNames->Delete();
      this->FileNames = nullptr;
    }
  }

  this->Modified();
}

//----------------------------------------------------------------------------
// This function sets an array containing file names
void svtkImageReader2::SetFileNames(svtkStringArray* filenames)
{
  if (filenames == this->FileNames)
  {
    return;
  }
  if (this->FileNames)
  {
    this->FileNames->Delete();
    this->FileNames = nullptr;
  }
  if (filenames)
  {
    this->FileNames = filenames;
    this->FileNames->Register(this);
    if (this->FileNames->GetNumberOfValues() > 0)
    {
      this->DataExtent[4] = 0;
      this->DataExtent[5] = this->FileNames->GetNumberOfValues() - 1;
    }
    delete[] this->FilePrefix;
    this->FilePrefix = nullptr;
    delete[] this->FileName;
    this->FileName = nullptr;
  }

  this->Modified();
}

//----------------------------------------------------------------------------
// This function sets the prefix of the file name. "image" would be the
// name of a series: image.1, image.2 ...
void svtkImageReader2::SetFilePrefix(const char* prefix)
{
  if (this->FilePrefix && prefix && (!strcmp(this->FilePrefix, prefix)))
  {
    return;
  }
  if (!prefix && !this->FilePrefix)
  {
    return;
  }
  delete[] this->FilePrefix;
  this->FilePrefix = nullptr;
  if (prefix)
  {
    this->FilePrefix = new char[strlen(prefix) + 1];
    strcpy(this->FilePrefix, prefix);

    delete[] this->FileName;
    this->FileName = nullptr;
    if (this->FileNames)
    {
      this->FileNames->Delete();
      this->FileNames = nullptr;
    }
  }

  this->Modified();
}

//----------------------------------------------------------------------------
// This function sets the pattern of the file name which turn a prefix
// into a file name. "%s.%03d" would be the
// pattern of a series: image.001, image.002 ...
void svtkImageReader2::SetFilePattern(const char* pattern)
{
  if (this->FilePattern && pattern && (!strcmp(this->FilePattern, pattern)))
  {
    return;
  }
  if (!pattern && !this->FilePattern)
  {
    return;
  }
  delete[] this->FilePattern;
  this->FilePattern = nullptr;
  if (pattern)
  {
    this->FilePattern = new char[strlen(pattern) + 1];
    strcpy(this->FilePattern, pattern);

    delete[] this->FileName;
    this->FileName = nullptr;
    if (this->FileNames)
    {
      this->FileNames->Delete();
      this->FileNames = nullptr;
    }
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkImageReader2::SetDataByteOrderToBigEndian()
{
#ifndef SVTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

//----------------------------------------------------------------------------
void svtkImageReader2::SetDataByteOrderToLittleEndian()
{
#ifdef SVTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

//----------------------------------------------------------------------------
void svtkImageReader2::SetDataByteOrder(int byteOrder)
{
  if (byteOrder == SVTK_FILE_BYTE_ORDER_BIG_ENDIAN)
  {
    this->SetDataByteOrderToBigEndian();
  }
  else
  {
    this->SetDataByteOrderToLittleEndian();
  }
}

//----------------------------------------------------------------------------
int svtkImageReader2::GetDataByteOrder()
{
#ifdef SVTK_WORDS_BIGENDIAN
  if (this->SwapBytes)
  {
    return SVTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
  }
  else
  {
    return SVTK_FILE_BYTE_ORDER_BIG_ENDIAN;
  }
#else
  if (this->SwapBytes)
  {
    return SVTK_FILE_BYTE_ORDER_BIG_ENDIAN;
  }
  else
  {
    return SVTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
  }
#endif
}

//----------------------------------------------------------------------------
const char* svtkImageReader2::GetDataByteOrderAsString()
{
#ifdef SVTK_WORDS_BIGENDIAN
  if (this->SwapBytes)
  {
    return "LittleEndian";
  }
  else
  {
    return "BigEndian";
  }
#else
  if (this->SwapBytes)
  {
    return "BigEndian";
  }
  else
  {
    return "LittleEndian";
  }
#endif
}

//----------------------------------------------------------------------------
void svtkImageReader2::PrintSelf(ostream& os, svtkIndent indent)
{
  int idx;

  this->Superclass::PrintSelf(os, indent);

  // this->File, this->Colors need not be printed
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "FileNames: " << this->FileNames << "\n";
  os << indent << "FilePrefix: " << (this->FilePrefix ? this->FilePrefix : "(none)") << "\n";
  os << indent << "FilePattern: " << (this->FilePattern ? this->FilePattern : "(none)") << "\n";

  os << indent << "FileNameSliceOffset: " << this->FileNameSliceOffset << "\n";
  os << indent << "FileNameSliceSpacing: " << this->FileNameSliceSpacing << "\n";

  os << indent << "DataScalarType: " << svtkImageScalarTypeNameMacro(this->DataScalarType) << "\n";
  os << indent << "NumberOfScalarComponents: " << this->NumberOfScalarComponents << "\n";

  os << indent << "File Dimensionality: " << this->FileDimensionality << "\n";

  os << indent << "File Lower Left: " << (this->FileLowerLeft ? "On\n" : "Off\n");

  os << indent << "Swap Bytes: " << (this->SwapBytes ? "On\n" : "Off\n");

  os << indent << "DataIncrements: (" << this->DataIncrements[0];
  for (idx = 1; idx < 2; ++idx)
  {
    os << ", " << this->DataIncrements[idx];
  }
  os << ")\n";

  os << indent << "DataExtent: (" << this->DataExtent[0];
  for (idx = 1; idx < 6; ++idx)
  {
    os << ", " << this->DataExtent[idx];
  }
  os << ")\n";

  os << indent << "DataSpacing: (" << this->DataSpacing[0];
  for (idx = 1; idx < 3; ++idx)
  {
    os << ", " << this->DataSpacing[idx];
  }
  os << ")\n";

  os << indent << "DataDirection: (" << this->DataDirection[0];
  for (idx = 1; idx < 9; ++idx)
  {
    os << ", " << this->DataDirection[idx];
  }
  os << ")\n";

  os << indent << "DataOrigin: (" << this->DataOrigin[0];
  for (idx = 1; idx < 3; ++idx)
  {
    os << ", " << this->DataOrigin[idx];
  }
  os << ")\n";

  os << indent << "HeaderSize: " << this->HeaderSize << "\n";

  if (this->InternalFileName)
  {
    os << indent << "Internal File Name: " << this->InternalFileName << "\n";
  }
  else
  {
    os << indent << "Internal File Name: (none)\n";
  }
}

//----------------------------------------------------------------------------
void svtkImageReader2::ExecuteInformation()
{
  // this is empty, the idea is that converted filters should implement
  // RequestInformation. But to help out old filters we will call
  // ExecuteInformation and hope that the subclasses correctly set the ivars
  // and not the output.
}

//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
int svtkImageReader2::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  this->SetErrorCode(svtkErrorCode::NoError);
  // call for backwards compatibility
  this->ExecuteInformation();
  // Check for any error set by downstream filter (IO in most case)
  if (this->GetErrorCode())
  {
    return 0;
  }

  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // if a list of file names is supplied, set slice extent
  if (this->FileNames && this->FileNames->GetNumberOfValues() > 0)
  {
    this->DataExtent[4] = 0;
    this->DataExtent[5] = this->FileNames->GetNumberOfValues() - 1;
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DataExtent, 6);
  outInfo->Set(svtkDataObject::SPACING(), this->DataSpacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), this->DataOrigin, 3);
  outInfo->Set(svtkDataObject::DIRECTION(), this->DataDirection, 9);

  svtkDataObject::SetPointDataActiveScalarInfo(
    outInfo, this->DataScalarType, this->NumberOfScalarComponents);

  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
// Manual initialization.
void svtkImageReader2::SetHeaderSize(unsigned long size)
{
  if (size != this->HeaderSize)
  {
    this->HeaderSize = size;
    this->Modified();
  }
  this->ManualHeaderSize = 1;
}

//----------------------------------------------------------------------------
template <class T>
unsigned long svtkImageReader2GetSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
// This function opens a file to determine the file size, and to
// automatically determine the header size.
void svtkImageReader2::ComputeDataIncrements()
{
  int idx;
  unsigned long fileDataLength;

  // Determine the expected length of the data ...
  switch (this->DataScalarType)
  {
    svtkTemplateMacro(fileDataLength = svtkImageReader2GetSize(static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro(<< "Unknown DataScalarType");
      return;
  }

  fileDataLength *= this->NumberOfScalarComponents;

  // compute the fileDataLength (in units of bytes)
  for (idx = 0; idx < 3; ++idx)
  {
    this->DataIncrements[idx] = fileDataLength;
    fileDataLength =
      fileDataLength * (this->DataExtent[idx * 2 + 1] - this->DataExtent[idx * 2] + 1);
  }
  this->DataIncrements[3] = fileDataLength;
}

//----------------------------------------------------------------------------
void svtkImageReader2::CloseFile()
{
  if (this->File)
  {
    delete this->File;
    this->File = nullptr;
  }
}

//----------------------------------------------------------------------------
int svtkImageReader2::OpenFile()
{
  if (!this->FileName && !this->FilePattern && !this->FileNames)
  {
    svtkErrorMacro(<< "Either a FileName, FileNames, or FilePattern"
                  << " must be specified.");
    return 0;
  }

  this->CloseFile();
  // Open the new file
  svtkDebugMacro(<< "Initialize: opening file " << this->InternalFileName);
  svtksys::SystemTools::Stat_t fs;
  if (!svtksys::SystemTools::Stat(this->InternalFileName, &fs))
  {
    std::ios_base::openmode mode = ios::in;
#ifdef _WIN32
    mode |= ios::binary;
#endif
    this->File = new svtksys::ifstream(this->InternalFileName, mode);
  }
  if (!this->File || this->File->fail())
  {
    svtkErrorMacro(<< "Initialize: Could not open file " << this->InternalFileName);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
unsigned long svtkImageReader2::GetHeaderSize()
{
  unsigned long firstIdx;

  if (this->FileNames)
  {
    // if FileNames is used, indexing always starts at zero
    firstIdx = 0;
  }
  else
  {
    // FilePrefix uses the DataExtent to figure out the first slice index
    firstIdx = this->DataExtent[4];
  }

  return this->GetHeaderSize(firstIdx);
}

//----------------------------------------------------------------------------
unsigned long svtkImageReader2::GetHeaderSize(unsigned long idx)
{
  if (!this->FileName && !this->FilePattern)
  {
    svtkErrorMacro(<< "Either a FileName or FilePattern must be specified.");
    return 0;
  }
  if (!this->ManualHeaderSize)
  {
    this->ComputeDataIncrements();

    // make sure we figure out a filename to open
    this->ComputeInternalFileName(idx);

    svtksys::SystemTools::Stat_t statbuf;
    if (!svtksys::SystemTools::Stat(this->InternalFileName, &statbuf))
    {
      return (int)(statbuf.st_size - (long)this->DataIncrements[this->GetFileDimensionality()]);
    }
  }

  return this->HeaderSize;
}

//----------------------------------------------------------------------------
void svtkImageReader2::SeekFile(int i, int j, int k)
{
  unsigned long streamStart;

  // convert data extent into constants that can be used to seek.
  streamStart = (i - this->DataExtent[0]) * this->DataIncrements[0];

  if (this->FileLowerLeft)
  {
    streamStart = streamStart + (j - this->DataExtent[2]) * this->DataIncrements[1];
  }
  else
  {
    streamStart =
      streamStart + (this->DataExtent[3] - this->DataExtent[2] - j) * this->DataIncrements[1];
  }

  // handle three and four dimensional files
  if (this->GetFileDimensionality() >= 3)
  {
    streamStart = streamStart + (k - this->DataExtent[4]) * this->DataIncrements[2];
  }

  streamStart += this->GetHeaderSize(k);

  // error checking
  if (!this->File)
  {
    svtkWarningMacro(<< "File must be specified.");
    return;
  }

  this->File->seekg((long)streamStart, ios::beg);
  if (this->File->fail())
  {
    svtkWarningMacro("File operation failed.");
    return;
  }
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void svtkImageReader2Update(svtkImageReader2* self, svtkImageData* data, OT* outPtr)
{
  svtkIdType outIncr[3];
  OT *outPtr1, *outPtr2;
  long streamRead;
  int idx1, idx2, nComponents;
  int outExtent[6];
  unsigned long count = 0;
  unsigned long target;

  // Get the requested extents and increments
  data->GetExtent(outExtent);
  data->GetIncrements(outIncr);
  nComponents = data->GetNumberOfScalarComponents();

  // length of a row, num pixels read at a time
  int pixelRead = outExtent[1] - outExtent[0] + 1;
  streamRead = (long)(pixelRead * nComponents * sizeof(OT));

  // create a buffer to hold a row of the data
  target =
    (unsigned long)((outExtent[5] - outExtent[4] + 1) * (outExtent[3] - outExtent[2] + 1) / 50.0);
  target++;

  // read the data row by row
  if (self->GetFileDimensionality() == 3)
  {
    self->ComputeInternalFileName(0);
    if (!self->OpenFile())
    {
      return;
    }
  }
  outPtr2 = outPtr;
  for (idx2 = outExtent[4]; idx2 <= outExtent[5]; ++idx2)
  {
    if (self->GetFileDimensionality() == 2)
    {
      self->ComputeInternalFileName(idx2);
      if (!self->OpenFile())
      {
        return;
      }
    }
    outPtr1 = outPtr2;
    for (idx1 = outExtent[2]; !self->AbortExecute && idx1 <= outExtent[3]; ++idx1)
    {
      if (!(count % target))
      {
        self->UpdateProgress(count / (50.0 * target));
      }
      count++;

      // seek to the correct row
      self->SeekFile(outExtent[0], idx1, idx2);
      // read the row.
      if (!self->GetFile()->read((char*)outPtr1, streamRead))
      {
        svtkGenericWarningMacro("File operation failed. row = "
          << idx1 << ", Read = " << streamRead
          << ", FilePos = " << static_cast<svtkIdType>(self->GetFile()->tellg()));
        return;
      }
      // handle swapping
      if (self->GetSwapBytes() && sizeof(OT) > 1)
      {
        svtkByteSwap::SwapVoidRange(outPtr1, pixelRead * nComponents, sizeof(OT));
      }
      outPtr1 += outIncr[1];
    }
    // move to the next image in the file and data
    outPtr2 += outIncr[2];
  }
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void svtkImageReader2::ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo)
{
  svtkImageData* data = this->AllocateOutputData(output, outInfo);

  void* ptr;

  if (!this->FileName && !this->FilePattern)
  {
    svtkErrorMacro("Either a valid FileName or FilePattern must be specified.");
    return;
  }

  data->GetPointData()->GetScalars()->SetName("ImageFile");

#ifndef NDEBUG
  int* ext = data->GetExtent();
#endif

  svtkDebugMacro("Reading extent: " << ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3]
                                   << ", " << ext[4] << ", " << ext[5]);

  this->ComputeDataIncrements();

  // Call the correct templated function for the output
  ptr = data->GetScalarPointer();
  switch (this->GetDataScalarType())
  {
    svtkTemplateMacro(svtkImageReader2Update(this, data, (SVTK_TT*)(ptr)));
    default:
      svtkErrorMacro(<< "UpdateFromFile: Unknown data type");
  }
}

//----------------------------------------------------------------------------
void svtkImageReader2::SetMemoryBuffer(const void* membuf)
{
  if (this->MemoryBuffer != membuf)
  {
    this->MemoryBuffer = membuf;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageReader2::SetMemoryBufferLength(svtkIdType buflen)
{
  if (this->MemoryBufferLength != buflen)
  {
    this->MemoryBufferLength = buflen;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Set the data type of pixels in the file.
// If you want the output scalar type to have a different value, set it
// after this method is called.
void svtkImageReader2::SetDataScalarType(int type)
{
  if (type == this->DataScalarType)
  {
    return;
  }

  this->Modified();
  this->DataScalarType = type;
  // Set the default output scalar type
  svtkImageData::SetScalarType(this->DataScalarType, this->GetOutputInformation(0));
}
