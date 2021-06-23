/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLWriter.h"

#include "svtkAOSDataArrayTemplate.h"
#include "svtkArrayDispatch.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkBase64OutputStream.h"
#include "svtkBitArray.h"
#include "svtkByteSwap.h"
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationIdTypeKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationIterator.h"
#include "svtkInformationKeyLookup.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationStringVectorKey.h"
#include "svtkInformationUnsignedLongKey.h"
#include "svtkInformationVector.h"
#include "svtkLZ4DataCompressor.h"
#include "svtkLZMADataCompressor.h"
#include "svtkNew.h"
#include "svtkOutputStream.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStdString.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkZLibDataCompressor.h"
#define svtkXMLOffsetsManager_DoNotInclude
#include "svtkXMLOffsetsManager.h"
#undef svtkXMLOffsetsManager_DoNotInclude
#define svtkXMLDataHeaderPrivate_DoNotInclude
#include "svtkXMLDataHeaderPrivate.h"
#undef svtkXMLDataHeaderPrivate_DoNotInclude
#include "svtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "svtkInformationStringKey.h"
#include "svtkNumberToString.h"
#include "svtkQuadratureSchemeDefinition.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLReaderVersion.h"
#include "svtksys/Encoding.hxx"
#include "svtksys/FStream.hxx"
#include <memory>

#include <cassert>
#include <sstream>
#include <string>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

#include <cctype> // for isalnum
#include <locale> // C++ locale

//*****************************************************************************
// Friend class to enable access for template functions to the protected
// writer methods.
class svtkXMLWriterHelper
{
public:
  static inline void SetProgressPartial(svtkXMLWriter* writer, double progress)
  {
    writer->SetProgressPartial(progress);
  }
  static inline int WriteBinaryDataBlock(
    svtkXMLWriter* writer, unsigned char* in_data, size_t numWords, int wordType)
  {
    return writer->WriteBinaryDataBlock(in_data, numWords, wordType);
  }
  static inline void* GetInt32IdTypeBuffer(svtkXMLWriter* writer)
  {
    return static_cast<void*>(writer->Int32IdTypeBuffer);
  }
  static inline unsigned char* GetByteSwapBuffer(svtkXMLWriter* writer)
  {
    return writer->ByteSwapBuffer;
  }
};

namespace
{

struct WriteBinaryDataBlockWorker
{
  svtkXMLWriter* Writer;
  int WordType;
  size_t MemWordSize;
  size_t OutWordSize;
  size_t NumWords;
  bool Result;

  WriteBinaryDataBlockWorker(
    svtkXMLWriter* writer, int wordType, size_t memWordSize, size_t outWordSize, size_t numWords)
    : Writer(writer)
    , WordType(wordType)
    , MemWordSize(memWordSize)
    , OutWordSize(outWordSize)
    , NumWords(numWords)
    , Result(false)
  {
  }

  //----------------------------------------------------------------------------
  // Specialize for AoS arrays.
  template <class ValueType>
  void operator()(svtkAOSDataArrayTemplate<ValueType>* array)
  {
    // Get the raw pointer to the array data:
    ValueType* iter = array->GetPointer(0);

    // generic implementation for fixed component length arrays.
    size_t blockWords = this->Writer->GetBlockSize() / this->OutWordSize;
    size_t memBlockSize = blockWords * this->MemWordSize;

    // Prepare a pointer and counter to move through the data.
    unsigned char* ptr = reinterpret_cast<unsigned char*>(iter);
    size_t wordsLeft = this->NumWords;

    // Do the complete blocks.
    svtkXMLWriterHelper::SetProgressPartial(this->Writer, 0);
    this->Result = true;
    while (this->Result && (wordsLeft >= blockWords))
    {
      if (!svtkXMLWriterHelper::WriteBinaryDataBlock(this->Writer, ptr, blockWords, this->WordType))
      {
        this->Result = false;
      }
      ptr += memBlockSize;
      wordsLeft -= blockWords;
      svtkXMLWriterHelper::SetProgressPartial(
        this->Writer, static_cast<float>(this->NumWords - wordsLeft) / this->NumWords);
    }

    // Do the last partial block if any.
    if (this->Result && (wordsLeft > 0))
    {
      if (!svtkXMLWriterHelper::WriteBinaryDataBlock(this->Writer, ptr, wordsLeft, this->WordType))
      {
        this->Result = 0;
      }
    }
    svtkXMLWriterHelper::SetProgressPartial(this->Writer, 1);
  }

#if defined(__clang__) && defined(__has_warning)
#if __has_warning("-Wunused-template")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-template"
#endif
#endif

  //----------------------------------------------------------------------------
  // Specialize for svtkBitArray
  void operator()(svtkBitArray* array)
  {
    // Get the raw pointer to the bit array data:
    unsigned char* data = array->GetPointer(0);

    // generic implementation for fixed component length arrays.
    size_t blockSize = this->Writer->GetBlockSize();

    // Prepare a pointer and counter to move through the data.
    unsigned char* ptr = reinterpret_cast<unsigned char*>(data);
    size_t totalBytes = (this->NumWords + 7) / 8;
    size_t bytesLeft = totalBytes;

    // Write out complete blocks.
    svtkXMLWriterHelper::SetProgressPartial(this->Writer, 0);
    this->Result = true;
    while (this->Result && (bytesLeft >= blockSize))
    {
      this->Result =
        svtkXMLWriterHelper::WriteBinaryDataBlock(this->Writer, ptr, blockSize, this->WordType) != 0;
      ptr += blockSize;
      bytesLeft -= blockSize;
      svtkXMLWriterHelper::SetProgressPartial(
        this->Writer, 1.f - static_cast<float>(bytesLeft) / totalBytes);
    }

    // Do the last partial block if any.
    if (this->Result && (bytesLeft > 0))
    {
      this->Result =
        svtkXMLWriterHelper::WriteBinaryDataBlock(this->Writer, ptr, bytesLeft, this->WordType) != 0;
    }
    svtkXMLWriterHelper::SetProgressPartial(this->Writer, 1);
  }

  //----------------------------------------------------------------------------
  // Specialize for non-AoS generic arrays:
  template <class DerivedType, typename ValueType>
  void operator()(svtkGenericDataArray<DerivedType, ValueType>* array)
  {
    // generic implementation for fixed component length arrays.
    size_t blockWords = this->Writer->GetBlockSize() / this->OutWordSize;

    // Prepare a buffer to move through the data.
    std::vector<unsigned char> buffer(blockWords * this->MemWordSize);
    size_t wordsLeft = this->NumWords;

    if (buffer.empty())
    {
      // No data -- bail here, since the calls to buffer[0] below will segfault.
      this->Result = false;
      return;
    }

    // Do the complete blocks.
    svtkXMLWriterHelper::SetProgressPartial(this->Writer, 0);
    this->Result = true;
    svtkIdType valueIdx = 0;
    while (this->Result && (wordsLeft >= blockWords))
    {
      // Copy data to contiguous buffer:
      ValueType* bufferIter = reinterpret_cast<ValueType*>(&buffer[0]);
      for (size_t i = 0; i < blockWords; ++i, ++valueIdx)
      {
        *bufferIter++ = array->GetValue(valueIdx);
      }

      if (!svtkXMLWriterHelper::WriteBinaryDataBlock(
            this->Writer, &buffer[0], blockWords, this->WordType))
      {
        this->Result = false;
      }
      wordsLeft -= blockWords;
      svtkXMLWriterHelper::SetProgressPartial(
        this->Writer, static_cast<float>(this->NumWords - wordsLeft) / this->NumWords);
    }

    // Do the last partial block if any.
    if (this->Result && (wordsLeft > 0))
    {
      ValueType* bufferIter = reinterpret_cast<ValueType*>(&buffer[0]);
      for (size_t i = 0; i < wordsLeft; ++i, ++valueIdx)
      {
        *bufferIter++ = array->GetValue(valueIdx);
      }

      if (!svtkXMLWriterHelper::WriteBinaryDataBlock(
            this->Writer, &buffer[0], wordsLeft, this->WordType))
      {
        this->Result = false;
      }
    }

    svtkXMLWriterHelper::SetProgressPartial(this->Writer, 1);
  }

// Undo warning suppression.
#if defined(__clang__) && defined(__has_warning)
#if __has_warning("-Wunused-template")
#pragma clang diagnostic pop
#endif
#endif

}; // End WriteBinaryDataBlockWorker

namespace
{
//----------------------------------------------------------------------------
// Specialize for svtkDataArrays, which implicitly cast everything to double:
template <class ValueType>
void WriteDataArrayFallback(ValueType*, svtkDataArray* array, WriteBinaryDataBlockWorker& worker)
{
  // generic implementation for fixed component length arrays.
  size_t blockWords = worker.Writer->GetBlockSize() / worker.OutWordSize;

  // Prepare a buffer to move through the data.
  std::vector<unsigned char> buffer(blockWords * worker.MemWordSize);
  size_t wordsLeft = worker.NumWords;

  if (buffer.empty())
  {
    // No data -- bail here, since the calls to buffer[0] below will segfault.
    worker.Result = false;
    return;
  }

  svtkIdType nComponents = array->GetNumberOfComponents();

  // Do the complete blocks.
  svtkXMLWriterHelper::SetProgressPartial(worker.Writer, 0);
  worker.Result = true;
  svtkIdType valueIdx = 0;
  while (worker.Result && (wordsLeft >= blockWords))
  {
    // Copy data to contiguous buffer:
    ValueType* bufferIter = reinterpret_cast<ValueType*>(&buffer[0]);
    for (size_t i = 0; i < blockWords; ++i, ++valueIdx)
    {
      *bufferIter++ =
        static_cast<ValueType>(array->GetComponent(valueIdx / nComponents, valueIdx % nComponents));
    }

    if (!svtkXMLWriterHelper::WriteBinaryDataBlock(
          worker.Writer, &buffer[0], blockWords, worker.WordType))
    {
      worker.Result = false;
    }
    wordsLeft -= blockWords;
    svtkXMLWriterHelper::SetProgressPartial(
      worker.Writer, static_cast<float>(worker.NumWords - wordsLeft) / worker.NumWords);
  }

  // Do the last partial block if any.
  if (worker.Result && (wordsLeft > 0))
  {
    ValueType* bufferIter = reinterpret_cast<ValueType*>(&buffer[0]);
    for (size_t i = 0; i < wordsLeft; ++i, ++valueIdx)
    {
      *bufferIter++ =
        static_cast<ValueType>(array->GetComponent(valueIdx / nComponents, valueIdx % nComponents));
    }

    if (!svtkXMLWriterHelper::WriteBinaryDataBlock(
          worker.Writer, &buffer[0], wordsLeft, worker.WordType))
    {
      worker.Result = false;
    }
  }

  svtkXMLWriterHelper::SetProgressPartial(worker.Writer, 1);
}

}

//----------------------------------------------------------------------------
// Specialize for string arrays:
static int svtkXMLWriterWriteBinaryDataBlocks(svtkXMLWriter* writer,
  svtkArrayIteratorTemplate<svtkStdString>* iter, int wordType, size_t outWordSize, size_t numStrings,
  int)
{
  svtkXMLWriterHelper::SetProgressPartial(writer, 0);
  svtkStdString::value_type* allocated_buffer = nullptr;
  svtkStdString::value_type* temp_buffer = nullptr;
  if (svtkXMLWriterHelper::GetInt32IdTypeBuffer(writer))
  {
    temp_buffer =
      reinterpret_cast<svtkStdString::value_type*>(svtkXMLWriterHelper::GetInt32IdTypeBuffer(writer));
  }
  else if (svtkXMLWriterHelper::GetByteSwapBuffer(writer))
  {
    temp_buffer =
      reinterpret_cast<svtkStdString::value_type*>(svtkXMLWriterHelper::GetByteSwapBuffer(writer));
  }
  else
  {
    allocated_buffer = new svtkStdString::value_type[writer->GetBlockSize() / outWordSize];
    temp_buffer = allocated_buffer;
  }

  // For string arrays, writing as binary requires that the strings are written
  // out into a contiguous block. This is essential since the compressor can
  // only compress complete blocks of data.
  size_t maxCharsPerBlock = writer->GetBlockSize() / outWordSize;

  size_t index = 0; // index in string array.
  int result = 1;
  svtkIdType stringOffset = 0; // num of chars of string written in previous block.
                              // this is required since a string may not fit completely in a block.

  while (result && index < numStrings) // write one block at a time.
  {
    size_t cur_offset = 0; // offset into the temp_buffer.
    while (index < numStrings && cur_offset < maxCharsPerBlock)
    {
      svtkStdString& str = iter->GetValue(static_cast<svtkIdType>(index));
      svtkStdString::size_type length = str.size();
      const char* data = str.c_str();
      data += stringOffset; // advance by the chars already written.
      length -= stringOffset;
      if (length == 0)
      {
        // just write the string termination char.
        temp_buffer[cur_offset++] = 0x0;
        stringOffset = 0;
        index++; // advance to the next string
      }
      else
      {
        size_t new_offset = cur_offset + length + 1; // (+1) for termination char.
        if (new_offset <= maxCharsPerBlock)
        {
          memcpy(&temp_buffer[cur_offset], data, length);
          cur_offset += length;
          temp_buffer[cur_offset++] = 0x0;
          stringOffset = 0;
          index++; // advance to the next string
        }
        else
        {
          size_t bytes_to_copy = (maxCharsPerBlock - cur_offset);
          stringOffset += static_cast<svtkIdType>(bytes_to_copy);
          memcpy(&temp_buffer[cur_offset], data, bytes_to_copy);
          cur_offset += bytes_to_copy;
          // do not advance, only partially written current string
        }
      }
    }
    if (cur_offset > 0)
    {
      // We have a block of data to write.
      result = svtkXMLWriterHelper::WriteBinaryDataBlock(
        writer, reinterpret_cast<unsigned char*>(temp_buffer), cur_offset, wordType);
      svtkXMLWriterHelper::SetProgressPartial(writer, static_cast<float>(index) / numStrings);
    }
  }

  delete[] allocated_buffer;
  allocated_buffer = nullptr;

  svtkXMLWriterHelper::SetProgressPartial(writer, 1);
  return result;
}

} // end anon namespace
//*****************************************************************************

svtkCxxSetObjectMacro(svtkXMLWriter, Compressor, svtkDataCompressor);
//----------------------------------------------------------------------------
svtkXMLWriter::svtkXMLWriter()
{
  this->FileName = nullptr;
  this->Stream = nullptr;
  this->WriteToOutputString = 0;

  // Default binary data mode is base-64 encoding.
  this->DataStream = svtkBase64OutputStream::New();

  // Byte order defaults to that of machine.
#ifdef SVTK_WORDS_BIGENDIAN
  this->ByteOrder = svtkXMLWriter::BigEndian;
#else
  this->ByteOrder = svtkXMLWriter::LittleEndian;
#endif
  this->HeaderType = svtkXMLWriter::UInt32;

  // Output svtkIdType size defaults to real size.
#ifdef SVTK_USE_64BIT_IDS
  this->IdType = svtkXMLWriter::Int64;
#else
  this->IdType = svtkXMLWriter::Int32;
#endif

  // Initialize compression data.
  this->BlockSize = 32768; // 2^15
  this->Compressor = svtkZLibDataCompressor::New();
  this->CompressionHeader = nullptr;
  this->Int32IdTypeBuffer = nullptr;
  this->ByteSwapBuffer = nullptr;

  this->EncodeAppendedData = 1;
  this->AppendedDataPosition = 0;
  this->DataMode = svtkXMLWriter::Appended;
  this->ProgressRange[0] = 0;
  this->ProgressRange[1] = 1;

  this->SetNumberOfOutputPorts(0);
  this->SetNumberOfInputPorts(1);

  this->OutFile = nullptr;
  this->OutStringStream = nullptr;

  // Time support
  this->NumberOfTimeSteps = 1;
  this->CurrentTimeIndex = 0;
  this->UserContinueExecuting = -1; // invalid state
  this->NumberOfTimeValues = nullptr;
  this->FieldDataOM = new OffsetsManagerGroup;
  this->UsePreviousVersion = true;
}

//----------------------------------------------------------------------------
svtkXMLWriter::~svtkXMLWriter()
{
  this->SetFileName(nullptr);
  this->DataStream->Delete();
  this->SetCompressor(nullptr);
  delete this->OutFile;
  this->OutFile = nullptr;
  delete this->OutStringStream;
  this->OutStringStream = nullptr;
  delete this->FieldDataOM;
  delete[] this->NumberOfTimeValues;
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetCompressorType(int compressorType)
{
  if (compressorType == NONE)
  {
    if (this->Compressor)
    {
      this->Compressor->Delete();
      this->Compressor = nullptr;
      this->Modified();
    }
  }
  else if (compressorType == ZLIB)
  {
    if (this->Compressor && !this->Compressor->IsTypeOf("svtkZLibDataCompressor"))
    {
      this->Compressor->Delete();
    }
    this->Compressor = svtkZLibDataCompressor::New();
    this->Compressor->SetCompressionLevel(this->CompressionLevel);
    this->Modified();
  }
  else if (compressorType == LZ4)
  {
    if (this->Compressor && !this->Compressor->IsTypeOf("svtkLZ4DataCompressor"))
    {
      this->Compressor->Delete();
    }
    this->Compressor = svtkLZ4DataCompressor::New();
    this->Compressor->SetCompressionLevel(this->CompressionLevel);
    this->Modified();
  }
  else if (compressorType == LZMA)
  {
    if (this->Compressor && !this->Compressor->IsTypeOf("svtkLZMADataCompressor"))
    {
      this->Compressor->Delete();
    }
    this->Compressor = svtkLZMADataCompressor::New();
    this->Compressor->SetCompressionLevel(this->CompressionLevel);
    this->Modified();
  }
  else
  {
    svtkWarningMacro("Invalid compressorType:" << compressorType);
  }
}
//----------------------------------------------------------------------------
void svtkXMLWriter::SetCompressionLevel(int compressionLevel)
{
  int min = 1;
  int max = 9;
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting "
                << "CompressionLevel  to " << compressionLevel);
  if (this->CompressionLevel !=
    (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel)))
  {
    this->CompressionLevel =
      (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel));
    if (this->Compressor)
    {
      this->Compressor->SetCompressionLevel(compressionLevel);
    }
    this->Modified();
  }
}
//----------------------------------------------------------------------------
void svtkXMLWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  if (this->ByteOrder == svtkXMLWriter::BigEndian)
  {
    os << indent << "ByteOrder: BigEndian\n";
  }
  else
  {
    os << indent << "ByteOrder: LittleEndian\n";
  }
  if (this->IdType == svtkXMLWriter::Int32)
  {
    os << indent << "IdType: Int32\n";
  }
  else
  {
    os << indent << "IdType: Int64\n";
  }
  if (this->DataMode == svtkXMLWriter::Ascii)
  {
    os << indent << "DataMode: Ascii\n";
  }
  else if (this->DataMode == svtkXMLWriter::Binary)
  {
    os << indent << "DataMode: Binary\n";
  }
  else
  {
    os << indent << "DataMode: Appended\n";
  }
  if (this->Compressor)
  {
    os << indent << "Compressor: " << this->Compressor << "\n";
  }
  else
  {
    os << indent << "Compressor: (none)\n";
  }
  os << indent << "EncodeAppendedData: " << this->EncodeAppendedData << "\n";
  os << indent << "BlockSize: " << this->BlockSize << "\n";
  if (this->Stream)
  {
    os << indent << "Stream: " << this->Stream << "\n";
  }
  else
  {
    os << indent << "Stream: (none)\n";
  }
  os << indent << "NumberOfTimeSteps:" << this->NumberOfTimeSteps << "\n";
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkXMLWriter::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetByteOrderToBigEndian()
{
  this->SetByteOrder(svtkXMLWriter::BigEndian);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetByteOrderToLittleEndian()
{
  this->SetByteOrder(svtkXMLWriter::LittleEndian);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetHeaderType(int t)
{
  if (t != svtkXMLWriter::UInt32 && t != svtkXMLWriter::UInt64)
  {
    svtkErrorMacro(<< this->GetClassName() << " (" << this << "): cannot set HeaderType to " << t);
    return;
  }
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting HeaderType to " << t);
  if (this->HeaderType != t)
  {
    this->HeaderType = t;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetHeaderTypeToUInt32()
{
  this->SetHeaderType(svtkXMLWriter::UInt32);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetHeaderTypeToUInt64()
{
  this->SetHeaderType(svtkXMLWriter::UInt64);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetIdType(int t)
{
#if !defined(SVTK_USE_64BIT_IDS)
  if (t == svtkXMLWriter::Int64)
  {
    svtkErrorMacro("Support for Int64 svtkIdType not compiled in SVTK.");
    return;
  }
#endif
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting IdType to " << t);
  if (this->IdType != t)
  {
    this->IdType = t;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetIdTypeToInt32()
{
  this->SetIdType(svtkXMLWriter::Int32);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetIdTypeToInt64()
{
  this->SetIdType(svtkXMLWriter::Int64);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetDataModeToAscii()
{
  this->SetDataMode(svtkXMLWriter::Ascii);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetDataModeToBinary()
{
  this->SetDataMode(svtkXMLWriter::Binary);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetDataModeToAppended()
{
  this->SetDataMode(svtkXMLWriter::Appended);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetBlockSize(size_t blockSize)
{
  // Enforce constraints on block size.
  size_t nbs = blockSize;
#if SVTK_SIZEOF_DOUBLE > SVTK_SIZEOF_ID_TYPE
  typedef double LargestScalarType;
#else
  typedef svtkIdType LargestScalarType;
#endif
  size_t remainder = nbs % sizeof(LargestScalarType);
  if (remainder)
  {
    nbs -= remainder;
    if (nbs < sizeof(LargestScalarType))
    {
      nbs = sizeof(LargestScalarType);
    }
    svtkWarningMacro("BlockSize must be a multiple of "
      << int(sizeof(LargestScalarType)) << ".  Using " << nbs << " instead of " << blockSize
      << ".");
  }
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting BlockSize to " << nbs);
  if (this->BlockSize != nbs)
  {
    this->BlockSize = nbs;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkXMLWriter::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkXMLWriter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  this->SetErrorCode(svtkErrorCode::NoError);

  // Make sure we have a file to write.
  if (!this->Stream && !this->FileName && !this->WriteToOutputString)
  {
    svtkErrorMacro("Writer called with no FileName set.");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return 0;
  }

  // We are just starting to write.  Do not call
  // UpdateProgressDiscrete because we want a 0 progress callback the
  // first time.
  this->UpdateProgress(0);

  // Initialize progress range to entire 0..1 range.
  float wholeProgressRange[2] = { 0.f, 1.f };
  this->SetProgressRange(wholeProgressRange, 0, 1);

  // Check input validity and call the real writing code.
  int result = this->WriteInternal();

  // If writing failed, delete the file.
  if (!result)
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->DeleteAFile();
  }

  // We have finished writing.
  this->UpdateProgressDiscrete(1);

  return result;
}
//----------------------------------------------------------------------------
int svtkXMLWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    svtkErrorMacro("No input provided!");
    return 0;
  }

  // always write even if the data hasn't changed
  this->Modified();

  this->Update();
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::OpenStream()
{
  if (this->Stream)
  {
    // Rewind stream to the beginning.
    this->Stream->seekp(0);
  }
  else
  {
    if (this->WriteToOutputString)
    {
      if (!this->OpenString())
      {
        return 0;
      }
    }
    else
    {
      if (!this->OpenFile())
      {
        return 0;
      }
    }
  }

  // Make sure sufficient precision is used in the ascii
  // representation of data and meta-data.
  this->Stream->precision(11);

  // Setup the output streams.
  this->DataStream->SetStream(this->Stream);

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::OpenFile()
{
  delete this->OutFile;
  this->OutFile = nullptr;

  // Strip trailing whitespace from the filename.
  int len = static_cast<int>(strlen(this->FileName));
  for (int i = len - 1; i >= 0; i--)
  {
    if (isalnum(this->FileName[i]))
    {
      break;
    }
    this->FileName[i] = 0;
  }

  std::ios_base::openmode mode = ios::out;
#ifdef _WIN32
  mode |= ios::binary;
#endif
  this->OutFile = new svtksys::ofstream(this->FileName, mode);
  if (!this->OutFile || !*this->OutFile)
  {
    svtkErrorMacro("Error opening output file \"" << this->FileName << "\"");
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    svtkErrorMacro(
      "Error code \"" << svtkErrorCode::GetStringFromErrorCode(this->GetErrorCode()) << "\"");
    return 0;
  }
  this->Stream = this->OutFile;

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::OpenString()
{
  delete this->OutStringStream;
  this->OutStringStream = new std::ostringstream();
  this->Stream = this->OutStringStream;

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLWriter::CloseStream()
{
  // Cleanup the output streams.
  this->DataStream->SetStream(nullptr);

  if (this->WriteToOutputString)
  {
    this->CloseString();
  }
  else
  {
    this->CloseFile();
  }

  this->Stream = nullptr;
}

//----------------------------------------------------------------------------
void svtkXMLWriter::CloseFile()
{
  if (this->OutFile)
  {
    // We opened a file.  Close it.
    delete this->OutFile;
    this->OutFile = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::CloseString()
{
  if (this->OutStringStream)
  {
    this->OutputString = this->OutStringStream->str();
    delete this->OutStringStream;
    this->OutStringStream = nullptr;
  }
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteInternal()
{
  if (!this->OpenStream())
  {
    return 0;
  }

  (*this->Stream).imbue(std::locale::classic());

  // Tell the subclass to write the data.
  int result = this->WriteData();

  // if user manipulate execution don't try closing file
  if (this->UserContinueExecuting != 1)
  {
    this->CloseStream();
  }

  return result;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::GetDataSetMajorVersion()
{
  if (this->UsePreviousVersion)
  {
    return (this->HeaderType == svtkXMLWriter::UInt64) ? 1 : 0;
  }
  else
  {
    return svtkXMLReaderMajorVersion;
  }
}

//----------------------------------------------------------------------------
int svtkXMLWriter::GetDataSetMinorVersion()
{
  if (this->UsePreviousVersion)
  {
    return (this->HeaderType == svtkXMLWriter::UInt64) ? 0 : 1;
  }
  else
  {
    return svtkXMLReaderMinorVersion;
  }
}

//----------------------------------------------------------------------------
svtkDataSet* svtkXMLWriter::GetInputAsDataSet()
{
  return static_cast<svtkDataSet*>(this->GetInput());
}

//----------------------------------------------------------------------------
int svtkXMLWriter::StartFile()
{
  ostream& os = *(this->Stream);

  // If this will really be a valid XML file, put the XML header at
  // the top.
  if (this->EncodeAppendedData)
  {
    os << "<?xml version=\"1.0\"?>\n";
  }

  os.imbue(std::locale::classic());

  // Open the document-level element.  This will contain the rest of
  // the elements.
  os << "<SVTKFile";
  this->WriteFileAttributes();
  os << ">\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteFileAttributes()
{
  ostream& os = *(this->Stream);

  // Write the file's type.
  this->WriteStringAttribute("type", this->GetDataSetName());

  // Write the version number of the file.
  os << " version=\"" << this->GetDataSetMajorVersion() << "." << this->GetDataSetMinorVersion()
     << "\"";

  // Write the byte order for the file.
  if (this->ByteOrder == svtkXMLWriter::BigEndian)
  {
    os << " byte_order=\"BigEndian\"";
  }
  else
  {
    os << " byte_order=\"LittleEndian\"";
  }

  // Write the header type for binary data.
  if (this->HeaderType == svtkXMLWriter::UInt64)
  {
    os << " header_type=\"UInt64\"";
  }
  else
  {
    os << " header_type=\"UInt32\"";
  }

  // Write the compressor that will be used for the file.
  if (this->Compressor)
  {
    os << " compressor=\"" << this->Compressor->GetClassName() << "\"";
  }
}

//----------------------------------------------------------------------------
int svtkXMLWriter::EndFile()
{
  ostream& os = *(this->Stream);

  // Close the document-level element.
  os << "</SVTKFile>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLWriter::DeleteAFile()
{
  if (!this->Stream && this->FileName)
  {
    this->DeleteAFile(this->FileName);
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::DeleteAFile(const char* name)
{
  unlink(name);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::StartAppendedData()
{
  ostream& os = *(this->Stream);
  os << "  <AppendedData encoding=\"" << (this->EncodeAppendedData ? "base64" : "raw") << "\">\n";
  os << "   _";
  this->AppendedDataPosition = os.tellp();

  // Setup proper output encoding.
  if (this->EncodeAppendedData)
  {
    svtkBase64OutputStream* base64 = svtkBase64OutputStream::New();
    this->SetDataStream(base64);
    base64->Delete();
  }
  else
  {
    svtkOutputStream* raw = svtkOutputStream::New();
    this->SetDataStream(raw);
    raw->Delete();
  }

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::EndAppendedData()
{
  ostream& os = *(this->Stream);
  os << "\n";
  os << "  </AppendedData>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
svtkTypeInt64 svtkXMLWriter::ReserveAttributeSpace(const char* attr, size_t length)
{
  // Save the starting stream position.
  ostream& os = *(this->Stream);
  svtkTypeInt64 startPosition = os.tellp();

  // By default write an empty valid xml: attr="".  In most case it
  // will be overwritten but we guarantee that the xml produced will
  // be valid in case we stop writing too early.
  os << " " << attr << "=\"\"";

  // Now reserve space for the value.
  for (size_t i = 0; i < length; ++i)
  {
    os << " ";
  }

  // Flush the stream to make sure the system tries to write now and
  // test for a write error reported by the system.
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }

  // Return the position at which to write the attribute later.
  return startPosition;
}

//----------------------------------------------------------------------------
svtkTypeInt64 svtkXMLWriter::GetAppendedDataOffset()
{
  svtkTypeInt64 pos = this->Stream->tellp();
  return (pos - this->AppendedDataPosition);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteAppendedDataOffset(
  svtkTypeInt64 streamPos, svtkTypeInt64& lastoffset, const char* attr)
{
  // Write an XML attribute with the given name.  The value is the
  // current appended data offset.  Starts writing at the given stream
  // position, and returns the ending position.  If attr is 0, writes
  // only the double quotes.  In all cases, the final stream position
  // is left the same as before the call.
  ostream& os = *(this->Stream);
  svtkTypeInt64 returnPos = os.tellp();
  svtkTypeInt64 offset = returnPos - this->AppendedDataPosition;
  lastoffset = offset; // saving result
  os.seekp(std::streampos(streamPos));
  if (attr)
  {
    os << " " << attr << "=";
  }
  os << "\"" << offset << "\"";
  os.seekp(std::streampos(returnPos));

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::ForwardAppendedDataOffset(
  svtkTypeInt64 streamPos, svtkTypeInt64 offset, const char* attr)
{
  ostream& os = *(this->Stream);
  std::streampos returnPos = os.tellp();
  os.seekp(std::streampos(streamPos));
  if (attr)
  {
    os << " " << attr << "=";
  }
  os << "\"" << offset << "\"";
  os.seekp(returnPos);

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::ForwardAppendedDataDouble(svtkTypeInt64 streamPos, double value, const char* attr)
{
  ostream& os = *(this->Stream);
  std::streampos returnPos = os.tellp();
  os.seekp(std::streampos(streamPos));
  if (attr)
  {
    os << " " << attr << "=";
  }
  os << "\"" << value << "\"";
  os.seekp(returnPos);

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteBinaryData(svtkAbstractArray* a)
{
  int wordType = a->GetDataType();

  size_t dataSize;
  if (wordType != SVTK_BIT)
  {
    dataSize = this->GetOutputWordTypeSize(wordType) * static_cast<size_t>(a->GetDataSize());
  }
  else
  { // svtkBitArray returns 0 for GetDataSize:
    dataSize = (a->GetNumberOfValues() + 7) / 8;
  }

  if (this->Compressor)
  {
    // Need to compress the data.  Create compression header.  This
    // reserves enough space in the output.
    if (!this->CreateCompressionHeader(dataSize))
    {
      return 0;
    }
    // Start writing the data.
    int result = this->DataStream->StartWriting();

    // Process the actual data.
    if (result && !this->WriteBinaryDataInternal(a))
    {
      result = 0;
    }

    // Finish writing the data.
    if (result && !this->DataStream->EndWriting())
    {
      result = 0;
    }

    // Go back and write the real compression header in its proper place.
    if (result && !this->WriteCompressionHeader())
    {
      result = 0;
    }

    // Destroy the compression header if it was used.
    delete this->CompressionHeader;
    this->CompressionHeader = nullptr;

    return result;
  }
  else
  {
    // Start writing the data.
    if (!this->DataStream->StartWriting())
    {
      return 0;
    }

    // No data compression.  The header is just the length of the data.
    std::unique_ptr<svtkXMLDataHeader> uh(svtkXMLDataHeader::New(this->HeaderType, 1));
    if (!uh->Set(0, dataSize))
    {
      svtkErrorMacro("Array \"" << a->GetName() << "\" is too large.  Set HeaderType to UInt64.");
      this->SetErrorCode(svtkErrorCode::FileFormatError);
      return 0;
    }
    this->PerformByteSwap(uh->Data(), uh->WordCount(), uh->WordSize());
    int writeRes = this->DataStream->Write(uh->Data(), uh->DataSize());
    this->Stream->flush();
    if (this->Stream->fail())
    {
      this->SetErrorCode(svtkErrorCode::GetLastSystemError());
      return 0;
    }
    if (!writeRes)
    {
      return 0;
    }

    // Process the actual data.
    if (!this->WriteBinaryDataInternal(a))
    {
      return 0;
    }

    // Finish writing the data.
    if (!this->DataStream->EndWriting())
    {
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteBinaryDataInternal(svtkAbstractArray* a)
{
  // Break into blocks and handle each one separately.  This allows
  // for better random access when reading compressed data and saves
  // memory during writing.

  // The size of the blocks written (before compression) is
  // this->BlockSize.  We need to support the possibility that the
  // size of data in memory and the size on disk are different.  This
  // is necessary to allow svtkIdType to be converted to UInt32 for
  // writing.

  int wordType = a->GetDataType();
  size_t memWordSize = this->GetWordTypeSize(wordType);
  size_t outWordSize = this->GetOutputWordTypeSize(wordType);

#ifdef SVTK_USE_64BIT_IDS
  // If the type is svtkIdType, it may need to be converted to the type
  // requested for output.
  if ((wordType == SVTK_ID_TYPE) && (this->IdType == svtkXMLWriter::Int32))
  {
    size_t blockWordsEstimate = this->BlockSize / outWordSize;
    this->Int32IdTypeBuffer = new Int32IdType[blockWordsEstimate];
  }
#endif

  // Decide if we need to byte swap.
#ifdef SVTK_WORDS_BIGENDIAN
  if (outWordSize > 1 && this->ByteOrder != svtkXMLWriter::BigEndian)
#else
  if (outWordSize > 1 && this->ByteOrder != svtkXMLWriter::LittleEndian)
#endif
  {
    // We need to byte swap.  Prepare a buffer large enough for one
    // block.
    if (this->Int32IdTypeBuffer)
    {
      // Just swap in-place in the converted id-type buffer.
      this->ByteSwapBuffer = reinterpret_cast<unsigned char*>(this->Int32IdTypeBuffer);
    }
    else
    {
      // The maximum nlock size if this->BlockSize. The actual data in the block
      // may be lesser.
      this->ByteSwapBuffer = new unsigned char[this->BlockSize];
    }
  }
  int ret;

  size_t numValues = static_cast<size_t>(a->GetNumberOfComponents() * a->GetNumberOfTuples());

  if (wordType == SVTK_STRING)
  {
    svtkArrayIterator* aiter = a->NewIterator();
    svtkArrayIteratorTemplate<svtkStdString>* iter =
      svtkArrayIteratorTemplate<svtkStdString>::SafeDownCast(aiter);
    if (iter)
    {
      ret = svtkXMLWriterWriteBinaryDataBlocks(this, iter, wordType, outWordSize, numValues, 1);
    }
    else
    {
      svtkWarningMacro("Unsupported iterator for data type : " << wordType);
      ret = 0;
    }
    aiter->Delete();
  }
  else if (svtkDataArray* da = svtkArrayDownCast<svtkDataArray>(a))
  {
    // Create a dispatcher that also handles svtkBitArray:
    using svtkArrayDispatch::Arrays;
    using XMLArrays = svtkTypeList::Append<Arrays, svtkBitArray>::Result;
    using Dispatcher = svtkArrayDispatch::DispatchByArray<XMLArrays>;

    WriteBinaryDataBlockWorker worker(this, wordType, memWordSize, outWordSize, numValues);
    if (!Dispatcher::Execute(da, worker))
    {
      switch (wordType)
      {
#if !defined(SVTK_LEGACY_REMOVE)
        case SVTK___INT64:
        case SVTK_UNSIGNED___INT64:
#endif
        case SVTK_LONG_LONG:
        case SVTK_UNSIGNED_LONG_LONG:
#ifdef SVTK_USE_64BIT_IDS
        case SVTK_ID_TYPE:
#endif
          svtkWarningMacro("Using legacy svtkDataArray API, which may result "
                          "in precision loss");
          break;
        default:
          break;
      }

      switch (wordType)
      {
        svtkTemplateMacro(WriteDataArrayFallback(static_cast<SVTK_TT*>(nullptr), da, worker));
        default:
          svtkWarningMacro("Unsupported data type: " << wordType);
          break;
      }
    }
    ret = worker.Result ? 1 : 0;
  }
  else
  {
    svtkWarningMacro("Not writing array '" << a->GetName()
                                          << "': Unsupported "
                                             "array type: "
                                          << a->GetClassName());
    ret = 0;
  }

  // Free the byte swap buffer if it was allocated.
  if (!this->Int32IdTypeBuffer)
  {
    delete[] this->ByteSwapBuffer;
    this->ByteSwapBuffer = nullptr;
  }

#ifdef SVTK_USE_64BIT_IDS
  // Free the id-type conversion buffer if it was allocated.
  delete[] this->Int32IdTypeBuffer;
  this->Int32IdTypeBuffer = nullptr;
  // The swap and ID buffers are shared. Guard against double frees:
  this->ByteSwapBuffer = nullptr;
#endif
  return ret;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteBinaryDataBlock(unsigned char* in_data, size_t numWords, int wordType)
{
  unsigned char* data = in_data;
#ifdef SVTK_USE_64BIT_IDS
  // If the type is svtkIdType, it may need to be converted to the type
  // requested for output.
  if ((wordType == SVTK_ID_TYPE) && (this->IdType == svtkXMLWriter::Int32))
  {
    svtkIdType* idBuffer = reinterpret_cast<svtkIdType*>(in_data);

    for (size_t i = 0; i < numWords; ++i)
    {
      this->Int32IdTypeBuffer[i] = static_cast<Int32IdType>(idBuffer[i]);
    }

    data = reinterpret_cast<unsigned char*>(this->Int32IdTypeBuffer);
  }
#endif

  // Get the word size of the data buffer.  This is now the size that
  // will be written.
  size_t wordSize = this->GetOutputWordTypeSize(wordType);

  // If we need to byte swap, do it now.
  if (this->ByteSwapBuffer)
  {
    // If we are converting svtkIdType to 32-bit integer data, the data
    // are already in the byte swap buffer because we share the
    // conversion buffer.  Otherwise, we need to copy the data before
    // byte swapping.
    if (data != this->ByteSwapBuffer)
    {
      memcpy(this->ByteSwapBuffer, data, numWords * wordSize);
      data = this->ByteSwapBuffer;
    }
    this->PerformByteSwap(this->ByteSwapBuffer, numWords, wordSize);
  }

  // Now pass the data to the next write phase.
  if (this->Compressor)
  {
    int res = this->WriteCompressionBlock(data, numWords * wordSize);
    this->Stream->flush();
    if (this->Stream->fail())
    {
      this->SetErrorCode(svtkErrorCode::GetLastSystemError());
      return 0;
    }
    return res;
  }
  else
  {
    int res = this->DataStream->Write(data, numWords * wordSize);
    this->Stream->flush();
    if (this->Stream->fail())
    {
      this->SetErrorCode(svtkErrorCode::GetLastSystemError());
      return 0;
    }
    return res;
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::PerformByteSwap(void* data, size_t numWords, size_t wordSize)
{
  char* ptr = static_cast<char*>(data);
  if (this->ByteOrder == svtkXMLWriter::BigEndian)
  {
    switch (wordSize)
    {
      case 1:
        break;
      case 2:
        svtkByteSwap::Swap2BERange(ptr, numWords);
        break;
      case 4:
        svtkByteSwap::Swap4BERange(ptr, numWords);
        break;
      case 8:
        svtkByteSwap::Swap8BERange(ptr, numWords);
        break;
      default:
        svtkErrorMacro("Unsupported data type size " << wordSize);
    }
  }
  else
  {
    switch (wordSize)
    {
      case 1:
        break;
      case 2:
        svtkByteSwap::Swap2LERange(ptr, numWords);
        break;
      case 4:
        svtkByteSwap::Swap4LERange(ptr, numWords);
        break;
      case 8:
        svtkByteSwap::Swap8LERange(ptr, numWords);
        break;
      default:
        svtkErrorMacro("Unsupported data type size " << wordSize);
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetDataStream(svtkOutputStream* arg)
{
  if (this->DataStream != arg)
  {
    if (this->DataStream != nullptr)
    {
      this->DataStream->UnRegister(this);
    }
    this->DataStream = arg;
    if (this->DataStream != nullptr)
    {
      this->DataStream->Register(this);
      this->DataStream->SetStream(this->Stream);
    }
  }
}

//----------------------------------------------------------------------------
int svtkXMLWriter::CreateCompressionHeader(size_t size)
{
  // Allocate and initialize the compression header.
  // The format is this:
  //  struct header {
  //    HeaderType number_of_blocks;
  //    HeaderType uncompressed_block_size;
  //    HeaderType uncompressed_last_block_size;
  //    HeaderType compressed_block_sizes[number_of_blocks];
  //  }

  // Find the size and number of blocks.
  size_t numFullBlocks = size / this->BlockSize;
  size_t lastBlockSize = size % this->BlockSize;
  size_t numBlocks = numFullBlocks + (lastBlockSize ? 1 : 0);
  this->CompressionHeader = svtkXMLDataHeader::New(this->HeaderType, 3 + numBlocks);

  // Write out dummy header data.
  this->CompressionHeaderPosition = this->Stream->tellp();
  int result = (this->DataStream->StartWriting() &&
    this->DataStream->Write(this->CompressionHeader->Data(), this->CompressionHeader->DataSize()) &&
    this->DataStream->EndWriting());

  this->Stream->flush();
  if (this->Stream->fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    return 0;
  }

  // Fill in known header data now.
  this->CompressionHeader->Set(0, numBlocks);
  this->CompressionHeader->Set(1, this->BlockSize);
  this->CompressionHeader->Set(2, lastBlockSize);

  // Initialize counter for block writing.
  this->CompressionBlockNumber = 0;

  return result;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteCompressionBlock(unsigned char* data, size_t size)
{
  // Compress the data.
  svtkUnsignedCharArray* outputArray = this->Compressor->Compress(data, size);

  // Find the compressed size.
  size_t outputSize = outputArray->GetNumberOfTuples();
  unsigned char* outputPointer = outputArray->GetPointer(0);

  // Write the compressed data.
  int result = this->DataStream->Write(outputPointer, outputSize);
  this->Stream->flush();
  if (this->Stream->fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }

  // Store the resulting compressed size in the compression header.
  this->CompressionHeader->Set(3 + this->CompressionBlockNumber++, outputSize);

  outputArray->Delete();

  return result;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteCompressionHeader()
{
  // Write real compression header back into stream.
  std::streampos returnPosition = this->Stream->tellp();

  // Need to byte-swap header.
  this->PerformByteSwap(this->CompressionHeader->Data(), this->CompressionHeader->WordCount(),
    this->CompressionHeader->WordSize());

  if (!this->Stream->seekp(std::streampos(this->CompressionHeaderPosition)))
  {
    return 0;
  }
  int result = (this->DataStream->StartWriting() &&
    this->DataStream->Write(this->CompressionHeader->Data(), this->CompressionHeader->DataSize()) &&
    this->DataStream->EndWriting());
  this->Stream->flush();
  if (this->Stream->fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    return 0;
  }

  if (!this->Stream->seekp(returnPosition))
  {
    return 0;
  }
  return result;
}

//----------------------------------------------------------------------------
size_t svtkXMLWriter::GetOutputWordTypeSize(int dataType)
{
#ifdef SVTK_USE_64BIT_IDS
  // If the type is svtkIdType, it may need to be converted to the type
  // requested for output.
  if ((dataType == SVTK_ID_TYPE) && (this->IdType == svtkXMLWriter::Int32))
  {
    return 4;
  }
#endif
  return this->GetWordTypeSize(dataType);
}

//----------------------------------------------------------------------------
template <class T>
size_t svtkXMLWriterGetWordTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
size_t svtkXMLWriter::GetWordTypeSize(int dataType)
{
  size_t size = 1;
  switch (dataType)
  {
    svtkTemplateMacro(size = svtkXMLWriterGetWordTypeSize(static_cast<SVTK_TT*>(nullptr)));

    case SVTK_STRING:
      size = sizeof(svtkStdString::value_type);
      break;

    case SVTK_BIT:
      size = 1;
      break;

    default:
      svtkWarningMacro("Unsupported data type: " << dataType);
      break;
  }
  return size;
}

//----------------------------------------------------------------------------
const char* svtkXMLWriter::GetWordTypeName(int dataType)
{
  char isSigned = 0;
  int size = 0;

  // These string values must match svtkXMLDataElement::GetWordTypeAttribute().
  switch (dataType)
  {
    case SVTK_BIT:
      return "Bit";
    case SVTK_STRING:
      return "String";
    case SVTK_FLOAT:
      return "Float32";
    case SVTK_DOUBLE:
      return "Float64";
    case SVTK_ID_TYPE:
    {
      switch (this->IdType)
      {
        case svtkXMLWriter::Int32:
          return "Int32";
        case svtkXMLWriter::Int64:
          return "Int64";
        default:
          return nullptr;
      }
    }
#if SVTK_TYPE_CHAR_IS_SIGNED
    case SVTK_CHAR:
      isSigned = 1;
      size = sizeof(char);
      break;
#else
    case SVTK_CHAR:
      isSigned = 0;
      size = sizeof(char);
      break;
#endif
    case SVTK_INT:
      isSigned = 1;
      size = sizeof(int);
      break;
    case SVTK_LONG:
      isSigned = 1;
      size = sizeof(long);
      break;
    case SVTK_SHORT:
      isSigned = 1;
      size = sizeof(short);
      break;
    case SVTK_SIGNED_CHAR:
      isSigned = 1;
      size = sizeof(signed char);
      break;
    case SVTK_UNSIGNED_CHAR:
      isSigned = 0;
      size = sizeof(unsigned char);
      break;
    case SVTK_UNSIGNED_INT:
      isSigned = 0;
      size = sizeof(unsigned int);
      break;
    case SVTK_UNSIGNED_LONG:
      isSigned = 0;
      size = sizeof(unsigned long);
      break;
    case SVTK_UNSIGNED_SHORT:
      isSigned = 0;
      size = sizeof(unsigned short);
      break;
    case SVTK_LONG_LONG:
      isSigned = 1;
      size = sizeof(long long);
      break;
    case SVTK_UNSIGNED_LONG_LONG:
      isSigned = 0;
      size = sizeof(unsigned long long);
      break;
    default:
    {
      svtkWarningMacro("Unsupported data type: " << dataType);
    }
    break;
  }
  const char* type = nullptr;
  switch (size)
  {
    case 1:
      type = isSigned ? "Int8" : "UInt8";
      break;
    case 2:
      type = isSigned ? "Int16" : "UInt16";
      break;
    case 4:
      type = isSigned ? "Int32" : "UInt32";
      break;
    case 8:
      type = isSigned ? "Int64" : "UInt64";
      break;
    default:
    {
      svtkErrorMacro("Data type size " << size << " not supported by SVTK XML format.");
    }
  }
  return type;
}

//----------------------------------------------------------------------------
template <class T>
int svtkXMLWriterWriteVectorAttribute(ostream& os, const char* name, int length, T* data)
{
  svtkNumberToString convert;
  os << " " << name << "=\"";
  if (length)
  {
    os << convert(data[0]);
    for (int i = 1; i < length; ++i)
    {
      os << " " << convert(data[i]);
    }
  }
  os << "\"";
  return os ? 1 : 0;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteScalarAttribute(const char* name, int data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteScalarAttribute(const char* name, float data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteScalarAttribute(const char* name, double data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}

//----------------------------------------------------------------------------
#ifdef SVTK_USE_64BIT_IDS
int svtkXMLWriter::WriteScalarAttribute(const char* name, svtkIdType data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}
#endif

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteVectorAttribute(const char* name, int length, int* data)
{
  int res = svtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);

  this->Stream->flush();
  if (this->Stream->fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  return res;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteVectorAttribute(const char* name, int length, float* data)
{
  int res = svtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);

  this->Stream->flush();
  if (this->Stream->fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  return res;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteVectorAttribute(const char* name, int length, double* data)
{
  int res = svtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);

  this->Stream->flush();
  if (this->Stream->fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  return res;
}

//----------------------------------------------------------------------------
#ifdef SVTK_USE_64BIT_IDS
int svtkXMLWriter::WriteVectorAttribute(const char* name, int length, svtkIdType* data)
{
  int res = svtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);

  this->Stream->flush();
  if (this->Stream->fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  return res;
}
#endif

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteDataModeAttribute(const char* name)
{
  ostream& os = *(this->Stream);
  os << " " << name << "=\"";
  if (this->DataMode == svtkXMLWriter::Appended)
  {
    os << "appended";
  }
  else if (this->DataMode == svtkXMLWriter::Binary)
  {
    os << "binary";
  }
  else
  {
    os << "ascii";
  }
  os << "\"";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  return (os ? 1 : 0);
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteWordTypeAttribute(const char* name, int dataType)
{
  ostream& os = *(this->Stream);
  const char* value = this->GetWordTypeName(dataType);
  if (!value)
  {
    return 0;
  }
  os << " " << name << "=\"" << value << "\"";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  return os ? 1 : 0;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteStringAttribute(const char* name, const char* value)
{
  ostream& os = *(this->Stream);
  os << " " << name << "=\"" << value << "\"";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  return os ? 1 : 0;
}

//----------------------------------------------------------------------------
// Methods used for serializing svtkInformation. ------------------------------
namespace
{

// Write generic key information to element.
void prepElementForInfo(svtkInformationKey* key, svtkXMLDataElement* element)
{
  element->SetName("InformationKey");
  element->SetAttribute("name", key->GetName());
  element->SetAttribute("location", key->GetLocation());
}

template <class KeyType>
void writeScalarInfo(KeyType* key, svtkInformation* info, std::ostream& os, svtkIndent indent)
{
  svtkNew<svtkXMLDataElement> element;
  prepElementForInfo(key, element);

  std::ostringstream str;
  str.precision(11); // Same used for ASCII array data.
  str << key->Get(info);

  str.str("");
  str << key->Get(info);
  element->SetCharacterData(str.str().c_str(), static_cast<int>(str.str().size()));

  element->PrintXML(os, indent);
}

template <class KeyType>
void writeVectorInfo(KeyType* key, svtkInformation* info, std::ostream& os, svtkIndent indent)
{
  svtkNew<svtkXMLDataElement> element;
  prepElementForInfo(key, element);

  std::ostringstream str;
  str.precision(11); // Same used for ASCII array data.
  int length = key->Length(info);
  str << length;
  element->SetAttribute("length", str.str().c_str());

  for (int i = 0; i < length; ++i)
  {
    svtkNew<svtkXMLDataElement> value;
    value->SetName("Value");

    str.str("");
    str << i;
    value->SetAttribute("index", str.str().c_str());

    str.str("");
    str << key->Get(info, i);
    value->SetCharacterData(str.str().c_str(), static_cast<int>(str.str().size()));

    element->AddNestedElement(value);
  }

  element->PrintXML(os, indent);
}

} // end anon namespace
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
bool svtkXMLWriter::WriteInformation(svtkInformation* info, svtkIndent indent)
{
  bool result = false;
  svtkNew<svtkInformationIterator> iter;
  iter->SetInformationWeak(info);
  svtkInformationKey* key = nullptr;
  svtkIndent nextIndent = indent.GetNextIndent();
  for (iter->InitTraversal(); (key = iter->GetCurrentKey()); iter->GoToNextItem())
  {
    svtkInformationDoubleKey* dKey = nullptr;
    svtkInformationDoubleVectorKey* dvKey = nullptr;
    svtkInformationIdTypeKey* idKey = nullptr;
    svtkInformationIntegerKey* iKey = nullptr;
    svtkInformationIntegerVectorKey* ivKey = nullptr;
    svtkInformationStringKey* sKey = nullptr;
    svtkInformationStringVectorKey* svKey = nullptr;
    svtkInformationUnsignedLongKey* ulKey = nullptr;
    typedef svtkInformationQuadratureSchemeDefinitionVectorKey QuadDictKey;
    QuadDictKey* qdKey = nullptr;
    if ((dKey = svtkInformationDoubleKey::SafeDownCast(key)))
    {
      writeScalarInfo(dKey, info, *this->Stream, nextIndent);
      result = true;
    }
    else if ((dvKey = svtkInformationDoubleVectorKey::SafeDownCast(key)))
    {
      writeVectorInfo(dvKey, info, *this->Stream, nextIndent);
      result = true;
    }
    else if ((idKey = svtkInformationIdTypeKey::SafeDownCast(key)))
    {
      writeScalarInfo(idKey, info, *this->Stream, nextIndent);
      result = true;
    }
    else if ((iKey = svtkInformationIntegerKey::SafeDownCast(key)))
    {
      writeScalarInfo(iKey, info, *this->Stream, nextIndent);
      result = true;
    }
    else if ((ivKey = svtkInformationIntegerVectorKey::SafeDownCast(key)))
    {
      writeVectorInfo(ivKey, info, *this->Stream, nextIndent);
      result = true;
    }
    else if ((sKey = svtkInformationStringKey::SafeDownCast(key)))
    {
      writeScalarInfo(sKey, info, *this->Stream, nextIndent);
      result = true;
    }
    else if ((svKey = svtkInformationStringVectorKey::SafeDownCast(key)))
    {
      writeVectorInfo(svKey, info, *this->Stream, nextIndent);
      result = true;
    }
    else if ((ulKey = svtkInformationUnsignedLongKey::SafeDownCast(key)))
    {
      writeScalarInfo(ulKey, info, *this->Stream, nextIndent);
      result = true;
    }
    else if ((qdKey = QuadDictKey::SafeDownCast(key)))
    { // Special case:
      svtkNew<svtkXMLDataElement> element;
      qdKey->SaveState(info, element);
      element->PrintXML(*this->Stream, nextIndent);
      result = true;
    }
    else
    {
      svtkDebugMacro("Could not serialize information with key " << key->GetLocation()
                                                                << "::" << key->GetName()
                                                                << ": "
                                                                   "Unsupported key type '"
                                                                << key->GetClassName() << "'.");
    }
  }
  return result;
}

//----------------------------------------------------------------------------
// This method is provided so that the specialization code for certain types
// can be minimal.
template <class T>
inline ostream& svtkXMLWriteAsciiValue(ostream& os, const T& value)
{
  svtkNumberToString convert;
  os << convert(value);
  return os;
}

//----------------------------------------------------------------------------
template <>
inline ostream& svtkXMLWriteAsciiValue(ostream& os, const char& c)
{
  os << short(c);
  return os;
}

//----------------------------------------------------------------------------
template <>
inline ostream& svtkXMLWriteAsciiValue(ostream& os, const unsigned char& c)
{
  os << static_cast<unsigned short>(c);
  return os;
}

//----------------------------------------------------------------------------
template <>
inline ostream& svtkXMLWriteAsciiValue(ostream& os, const signed char& c)
{
  os << short(c);
  return os;
}

//----------------------------------------------------------------------------
template <>
inline ostream& svtkXMLWriteAsciiValue(ostream& os, const svtkStdString& str)
{
  svtkStdString::const_iterator iter;
  for (iter = str.begin(); iter != str.end(); ++iter)
  {
    svtkXMLWriteAsciiValue(os, *iter);
    os << " ";
  }
  char delim = 0x0;
  return svtkXMLWriteAsciiValue(os, delim);
}

//----------------------------------------------------------------------------
template <class iterT>
int svtkXMLWriteAsciiData(ostream& os, iterT* iter, svtkIndent indent)
{
  if (!iter)
  {
    return 0;
  }
  size_t columns = 6;
  size_t length = iter->GetNumberOfTuples() * iter->GetNumberOfComponents();

  size_t rows = length / columns;
  size_t lastRowLength = length % columns;
  svtkIdType index = 0;
  for (size_t r = 0; r < rows; ++r)
  {
    os << indent;
    svtkXMLWriteAsciiValue(os, iter->GetValue(index++));
    for (size_t c = 1; c < columns; ++c)
    {
      os << " ";
      svtkXMLWriteAsciiValue(os, iter->GetValue(index++));
    }
    os << "\n";
  }
  if (lastRowLength > 0)
  {
    os << indent;
    svtkXMLWriteAsciiValue(os, iter->GetValue(index++));
    for (size_t c = 1; c < lastRowLength; ++c)
    {
      os << " ";
      svtkXMLWriteAsciiValue(os, iter->GetValue(index++));
    }
    os << "\n";
  }
  return os ? 1 : 0;
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WriteAsciiData(svtkAbstractArray* a, svtkIndent indent)
{
  svtkArrayIterator* iter = a->NewIterator();
  ostream& os = *(this->Stream);
  int ret;
  switch (a->GetDataType())
  {
    svtkArrayIteratorTemplateMacro(
      ret = svtkXMLWriteAsciiData(os, static_cast<SVTK_TT*>(iter), indent));
    default:
      ret = 0;
      break;
  }
  iter->Delete();
  return ret;
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteArrayAppended(svtkAbstractArray* a, svtkIndent indent, OffsetsManager& offs,
  const char* alternateName, int writeNumTuples, int timestep)
{
  ostream& os = *(this->Stream);
  // Write the header <DataArray or <Array:
  this->WriteArrayHeader(a, indent, alternateName, writeNumTuples, timestep);
  int shortFormatTag = 1; // close with: />
  //
  if (svtkArrayDownCast<svtkDataArray>(a))
  {
    // write the scalar range of this data array, we reserver space because we
    // don't actually have the data at this point
    offs.GetRangeMinPosition(timestep) = this->ReserveAttributeSpace("RangeMin");
    offs.GetRangeMaxPosition(timestep) = this->ReserveAttributeSpace("RangeMax");
  }
  else
  {
    // ranges are not written for non-data arrays.
    offs.GetRangeMinPosition(timestep) = -1;
    offs.GetRangeMaxPosition(timestep) = -1;
  }

  //
  offs.GetPosition(timestep) = this->ReserveAttributeSpace("offset");

  // Write information in the recognized keys associated with this array.
  svtkInformation* info = a->GetInformation();
  bool hasInfo = info && info->GetNumberOfKeys() > 0;
  if (hasInfo)
  {
    // close header with </DataArray> or </Array> before writing information:
    os << ">" << endl;
    shortFormatTag = 0; // Tells WriteArrayFooter that the header is closed.

    this->WriteInformation(info, indent);
  }

  // Close tag.
  this->WriteArrayFooter(os, indent, a, shortFormatTag);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteArrayAppendedData(
  svtkAbstractArray* a, svtkTypeInt64 pos, svtkTypeInt64& lastoffset)
{
  this->WriteAppendedDataOffset(pos, lastoffset, "offset");
  this->WriteBinaryData(a);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteArrayHeader(svtkAbstractArray* a, svtkIndent indent,
  const char* alternateName, int writeNumTuples, int timestep)
{
  ostream& os = *(this->Stream);
  if (svtkArrayDownCast<svtkDataArray>(a))
  {
    os << indent << "<DataArray";
  }
  else
  {
    os << indent << "<Array";
  }
  this->WriteWordTypeAttribute("type", a->GetDataType());
  if (a->GetDataType() == SVTK_ID_TYPE)
  {
    this->WriteScalarAttribute("IdType", 1);
  }
  if (alternateName)
  {
    this->WriteStringAttribute("Name", alternateName);
  }
  else if (const char* arrayName = a->GetName())
  {
    this->WriteStringAttribute("Name", arrayName);
  }
  else
  {
    // Generate a name for this array.
    std::ostringstream name;
    void* p = a;
    name << "Array " << p;
    this->WriteStringAttribute("Name", name.str().c_str());
  }
  if (a->GetNumberOfComponents() > 1)
  {
    this->WriteScalarAttribute("NumberOfComponents", a->GetNumberOfComponents());
  }

  // always write out component names, even if only 1 component
  std::ostringstream buff;
  const char* compName = nullptr;
  for (int i = 0; i < a->GetNumberOfComponents(); ++i)
  {
    // get the component names
    buff << "ComponentName" << i;
    compName = a->GetComponentName(i);
    if (compName)
    {
      this->WriteStringAttribute(buff.str().c_str(), compName);
      compName = nullptr;
    }
    buff.str("");
    buff.clear();
  }

  if (this->NumberOfTimeSteps > 1)
  {
    this->WriteScalarAttribute("TimeStep", timestep);
  }
  else
  {
    // assert(timestep == -1); //FieldData problem
  }
  if (writeNumTuples)
  {
    this->WriteScalarAttribute("NumberOfTuples", a->GetNumberOfTuples());
  }

  this->WriteDataModeAttribute("format");
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteArrayFooter(
  ostream& os, svtkIndent indent, svtkAbstractArray* a, int shortFormat)
{
  // Close the tag: </DataArray>, </Array> or />
  if (shortFormat)
  {
    os << "/>" << endl;
  }
  else
  {
    svtkDataArray* da = svtkArrayDownCast<svtkDataArray>(a);
    os << indent << (da ? "</DataArray>" : "</Array>") << "\n";
  }
  // Force write and check for errors.
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteInlineData(svtkAbstractArray* a, svtkIndent indent)
{
  if (this->DataMode == svtkXMLWriter::Binary)
  {
    ostream& os = *(this->Stream);
    os << indent;
    this->WriteBinaryData(a);
    os << "\n";
  }
  else
  {
    this->WriteAsciiData(a, indent);
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteArrayInline(
  svtkAbstractArray* a, svtkIndent indent, const char* alternateName, int writeNumTuples)
{
  ostream& os = *(this->Stream);
  // Write the header <DataArray or <Array:
  this->WriteArrayHeader(a, indent, alternateName, writeNumTuples, 0);
  //
  svtkDataArray* da = svtkArrayDownCast<svtkDataArray>(a);
  if (da)
  {
    // write the range
    this->WriteScalarAttribute("RangeMin", da->GetRange(-1)[0]);
    this->WriteScalarAttribute("RangeMax", da->GetRange(-1)[1]);
  }
  // Close the header
  os << ">\n";
  // Write the inline data.
  this->WriteInlineData(a, indent.GetNextIndent());
  // Write information keys associated with this array.
  svtkInformation* info = a->GetInformation();
  if (info && info->GetNumberOfKeys() > 0)
  {
    this->WriteInformation(info, indent);
  }
  // Close tag.
  this->WriteArrayFooter(os, indent, a, 0);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::UpdateFieldData(svtkFieldData* fieldDataCopy)
{
  svtkDataObject* input = this->GetInput();
  svtkFieldData* fieldData = input->GetFieldData();
  svtkInformation* meta = input->GetInformation();
  bool hasTime = meta->Has(svtkDataObject::DATA_TIME_STEP()) ? true : false;
  if ((!fieldData || !fieldData->GetNumberOfArrays()) && !hasTime)
  {
    fieldDataCopy->Initialize();
    return;
  }

  fieldDataCopy->ShallowCopy(fieldData);
  if (hasTime)
  {
    svtkNew<svtkDoubleArray> time;
    time->SetNumberOfTuples(1);
    time->SetTypedComponent(0, 0, meta->Get(svtkDataObject::DATA_TIME_STEP()));
    time->SetName("TimeValue");
    fieldDataCopy->AddArray(time);
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteFieldData(svtkIndent indent)
{
  svtkNew<svtkFieldData> fieldDataCopy;
  this->UpdateFieldData(fieldDataCopy);

  if (!fieldDataCopy->GetNumberOfArrays())
  {
    return;
  }

  if (this->DataMode == svtkXMLWriter::Appended)
  {
    this->WriteFieldDataAppended(fieldDataCopy, indent, this->FieldDataOM);
  }
  else
  {
    // Write the point data arrays.
    this->WriteFieldDataInline(fieldDataCopy, indent);
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteFieldDataInline(svtkFieldData* fd, svtkIndent indent)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(fd->GetNumberOfArrays());

  os << indent << "<FieldData>\n";

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  for (int i = 0; i < fd->GetNumberOfArrays(); ++i)
  {
    this->SetProgressRange(progressRange, i, fd->GetNumberOfArrays());
    this->WriteArrayInline(fd->GetAbstractArray(i), indent.GetNextIndent(), names[i], 1);
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      this->DestroyStringArray(fd->GetNumberOfArrays(), names);
      return;
    }
  }

  os << indent << "</FieldData>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    this->DestroyStringArray(fd->GetNumberOfArrays(), names);
    return;
  }

  this->DestroyStringArray(fd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePointDataInline(svtkPointData* pd, svtkIndent indent)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(pd->GetNumberOfArrays());

  os << indent << "<PointData";
  this->WriteAttributeIndices(pd, names);

  if (this->ErrorCode != svtkErrorCode::NoError)
  {
    this->DestroyStringArray(pd->GetNumberOfArrays(), names);
    return;
  }

  os << ">\n";

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
  {
    this->SetProgressRange(progressRange, i, pd->GetNumberOfArrays());
    svtkAbstractArray* a = pd->GetAbstractArray(i);
    this->WriteArrayInline(a, indent.GetNextIndent(), names[i]);
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      this->DestroyStringArray(pd->GetNumberOfArrays(), names);
      return;
    }
  }

  os << indent << "</PointData>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    this->DestroyStringArray(pd->GetNumberOfArrays(), names);
    return;
  }

  this->DestroyStringArray(pd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteCellDataInline(svtkCellData* cd, svtkIndent indent)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(cd->GetNumberOfArrays());

  os << indent << "<CellData";
  this->WriteAttributeIndices(cd, names);

  if (this->ErrorCode != svtkErrorCode::NoError)
  {
    this->DestroyStringArray(cd->GetNumberOfArrays(), names);
    return;
  }

  os << ">\n";

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  for (int i = 0; i < cd->GetNumberOfArrays(); ++i)
  {
    this->SetProgressRange(progressRange, i, cd->GetNumberOfArrays());
    svtkAbstractArray* a = cd->GetAbstractArray(i);
    this->WriteArrayInline(a, indent.GetNextIndent(), names[i]);
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      this->DestroyStringArray(cd->GetNumberOfArrays(), names);
      return;
    }
  }

  os << indent << "</CellData>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    this->DestroyStringArray(cd->GetNumberOfArrays(), names);
    return;
  }

  this->DestroyStringArray(cd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteFieldDataAppended(
  svtkFieldData* fd, svtkIndent indent, OffsetsManagerGroup* fdManager)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(fd->GetNumberOfArrays());

  os << indent << "<FieldData>\n";

  // When we want to write index arrays with String Arrays, we will
  // have to determine the actual arrays written out to the file
  // and allocate the fdManager accordingly.
  fdManager->Allocate(fd->GetNumberOfArrays());
  for (int i = 0; i < fd->GetNumberOfArrays(); ++i)
  {
    fdManager->GetElement(i).Allocate(1);
    this->WriteArrayAppended(
      fd->GetAbstractArray(i), indent.GetNextIndent(), fdManager->GetElement(i), names[i], 1, 0);
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      this->DestroyStringArray(fd->GetNumberOfArrays(), names);
      return;
    }
  }
  os << indent << "</FieldData>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  this->DestroyStringArray(fd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteFieldDataAppendedData(
  svtkFieldData* fd, int timestep, OffsetsManagerGroup* fdManager)
{
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  fdManager->Allocate(fd->GetNumberOfArrays());
  for (int i = 0; i < fd->GetNumberOfArrays(); ++i)
  {
    fdManager->GetElement(i).Allocate(this->NumberOfTimeSteps);
    this->SetProgressRange(progressRange, i, fd->GetNumberOfArrays());
    this->WriteArrayAppendedData(fd->GetAbstractArray(i),
      fdManager->GetElement(i).GetPosition(timestep),
      fdManager->GetElement(i).GetOffsetValue(timestep));
    svtkDataArray* da = fd->GetArray(i);
    if (da)
    {
      // Write ranges only for data arrays.
      double* range = da->GetRange(-1);
      this->ForwardAppendedDataDouble(
        fdManager->GetElement(i).GetRangeMinPosition(timestep), range[0], "RangeMin");
      this->ForwardAppendedDataDouble(
        fdManager->GetElement(i).GetRangeMaxPosition(timestep), range[1], "RangeMax");
    }
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      return;
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePointDataAppended(
  svtkPointData* pd, svtkIndent indent, OffsetsManagerGroup* pdManager)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(pd->GetNumberOfArrays());

  os << indent << "<PointData";
  this->WriteAttributeIndices(pd, names);

  if (this->ErrorCode != svtkErrorCode::NoError)
  {
    this->DestroyStringArray(pd->GetNumberOfArrays(), names);
    return;
  }

  os << ">\n";

  pdManager->Allocate(pd->GetNumberOfArrays());
  for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
  {
    pdManager->GetElement(i).Allocate(this->NumberOfTimeSteps);
    for (int t = 0; t < this->NumberOfTimeSteps; ++t)
    {
      this->WriteArrayAppended(
        pd->GetAbstractArray(i), indent.GetNextIndent(), pdManager->GetElement(i), names[i], 0, t);
      if (this->ErrorCode != svtkErrorCode::NoError)
      {
        this->DestroyStringArray(pd->GetNumberOfArrays(), names);
        return;
      }
    }
  }

  os << indent << "</PointData>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  this->DestroyStringArray(pd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePointDataAppendedData(
  svtkPointData* pd, int timestep, OffsetsManagerGroup* pdManager)
{
  float progressRange[2] = { 0.f, 0.f };

  this->GetProgressRange(progressRange);
  for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
  {
    this->SetProgressRange(progressRange, i, pd->GetNumberOfArrays());
    svtkMTimeType mtime = pd->GetMTime();
    // Only write pd if MTime has changed
    svtkMTimeType& pdMTime = pdManager->GetElement(i).GetLastMTime();
    svtkAbstractArray* a = pd->GetAbstractArray(i);
    if (pdMTime != mtime || timestep == 0)
    {
      pdMTime = mtime;
      this->WriteArrayAppendedData(a, pdManager->GetElement(i).GetPosition(timestep),
        pdManager->GetElement(i).GetOffsetValue(timestep));
      if (this->ErrorCode != svtkErrorCode::NoError)
      {
        return;
      }
    }
    else
    {
      assert(timestep > 0);
      pdManager->GetElement(i).GetOffsetValue(timestep) =
        pdManager->GetElement(i).GetOffsetValue(timestep - 1);
      this->ForwardAppendedDataOffset(pdManager->GetElement(i).GetPosition(timestep),
        pdManager->GetElement(i).GetOffsetValue(timestep), "offset");
    }
    svtkDataArray* d = svtkArrayDownCast<svtkDataArray>(a);
    if (d)
    {
      // ranges are only written in case of Data Arrays.
      double* range = d->GetRange(-1);
      this->ForwardAppendedDataDouble(
        pdManager->GetElement(i).GetRangeMinPosition(timestep), range[0], "RangeMin");
      this->ForwardAppendedDataDouble(
        pdManager->GetElement(i).GetRangeMaxPosition(timestep), range[1], "RangeMax");
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteCellDataAppended(
  svtkCellData* cd, svtkIndent indent, OffsetsManagerGroup* cdManager)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(cd->GetNumberOfArrays());

  os << indent << "<CellData";
  this->WriteAttributeIndices(cd, names);

  if (this->ErrorCode != svtkErrorCode::NoError)
  {
    this->DestroyStringArray(cd->GetNumberOfArrays(), names);
    return;
  }

  os << ">\n";

  cdManager->Allocate(cd->GetNumberOfArrays());
  for (int i = 0; i < cd->GetNumberOfArrays(); ++i)
  {
    cdManager->GetElement(i).Allocate(this->NumberOfTimeSteps);
    for (int t = 0; t < this->NumberOfTimeSteps; ++t)
    {
      this->WriteArrayAppended(
        cd->GetAbstractArray(i), indent.GetNextIndent(), cdManager->GetElement(i), names[i], 0, t);
      if (this->ErrorCode != svtkErrorCode::NoError)
      {
        this->DestroyStringArray(cd->GetNumberOfArrays(), names);
        return;
      }
    }
  }

  os << indent << "</CellData>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
  this->DestroyStringArray(cd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteCellDataAppendedData(
  svtkCellData* cd, int timestep, OffsetsManagerGroup* cdManager)
{
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  for (int i = 0; i < cd->GetNumberOfArrays(); ++i)
  {
    this->SetProgressRange(progressRange, i, cd->GetNumberOfArrays());
    svtkMTimeType mtime = cd->GetMTime();
    // Only write pd if MTime has changed
    svtkMTimeType& cdMTime = cdManager->GetElement(i).GetLastMTime();
    svtkAbstractArray* a = cd->GetAbstractArray(i);
    if (cdMTime != mtime)
    {
      cdMTime = mtime;
      this->WriteArrayAppendedData(a, cdManager->GetElement(i).GetPosition(timestep),
        cdManager->GetElement(i).GetOffsetValue(timestep));
      if (this->ErrorCode != svtkErrorCode::NoError)
      {
        return;
      }
    }
    else
    {
      assert(timestep > 0);
      cdManager->GetElement(i).GetOffsetValue(timestep) =
        cdManager->GetElement(i).GetOffsetValue(timestep - 1);
      this->ForwardAppendedDataOffset(cdManager->GetElement(i).GetPosition(timestep),
        cdManager->GetElement(i).GetOffsetValue(timestep), "offset");
    }
    svtkDataArray* d = svtkArrayDownCast<svtkDataArray>(a);
    if (d)
    {
      double* range = d->GetRange(-1);
      this->ForwardAppendedDataDouble(
        cdManager->GetElement(i).GetRangeMinPosition(timestep), range[0], "RangeMin");
      this->ForwardAppendedDataDouble(
        cdManager->GetElement(i).GetRangeMaxPosition(timestep), range[1], "RangeMax");
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteAttributeIndices(svtkDataSetAttributes* dsa, char** names)
{
  int attributeIndices[svtkDataSetAttributes::NUM_ATTRIBUTES];
  dsa->GetAttributeIndices(attributeIndices);
  for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
  {
    if (attributeIndices[i] >= 0)
    {
      const char* attrName = dsa->GetAttributeTypeAsString(i);
      svtkDataArray* a = dsa->GetArray(attributeIndices[i]);
      const char* arrayName = a->GetName();
      if (!arrayName)
      {
        // Assign a name to the array.
        names[attributeIndices[i]] = new char[strlen(attrName) + 2];
        strcpy(names[attributeIndices[i]], attrName);
        strcat(names[attributeIndices[i]], "_");
        arrayName = names[attributeIndices[i]];
      }
      this->WriteStringAttribute(attrName, arrayName);
      if (this->ErrorCode != svtkErrorCode::NoError)
      {
        return;
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePointsAppended(
  svtkPoints* points, svtkIndent indent, OffsetsManager* ptManager)
{
  ostream& os = *(this->Stream);

  // Only write points if they exist.
  os << indent << "<Points>\n";
  if (points)
  {
    for (int t = 0; t < this->NumberOfTimeSteps; ++t)
    {
      this->WriteArrayAppended(
        points->GetData(), indent.GetNextIndent(), *ptManager, nullptr, 0, t);
    }
  }
  os << indent << "</Points>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePointsAppendedData(
  svtkPoints* points, int timestep, OffsetsManager* ptManager)
{
  // Only write points if they exist.
  if (points)
  {
    svtkMTimeType mtime = points->GetMTime();
    // Only write points if MTime has changed
    svtkMTimeType& pointsMTime = ptManager->GetLastMTime();
    // since points->Data is a svtkDataArray.
    svtkDataArray* outPoints = points->GetData();
    if (pointsMTime != mtime || timestep == 0)
    {
      pointsMTime = mtime;
      this->WriteArrayAppendedData(
        outPoints, ptManager->GetPosition(timestep), ptManager->GetOffsetValue(timestep));
    }
    else
    {
      assert(timestep > 0);
      ptManager->GetOffsetValue(timestep) = ptManager->GetOffsetValue(timestep - 1);
      this->ForwardAppendedDataOffset(
        ptManager->GetPosition(timestep), ptManager->GetOffsetValue(timestep), "offset");
    }
    double* range = outPoints->GetRange(-1);
    this->ForwardAppendedDataDouble(ptManager->GetRangeMinPosition(timestep), range[0], "RangeMin");
    this->ForwardAppendedDataDouble(ptManager->GetRangeMaxPosition(timestep), range[1], "RangeMax");
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePointsInline(svtkPoints* points, svtkIndent indent)
{
  ostream& os = *(this->Stream);
  // Only write points if they exist.
  os << indent << "<Points>\n";
  if (points)
  {
    svtkAbstractArray* outPoints = points->GetData();
    this->WriteArrayInline(outPoints, indent.GetNextIndent());
  }
  os << indent << "</Points>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteCoordinatesInline(
  svtkDataArray* xc, svtkDataArray* yc, svtkDataArray* zc, svtkIndent indent)
{
  ostream& os = *(this->Stream);

  // Only write coordinates if they exist.
  os << indent << "<Coordinates>\n";
  if (xc && yc && zc)
  {

    // Split progress over the three coordinates arrays.
    svtkIdType total = (xc->GetNumberOfTuples() + yc->GetNumberOfTuples() + zc->GetNumberOfTuples());
    if (total == 0)
    {
      total = 1;
    }
    float fractions[4] = { 0, float(xc->GetNumberOfTuples()) / total,
      float(xc->GetNumberOfTuples() + yc->GetNumberOfTuples()) / total, 1 };
    float progressRange[2] = { 0.f, 0.f };
    this->GetProgressRange(progressRange);

    this->SetProgressRange(progressRange, 0, fractions);
    this->WriteArrayInline(xc, indent.GetNextIndent());
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      return;
    }

    this->SetProgressRange(progressRange, 1, fractions);
    this->WriteArrayInline(yc, indent.GetNextIndent());
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      return;
    }

    this->SetProgressRange(progressRange, 2, fractions);
    this->WriteArrayInline(zc, indent.GetNextIndent());
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      return;
    }
  }
  os << indent << "</Coordinates>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    return;
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteCoordinatesAppended(svtkDataArray* xc, svtkDataArray* yc, svtkDataArray* zc,
  svtkIndent indent, OffsetsManagerGroup* coordManager)
{
  ostream& os = *(this->Stream);

  // Helper for the 'for' loop
  svtkDataArray* allcoords[3];
  allcoords[0] = xc;
  allcoords[1] = yc;
  allcoords[2] = zc;

  // Only write coordinates if they exist.
  os << indent << "<Coordinates>\n";
  coordManager->Allocate(3);
  if (xc && yc && zc)
  {
    for (int i = 0; i < 3; ++i)
    {
      coordManager->GetElement(i).Allocate(this->NumberOfTimeSteps);
      for (int t = 0; t < this->NumberOfTimeSteps; ++t)
      {
        this->WriteArrayAppended(
          allcoords[i], indent.GetNextIndent(), coordManager->GetElement(i), nullptr, 0, t);
        if (this->ErrorCode != svtkErrorCode::NoError)
        {
          return;
        }
      }
    }
  }
  os << indent << "</Coordinates>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteCoordinatesAppendedData(svtkDataArray* xc, svtkDataArray* yc,
  svtkDataArray* zc, int timestep, OffsetsManagerGroup* coordManager)
{
  // Only write coordinates if they exist.
  if (xc && yc && zc)
  {
    // Split progress over the three coordinates arrays.
    svtkIdType total = (xc->GetNumberOfTuples() + yc->GetNumberOfTuples() + zc->GetNumberOfTuples());
    if (total == 0)
    {
      total = 1;
    }
    float fractions[4] = { 0, float(xc->GetNumberOfTuples()) / total,
      float(xc->GetNumberOfTuples() + yc->GetNumberOfTuples()) / total, 1 };
    float progressRange[2] = { 0.f, 0.f };
    this->GetProgressRange(progressRange);

    // Helper for the 'for' loop
    svtkDataArray* allcoords[3];
    allcoords[0] = xc;
    allcoords[1] = yc;
    allcoords[2] = zc;

    for (int i = 0; i < 3; ++i)
    {
      this->SetProgressRange(progressRange, i, fractions);
      svtkMTimeType mtime = allcoords[i]->GetMTime();
      // Only write pd if MTime has changed
      svtkMTimeType& coordMTime = coordManager->GetElement(i).GetLastMTime();
      if (coordMTime != mtime)
      {
        coordMTime = mtime;
        this->WriteArrayAppendedData(allcoords[i],
          coordManager->GetElement(i).GetPosition(timestep),
          coordManager->GetElement(i).GetOffsetValue(timestep));
        if (this->ErrorCode != svtkErrorCode::NoError)
        {
          return;
        }
      }
      else
      {
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePPointData(svtkPointData* pd, svtkIndent indent)
{
  if (pd->GetNumberOfArrays() == 0)
  {
    return;
  }
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(pd->GetNumberOfArrays());

  os << indent << "<PPointData";
  this->WriteAttributeIndices(pd, names);
  if (this->ErrorCode != svtkErrorCode::NoError)
  {
    this->DestroyStringArray(pd->GetNumberOfArrays(), names);
    return;
  }
  os << ">\n";

  for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
  {
    this->WritePArray(pd->GetAbstractArray(i), indent.GetNextIndent(), names[i]);
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      this->DestroyStringArray(pd->GetNumberOfArrays(), names);
      return;
    }
  }

  os << indent << "</PPointData>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }

  this->DestroyStringArray(pd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePCellData(svtkCellData* cd, svtkIndent indent)
{
  if (cd->GetNumberOfArrays() == 0)
  {
    return;
  }
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(cd->GetNumberOfArrays());

  os << indent << "<PCellData";
  this->WriteAttributeIndices(cd, names);
  os << ">\n";

  for (int i = 0; i < cd->GetNumberOfArrays(); ++i)
  {
    this->WritePArray(cd->GetAbstractArray(i), indent.GetNextIndent(), names[i]);
  }

  os << indent << "</PCellData>\n";

  this->DestroyStringArray(cd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePPoints(svtkPoints* points, svtkIndent indent)
{
  ostream& os = *(this->Stream);
  // Only write points if they exist.
  os << indent << "<PPoints>\n";
  if (points)
  {
    this->WritePArray(points->GetData(), indent.GetNextIndent());
  }
  os << indent << "</PPoints>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePArray(svtkAbstractArray* a, svtkIndent indent, const char* alternateName)
{
  svtkDataArray* d = svtkArrayDownCast<svtkDataArray>(a);
  ostream& os = *(this->Stream);
  if (d)
  {
    os << indent << "<PDataArray";
  }
  else
  {
    os << indent << "<PArray";
  }
  this->WriteWordTypeAttribute("type", a->GetDataType());
  if (a->GetDataType() == SVTK_ID_TYPE)
  {
    this->WriteScalarAttribute("IdType", 1);
  }
  if (alternateName)
  {
    this->WriteStringAttribute("Name", alternateName);
  }
  else
  {
    const char* arrayName = a->GetName();
    if (arrayName)
    {
      this->WriteStringAttribute("Name", arrayName);
    }
  }
  if (a->GetNumberOfComponents() > 1)
  {
    this->WriteScalarAttribute("NumberOfComponents", a->GetNumberOfComponents());
  }
  os << "/>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePCoordinates(
  svtkDataArray* xc, svtkDataArray* yc, svtkDataArray* zc, svtkIndent indent)
{
  ostream& os = *(this->Stream);

  // Only write coordinates if they exist.
  os << indent << "<PCoordinates>\n";
  if (xc && yc && zc)
  {
    this->WritePArray(xc, indent.GetNextIndent());
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      return;
    }
    this->WritePArray(yc, indent.GetNextIndent());
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      return;
    }
    this->WritePArray(zc, indent.GetNextIndent());
    if (this->ErrorCode != svtkErrorCode::NoError)
    {
      return;
    }
  }
  os << indent << "</PCoordinates>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
  }
}

//----------------------------------------------------------------------------
char** svtkXMLWriter::CreateStringArray(int numStrings)
{
  char** strings = new char*[numStrings];
  for (int i = 0; i < numStrings; ++i)
  {
    strings[i] = nullptr;
  }
  return strings;
}

//----------------------------------------------------------------------------
void svtkXMLWriter::DestroyStringArray(int numStrings, char** strings)
{
  for (int i = 0; i < numStrings; ++i)
  {
    delete[] strings[i];
  }
  delete[] strings;
}

//----------------------------------------------------------------------------
void svtkXMLWriter::GetProgressRange(float range[2])
{
  range[0] = this->ProgressRange[0];
  range[1] = this->ProgressRange[1];
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetProgressRange(const float range[2], int curStep, int numSteps)
{
  float stepSize = (range[1] - range[0]) / numSteps;
  this->ProgressRange[0] = range[0] + stepSize * curStep;
  this->ProgressRange[1] = range[0] + stepSize * (curStep + 1);
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetProgressRange(const float range[2], int curStep, const float* fractions)
{
  float width = range[1] - range[0];
  this->ProgressRange[0] = range[0] + fractions[curStep] * width;
  this->ProgressRange[1] = range[0] + fractions[curStep + 1] * width;
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::SetProgressPartial(float fraction)
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  this->UpdateProgressDiscrete(this->ProgressRange[0] + fraction * width);
}

//----------------------------------------------------------------------------
void svtkXMLWriter::UpdateProgressDiscrete(float progress)
{
  if (!this->AbortExecute)
  {
    // Round progress to nearest 100th.
    float rounded = static_cast<float>(static_cast<int>((progress * 100) + 0.5f)) / 100.f;
    if (this->GetProgress() != rounded)
    {
      this->UpdateProgress(rounded);
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WritePrimaryElementAttributes(ostream& os, svtkIndent indent)
{
  // Write the time step if any:
  if (this->NumberOfTimeSteps > 1)
  {
    // First thing allocate NumberOfTimeValues
    assert(this->NumberOfTimeValues == nullptr);
    this->NumberOfTimeValues = new svtkTypeInt64[this->NumberOfTimeSteps];
    os << indent << "TimeValues=\"\n";

    std::string blankline = std::string(40, ' '); // enough room for precision
    for (int i = 0; i < this->NumberOfTimeSteps; i++)
    {
      this->NumberOfTimeValues[i] = os.tellp();
      os << blankline.c_str() << "\n";
    }
    os << "\"";
  }
}

//----------------------------------------------------------------------------
int svtkXMLWriter::WritePrimaryElement(ostream& os, svtkIndent indent)
{
  // Open the primary element.
  os << indent << "<" << this->GetDataSetName();

  this->WritePrimaryElementAttributes(os, indent);

  // Close the primary element:
  os << ">\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

// The following function are designed to be called outside of the SVTK pipeline
// typically from a C interface or when ParaView want to control the writing
//----------------------------------------------------------------------------
void svtkXMLWriter::Start()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    svtkErrorMacro("No input provided!");
    return;
  }
  this->UserContinueExecuting = 1;
}

//----------------------------------------------------------------------------
// The function does not make sense in the general case but we need to handle
// the case where the simulation stop before reaching the number of steps
// specified by the user. Therefore the CurrentTimeIndex is never equal
// to NumberOfTimeStep and thus we need to force closing of the xml file
void svtkXMLWriter::Stop()
{
  this->UserContinueExecuting = 0;
  this->Modified();
  this->Update();
  this->UserContinueExecuting = -1; // put back the writer in initial state
}

//----------------------------------------------------------------------------
void svtkXMLWriter::WriteNextTime(double time)
{
  this->Modified();
  this->Update();

  ostream& os = *(this->Stream);

  if (this->NumberOfTimeValues)
  {
    // Write user specified time value in the TimeValues attribute
    std::streampos returnPos = os.tellp();
    svtkTypeInt64 t = this->NumberOfTimeValues[this->CurrentTimeIndex - 1];
    os.seekp(std::streampos(t));
    os << time;
    os.seekp(returnPos);
  }
}
