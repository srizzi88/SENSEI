/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPVWebGLExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkPVWebGLExporter_h
#define svtkPVWebGLExporter_h

#include "svtkExporter.h"
#include "svtkWebGLExporterModule.h" // needed for export macro

class SVTKWEBGLEXPORTER_EXPORT svtkPVWebGLExporter : public svtkExporter
{
public:
  static svtkPVWebGLExporter* New();
  svtkTypeMacro(svtkPVWebGLExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Description:
  // Specify the name of the VRML file to write.
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

protected:
  svtkPVWebGLExporter();
  ~svtkPVWebGLExporter() override;

  void WriteData() override;

  char* FileName;

private:
  svtkPVWebGLExporter(const svtkPVWebGLExporter&) = delete;
  void operator=(const svtkPVWebGLExporter&) = delete;
};

#endif
