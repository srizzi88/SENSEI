/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVRMLExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVRMLExporter
 * @brief   export a scene into VRML 2.0 format.
 *
 * svtkVRMLExporter is a concrete subclass of svtkExporter that writes VRML 2.0
 * files. This is based on the VRML 2.0 draft #3 but it should be pretty
 * stable since we aren't using any of the newer features.
 *
 * @sa
 * svtkExporter
 */

#ifndef svtkVRMLExporter_h
#define svtkVRMLExporter_h

#include "svtkExporter.h"
#include "svtkIOExportModule.h" // For export macro

class svtkLight;
class svtkActor;
class svtkPoints;
class svtkDataArray;
class svtkUnsignedCharArray;
class svtkPolyData;
class svtkPointData;

class SVTKIOEXPORT_EXPORT svtkVRMLExporter : public svtkExporter
{
public:
  static svtkVRMLExporter* New();
  svtkTypeMacro(svtkVRMLExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the name of the VRML file to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the Speed of navigation. Default is 4.
   */
  svtkSetMacro(Speed, double);
  svtkGetMacro(Speed, double);
  //@}

  /**
   * Set the file pointer to write to. This will override
   * a FileName if specified.
   */
  void SetFilePointer(FILE*);

protected:
  svtkVRMLExporter();
  ~svtkVRMLExporter() override;

  void WriteData() override;
  void WriteALight(svtkLight* aLight, FILE* fp);
  void WriteAnActor(svtkActor* anActor, FILE* fp);
  void WritePointData(svtkPoints* points, svtkDataArray* normals, svtkDataArray* tcoords,
    svtkUnsignedCharArray* colors, FILE* fp);
  void WriteShapeBegin(svtkActor* actor, FILE* fileP, svtkPolyData* polyData, svtkPointData* pntData,
    svtkUnsignedCharArray* color);
  void WriteShapeEnd(FILE* fileP);
  char* FileName;
  FILE* FilePointer;
  double Speed;

private:
  svtkVRMLExporter(const svtkVRMLExporter&) = delete;
  void operator=(const svtkVRMLExporter&) = delete;
};

#endif
