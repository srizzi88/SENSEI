/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericMovieWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGenericMovieWriter
 * @brief   an abstract movie writer class.
 *
 * svtkGenericMovieWriter is the abstract base class for several movie
 * writers. The input type is a svtkImageData. The Start() method will
 * open and create the file, the Write() method will output a frame to
 * the file (i.e. the contents of the svtkImageData), End() will finalize
 * and close the file.
 * @sa
 * svtkAVIWriter
 */

#ifndef svtkGenericMovieWriter_h
#define svtkGenericMovieWriter_h

#include "svtkIOMovieModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class svtkImageData;

class SVTKIOMOVIE_EXPORT svtkGenericMovieWriter : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkGenericMovieWriter, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of avi file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * These methods start writing an Movie file, write a frame to the file
   * and then end the writing process.
   */
  virtual void Start() = 0;
  virtual void Write() = 0;
  virtual void End() = 0;
  //@}

  //@{
  /**
   * Was there an error on the last write performed?
   */
  svtkGetMacro(Error, int);
  //@}

  /**
   * Converts svtkErrorCodes and svtkGenericMovieWriter errors to strings.
   */
  static const char* GetStringFromErrorCode(unsigned long event);

  enum MovieWriterErrorIds
  {
    UserError = 40000, // must match svtkErrorCode::UserError
    InitError,
    NoInputError,
    CanNotCompress,
    CanNotFormat,
    ChangedResolutionError
  };

protected:
  svtkGenericMovieWriter();
  ~svtkGenericMovieWriter() override;

  char* FileName;
  int Error;

private:
  svtkGenericMovieWriter(const svtkGenericMovieWriter&) = delete;
  void operator=(const svtkGenericMovieWriter&) = delete;
};

#endif
