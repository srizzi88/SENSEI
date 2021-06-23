/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebGLDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWebGLDataSet
 *
 * svtkWebGLDataSet represent vertices, lines, polygons, and triangles.
 */

#ifndef svtkWebGLDataSet_h
#define svtkWebGLDataSet_h

#include "svtkObject.h"
#include "svtkWebGLExporterModule.h" // needed for export macro

#include "svtkWebGLObject.h" // Needed for the enum
#include <string>           // needed for md5

class SVTKWEBGLEXPORTER_EXPORT svtkWebGLDataSet : public svtkObject
{
public:
  static svtkWebGLDataSet* New();
  svtkTypeMacro(svtkWebGLDataSet, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void SetVertices(float* v, int size);
  void SetIndexes(short* i, int size);
  void SetNormals(float* n);
  void SetColors(unsigned char* c);
  void SetPoints(float* p, int size);
  void SetTCoords(float* t);
  void SetMatrix(float* m);
  void SetType(WebGLObjectTypes t);

  unsigned char* GetBinaryData();
  int GetBinarySize();
  void GenerateBinaryData();
  bool HasChanged();

  std::string GetMD5();

protected:
  svtkWebGLDataSet();
  ~svtkWebGLDataSet() override;

  int NumberOfVertices;
  int NumberOfPoints;
  int NumberOfIndexes;
  WebGLObjectTypes webGLType;

  float* Matrix;
  float* vertices;
  float* normals;
  short* indexes;
  float* points;
  float* tcoords;
  unsigned char* colors;
  unsigned char* binary; // Data in binary
  int binarySize;        // Size of the data in binary
  bool hasChanged;
  std::string MD5;

private:
  svtkWebGLDataSet(const svtkWebGLDataSet&) = delete;
  void operator=(const svtkWebGLDataSet&) = delete;
};

#endif
