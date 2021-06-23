/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSingleVTPExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSingleVTPExporter
 * @brief   export a scene into a single vtp file and png texture
 *
 * svtkSingleVTPExporter is a concrete subclass of svtkExporter that writes
 * a .vtp file and a .png file containing the polydata and texture
 * elements of the scene.
 *
 * If ActiveRenderer is specified then it exports contents of
 * ActiveRenderer. Otherwise it exports contents of all renderers.
 *
 * @sa
 * svtkExporter
 */

#ifndef svtkSingleVTPExporter_h
#define svtkSingleVTPExporter_h

#include "svtkExporter.h"
#include "svtkIOExportModule.h" // For export macro
#include <vector>              // for method args

class svtkActor;
class svtkPolyData;
class svtkTexture;

class SVTKIOEXPORT_EXPORT svtkSingleVTPExporter : public svtkExporter
{
public:
  static svtkSingleVTPExporter* New();
  svtkTypeMacro(svtkSingleVTPExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the prefix of the files to write out. The resulting filenames
   * will have .vtp and .png appended to them.
   */
  svtkSetStringMacro(FilePrefix);
  svtkGetStringMacro(FilePrefix);
  //@}

  // computes the file prefix from a filename by removing
  // the .vtp extension if present. Useful for APIs that
  // are filename centric.
  void SetFileName(const char*);

protected:
  svtkSingleVTPExporter();
  ~svtkSingleVTPExporter() override;

  void WriteData() override;

  class actorData
  {
  public:
    svtkActor* Actor = nullptr;
    svtkTexture* Texture = nullptr;
    int ImagePosition[2];
    double URange[2];
    double VRange[2];
    bool HaveRepeatingTexture = false;
  };
  int TextureSize[2];
  void WriteTexture(std::vector<actorData>& actors);
  void WriteVTP(std::vector<actorData>& actors);
  char* FilePrefix;

  // handle repeating textures by subdividing triangles
  // so that they do not span mode than 0.0-1.5 of texture
  // range.
  svtkPolyData* FixTextureCoordinates(svtkPolyData*);

  // recursive method that handles one triangle
  void ProcessTriangle(const svtkIdType* pts, svtkPolyData* out);

private:
  svtkSingleVTPExporter(const svtkSingleVTPExporter&) = delete;
  void operator=(const svtkSingleVTPExporter&) = delete;
};

#endif
