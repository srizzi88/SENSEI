/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLGL2PSExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLGL2PSExporter
 * @brief   OpenGL2 implementation of GL2PS exporter.
 *
 *
 * Implementation of svtkGL2PSExporter for the OpenGL2 backend.
 */

#ifndef svtkOpenGLGL2PSExporter_h
#define svtkOpenGLGL2PSExporter_h

#include "svtkGL2PSExporter.h"
#include "svtkIOExportGL2PSModule.h" // For export macro

class svtkImageData;

class SVTKIOEXPORTGL2PS_EXPORT svtkOpenGLGL2PSExporter : public svtkGL2PSExporter
{
public:
  static svtkOpenGLGL2PSExporter* New();
  svtkTypeMacro(svtkOpenGLGL2PSExporter, svtkGL2PSExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkOpenGLGL2PSExporter();
  ~svtkOpenGLGL2PSExporter() override;

  void WriteData() override;

  bool RasterizeBackground(svtkImageData* image);
  bool CaptureVectorProps();

private:
  svtkOpenGLGL2PSExporter(const svtkOpenGLGL2PSExporter&) = delete;
  void operator=(const svtkOpenGLGL2PSExporter&) = delete;
};

#endif // svtkOpenGLGL2PSExporter_h
