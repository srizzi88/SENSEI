/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockPLOT3DReaderInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkMultiBlockPLOT3DReaderInternals_h
#define svtkMultiBlockPLOT3DReaderInternals_h

#include "svtkByteSwap.h"
#include "svtkIOParallelModule.h" // For export macro
#include "svtkMultiBlockPLOT3DReader.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"

#include <exception>
#include <vector>

class svtkMultiProcessController;

#ifdef _WIN64
#define svtk_fseek _fseeki64
#define svtk_ftell _ftelli64
#define svtk_off_t __int64
#else
#define svtk_fseek fseek
#define svtk_ftell ftell
#define svtk_off_t long
#endif

struct svtkMultiBlockPLOT3DReaderInternals
{
  struct Dims
  {

    Dims() { memset(this->Values, 0, 3 * sizeof(int)); }

    int Values[3];
  };

  std::vector<Dims> Dimensions;
  std::vector<svtkSmartPointer<svtkStructuredGrid> > Blocks;

  struct InternalSettings
  {
    int BinaryFile;
    int ByteOrder;
    int HasByteCount;
    int MultiGrid;
    int NumberOfDimensions;
    int Precision; // in bytes
    int IBlanking;
    InternalSettings()
      : BinaryFile(1)
      , ByteOrder(svtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN)
      , HasByteCount(1)
      , MultiGrid(0)
      , NumberOfDimensions(3)
      , Precision(4)
      , IBlanking(0)
    {
    }
  };

  InternalSettings Settings;
  bool NeedToCheckXYZFile;

  svtkMultiBlockPLOT3DReaderInternals()
    : NeedToCheckXYZFile(true)
  {
  }

  int ReadInts(FILE* fp, int n, int* val);
  void CheckBinaryFile(FILE* fp, size_t fileSize);
  int CheckByteOrder(FILE* fp);
  int CheckByteCount(FILE* fp);
  int CheckMultiGrid(FILE* fp);
  int Check2DGeom(FILE* fp);
  int CheckBlankingAndPrecision(FILE* fp);
  int CheckCFile(FILE* fp, size_t fileSize);
  size_t CalculateFileSize(int mgrid,
    int precision, // in bytes
    int blanking, int ndims, int hasByteCount, int nGrids, int* gridDims);
  size_t CalculateFileSizeForBlock(int precision, // in bytes
    int blanking, int ndims, int hasByteCount, int* gridDims);

  static void CalculateSkips(
    const int extent[6], const int wextent[6], svtkIdType& preskip, svtkIdType& postskip)
  {
    svtkIdType nPtsInPlane = static_cast<svtkIdType>(wextent[1] + 1) * (wextent[3] + 1);
    preskip = nPtsInPlane * extent[4];
    postskip = nPtsInPlane * (wextent[5] - extent[5]);
  }
};

namespace
{
class Plot3DException : public std::exception
{
};
}

// Description:
// svtkMultiBlockPLOT3DReaderRecord represents a data record in the file. For
// binary Plot3D files with record separators (i.e. leading and trailing length
// field per record see: https://software.intel.com/en-us/node/525311), if the
// record length is greater than  2,147,483,639 bytes, the record get split into
// multiple records. This class allows use to manage that.
// It corresponds to a complete record i.e. including all the records when split
// among multiple records due to length limit.
class SVTKIOPARALLEL_EXPORT svtkMultiBlockPLOT3DReaderRecord
{
  struct svtkSubRecord
  {
    svtkTypeUInt64 HeaderOffset;
    svtkTypeUInt64 FooterOffset;
  };

  typedef std::vector<svtkSubRecord> VectorOfSubRecords;
  VectorOfSubRecords SubRecords;

public:
  // Description:
  // A type for collection of sub-record separators i.e. separators encountered
  // within a record when the record length is greater than 2,147,483,639 bytes.
  typedef std::vector<svtkTypeUInt64> SubRecordSeparators;

  // Description:
  // Since a sub-record separator is made up of the trailing length field of a
  // sub-record and the leading length field of the next sub-record, it's length
  // is two ints.
  static const int SubRecordSeparatorWidth = sizeof(int) * 2;

  // Description:
  // Initialize metadata about the record located at the given offset.
  // This reads the file on the root node to populate record information,
  // seeking and marching forward through the file if the record comprises of
  // multiple sub-records. The file is reset back to the original starting
  // position when done.
  //
  // This method has no effect for non-binary files or files that don't have
  // record separators i.e. HasByteCount == 0.
  bool Initialize(FILE* fp, svtkTypeUInt64 offset,
    const svtkMultiBlockPLOT3DReaderInternals::InternalSettings& settings,
    svtkMultiProcessController* controller);

  // Description:
  // Returns true if:
  // 1. file doesn't comprise of records i.e. ASCII or doesn't have byte-count markers.
  // 2. offset is same as the start offset for this record.
  bool AtStart(svtkTypeUInt64 offset)
  {
    return (this->SubRecords.size() == 0 || this->SubRecords.front().HeaderOffset == offset);
  }

  // Description:
  // Returns true if:
  // 1. file doesn't comprise of records i.e. ASCII or doesn't have byte-count markers.
  // 2. offset is at the end of this record i.e. the start of the next record.
  bool AtEnd(svtkTypeUInt64 offset)
  {
    return (this->SubRecords.size() == 0 ||
      (this->SubRecords.back().FooterOffset + sizeof(int) == offset));
  }

  // Description:
  // Returns the location of SubRecordSeparators (bad two 4-byte ints) between startOffset and
  // (startOffset + length).
  SubRecordSeparators GetSubRecordSeparators(svtkTypeUInt64 startOffset, svtkTypeUInt64 length) const;

  // Description:
  // When reading between file offsets \c start and  \c (start + length) from the file, if it has
  // any sub-record separators, this method splits the read into chunks so that it skips the
  // sub-record separators. The returned value is a vector of pairs (offset, length-in-bytes).
  static std::vector<std::pair<svtkTypeUInt64, svtkTypeUInt64> > GetChunksToRead(
    svtkTypeUInt64 start, svtkTypeUInt64 length, const std::vector<svtkTypeUInt64>& markers);

  // Description:
  // If the block in file (start, start+length) steps over sub-record separators
  // within this record, then this method will return a new length that includes
  // the bytes for the separators to be skipped. Otherwise, simply returns the
  // length.
  svtkTypeUInt64 GetLengthWithSeparators(svtkTypeUInt64 start, svtkTypeUInt64 length) const;

  std::vector<std::pair<svtkTypeUInt64, svtkTypeUInt64> > GetChunksToRead(
    svtkTypeUInt64 start, svtkTypeUInt64 length) const
  {
    return this->GetChunksToRead(start, length, this->GetSubRecordSeparators(start, length));
  }
};

#endif
// SVTK-HeaderTest-Exclude: svtkMultiBlockPLOT3DReaderInternals.h
