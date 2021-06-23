/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOOGLExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOOGLExporter
 * @brief   export a scene into Geomview OOGL format.
 *
 * svtkOOGLExporter is a concrete subclass of svtkExporter that writes
 * Geomview OOGL files.
 *
 * @sa
 * svtkExporter
 */

#ifndef svtkOOGLExporter_h
#define svtkOOGLExporter_h

#include "svtkExporter.h"
#include "svtkIOExportModule.h" // For export macro

class svtkLight;
class svtkActor;

class SVTKIOEXPORT_EXPORT svtkOOGLExporter : public svtkExporter
{
public:
  static svtkOOGLExporter* New();
  svtkTypeMacro(svtkOOGLExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the name of the Geomview file to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkOOGLExporter();
  ~svtkOOGLExporter() override;

  void WriteData() override;
  void WriteALight(svtkLight* aLight, FILE* fp);
  void WriteAnActor(svtkActor* anActor, FILE* fp, int count);
  char* FileName;

private:
  svtkOOGLExporter(const svtkOOGLExporter&) = delete;
  void operator=(const svtkOOGLExporter&) = delete;
};

#endif
