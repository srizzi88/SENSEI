/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLZ4DataCompressor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLZ4DataCompressor.h"
#include "svtkObjectFactory.h"
#include "svtk_lz4.h"

svtkStandardNewMacro(svtkLZ4DataCompressor);

//----------------------------------------------------------------------------
svtkLZ4DataCompressor::svtkLZ4DataCompressor()
{
  this->AccelerationLevel = 1;
}

//----------------------------------------------------------------------------
svtkLZ4DataCompressor::~svtkLZ4DataCompressor() = default;

//----------------------------------------------------------------------------
void svtkLZ4DataCompressor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AccelerationLevel: " << this->AccelerationLevel << endl;
}

//----------------------------------------------------------------------------
size_t svtkLZ4DataCompressor::CompressBuffer(unsigned char const* uncompressedData,
  size_t uncompressedSize, unsigned char* compressedData, size_t compressionSpace)
{
  const char* ud = reinterpret_cast<const char*>(uncompressedData);
  char* cd = reinterpret_cast<char*>(compressedData);
  // Call LZ4's compress function.
  int cs = LZ4_compress_fast(ud, cd, static_cast<int>(uncompressedSize),
    static_cast<int>(compressionSpace), this->AccelerationLevel);
  if (cs == 0)
  {
    svtkErrorMacro("LZ4 error while compressing data.");
  }
  return static_cast<size_t>(cs);
}

//----------------------------------------------------------------------------
size_t svtkLZ4DataCompressor::UncompressBuffer(unsigned char const* compressedData,
  size_t compressedSize, unsigned char* uncompressedData, size_t uncompressedSize)
{
  char* ud = reinterpret_cast<char*>(uncompressedData);
  const char* cd = reinterpret_cast<const char*>(compressedData);
  int us = LZ4_decompress_safe(
    cd, ud, static_cast<int>(compressedSize), static_cast<int>(uncompressedSize));
  if (us < 0)
  {
    svtkErrorMacro("Zlib error while uncompressing data.");
    return 0;
  }
  // Make sure the output size matched that expected.
  if (us != static_cast<int>(uncompressedSize))
  {
    svtkErrorMacro("Decompression produced incorrect size.\n"
                  "Expected "
      << uncompressedSize << " and got " << us);
    return 0;
  }
  return static_cast<size_t>(us);
}
//----------------------------------------------------------------------------
int svtkLZ4DataCompressor::GetCompressionLevel()
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning CompressionLevel "
                << (10 - this->AccelerationLevel));
  return 10 - this->AccelerationLevel;
}
//----------------------------------------------------------------------------
void svtkLZ4DataCompressor::SetCompressionLevel(int compressionLevel)
{
  int min = 1;
  int max = 9;
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting CompressionLevel to "
                << compressionLevel);
  // In order to make an intuitive interface for svtkDataCompressor objects
  // we accept compressionLevel values 1..9. 1 is fastest, 9 is slowest
  // 1 is worst compression, 9 is best compression. LZ4 acceleration works inversely, with no upper
  // bound. Note: LZ4 Acceleration set/get exists in header file, with no upper bound.
  if (this->AccelerationLevel !=
    (10 - (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel))))
  {
    this->AccelerationLevel =
      10 - (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel));
    this->Modified();
  }
}

//----------------------------------------------------------------------------
size_t svtkLZ4DataCompressor::GetMaximumCompressionSpace(size_t size)
{
  return LZ4_COMPRESSBOUND(size);
}
