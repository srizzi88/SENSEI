/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLVertexBufferObjectCache
 * @brief   manage vertex buffer objects shared within a context
 *
 * This class allows mappers to share VBOs. Specifically it
 * is used by the V..B..O..Group to see if a VBO already exists
 * for a given svtkDataArray.
 *
 *
 *
 */

#ifndef svtkOpenGLVertexBufferObjectCache_h
#define svtkOpenGLVertexBufferObjectCache_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include <map>                         // for methods

class svtkOpenGLVertexBufferObject;
class svtkDataArray;
class svtkTimeStamp;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLVertexBufferObjectCache : public svtkObject
{
public:
  static svtkOpenGLVertexBufferObjectCache* New();
  svtkTypeMacro(svtkOpenGLVertexBufferObjectCache, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Returns the vertex buffer object which holds the
   * data array's data. If such a VBO does not exist
   * a new empty VBO will be created you need to append to.
   * The return value has been registered, you are responsible
   * for deleting it. The data array pointers are also registered.
   */
  svtkOpenGLVertexBufferObject* GetVBO(svtkDataArray* array, int destType);

  /**
   * Removes all references to a given vertex buffer
   * object.
   */
  void RemoveVBO(svtkOpenGLVertexBufferObject* vbo);

  typedef std::map<svtkDataArray*, svtkOpenGLVertexBufferObject*> VBOMap;

protected:
  svtkOpenGLVertexBufferObjectCache();
  ~svtkOpenGLVertexBufferObjectCache() override;

  VBOMap MappedVBOs;

private:
  svtkOpenGLVertexBufferObjectCache(const svtkOpenGLVertexBufferObjectCache&) = delete;
  void operator=(const svtkOpenGLVertexBufferObjectCache&) = delete;
};

#endif
