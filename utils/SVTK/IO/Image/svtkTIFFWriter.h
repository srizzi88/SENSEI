/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTIFFWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTIFFWriter
 * @brief   write out image data as a TIFF file
 *
 * svtkTIFFWriter writes image data as a TIFF data file. Data can be written
 * uncompressed or compressed. Several forms of compression are supported
 * including packed bits, JPEG, deflation, and LZW. (Note: LZW compression
 * is currently under patent in the US and is disabled until the patent
 * expires. However, the mechanism for supporting this compression is available
 * for those with a valid license or to whom the patent does not apply.)
 */

#ifndef svtkTIFFWriter_h
#define svtkTIFFWriter_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageWriter.h"

class SVTKIOIMAGE_EXPORT svtkTIFFWriter : public svtkImageWriter
{
public:
  static svtkTIFFWriter* New();
  svtkTypeMacro(svtkTIFFWriter, svtkImageWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The main interface which triggers the writer to start.
   */
  void Write() override;

  enum
  { // Compression types
    NoCompression,
    PackBits,
    JPEG,
    Deflate,
    LZW
  };

  //@{
  /**
   * Set compression type. Sinze LZW compression is patented outside US, the
   * additional work steps have to be taken in order to use that compression.
   */
  svtkSetClampMacro(Compression, int, NoCompression, LZW);
  svtkGetMacro(Compression, int);
  void SetCompressionToNoCompression() { this->SetCompression(NoCompression); }
  void SetCompressionToPackBits() { this->SetCompression(PackBits); }
  void SetCompressionToJPEG() { this->SetCompression(JPEG); }
  void SetCompressionToDeflate() { this->SetCompression(Deflate); }
  void SetCompressionToLZW() { this->SetCompression(LZW); }
  //@}

protected:
  svtkTIFFWriter();
  ~svtkTIFFWriter() override {}

  void WriteFile(ostream* file, svtkImageData* data, int ext[6], int wExt[6]) override;
  void WriteFileHeader(ostream*, svtkImageData*, int wExt[6]) override;
  void WriteFileTrailer(ostream*, svtkImageData*) override;

  void* TIFFPtr;
  int Compression;
  int Width;
  int Height;
  int Pages;
  double XResolution;
  double YResolution;

private:
  svtkTIFFWriter(const svtkTIFFWriter&) = delete;
  void operator=(const svtkTIFFWriter&) = delete;

  template <typename T>
  void WriteVolume(T* buffer);
};

#endif
