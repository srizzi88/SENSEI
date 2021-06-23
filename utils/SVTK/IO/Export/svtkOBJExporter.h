/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOBJExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOBJExporter
 * @brief   export a scene into Wavefront format.
 *
 * svtkOBJExporter is a concrete subclass of svtkExporter that writes wavefront
 * .OBJ files in ASCII form. It also writes out a mtl file that contains the
 * material properties. The filenames are derived by appending the .obj and
 * .mtl suffix onto the user specified FilePrefix.
 *
 * @sa
 * svtkExporter
 */

#ifndef svtkOBJExporter_h
#define svtkOBJExporter_h

#include "svtkExporter.h"
#include "svtkIOExportModule.h" // For export macro
#include <fstream>             // For ofstream
#include <map>                 // For map
#include <vector>              // For string

class svtkActor;
class svtkTexture;

class SVTKIOEXPORT_EXPORT svtkOBJExporter : public svtkExporter
{
public:
  static svtkOBJExporter* New();
  svtkTypeMacro(svtkOBJExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the prefix of the files to write out. The resulting filenames
   * will have .obj and .mtl appended to them.
   */
  svtkSetStringMacro(FilePrefix);
  svtkGetStringMacro(FilePrefix);
  //@}

  //@{
  /**
   * Specify comment string that will be written to the obj file header.
   */
  svtkSetStringMacro(OBJFileComment);
  svtkGetStringMacro(OBJFileComment);
  //@}

  //@{
  /**
   * Specify comment string that will be written to the mtl file header.
   */
  svtkSetStringMacro(MTLFileComment);
  svtkGetStringMacro(MTLFileComment);
  //@}

protected:
  svtkOBJExporter();
  ~svtkOBJExporter() override;

  void WriteData() override;
  void WriteAnActor(
    svtkActor* anActor, std::ostream& fpObj, std::ostream& fpMat, std::string& modelName, int& id);
  char* FilePrefix;
  char* OBJFileComment;
  char* MTLFileComment;
  bool FlipTexture;
  std::map<std::string, svtkTexture*> TextureFileMap;

private:
  svtkOBJExporter(const svtkOBJExporter&) = delete;
  void operator=(const svtkOBJExporter&) = delete;
};

#endif
