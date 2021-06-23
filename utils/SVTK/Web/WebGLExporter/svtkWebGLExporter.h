/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebGLExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWebGLExporter
 *
 * svtkWebGLExporter export the data of the scene to be used in the WebGL.
 */

#ifndef svtkWebGLExporter_h
#define svtkWebGLExporter_h

class svtkActor;
class svtkActor2D;
class svtkCellData;
class svtkMapper;
class svtkPointData;
class svtkPolyData;
class svtkRenderer;
class svtkRendererCollection;
class svtkTriangleFilter;
class svtkWebGLObject;
class svtkWebGLPolyData;

#include "svtkObject.h"
#include "svtkWebGLExporterModule.h" // needed for export macro

#include <string> // needed for internal structure

typedef enum
{
  SVTK_ONLYCAMERA = 0,
  SVTK_ONLYWIDGET = 1,
  SVTK_PARSEALL = 2
} SVTKParseType;

class SVTKWEBGLEXPORTER_EXPORT svtkWebGLExporter : public svtkObject
{
public:
  static svtkWebGLExporter* New();
  svtkTypeMacro(svtkWebGLExporter, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get all the needed information from the svtkRenderer
   */
  void parseScene(svtkRendererCollection* renderers, const char* viewId, int parseType);
  // Generate and return the Metadata
  void exportStaticScene(svtkRendererCollection* renderers, int width, int height, std::string path);
  const char* GenerateMetadata();
  const char* GetId();
  svtkWebGLObject* GetWebGLObject(int index);
  int GetNumberOfObjects();
  bool hasChanged();
  void SetCenterOfRotation(float a1, float a2, float a3);
  void SetMaxAllowedSize(int mesh, int lines);
  void SetMaxAllowedSize(int size);
  //@}

  static void ComputeMD5(const unsigned char* content, int size, std::string& hash);

protected:
  svtkWebGLExporter();
  ~svtkWebGLExporter() override;

  void parseRenderer(svtkRenderer* render, const char* viewId, bool onlyWidget, void* mapTime);
  void generateRendererData(svtkRendererCollection* renderers, const char* viewId);
  void parseActor(
    svtkActor* actor, svtkMTimeType actorTime, size_t rendererId, int layer, bool isWidget);
  void parseActor2D(
    svtkActor2D* actor, svtkMTimeType actorTime, size_t renderId, int layer, bool isWidget);
  const char* GenerateExportMetadata();

  // Get the dataset from the mapper
  svtkTriangleFilter* GetPolyData(svtkMapper* mapper, svtkMTimeType& dataMTime);

  svtkTriangleFilter* TriangleFilter;  // Last Polygon Dataset Parse
  double CameraLookAt[10];            // Camera Look At (fov, position[3], up[3], eye[3])
  bool GradientBackground;            // If the scene use a gradient background
  double Background1[3];              // Background color of the rendering screen (RGB)
  double Background2[3];              // Scond background color
  double SceneSize[3];                // Size of the bounding box of the scene
  std::string SceneId;                // Id of the parsed scene
  float CenterOfRotation[3];          // Center Of Rotation
  int meshObjMaxSize, lineObjMaxSize; // Max size of object allowed (faces)
  std::string renderersMetaData;
  bool hasWidget;

private:
  svtkWebGLExporter(const svtkWebGLExporter&) = delete;
  void operator=(const svtkWebGLExporter&) = delete;

  class svtkInternal;
  svtkInternal* Internal;
};

#endif
