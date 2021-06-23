/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkOpenGLVertexArrayObject_h
#define svtkOpenGLVertexArrayObject_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // for export macro
#include <string>                      // For API.

class svtkShaderProgram;
class svtkOpenGLBufferObject;
class svtkOpenGLVertexBufferObject;

/**
 * @brief The VertexArrayObject class uses, or emulates, vertex array objects.
 * These are extremely useful for setup/tear down of vertex attributes, and can
 * offer significant performance benefits when the hardware supports them.
 *
 * It should be noted that this object is very lightweight, and it assumes the
 * objects being used are correctly set up. Even without support for VAOs this
 * class caches the array locations, types, etc and avoids repeated look ups. It
 * it bound to a single ShaderProgram object.
 */

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLVertexArrayObject : public svtkObject
{
public:
  static svtkOpenGLVertexArrayObject* New();
  svtkTypeMacro(svtkOpenGLVertexArrayObject, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Bind();

  void Release();

  void ReleaseGraphicsResources();

  void ShaderProgramChanged();

  bool AddAttributeArray(svtkShaderProgram* program, svtkOpenGLBufferObject* buffer,
    const std::string& name, int offset, size_t stride, int elementType, int elementTupleSize,
    bool normalize)
  {
    return this->AddAttributeArrayWithDivisor(
      program, buffer, name, offset, stride, elementType, elementTupleSize, normalize, 0, false);
  }

  bool AddAttributeArray(svtkShaderProgram* program, svtkOpenGLVertexBufferObject* buffer,
    const std::string& name, int offset, bool normalize);

  bool AddAttributeArrayWithDivisor(svtkShaderProgram* program, svtkOpenGLBufferObject* buffer,
    const std::string& name, int offset, size_t stride, int elementType, int elementTupleSize,
    bool normalize, int divisor, bool isMatrix);

  bool AddAttributeMatrixWithDivisor(svtkShaderProgram* program, svtkOpenGLBufferObject* buffer,
    const std::string& name, int offset, size_t stride, int elementType, int elementTupleSize,
    bool normalize, int divisor, int tupleOffset);

  bool RemoveAttributeArray(const std::string& name);

  // Force this VAO to emulate a vertex array object even if
  // the system supports VAOs. This can be useful in cases where
  // the vertex array object does not handle all extensions.
  void SetForceEmulation(bool val);

protected:
  svtkOpenGLVertexArrayObject();
  ~svtkOpenGLVertexArrayObject() override;

private:
  svtkOpenGLVertexArrayObject(const svtkOpenGLVertexArrayObject&) = delete;
  void operator=(const svtkOpenGLVertexArrayObject&) = delete;
  class Private;
  Private* Internal;
};

#endif // svtkOpenGLVertexArrayObject_h
