/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkOpenGLIndexBufferObject_h
#define svtkOpenGLIndexBufferObject_h

#include "svtkOpenGLBufferObject.h"
#include "svtkRenderingOpenGL2Module.h" // for export macro

/**
 * @brief OpenGL vertex buffer object
 *
 * OpenGL buffer object to store geometry and/or attribute data on the
 * GPU.
 */

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLIndexBufferObject : public svtkOpenGLBufferObject
{
public:
  static svtkOpenGLIndexBufferObject* New();
  svtkTypeMacro(svtkOpenGLIndexBufferObject, svtkOpenGLBufferObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Sizes/offsets are all in bytes as OpenGL API expects them.
  size_t IndexCount; // Number of indices in the VBO

  // Description:
  // used to create an IBO for triangle primitives
  size_t CreateTriangleIndexBuffer(svtkCellArray* cells, svtkPoints* points);

  // Description:
  // used to create an IBO for triangle primitives
  static void AppendTriangleIndexBuffer(std::vector<unsigned int>& indexArray, svtkCellArray* cells,
    svtkPoints* points, svtkIdType vertexOffset);

  // Description:
  // create a IBO for wireframe polys/tris
  size_t CreateTriangleLineIndexBuffer(svtkCellArray* cells);

  // Description:
  // used to create an IBO for line primitives
  static void AppendLineIndexBuffer(
    std::vector<unsigned int>& indexArray, svtkCellArray* cells, svtkIdType vertexOffset);

  // Description:
  // create a IBO for wireframe polys/tris
  size_t CreateLineIndexBuffer(svtkCellArray* cells);

  // Description:
  // create a IBO for wireframe polys/tris
  static void AppendTriangleLineIndexBuffer(
    std::vector<unsigned int>& indexArray, svtkCellArray* cells, svtkIdType vertexOffset);

  // Description:
  // used to create an IBO for primitives as points
  size_t CreatePointIndexBuffer(svtkCellArray* cells);

  // Description:
  // used to create an IBO for primitives as points
  static void AppendPointIndexBuffer(
    std::vector<unsigned int>& indexArray, svtkCellArray* cells, svtkIdType vertexOffset);

  // Description:
  // used to create an IBO for line strips and triangle strips
  size_t CreateStripIndexBuffer(svtkCellArray* cells, bool wireframeTriStrips);

  static void AppendStripIndexBuffer(std::vector<unsigned int>& indexArray, svtkCellArray* cells,
    svtkIdType vertexOffset, bool wireframeTriStrips);

  // Description:
  // special index buffer for polys wireframe with edge visibilityflags
  static void AppendEdgeFlagIndexBuffer(std::vector<unsigned int>& indexArray, svtkCellArray* cells,
    svtkIdType vertexOffset, svtkDataArray* edgeflags);

  size_t CreateEdgeFlagIndexBuffer(svtkCellArray* cells, svtkDataArray* edgeflags);

  // Description:
  // used to create an IBO for cell Vertices as points
  size_t CreateVertexIndexBuffer(svtkCellArray** cells);

  // Description:
  // used to create an IBO for primitives as points
  static void AppendVertexIndexBuffer(
    std::vector<unsigned int>& indexArray, svtkCellArray** cells, svtkIdType vertexOffset);

protected:
  svtkOpenGLIndexBufferObject();
  ~svtkOpenGLIndexBufferObject() override;

private:
  svtkOpenGLIndexBufferObject(const svtkOpenGLIndexBufferObject&) = delete;
  void operator=(const svtkOpenGLIndexBufferObject&) = delete;
};

#endif
