/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkZLibDataCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkZLibDataCompressor
 * @brief   Data compression using zlib.
 *
 * svtkZLibDataCompressor provides a concrete svtkDataCompressor class
 * using zlib for compressing and uncompressing data.
 */

#ifndef svtkZLibDataCompressor_h
#define svtkZLibDataCompressor_h

#include "svtkDataCompressor.h"
#include "svtkIOCoreModule.h" // For export macro

class SVTKIOCORE_EXPORT svtkZLibDataCompressor : public svtkDataCompressor
{
public:
  svtkTypeMacro(svtkZLibDataCompressor, svtkDataCompressor);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkZLibDataCompressor* New();

  /**
   * Get the maximum space that may be needed to store data of the
   * given uncompressed size after compression.  This is the minimum
   * size of the output buffer that can be passed to the four-argument
   * Compress method.
   */
  size_t GetMaximumCompressionSpace(size_t size) override;

  //@{
  /**
   *  Get/Set the compression level.
   */
  // Compression level getter required by svtkDataCompressor.
  int GetCompressionLevel() override;

  // Compression level setter required by svtkDataCompresor.
  void SetCompressionLevel(int compressionLevel) override;
  //@}

protected:
  svtkZLibDataCompressor();
  ~svtkZLibDataCompressor() override;

  int CompressionLevel;

  // Compression method required by svtkDataCompressor.
  size_t CompressBuffer(unsigned char const* uncompressedData, size_t uncompressedSize,
    unsigned char* compressedData, size_t compressionSpace) override;
  // Decompression method required by svtkDataCompressor.
  size_t UncompressBuffer(unsigned char const* compressedData, size_t compressedSize,
    unsigned char* uncompressedData, size_t uncompressedSize) override;

private:
  svtkZLibDataCompressor(const svtkZLibDataCompressor&) = delete;
  void operator=(const svtkZLibDataCompressor&) = delete;
};

#endif
