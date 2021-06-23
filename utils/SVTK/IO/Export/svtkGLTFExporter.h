/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGLTFExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGLTFExporter
 * @brief   export a scene into GLTF 2.0 format.
 *
 * svtkGLTFExporter is a concrete subclass of svtkExporter that writes GLTF 2.0
 * files. It currently only supports a very small subset of what SVTK can do
 * including polygonal meshes with optional vertex colors. Over time the class
 * can be expanded to support more and more of what SVTK renders.
 *
 * It should be noted that gltf is a format for rendering data. As such
 * it stores what the SVTK scene renders as, not the underlying data. For
 * example it currently does not support quads or higher sided polygons
 * although SVTK does. As such taking an exported gltf file and then selecting
 * wireframe in a viewer will give all triangles where SVTK's rendering
 * would correctly draw the original polygons. etc.
 *
 * @sa
 * svtkExporter
 */

#ifndef svtkGLTFExporter_h
#define svtkGLTFExporter_h

#include "svtkExporter.h"
#include "svtkIOExportModule.h" // For export macro

#include <string> // for std::string

class SVTKIOEXPORT_EXPORT svtkGLTFExporter : public svtkExporter
{
public:
  static svtkGLTFExporter* New();
  svtkTypeMacro(svtkGLTFExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the name of the GLTF file to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Should the binary data be included in the json file as a base64
   * string.
   */
  svtkGetMacro(InlineData, bool);
  svtkSetMacro(InlineData, bool);
  svtkBooleanMacro(InlineData, bool);
  //@}

  //@{
  /**
   * It looks for a point array called
   * NORMAL in the data and it saves it in the
   * GLTF file if found.
   * NORMAL is the vertex normal. Cesium needs this to render buildings correctly
   * if there is no texture.
   */
  svtkGetMacro(SaveNormal, bool);
  svtkSetMacro(SaveNormal, bool);
  svtkBooleanMacro(SaveNormal, bool);
  //@}

  //@{
  /**
   * It looks for point arrays called
   * _BATCHID in the data and it saves it in the
   * GLTF file if found.
   * _BATCHID is an index used in 3D Tiles b3dm format. This format stores
   * a binary gltf with a mesh that has several objects (buildings).
   * Objects are indexed from 0 to number of objects - 1, all points
   * of an objects have the same index. These index values are stored
   * in _BATCHID
   */
  svtkGetMacro(SaveBatchId, bool);
  svtkSetMacro(SaveBatchId, bool);
  svtkBooleanMacro(SaveBatchId, bool);
  //@}



  /**
   * Write the result to a string instead of a file
   */
  std::string WriteToString();

  /**
   * Write the result to a provided ostream
   */
  void WriteToStream(ostream& out);

protected:
  svtkGLTFExporter();
  ~svtkGLTFExporter() override;

  void WriteData() override;


  char* FileName;
  bool InlineData;
  bool SaveNormal;
  bool SaveBatchId;

private:
  svtkGLTFExporter(const svtkGLTFExporter&) = delete;
  void operator=(const svtkGLTFExporter&) = delete;
};

#endif
