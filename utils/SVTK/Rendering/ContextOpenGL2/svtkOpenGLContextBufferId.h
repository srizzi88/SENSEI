/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLContextBufferId.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLContextBufferId
 * @brief   2D array of ids stored in VRAM.
 *
 *
 * An 2D array where each element is the id of an entity drawn at the given
 * pixel.
 */

#ifndef svtkOpenGLContextBufferId_h
#define svtkOpenGLContextBufferId_h

#include "svtkAbstractContextBufferId.h"
#include "svtkRenderingContextOpenGL2Module.h" // For export macro

class svtkTextureObject;
class svtkOpenGLRenderWindow;

class SVTKRENDERINGCONTEXTOPENGL2_EXPORT svtkOpenGLContextBufferId : public svtkAbstractContextBufferId
{
public:
  svtkTypeMacro(svtkOpenGLContextBufferId, svtkAbstractContextBufferId);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Painter object.
   */
  static svtkOpenGLContextBufferId* New();

  /**
   * Release any graphics resources that are being consumed by this object.
   */
  void ReleaseGraphicsResources() override;

  //@{
  /**
   * Set/Get the OpenGL context owning the texture object resource.
   */
  void SetContext(svtkRenderWindow* context) override;
  svtkRenderWindow* GetContext() override;
  //@}

  /**
   * Returns if the context supports the required extensions.
   * \pre context_is_set: this->GetContext()!=0
   */
  bool IsSupported() override;

  /**
   * Allocate the memory for at least Width*Height elements.
   * \pre positive_width: GetWidth()>0
   * \pre positive_height: GetHeight()>0
   * \pre context_is_set: this->GetContext()!=0
   */
  void Allocate() override;

  /**
   * Tell if the buffer has been allocated.
   */
  bool IsAllocated() const override;

  /**
   * Copy the contents of the current read buffer to the internal texture
   * starting at lower left corner of the framebuffer (srcXmin,srcYmin).
   * \pre is_allocated: this->IsAllocated()
   */
  void SetValues(int srcXmin, int srcYmin) override;

  /**
   * Return item under abscissa x and ordinate y.
   * Abscissa go from left to right.
   * Ordinate go from bottom to top.
   * The return value is -1 if there is no item.
   * \pre is_allocated: IsAllocated()
   * \post valid_result: result>=-1
   */
  svtkIdType GetPickedItem(int x, int y) override;

protected:
  svtkOpenGLContextBufferId();
  ~svtkOpenGLContextBufferId() override;

  svtkOpenGLRenderWindow* Context;
  svtkTextureObject* Texture;

private:
  svtkOpenGLContextBufferId(const svtkOpenGLContextBufferId&) = delete;
  void operator=(const svtkOpenGLContextBufferId&) = delete;
};

#endif // #ifndef svtkOpenGLContextBufferId_h
