/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAVIWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAVIWriter
 * @brief   Writes Windows AVI files.
 *
 * svtkAVIWriter writes AVI files. Note that this class in only available
 * on the Microsoft Windows platform. The data type of the file is
 * unsigned char regardless of the input type.
 * @sa
 * svtkGenericMovieWriter
 */

#ifndef svtkAVIWriter_h
#define svtkAVIWriter_h

#include "svtkGenericMovieWriter.h"
#include "svtkIOMovieModule.h" // For export macro

class svtkAVIWriterInternal;

class SVTKIOMOVIE_EXPORT svtkAVIWriter : public svtkGenericMovieWriter
{
public:
  static svtkAVIWriter* New();
  svtkTypeMacro(svtkAVIWriter, svtkGenericMovieWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * These methods start writing an AVI file, write a frame to the file
   * and then end the writing process.
   */
  void Start() override;
  void Write() override;
  void End() override;
  //@}

  //@{
  /**
   * Set/Get the frame rate, in frame/s.
   */
  svtkSetClampMacro(Rate, int, 1, 5000);
  svtkGetMacro(Rate, int);
  //@}

  //@{
  /**
   * Set/Get the compression quality.
   * 0 means worst quality and smallest file size
   * 2 means best quality and largest file size
   */
  svtkSetClampMacro(Quality, int, 0, 2);
  svtkGetMacro(Quality, int);
  //@}

  //@{
  /**
   * Set/Get if the user should be prompted for compression options, i.e.
   * pick a compressor, set the compression rate (override Rate), etc.).
   * Default is OFF (legacy).
   */
  svtkSetMacro(PromptCompressionOptions, int);
  svtkGetMacro(PromptCompressionOptions, int);
  svtkBooleanMacro(PromptCompressionOptions, int);
  //@}

  //@{
  /**
   * Set/Get the compressor FourCC.
   * A FourCC (literally, four-character code) is a sequence of four bytes
   * used to uniquely identify data formats. [...] One of the most well-known
   * uses of FourCCs is to identify the video codec used in AVI files.
   * Common identifiers include DIVX, XVID, and H264.
   * http://en.wikipedia.org/wiki/FourCC.
   * Default value is:
   * - MSVC
   * Other examples include:
   * - DIB: Full Frames (Uncompressed)
   * - LAGS: Lagarith Lossless Codec
   * - MJPG: M-JPG, aka Motion JPEG (say, Pegasus Imaging PicVideo M-JPEG)
   * Links:
   * - http://www.fourcc.org/
   * - http://www.microsoft.com/whdc/archive/fourcc.mspx
   * - http://abcavi.kibi.ru/fourcc.php
   */
  svtkSetStringMacro(CompressorFourCC);
  svtkGetStringMacro(CompressorFourCC);
  //@}

protected:
  svtkAVIWriter();
  ~svtkAVIWriter() override;

  svtkAVIWriterInternal* Internals;

  int Rate;
  int Time;
  int Quality;
  int PromptCompressionOptions;
  char* CompressorFourCC;

private:
  svtkAVIWriter(const svtkAVIWriter&) = delete;
  void operator=(const svtkAVIWriter&) = delete;
};

#endif
