/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLProjectedTetrahedraMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   svtkOpenGLProjectedTetrahedraMapper
 * @brief   OpenGL implementation of PT
 *
 * @bug
 * This mapper relies highly on the implementation of the OpenGL pipeline.
 * A typical hardware driver has lots of options and some settings can
 * cause this mapper to produce artifacts.
 *
 */

#ifndef svtkOpenGLProjectedTetrahedraMapper_h
#define svtkOpenGLProjectedTetrahedraMapper_h

#include "svtkProjectedTetrahedraMapper.h"
#include "svtkRenderingVolumeOpenGL2Module.h" // For export macro

#include "svtkOpenGLHelper.h" // used for ivars

class svtkVisibilitySort;
class svtkUnsignedCharArray;
class svtkFloatArray;
class svtkRenderWindow;
class svtkOpenGLFramebufferObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLVertexBufferObject;

class SVTKRENDERINGVOLUMEOPENGL2_EXPORT svtkOpenGLProjectedTetrahedraMapper
  : public svtkProjectedTetrahedraMapper
{
public:
  svtkTypeMacro(svtkOpenGLProjectedTetrahedraMapper, svtkProjectedTetrahedraMapper);
  static svtkOpenGLProjectedTetrahedraMapper* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void ReleaseGraphicsResources(svtkWindow* window) override;

  void Render(svtkRenderer* renderer, svtkVolume* volume) override;

  //@{
  /**
   * Set/get whether to use floating-point rendering buffers rather
   * than the default.
   */
  svtkSetMacro(UseFloatingPointFrameBuffer, bool);
  svtkGetMacro(UseFloatingPointFrameBuffer, bool);
  svtkBooleanMacro(UseFloatingPointFrameBuffer, bool);
  //@}

  /**
   * Return true if the rendering context provides
   * the nececessary functionality to use this class.
   */
  bool IsSupported(svtkRenderWindow* context) override;

protected:
  svtkOpenGLProjectedTetrahedraMapper();
  ~svtkOpenGLProjectedTetrahedraMapper() override;

  void Initialize(svtkRenderer* ren);
  bool Initialized;
  int CurrentFBOWidth, CurrentFBOHeight;
  bool AllocateFOResources(svtkRenderer* ren);
  bool CanDoFloatingPointFrameBuffer;
  bool FloatingPointFrameBufferResourcesAllocated;
  bool UseFloatingPointFrameBuffer;
  bool HasHardwareSupport;

  svtkUnsignedCharArray* Colors;
  int UsingCellColors;

  svtkFloatArray* TransformedPoints;

  float MaxCellSize;
  svtkTimeStamp InputAnalyzedTime;
  svtkTimeStamp ColorsMappedTime;

  // The VBO and its layout.
  svtkOpenGLVertexBufferObject* VBO;

  // Structures for the various cell types we render.
  svtkOpenGLHelper Tris;

  int GaveError;

  svtkVolumeProperty* LastProperty;

  svtkOpenGLFramebufferObject* Framebuffer;

  float* SqrtTable;
  float SqrtTableBias;

  virtual void ProjectTetrahedra(
    svtkRenderer* renderer, svtkVolume* volume, svtkOpenGLRenderWindow* renWin);

  float GetCorrectedDepth(float x, float y, float z1, float z2,
    const float inverse_projection_mat[16], int use_linear_depth_correction,
    float linear_depth_correction);

  /**
   * Update progress ensuring that OpenGL state is saved and restored before
   * invoking progress.
   */
  void GLSafeUpdateProgress(double value, svtkOpenGLRenderWindow* context);

private:
  svtkOpenGLProjectedTetrahedraMapper(const svtkOpenGLProjectedTetrahedraMapper&) = delete;
  void operator=(const svtkOpenGLProjectedTetrahedraMapper&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
};

#endif
