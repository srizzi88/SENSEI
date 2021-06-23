/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIVExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkIVExporter
 * @brief   export a scene into OpenInventor 2.0 format.
 *
 * svtkIVExporter is a concrete subclass of svtkExporter that writes
 * OpenInventor 2.0 files.
 *
 * @sa
 * svtkExporter
 */

#ifndef svtkIVExporter_h
#define svtkIVExporter_h

#include "svtkExporter.h"
#include "svtkIOExportModule.h" // For export macro

class svtkLight;
class svtkActor;
class svtkPoints;
class svtkDataArray;
class svtkUnsignedCharArray;

class SVTKIOEXPORT_EXPORT svtkIVExporter : public svtkExporter
{
public:
  static svtkIVExporter* New();
  svtkTypeMacro(svtkIVExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the name of the OpenInventor file to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkIVExporter();
  ~svtkIVExporter() override;

  void WriteData() override;
  void WriteALight(svtkLight* aLight, FILE* fp);
  void WriteAnActor(svtkActor* anActor, FILE* fp);
  void WritePointData(svtkPoints* points, svtkDataArray* normals, svtkDataArray* tcoords,
    svtkUnsignedCharArray* colors, FILE* fp);
  char* FileName;

private:
  svtkIVExporter(const svtkIVExporter&) = delete;
  void operator=(const svtkIVExporter&) = delete;
};

#endif
