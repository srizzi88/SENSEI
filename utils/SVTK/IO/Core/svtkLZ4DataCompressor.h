/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLZ4DataCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLZ4DataCompressor
 * @brief   Data compression using LZ4.
 *
 * svtkLZ4DataCompressor provides a concrete svtkDataCompressor class
 * using LZ4 for compressing and uncompressing data.
 */

#ifndef svtkLZ4DataCompressor_h
#define svtkLZ4DataCompressor_h

#include "svtkDataCompressor.h"
#include "svtkIOCoreModule.h" // For export macro

class SVTKIOCORE_EXPORT svtkLZ4DataCompressor : public svtkDataCompressor
{
public:
  svtkTypeMacro(svtkLZ4DataCompressor, svtkDataCompressor);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkLZ4DataCompressor* New();

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
  // Compression level getter required by svtkDataCompressor.
  int GetCompressionLevel() override;

  // Compression level setter required by svtkDataCompresor.
  void SetCompressionLevel(int compressionLevel) override;

  // Direct setting of AccelerationLevel allows more direct
  // control over LZ4 compressor
  svtkSetClampMacro(AccelerationLevel, int, 1, SVTK_INT_MAX);
  svtkGetMacro(AccelerationLevel, int);

protected:
  svtkLZ4DataCompressor();
  ~svtkLZ4DataCompressor() override;

  int AccelerationLevel;

  // Compression method required by svtkDataCompressor.
  size_t CompressBuffer(unsigned char const* uncompressedData, size_t uncompressedSize,
    unsigned char* compressedData, size_t compressionSpace) override;
  // Decompression method required by svtkDataCompressor.
  size_t UncompressBuffer(unsigned char const* compressedData, size_t compressedSize,
    unsigned char* uncompressedData, size_t uncompressedSize) override;

private:
  svtkLZ4DataCompressor(const svtkLZ4DataCompressor&) = delete;
  void operator=(const svtkLZ4DataCompressor&) = delete;
};

#endif
