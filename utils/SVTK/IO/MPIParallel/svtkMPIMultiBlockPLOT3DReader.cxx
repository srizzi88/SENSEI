/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPIMultiBlockPLOT3DReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMPIMultiBlockPLOT3DReader.h"

#include "svtkByteSwap.h"
#include "svtkDoubleArray.h"
#include "svtkErrorCode.h"
#include "svtkFloatArray.h"
#include "svtkIntArray.h"
#include "svtkMPI.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockPLOT3DReaderInternals.h"
#include "svtkObjectFactory.h"
#include "svtkStructuredData.h"
#include <cassert>
#include <exception>

#define DEFINE_MPI_TYPE(ctype, mpitype)                                                            \
  template <>                                                                                      \
  struct mpi_type<ctype>                                                                           \
  {                                                                                                \
    static MPI_Datatype type() { return mpitype; }                                                 \
  };

namespace
{
template <class T>
struct mpi_type
{
};
DEFINE_MPI_TYPE(char, MPI_CHAR);
DEFINE_MPI_TYPE(signed char, MPI_SIGNED_CHAR);
DEFINE_MPI_TYPE(unsigned char, MPI_UNSIGNED_CHAR);
DEFINE_MPI_TYPE(short, MPI_SHORT);
DEFINE_MPI_TYPE(unsigned short, MPI_UNSIGNED_SHORT);
DEFINE_MPI_TYPE(int, MPI_INT);
DEFINE_MPI_TYPE(unsigned int, MPI_UNSIGNED);
DEFINE_MPI_TYPE(long, MPI_LONG);
DEFINE_MPI_TYPE(unsigned long, MPI_UNSIGNED_LONG);
DEFINE_MPI_TYPE(float, MPI_FLOAT);
DEFINE_MPI_TYPE(double, MPI_DOUBLE);
DEFINE_MPI_TYPE(long long, MPI_LONG_LONG);
DEFINE_MPI_TYPE(unsigned long long, MPI_UNSIGNED_LONG_LONG);

class MPIPlot3DException : public std::exception
{
};

template <class DataType>
class svtkMPIPLOT3DArrayReader
{
public:
  svtkMPIPLOT3DArrayReader()
    : ByteOrder(svtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN)
  {
  }

  svtkIdType ReadScalar(void* vfp, svtkTypeUInt64 offset, svtkIdType preskip, svtkIdType n,
    svtkIdType svtkNotUsed(postskip), DataType* scalar,
    const svtkMultiBlockPLOT3DReaderRecord& record = svtkMultiBlockPLOT3DReaderRecord())
  {
    svtkMPIOpaqueFileHandle* fp = reinterpret_cast<svtkMPIOpaqueFileHandle*>(vfp);
    assert(fp);

    // skip preskip if we're setting over subrecord separators.
    offset += record.GetLengthWithSeparators(offset, preskip * sizeof(DataType));

    // Let's see if we encounter markers while reading the data from current
    // position.
    std::vector<std::pair<svtkTypeUInt64, svtkTypeUInt64> > chunks =
      record.GetChunksToRead(offset, sizeof(DataType) * n);

    const int dummy_INT_MAX = 2e9; /// XXX: arbitrary limit that seems
                                   /// to work when reading large files.
    svtkIdType bytesread = 0;
    for (size_t cc = 0; cc < chunks.size(); cc++)
    {
      svtkTypeUInt64 start = chunks[cc].first;
      svtkTypeUInt64 length = chunks[cc].second;
      while (length > 0)
      {
        int segment = length > static_cast<svtkTypeUInt64>(dummy_INT_MAX) ? (length - dummy_INT_MAX)
                                                                         : static_cast<int>(length);

        MPI_Status status;
        if (MPI_File_read_at(fp->Handle, start, reinterpret_cast<char*>(scalar) + bytesread,
              segment, MPI_UNSIGNED_CHAR, &status) != MPI_SUCCESS)
        {
          return 0; // let's assume nothing was read.
        }
        start += segment;
        length -= segment;
        bytesread += segment;
      }
    }

    if (this->ByteOrder == svtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
    {
      if (sizeof(DataType) == 4)
      {
        svtkByteSwap::Swap4LERange(scalar, n);
      }
      else
      {
        svtkByteSwap::Swap8LERange(scalar, n);
      }
    }
    else
    {
      if (sizeof(DataType) == 4)
      {
        svtkByteSwap::Swap4BERange(scalar, n);
      }
      else
      {
        svtkByteSwap::Swap8BERange(scalar, n);
      }
    }
    return bytesread / sizeof(DataType);
  }

  svtkIdType ReadVector(void* vfp, svtkTypeUInt64 offset, int extent[6], int wextent[6], int numDims,
    DataType* vector, const svtkMultiBlockPLOT3DReaderRecord& record)
  {
    svtkIdType n = svtkStructuredData::GetNumberOfPoints(extent);
    svtkIdType totalN = svtkStructuredData::GetNumberOfPoints(wextent);

    // Setting to 0 in case numDims == 0. We still need to
    // populate an array with 3 components but the code below
    // does not read the 3rd component (it doesn't exist
    // in the file)
    memset(vector, 0, n * 3 * sizeof(DataType));

    svtkIdType retVal = 0;
    DataType* buffer = new DataType[n];
    for (int component = 0; component < numDims; component++)
    {
      svtkIdType preskip, postskip;
      svtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
      svtkIdType valread = this->ReadScalar(vfp, offset, preskip, n, postskip, buffer, record);
      if (valread != n)
      {
        return 0; // failed.
      }
      retVal += valread;
      for (svtkIdType i = 0; i < n; i++)
      {
        vector[3 * i + component] = buffer[i];
      }
      offset += record.GetLengthWithSeparators(offset, totalN * sizeof(DataType));
    }
    delete[] buffer;
    return retVal;
  }
  int ByteOrder;
};
}

svtkStandardNewMacro(svtkMPIMultiBlockPLOT3DReader);
//----------------------------------------------------------------------------
svtkMPIMultiBlockPLOT3DReader::svtkMPIMultiBlockPLOT3DReader()
{
  this->UseMPIIO = true;
}

//----------------------------------------------------------------------------
svtkMPIMultiBlockPLOT3DReader::~svtkMPIMultiBlockPLOT3DReader() {}

//----------------------------------------------------------------------------
bool svtkMPIMultiBlockPLOT3DReader::CanUseMPIIO()
{
  return (this->UseMPIIO && this->BinaryFile && this->Internal->Settings.NumberOfDimensions == 3 &&
    svtkMPIController::SafeDownCast(this->Controller) != nullptr);
}

//----------------------------------------------------------------------------
int svtkMPIMultiBlockPLOT3DReader::OpenFileForDataRead(void*& vfp, const char* fname)
{
  if (!this->CanUseMPIIO())
  {
    return this->Superclass::OpenFileForDataRead(vfp, fname);
  }

  svtkMPICommunicator* mpiComm =
    svtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
  assert(mpiComm);

  svtkMPIOpaqueFileHandle* handle = new svtkMPIOpaqueFileHandle();
  try
  {
    if (MPI_File_open(*mpiComm->GetMPIComm()->GetHandle(), const_cast<char*>(fname),
          MPI_MODE_RDONLY, MPI_INFO_NULL, &handle->Handle) != MPI_SUCCESS)
    {
      this->SetErrorCode(svtkErrorCode::FileNotFoundError);
      svtkErrorMacro("File: " << fname << " not found.");
      throw MPIPlot3DException();
    }
  }
  catch (const MPIPlot3DException&)
  {
    delete handle;
    vfp = nullptr;
    return SVTK_ERROR;
  }
  vfp = handle;
  return SVTK_OK;
}

//----------------------------------------------------------------------------
void svtkMPIMultiBlockPLOT3DReader::CloseFile(void* vfp)
{
  if (!this->CanUseMPIIO())
  {
    this->Superclass::CloseFile(vfp);
    return;
  }

  svtkMPIOpaqueFileHandle* handle = reinterpret_cast<svtkMPIOpaqueFileHandle*>(vfp);
  assert(handle);
  if (MPI_File_close(&handle->Handle) != MPI_SUCCESS)
  {
    svtkErrorMacro("Failed to close file!");
  }
}

//----------------------------------------------------------------------------
int svtkMPIMultiBlockPLOT3DReader::ReadIntScalar(void* vfp, int extent[6], int wextent[6],
  svtkDataArray* scalar, svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& record)

{
  if (!this->CanUseMPIIO())
  {
    return this->Superclass::ReadIntScalar(vfp, extent, wextent, scalar, offset, record);
  }

  svtkIdType n = svtkStructuredData::GetNumberOfPoints(extent);
  svtkMPIPLOT3DArrayReader<int> arrayReader;
  arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
  svtkIdType preskip, postskip;
  svtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
  svtkIntArray* intArray = static_cast<svtkIntArray*>(scalar);
  return arrayReader.ReadScalar(
           vfp, offset, preskip, n, postskip, intArray->GetPointer(0), record) == n;
}

//----------------------------------------------------------------------------
int svtkMPIMultiBlockPLOT3DReader::ReadScalar(void* vfp, int extent[6], int wextent[6],
  svtkDataArray* scalar, svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& record)
{
  if (!this->CanUseMPIIO())
  {
    return this->Superclass::ReadScalar(vfp, extent, wextent, scalar, offset, record);
  }

  svtkIdType n = svtkStructuredData::GetNumberOfPoints(extent);
  if (this->Internal->Settings.Precision == 4)
  {
    svtkMPIPLOT3DArrayReader<float> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    svtkIdType preskip, postskip;
    svtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
    svtkFloatArray* floatArray = static_cast<svtkFloatArray*>(scalar);
    return arrayReader.ReadScalar(
             vfp, offset, preskip, n, postskip, floatArray->GetPointer(0), record) == n;
  }
  else
  {
    svtkMPIPLOT3DArrayReader<double> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    svtkIdType preskip, postskip;
    svtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
    svtkDoubleArray* doubleArray = static_cast<svtkDoubleArray*>(scalar);
    return arrayReader.ReadScalar(
             vfp, offset, preskip, n, postskip, doubleArray->GetPointer(0), record) == n;
  }
}

//----------------------------------------------------------------------------
int svtkMPIMultiBlockPLOT3DReader::ReadVector(void* vfp, int extent[6], int wextent[6], int numDims,
  svtkDataArray* vector, svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& record)
{
  if (!this->CanUseMPIIO())
  {
    return this->Superclass::ReadVector(vfp, extent, wextent, numDims, vector, offset, record);
  }

  svtkIdType n = svtkStructuredData::GetNumberOfPoints(extent);
  svtkIdType nValues = n * numDims;
  if (this->Internal->Settings.Precision == 4)
  {
    svtkMPIPLOT3DArrayReader<float> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    svtkFloatArray* floatArray = static_cast<svtkFloatArray*>(vector);
    return arrayReader.ReadVector(
             vfp, offset, extent, wextent, numDims, floatArray->GetPointer(0), record) == nValues;
  }
  else
  {
    svtkMPIPLOT3DArrayReader<double> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    svtkDoubleArray* doubleArray = static_cast<svtkDoubleArray*>(vector);
    return arrayReader.ReadVector(
             vfp, offset, extent, wextent, numDims, doubleArray->GetPointer(0), record) == nValues;
  }
}

//----------------------------------------------------------------------------
void svtkMPIMultiBlockPLOT3DReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseMPIIO: " << this->UseMPIIO << endl;
}
