/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOBJWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOBJWriter
 * @brief   write wavefront obj file
 *
 * svtkOBJWriter writes wavefront obj (.obj) files in ASCII form.
 * OBJ files contain the geometry including lines, triangles and polygons.
 * Normals and texture coordinates on points are also written if they exist.
 * One can specify a texture passing a svtkImageData on port 1.
 * If a texture is set, additionals .mtl and .png files are generated. Those files have the same
 * name without obj extension.
 */

#ifndef svtkOBJWriter_h
#define svtkOBJWriter_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkWriter.h"

class svtkDataSet;
class svtkImageData;
class svtkPolyData;

class SVTKIOGEOMETRY_EXPORT svtkOBJWriter : public svtkWriter
{
public:
  static svtkOBJWriter* New();
  svtkTypeMacro(svtkOBJWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the inputs to this writer.
   */
  svtkPolyData* GetInputGeometry();
  svtkImageData* GetInputTexture();
  svtkDataSet* GetInput(int port);
  //@}

  //@{
  /**
   * Get/Set the file name of the OBJ file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkOBJWriter();
  ~svtkOBJWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  char* FileName;

private:
  svtkOBJWriter(const svtkOBJWriter&) = delete;
  void operator=(const svtkOBJWriter&) = delete;
};

#endif
