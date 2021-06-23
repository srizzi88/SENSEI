/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLZMADataCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLZMADataCompressor
 * @brief   Data compression using LZMA Utils.
 *
 * svtkLZMADataCompressor provides a concrete svtkDataCompressor class
 * using LZMA for compressing and uncompressing data.
 *
 * @par Thanks:
 * This svtkDataCompressor contributed by Quincy Wofford (qwofford@lanl.gov)
 * and John Patchett (patchett@lanl.gov), Los Alamos National Laboratory
 * (2017)
 *
 */

#ifndef svtkLZMADataCompressor_h
#define svtkLZMADataCompressor_h

#include "svtkDataCompressor.h"
#include "svtkIOCoreModule.h" // For export macro

class SVTKIOCORE_EXPORT svtkLZMADataCompressor : public svtkDataCompressor
{
public:
  svtkTypeMacro(svtkLZMADataCompressor, svtkDataCompressor);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkLZMADataCompressor* New();

  /**
   *  Get the maximum space that may be needed to store data of the
   *  given uncompressed size after compression.  This is the minimum
   *  size of the output buffer that can be passed to the four-argument
   *  Compress method.
   */
  size_t GetMaximumCompressionSpace(size_t size) override;
  /**
   *  Get/Set the compression level.
   */

  // Compression level setter required by svtkDataCompressor.
  void SetCompressionLevel(int compressionLevel) override;

  // Compression level getter required by svtkDataCompressor.
  int GetCompressionLevel() override;

protected:
  svtkLZMADataCompressor();
  ~svtkLZMADataCompressor() override;

  int CompressionLevel;

  // Compression method required by svtkDataCompressor.
  size_t CompressBuffer(unsigned char const* uncompressedData, size_t uncompressedSize,
    unsigned char* compressedData, size_t compressionSpace) override;
  // Decompression method required by svtkDataCompressor.
  size_t UncompressBuffer(unsigned char const* compressedData, size_t compressedSize,
    unsigned char* uncompressedData, size_t uncompressedSize) override;

private:
  svtkLZMADataCompressor(const svtkLZMADataCompressor&) = delete;
  void operator=(const svtkLZMADataCompressor&) = delete;
};

#endif
