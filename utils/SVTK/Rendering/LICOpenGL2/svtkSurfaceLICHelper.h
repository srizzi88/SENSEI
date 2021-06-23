/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSurfaceLICHelper
 *
 * A small collection of noise routines for LIC
 */

#ifndef svtkSurfaceLICHelper_h
#define svtkSurfaceLICHelper_h
#ifndef __SVTK_WRAP__

#include "svtkOpenGLHelper.h"
#include "svtkPixelExtent.h"
#include "svtkRenderingLICOpenGL2Module.h" // for export
#include "svtkSmartPointer.h"
#include "svtkTextureObject.h"
#include "svtkWeakPointer.h"
#include "svtk_glew.h"

#include <deque> // for methods

class svtkOpenGLFramebufferObject;
class svtkOpenGLRenderWindow;
class svtkPainterCommunicator;
class svtkImageData;
class svtkSurfaceLICComposite;
class svtkLineIntegralConvolution2D;
class svtkRenderer;
class svtkActor;
class svtkDataObject;

class svtkSurfaceLICHelper
{
public:
  svtkSurfaceLICHelper();
  ~svtkSurfaceLICHelper();

  /**
   * Check for OpenGL support
   */
  static bool IsSupported(svtkOpenGLRenderWindow* context);

  /**
   * Free textures and shader programs we're holding a reference to.
   */
  void ReleaseGraphicsResources(svtkWindow* win);

  /**
   * Free textures we're holding a reference to.
   */
  void ClearTextures();

  /**
   * Allocate textures.
   */
  void AllocateTextures(svtkOpenGLRenderWindow* context, int* viewsize);

  /**
   * Allocate a size texture, store in the given smart pointer.
   */
  void AllocateTexture(svtkOpenGLRenderWindow* context, int* viewsize,
    svtkSmartPointer<svtkTextureObject>& tex, int filter = svtkTextureObject::Nearest);

  /**
   * Allocate a size texture, store in the given smart pointer.
   */
  void AllocateDepthTexture(
    svtkOpenGLRenderWindow* context, int* viewsize, svtkSmartPointer<svtkTextureObject>& tex);

  /**
   * After LIC has been computed reset/clean internal state
   */
  void Updated();

  /**
   * Force all stages to re-execute. Necessary if the
   * context or communicator changes.
   */
  void UpdateAll();

  //@{
  /**
   * Convert viewport to texture coordinates
   */
  void ViewportQuadTextureCoords(GLfloat* tcoords)
  {
    tcoords[0] = tcoords[2] = 0.0f;
    tcoords[1] = tcoords[3] = 1.0f;
  }
  //@}

  /**
   * Convert a viewport to a bounding box and it's texture coordinates for a
   * screen size texture.
   */
  void ViewportQuadPoints(const svtkPixelExtent& viewportExt, GLfloat* quadpts)
  {
    viewportExt.GetData(quadpts);
  }

  /**
   * Convert a viewport to a bounding box and it's texture coordinates for a
   * screen size texture.
   */
  void ViewportQuadTextureCoords(
    const svtkPixelExtent& viewExt, const svtkPixelExtent& viewportExt, GLfloat* tcoords);

  //@{
  /**
   * Convert the entire view to a bounding box and it's texture coordinates for
   * a screen size texture.
   */
  void ViewQuadPoints(GLfloat* quadpts)
  {
    quadpts[0] = quadpts[2] = 0.0f;
    quadpts[1] = quadpts[3] = 1.0f;
  }
  //@}

  //@{
  /**
   * Convert the entire view to a bounding box and it's texture coordinates for
   * a screen size texture.
   */
  void ViewQuadTextureCoords(GLfloat* tcoords)
  {
    tcoords[0] = tcoords[2] = 0.0f;
    tcoords[1] = tcoords[3] = 1.0f;
  }
  //@}

  /**
   * Render a quad (to trigger a shader to run)
   */
  void RenderQuad(
    const svtkPixelExtent& viewExt, const svtkPixelExtent& viewportExt, svtkOpenGLHelper* cbo);

  /**
   * Compute the index into the 4x4 OpenGL ordered matrix.
   */
  inline int idx(int row, int col) { return 4 * col + row; }

  /**
   * given a axes aligned bounding box in
   * normalized device coordinates test for
   * view frustum visibility.
   * if all points are outside one of the
   * view frustum planes then this box
   * is not visible. we might have false
   * positive where more than one clip
   * plane intersects the box.
   */
  bool VisibilityTest(double ndcBBox[24]);

  /**
   * Given world space bounds,
   * compute bounding boxes in clip and normalized device
   * coordinates and perform view frustum visibility test.
   * return true if the bounds are visible. If so the passed
   * in extent object is initialized with the corresponding
   * screen space extents.
   */
  bool ProjectBounds(double PMV[16], int viewsize[2], double bounds[6], svtkPixelExtent& screenExt);

  /**
   * Compute screen space extents for each block in the input
   * dataset and for the entire dataset. Only visible blocks
   * are used in the computations.
   */
  int ProjectBounds(svtkRenderer* ren, svtkActor* actor, svtkDataObject* dobj, int viewsize[2],
    svtkPixelExtent& dataExt, std::deque<svtkPixelExtent>& blockExts);

  /**
   * Shrink an extent to tightly bound non-zero values
   */
  void GetPixelBounds(float* rgba, int ni, svtkPixelExtent& ext);

  /**
   * Shrink a set of extents to tightly bound non-zero values
   * cull extent if it's empty
   */
  void GetPixelBounds(float* rgba, int ni, std::deque<svtkPixelExtent>& blockExts);

  static void StreamingFindMinMax(
    svtkOpenGLFramebufferObject* fbo, std::deque<svtkPixelExtent>& blockExts, float& min, float& max);

  svtkSmartPointer<svtkImageData> Noise;
  svtkSmartPointer<svtkTextureObject> NoiseImage;
  svtkSmartPointer<svtkTextureObject> DepthImage;
  svtkSmartPointer<svtkTextureObject> GeometryImage;
  svtkSmartPointer<svtkTextureObject> VectorImage;
  svtkSmartPointer<svtkTextureObject> CompositeVectorImage;
  svtkSmartPointer<svtkTextureObject> MaskVectorImage;
  svtkSmartPointer<svtkTextureObject> CompositeMaskVectorImage;
  svtkSmartPointer<svtkTextureObject> LICImage;
  svtkSmartPointer<svtkTextureObject> RGBColorImage;
  svtkSmartPointer<svtkTextureObject> HSLColorImage;

  bool HasVectors;
  std::deque<svtkPixelExtent> BlockExts;

  svtkOpenGLHelper* ColorEnhancePass;
  svtkOpenGLHelper* CopyPass;
  svtkOpenGLHelper* ColorPass;

  int Viewsize[2];
  svtkSmartPointer<svtkSurfaceLICComposite> Compositor;
  svtkSmartPointer<svtkOpenGLFramebufferObject> FBO;

  svtkSmartPointer<svtkLineIntegralConvolution2D> LICer;
  svtkPainterCommunicator* Communicator;
  svtkPixelExtent DataSetExt;

  svtkWeakPointer<svtkOpenGLRenderWindow> Context;

  bool ContextNeedsUpdate;
  bool CommunicatorNeedsUpdate;

protected:
};

#endif
#endif
// SVTK-HeaderTest-Exclude: svtkSurfaceLICHelper.h
