/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebGLPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWebGLPolyData
 *
 * PolyData representation for WebGL.
 */

#ifndef svtkWebGLPolyData_h
#define svtkWebGLPolyData_h

#include "svtkWebGLExporterModule.h" // needed for export macro
#include "svtkWebGLObject.h"

class svtkActor;
class svtkMatrix4x4;
class svtkMapper;
class svtkPointData;
class svtkPolyData;
class svtkTriangleFilter;

class SVTKWEBGLEXPORTER_EXPORT svtkWebGLPolyData : public svtkWebGLObject
{
public:
  static svtkWebGLPolyData* New();
  svtkTypeMacro(svtkWebGLPolyData, svtkWebGLObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void GenerateBinaryData() override;
  unsigned char* GetBinaryData(int part) override;
  int GetBinarySize(int part) override;
  int GetNumberOfParts() override;

  void GetPoints(svtkTriangleFilter* polydata, svtkActor* actor, int maxSize);

  void GetLinesFromPolygon(svtkMapper* mapper, svtkActor* actor, int lineMaxSize, double* edgeColor);
  void GetLines(svtkTriangleFilter* polydata, svtkActor* actor, int lineMaxSize);
  void GetColorsFromPolyData(unsigned char* color, svtkPolyData* polydata, svtkActor* actor);

  // Get following data from the actor
  void GetPolygonsFromPointData(svtkTriangleFilter* polydata, svtkActor* actor, int maxSize);
  void GetPolygonsFromCellData(svtkTriangleFilter* polydata, svtkActor* actor, int maxSize);
  void GetColorsFromPointData(
    unsigned char* color, svtkPointData* pointdata, svtkPolyData* polydata, svtkActor* actor);

  void SetMesh(float* _vertices, int _numberOfVertices, int* _index, int _numberOfIndexes,
    float* _normals, unsigned char* _colors, float* _tcoords, int maxSize);
  void SetLine(float* _points, int _numberOfPoints, int* _index, int _numberOfIndex,
    unsigned char* _colors, int maxSize);
  void SetPoints(float* points, int numberOfPoints, unsigned char* colors, int maxSize);
  void SetTransformationMatrix(svtkMatrix4x4* m);

protected:
  svtkWebGLPolyData();
  ~svtkWebGLPolyData() override;

private:
  svtkWebGLPolyData(const svtkWebGLPolyData&) = delete;
  void operator=(const svtkWebGLPolyData&) = delete;

  class svtkInternal;
  svtkInternal* Internal;
};

#endif
