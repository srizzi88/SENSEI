/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOggTheoraWriter.h

  Copyright (c) Michael Wild, Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOggTheoraWriter
 * @brief   Uses the ogg and theora libraries to write video
 * files.
 *
 * svtkOggTheoraWriter is an adapter that allows SVTK to use the ogg and theora
 * libraries to write movie files.  This class creates .ogv files containing
 * theora encoded video without audio.
 *
 * This implementation is based on svtkFFMPEGWriter and uses some code derived
 * from the encoder example distributed with libtheora.
 *
 */

#ifndef svtkOggTheoraWriter_h
#define svtkOggTheoraWriter_h

#include "svtkGenericMovieWriter.h"
#include "svtkIOOggTheoraModule.h" // For export macro

class svtkOggTheoraWriterInternal;

class SVTKIOOGGTHEORA_EXPORT svtkOggTheoraWriter : public svtkGenericMovieWriter
{
public:
  static svtkOggTheoraWriter* New();
  svtkTypeMacro(svtkOggTheoraWriter, svtkGenericMovieWriter);
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
   * Set/Get the frame rate, in frame/s.
   */
  svtkSetClampMacro(Rate, int, 1, 5000);
  svtkGetMacro(Rate, int);
  //@}

  //@{
  /**
   * Is the video to be encoded using 4:2:0 subsampling?
   */
  svtkSetMacro(Subsampling, svtkTypeBool);
  svtkGetMacro(Subsampling, svtkTypeBool);
  svtkBooleanMacro(Subsampling, svtkTypeBool);
  //@}

protected:
  svtkOggTheoraWriter();
  ~svtkOggTheoraWriter() override;

  svtkOggTheoraWriterInternal* Internals;

  int Initialized;
  int Quality;
  int Rate;
  svtkTypeBool Subsampling;

private:
  svtkOggTheoraWriter(const svtkOggTheoraWriter&) = delete;
  void operator=(const svtkOggTheoraWriter&) = delete;
};

#endif
