/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkZLibDataCompressor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkZLibDataCompressor.h"
#include "svtkObjectFactory.h"
#include "svtk_zlib.h"

svtkStandardNewMacro(svtkZLibDataCompressor);

//----------------------------------------------------------------------------
svtkZLibDataCompressor::svtkZLibDataCompressor()
{
  this->CompressionLevel = Z_DEFAULT_COMPRESSION;
}

//----------------------------------------------------------------------------
svtkZLibDataCompressor::~svtkZLibDataCompressor() = default;

//----------------------------------------------------------------------------
void svtkZLibDataCompressor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CompressionLevel: " << this->CompressionLevel << endl;
}

//----------------------------------------------------------------------------
size_t svtkZLibDataCompressor::CompressBuffer(unsigned char const* uncompressedData,
  size_t uncompressedSize, unsigned char* compressedData, size_t compressionSpace)
{
  uLongf cs = static_cast<uLongf>(compressionSpace);
  Bytef* cd = reinterpret_cast<Bytef*>(compressedData);
  const Bytef* ud = reinterpret_cast<const Bytef*>(uncompressedData);
  uLong us = static_cast<uLong>(uncompressedSize);

  // Call zlib's compress function.
  if (compress2(cd, &cs, ud, us, this->CompressionLevel) != Z_OK)
  {
    svtkErrorMacro("Zlib error while compressing data.");
    return 0;
  }

  return static_cast<size_t>(cs);
}

//----------------------------------------------------------------------------
size_t svtkZLibDataCompressor::UncompressBuffer(unsigned char const* compressedData,
  size_t compressedSize, unsigned char* uncompressedData, size_t uncompressedSize)
{
  uLongf us = static_cast<uLongf>(uncompressedSize);
  Bytef* ud = reinterpret_cast<Bytef*>(uncompressedData);
  const Bytef* cd = reinterpret_cast<const Bytef*>(compressedData);
  uLong cs = static_cast<uLong>(compressedSize);

  // Call zlib's uncompress function.
  if (uncompress(ud, &us, cd, cs) != Z_OK)
  {
    svtkErrorMacro("Zlib error while uncompressing data.");
    return 0;
  }

  // Make sure the output size matched that expected.
  if (us != static_cast<uLongf>(uncompressedSize))
  {
    svtkErrorMacro("Decompression produced incorrect size.\n"
                  "Expected "
      << uncompressedSize << " and got " << us);
    return 0;
  }

  return static_cast<size_t>(us);
}
//----------------------------------------------------------------------------
int svtkZLibDataCompressor::GetCompressionLevel()
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning CompressionLevel "
                << this->CompressionLevel);
  return this->CompressionLevel;
}
//----------------------------------------------------------------------------
void svtkZLibDataCompressor::SetCompressionLevel(int compressionLevel)
{
  int min = 1;
  int max = 9;
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting CompressionLevel to "
                << compressionLevel);
  if (this->CompressionLevel !=
    (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel)))
  {
    this->CompressionLevel =
      (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel));
    this->Modified();
  }
}

//----------------------------------------------------------------------------
size_t svtkZLibDataCompressor::GetMaximumCompressionSpace(size_t size)
{
  // ZLib specifies that destination buffer must be 0.1% larger + 12 bytes.
  return size + (size + 999) / 1000 + 12;
}
