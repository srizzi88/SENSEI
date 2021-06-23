/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPixelBufferObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPixelBufferObject
 * @brief   abstracts an OpenGL pixel buffer object.
 *
 * Provides low-level access to PBO mapped memory. Used to transfer raw data
 * to/from PBO mapped memory and the application. Once data is transferred to
 * the PBO it can then be transferred to the GPU (eg texture memory). Data may
 * be uploaded from the application into a pixel buffer or downloaded from the
 * pixel buffer to the application. The svtkTextureObject is used to transfer
 * data from/to the PBO to/from texture memory on the GPU.
 * @sa
 * OpenGL Pixel Buffer Object Extension Spec (ARB_pixel_buffer_object):
 * http://www.opengl.org/registry/specs/ARB/pixel_buffer_object.txt
 * @warning
 * Since most PBO mapped don't support double format all double data is converted to
 * float and then uploaded.
 */

#ifndef svtkPixelBufferObject_h
#define svtkPixelBufferObject_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkWeakPointer.h"            // needed for svtkWeakPointer.

class svtkRenderWindow;
class svtkOpenGLExtensionManager;

class SVTKRENDERINGOPENGL2_EXPORT svtkPixelBufferObject : public svtkObject
{
public:
  // Usage values.
  enum
  {
    StreamDraw = 0,
    StreamRead,
    StreamCopy,
    StaticDraw,
    StaticRead,
    StaticCopy,
    DynamicDraw,
    DynamicRead,
    DynamicCopy,
    NumberOfUsages
  };

  static svtkPixelBufferObject* New();
  svtkTypeMacro(svtkPixelBufferObject, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the context. Context must be a svtkOpenGLRenderWindow.
   * This does not increase the reference count of the
   * context to avoid reference loops.
   * SetContext() may raise an error is the OpenGL context does not support the
   * required OpenGL extensions.
   */
  void SetContext(svtkRenderWindow* context);
  svtkRenderWindow* GetContext();
  //@}

  //@{
  /**
   * Usage is a performance hint.
   * Valid values are:
   * - StreamDraw specified once by A, used few times S
   * - StreamRead specified once by R, queried a few times by A
   * - StreamCopy specified once by R, used a few times S
   * - StaticDraw specified once by A, used many times S
   * - StaticRead specified once by R, queried many times by A
   * - StaticCopy specified once by R, used many times S
   * - DynamicDraw respecified repeatedly by A, used many times S
   * - DynamicRead respecified repeatedly by R, queried many times by A
   * - DynamicCopy respecified repeatedly by R, used many times S
   * A: the application
   * S: as the source for GL drawing and image specification commands.
   * R: reading data from the GL
   * Initial value is StaticDraw, as in OpenGL spec.
   */
  svtkGetMacro(Usage, int);
  svtkSetMacro(Usage, int);
  //@}

  //@{
  /**
   * Upload data to PBO mapped.
   * The input data can be freed after this call.
   * The data ptr is treated as an 1D array with the given number of tuples and
   * given number of components in each tuple to be copied to the PBO mapped. increment
   * is the offset added after the last component in each tuple is transferred.
   * Look at the documentation for ContinuousIncrements in svtkImageData for
   * details about how increments are specified.
   */
  bool Upload1D(int type, void* data, unsigned int numtuples, int comps, svtkIdType increment)
  {
    unsigned int newdims[3];
    newdims[0] = numtuples;
    newdims[1] = 1;
    newdims[2] = 1;
    svtkIdType newinc[3];
    newinc[0] = increment;
    newinc[1] = 0;
    newinc[2] = 0;
    return this->Upload3D(type, data, newdims, comps, newinc, 0, nullptr);
  }
  //@}

  //@{
  /**
   * Update data to PBO mapped sourcing it from a 2D array.
   * The input data can be freed after this call.
   * The data ptr is treated as a 2D array with increments indicating how to
   * iterate over the data.
   * Look at the documentation for ContinuousIncrements in svtkImageData for
   * details about how increments are specified.
   */
  bool Upload2D(int type, void* data, unsigned int dims[2], int comps, svtkIdType increments[2])
  {
    unsigned int newdims[3];
    newdims[0] = dims[0];
    newdims[1] = dims[1];
    newdims[2] = 1;
    svtkIdType newinc[3];
    newinc[0] = increments[0];
    newinc[1] = increments[1];
    newinc[2] = 0;
    return this->Upload3D(type, data, newdims, comps, newinc, 0, nullptr);
  }
  //@}

  /**
   * Update data to PBO mapped sourcing it from a 3D array.
   * The input data can be freed after this call.
   * The data ptr is treated as a 3D array with increments indicating how to
   * iterate over the data.
   * Look at the documentation for ContinuousIncrements in svtkImageData for
   * details about how increments are specified.
   */
  bool Upload3D(int type, void* data, unsigned int dims[3], int comps, svtkIdType increments[3],
    int components, int* componentList);

  //@{
  /**
   * Get the type with which the data is loaded into the PBO mapped.
   * eg. SVTK_FLOAT for float32, SVTK_CHAR for byte, SVTK_UNSIGNED_CHAR for
   * unsigned byte etc.
   */
  svtkGetMacro(Type, int);
  svtkSetMacro(Type, int);
  //@}

  //@{
  /**
   * Get the number of components used to initialize the buffer.
   */
  svtkGetMacro(Components, int);
  svtkSetMacro(Components, int);
  //@}

  //@{
  /**
   * Get the size of the data loaded into the PBO mapped memory. Size is
   * in the number of elements of the uploaded Type.
   */
  svtkGetMacro(Size, unsigned int);
  svtkSetMacro(Size, unsigned int);
  void SetSize(unsigned int nTups, int nComps);
  //@}

  //@{
  /**
   * Get the openGL buffer handle.
   */
  svtkGetMacro(Handle, unsigned int);
  //@}

  //@{
  /**
   * Download data from pixel buffer to the 1D array. The length of the array
   * must be equal to the size of the data in the memory.
   */
  bool Download1D(int type, void* data, unsigned int dim, int numcomps, svtkIdType increment)
  {
    unsigned int newdims[3];
    newdims[0] = dim;
    newdims[1] = 1;
    newdims[2] = 1;
    svtkIdType newincrements[3];
    newincrements[0] = increment;
    newincrements[1] = 0;
    newincrements[2] = 0;
    return this->Download3D(type, data, newdims, numcomps, newincrements);
  }
  //@}

  //@{
  /**
   * Download data from pixel buffer to the 2D array. (lengthx * lengthy)
   * must be equal to the size of the data in the memory.
   */
  bool Download2D(int type, void* data, unsigned int dims[2], int numcomps, svtkIdType increments[2])
  {
    unsigned int newdims[3];
    newdims[0] = dims[0];
    newdims[1] = dims[1];
    newdims[2] = 1;
    svtkIdType newincrements[3];
    newincrements[0] = increments[0];
    newincrements[1] = increments[1];
    newincrements[2] = 0;
    return this->Download3D(type, data, newdims, numcomps, newincrements);
  }
  //@}

  /**
   * Download data from pixel buffer to the 3D array.
   * (lengthx * lengthy * lengthz) must be equal to the size of the data in
   * the memory.
   */
  bool Download3D(
    int type, void* data, unsigned int dims[3], int numcomps, svtkIdType increments[3]);

  /**
   * Convenience methods for binding.
   */
  void BindToPackedBuffer() { this->Bind(PACKED_BUFFER); }

  void BindToUnPackedBuffer() { this->Bind(UNPACKED_BUFFER); }

  /**
   * Deactivate the buffer.
   */
  void UnBind();

  /**
   * Convenience api for mapping buffers to app address space.
   * See also MapBuffer.
   */
  void* MapPackedBuffer() { return this->MapBuffer(PACKED_BUFFER); }

  void* MapPackedBuffer(int type, unsigned int numtuples, int comps)
  {
    return this->MapBuffer(type, numtuples, comps, PACKED_BUFFER);
  }

  void* MapPackedBuffer(unsigned int numbytes) { return this->MapBuffer(numbytes, PACKED_BUFFER); }

  void* MapUnpackedBuffer() { return this->MapBuffer(UNPACKED_BUFFER); }

  void* MapUnpackedBuffer(int type, unsigned int numtuples, int comps)
  {
    return this->MapBuffer(type, numtuples, comps, UNPACKED_BUFFER);
  }

  void* MapUnpackedBuffer(unsigned int numbytes)
  {
    return this->MapBuffer(numbytes, UNPACKED_BUFFER);
  }

  /**
   * Convenience api for unmapping buffers from app address space.
   * See also UnmapBuffer.
   */
  void UnmapUnpackedBuffer() { this->UnmapBuffer(UNPACKED_BUFFER); }

  void UnmapPackedBuffer() { this->UnmapBuffer(PACKED_BUFFER); }

  // PACKED_BUFFER for download APP<-PBO
  // UNPACKED_BUFFER for upload APP->PBO
  enum BufferType
  {
    UNPACKED_BUFFER = 0,
    PACKED_BUFFER
  };

  /**
   * Make the buffer active.
   */
  void Bind(BufferType buffer);

  //@{
  /**
   * Map the buffer to our addresspace. Returns a pointer to the mapped memory
   * for read/write access. If type, tuples and components are specified new buffer
   * data will be allocated, else the current allocation is mapped. When finished
   * call UnmapBuffer.
   */
  void* MapBuffer(int type, unsigned int numtuples, int comps, BufferType mode);
  void* MapBuffer(unsigned int numbytes, BufferType mode);
  void* MapBuffer(BufferType mode);
  //@}

  /**
   * Un-map the buffer from our address space, OpenGL can then use/reclaim the
   * buffer contents.
   */
  void UnmapBuffer(BufferType mode);

  /**
   * Allocate PACKED/UNPACKED memory to hold numTuples*numComponents of svtkType.
   */
  void Allocate(int svtkType, unsigned int numtuples, int comps, BufferType mode);

  /**
   * Allocate PACKED/UNPACKED memory to hold nBytes of data.
   */
  void Allocate(unsigned int nbytes, BufferType mode);

  /**
   * Release the memory allocated without destroying the PBO handle.
   */
  void ReleaseMemory();

  /**
   * Returns if the context supports the required extensions.
   * Extension will be loaded when the context is set.
   */
  static bool IsSupported(svtkRenderWindow* renWin);

protected:
  svtkPixelBufferObject();
  ~svtkPixelBufferObject() override;

  /**
   * Loads all required OpenGL extensions. Must be called every time a new
   * context is set.
   */
  bool LoadRequiredExtensions(svtkRenderWindow* renWin);

  /**
   * Create the pixel buffer object.
   */
  void CreateBuffer();

  /**
   * Destroys the pixel buffer object.
   */
  void DestroyBuffer();

  int Usage;
  unsigned int BufferTarget; // GLenum
  int Type;
  int Components;
  unsigned int Size;
  svtkWeakPointer<svtkRenderWindow> Context;
  unsigned int Handle;

private:
  svtkPixelBufferObject(const svtkPixelBufferObject&) = delete;
  void operator=(const svtkPixelBufferObject&) = delete;
};

#endif
