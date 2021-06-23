/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFFMPEGWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFFMPEGWriter
 * @brief   Uses the FFMPEG library to write video files.
 *
 * svtkFFMPEGWriter is an adapter that allows SVTK to use the LGPL'd FFMPEG
 * library to write movie files. FFMPEG can create a variety of multimedia
 * file formats and can use a variety of encoding algorithms (codecs).
 * This class creates .avi files containing MP43 encoded video without
 * audio.
 *
 * The FFMPEG multimedia library source code can be obtained from
 * the sourceforge web site at http://ffmpeg.sourceforge.net/download.php
 * or is a tarball along with installation instructions at
 * http://www.svtk.org/files/support/ffmpeg_source.tar.gz
 *
 */

#ifndef svtkFFMPEGWriter_h
#define svtkFFMPEGWriter_h

#include "svtkGenericMovieWriter.h"
#include "svtkIOFFMPEGModule.h" // For export macro

class svtkFFMPEGWriterInternal;

class SVTKIOFFMPEG_EXPORT svtkFFMPEGWriter : public svtkGenericMovieWriter
{
public:
  static svtkFFMPEGWriter* New();
  svtkTypeMacro(svtkFFMPEGWriter, svtkGenericMovieWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * These methods start writing an Movie file, write a frame to the file
   * and then end the writing process.
   */
  void Start() override;
  void Write() override;
  void End() override;
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
   * Turns on(the default) or off compression.
   * Turning off compression overrides quality setting.
   */
  svtkSetMacro(Compression, bool);
  svtkGetMacro(Compression, bool);
  svtkBooleanMacro(Compression, bool);
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
   * Set/Get the bit-rate
   */
  svtkSetMacro(BitRate, int);
  svtkGetMacro(BitRate, int);
  //@}

  //@{
  /**
   * Set/Get the bit-rate tolerance
   */
  svtkSetMacro(BitRateTolerance, int);
  svtkGetMacro(BitRateTolerance, int);
  //@}

protected:
  svtkFFMPEGWriter();
  ~svtkFFMPEGWriter() override;

  svtkFFMPEGWriterInternal* Internals;

  int Initialized;
  int Quality;
  int Rate;
  int BitRate;
  int BitRateTolerance;
  bool Compression;

private:
  svtkFFMPEGWriter(const svtkFFMPEGWriter&) = delete;
  void operator=(const svtkFFMPEGWriter&) = delete;
};

#endif
