/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLPolyDataMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLPolyDataMapper2D
 * @brief   2D PolyData support for OpenGL
 *
 * svtkOpenGLPolyDataMapper2D provides 2D PolyData annotation support for
 * svtk under OpenGL.  Normally the user should use svtkPolyDataMapper2D
 * which in turn will use this class.
 *
 * @sa
 * svtkPolyDataMapper2D
 */

#ifndef svtkOpenGLPolyDataMapper2D_h
#define svtkOpenGLPolyDataMapper2D_h

#include "svtkNew.h"          // used for ivars
#include "svtkOpenGLHelper.h" // used for ivars
#include "svtkPolyDataMapper2D.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include <map>                         //for used data arrays & vbos
#include <string>                      // For API.
#include <vector>                      //for ivars

class svtkActor2D;
class svtkGenericOpenGLResourceFreeCallback;
class svtkMatrix4x4;
class svtkOpenGLBufferObject;
class svtkOpenGLCellToSVTKCellMap;
class svtkOpenGLHelper;
class svtkOpenGLVertexBufferObjectGroup;
class svtkPoints;
class svtkRenderer;
class svtkTextureObject;
class svtkTransform;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLPolyDataMapper2D : public svtkPolyDataMapper2D
{
public:
  svtkTypeMacro(svtkOpenGLPolyDataMapper2D, svtkPolyDataMapper2D);
  static svtkOpenGLPolyDataMapper2D* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Actually draw the poly data.
   */
  void RenderOverlay(svtkViewport* viewport, svtkActor2D* actor) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkOpenGLPolyDataMapper2D();
  ~svtkOpenGLPolyDataMapper2D() override;

  svtkGenericOpenGLResourceFreeCallback* ResourceCallback;

  /**
   * Does the shader source need to be recomputed
   */
  virtual bool GetNeedToRebuildShaders(svtkOpenGLHelper& cellBO, svtkViewport* ren, svtkActor2D* act);

  /**
   * Build the shader source code
   */
  virtual void BuildShaders(std::string& VertexCode, std::string& fragmentCode,
    std::string& geometryCode, svtkViewport* ren, svtkActor2D* act);

  /**
   * Determine what shader to use and compile/link it
   */
  virtual void UpdateShaders(svtkOpenGLHelper& cellBO, svtkViewport* viewport, svtkActor2D* act);

  /**
   * Set the shader parameteres related to the mapper/input data, called by UpdateShader
   */
  virtual void SetMapperShaderParameters(
    svtkOpenGLHelper& cellBO, svtkViewport* ren, svtkActor2D* act);

  /**
   * Set the shader parameteres related to the Camera
   */
  void SetCameraShaderParameters(svtkOpenGLHelper& cellBO, svtkViewport* viewport, svtkActor2D* act);

  /**
   * Set the shader parameteres related to the property
   */
  void SetPropertyShaderParameters(svtkOpenGLHelper& cellBO, svtkViewport* viewport, svtkActor2D* act);

  /**
   * Perform string replacements on the shader templates, called from
   * ReplaceShaderValues
   */
  virtual void ReplaceShaderPicking(std::string& fssource, svtkRenderer* ren, svtkActor2D* act);

  /**
   * Update the scene when necessary.
   */
  void UpdateVBO(svtkActor2D* act, svtkViewport* viewport);

  // The VBO and its layout.
  svtkOpenGLVertexBufferObjectGroup* VBOs;

  // Structures for the various cell types we render.
  svtkOpenGLHelper Points;
  svtkOpenGLHelper Lines;
  svtkOpenGLHelper Tris;
  svtkOpenGLHelper TriStrips;
  svtkOpenGLHelper* LastBoundBO;

  svtkTextureObject* CellScalarTexture;
  svtkOpenGLBufferObject* CellScalarBuffer;
  bool HaveCellScalars;
  int PrimitiveIDOffset;

  svtkTimeStamp VBOUpdateTime; // When was the VBO updated?
  svtkPoints* TransformedPoints;
  svtkNew<svtkTransform> VBOTransformInverse;
  svtkNew<svtkMatrix4x4> VBOShiftScale;

  int LastPickState;
  svtkTimeStamp PickStateChanged;

  // do we have wide lines that require special handling
  virtual bool HaveWideLines(svtkViewport*, svtkActor2D*);

  // stores the mapping from svtk cells to gl_PrimitiveId
  svtkNew<svtkOpenGLCellToSVTKCellMap> CellCellMap;

private:
  svtkOpenGLPolyDataMapper2D(const svtkOpenGLPolyDataMapper2D&) = delete;
  void operator=(const svtkOpenGLPolyDataMapper2D&) = delete;
};

#endif
