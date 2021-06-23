/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPostScriptWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPostScriptWriter
 * @brief   Writes an image as a PostScript file.
 *
 * svtkPostScriptWriter writes an image as a PostScript file using some
 * reasonable scalings and centered on the page which is assumed to be
 * about 8.5 by 11 inches. This is based loosely off of the code from
 * pnmtops.c. Right now there aren't any real options.
 */

#ifndef svtkPostScriptWriter_h
#define svtkPostScriptWriter_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageWriter.h"

class SVTKIOIMAGE_EXPORT svtkPostScriptWriter : public svtkImageWriter
{
public:
  static svtkPostScriptWriter* New();
  svtkTypeMacro(svtkPostScriptWriter, svtkImageWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPostScriptWriter() {}
  ~svtkPostScriptWriter() override {}

  void WriteFile(ostream* file, svtkImageData* data, int extent[6], int wExt[6]) override;
  void WriteFileHeader(ostream*, svtkImageData*, int wExt[6]) override;
  void WriteFileTrailer(ostream*, svtkImageData*) override;

private:
  svtkPostScriptWriter(const svtkPostScriptWriter&) = delete;
  void operator=(const svtkPostScriptWriter&) = delete;
};

#endif
