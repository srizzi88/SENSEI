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
 * @class   svtkOpenGLVertexBufferObjectGroup
 * @brief   manage vertex buffer objects shared within a mapper
 *
 * This class holds onto the VBOs that a mapper is using.
 * The basic operation is that during the render process
 * the mapper may cache a number of dataArrays as VBOs
 * associated with attributes. This class keep track of
 * freeing VBOs no longer used by the mapper and uploading
 * new data as needed.
 *
 * When using CacheCataArray the same array can be set
 * each time and this class will not rebuild or upload
 * unless needed.
 *
 * When using the AppendDataArray API no caching is done
 * and the VBOs will be rebuilt and uploaded each time.
 * So when appending th emapper need to handle checking
 * if the VBO should be updated.
 *
 * Use case:
 *   make this an ivar of your mapper
 *   vbg->CacheDataArray("vertexMC", svtkDataArray);
 *   vbg->BuildAllVBOs();
 *   if (vbg->GetMTime() > your VAO update time)
 *   {
 *     vbg->AddAllAttributesToVAO(...);
 *   }
 *
 * Appended Use case:
 *   make this an ivar of your mapper
 *   if (you need to update your VBOs)
 *   {
 *     vbg->ClearAllVBOs();
 *     vbg->AppendDataArray("vertexMC", svtkDataArray1);
 *     vbg->AppendDataArray("vertexMC", svtkDataArray2);
 *     vbg->AppendDataArray("vertexMC", svtkDataArray3);
 *     vbg->BuildAllVBOs();
 *     vbg->AddAllAttributesToVAO(...);
 *   }
 *
 * use VAO
 *
 */

#ifndef svtkOpenGLVertexBufferObjectGroup_h
#define svtkOpenGLVertexBufferObjectGroup_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include <map>                         // for methods
#include <string>                      // for ivars
#include <vector>                      // for ivars

class svtkDataArray;
class svtkOpenGLVertexArrayObject;
class svtkOpenGLVertexBufferObject;
class svtkOpenGLVertexBufferObjectCache;
class svtkShaderProgram;
class svtkViewport;
class svtkWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLVertexBufferObjectGroup : public svtkObject
{
public:
  static svtkOpenGLVertexBufferObjectGroup* New();
  svtkTypeMacro(svtkOpenGLVertexBufferObjectGroup, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Returns the number of components for this attribute
   * zero if the attribute does not exist
   */
  int GetNumberOfComponents(const char* attribute);

  /**
   * Returns the number of tuples for this attribute
   * zero if the attribute does not exist
   */
  int GetNumberOfTuples(const char* attribute);

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*);

  /**
   * Returns the VBO for an attribute, NULL if
   * it is not present.
   */
  svtkOpenGLVertexBufferObject* GetVBO(const char* attribute);

  /**
   * Attach all VBOs to their attributes
   */
  void AddAllAttributesToVAO(svtkShaderProgram* program, svtkOpenGLVertexArrayObject* vao);

  /**
   * used to remove a no longer needed attribute
   * Calling CacheDataArray with a nullptr
   * attribute will also work.
   */
  void RemoveAttribute(const char* attribute);

  /**
   * Set the data array for an attribute in the VBO Group
   * registers the data array until build is called
   * once this is called a valid VBO will exist
   */
  void CacheDataArray(
    const char* attribute, svtkDataArray* da, svtkOpenGLVertexBufferObjectCache* cache, int destType);
  void CacheDataArray(const char* attribute, svtkDataArray* da, svtkViewport* vp, int destType);

  /**
   * Check if the array already exists.
   * offset is the index of the first vertex of the array if it exists.
   * totalOffset is the total number of vertices in the appended arrays.
   * Note that if the array does not exist, offset is equal to totalOffset.
   */
  bool ArrayExists(
    const char* attribute, svtkDataArray* da, svtkIdType& offset, svtkIdType& totalOffset);

  /**
   * Append a data array for an attribute in the VBO Group
   * registers the data array until build is called
   */
  void AppendDataArray(const char* attribute, svtkDataArray* da, int destType);

  /**
   * using the data arrays in this group
   * build all the VBOs, once this has been called the
   * reference to the data arrays will be freed.
   */
  void BuildAllVBOs(svtkOpenGLVertexBufferObjectCache*);
  void BuildAllVBOs(svtkViewport*);

  /**
   * Force all the VBOs to be freed from this group.
   * Call this prior to starting appending operations.
   * Not needed for single array caching.
   */
  void ClearAllVBOs();

  /**
   * Clear all the data arrays. Typically an internal method.
   * Automatically called at the end of BuildAllVBOs to prepare
   * for the next set of attributes.
   */
  void ClearAllDataArrays();

  /**
   * Get the mtime of this groups VBOs
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkOpenGLVertexBufferObjectGroup();
  ~svtkOpenGLVertexBufferObjectGroup() override;

  std::map<std::string, svtkOpenGLVertexBufferObject*> UsedVBOs;
  std::map<std::string, std::vector<svtkDataArray*> > UsedDataArrays;
  std::map<std::string, std::map<svtkDataArray*, svtkIdType> > UsedDataArrayMaps;
  std::map<std::string, svtkIdType> UsedDataArraySizes;

private:
  svtkOpenGLVertexBufferObjectGroup(const svtkOpenGLVertexBufferObjectGroup&) = delete;
  void operator=(const svtkOpenGLVertexBufferObjectGroup&) = delete;
};

#endif
