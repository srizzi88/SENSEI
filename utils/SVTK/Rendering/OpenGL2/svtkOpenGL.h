/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGL.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkOpenGL_h
#define svtkOpenGL_h

#include "svtkConfigure.h"
#include "svtkRenderingOpenGLConfigure.h" // For SVTK_USE_COCOA

// Must be included before `gl.h` due to glew.
#include "svtkOpenGLError.h"

// To prevent gl.h to include glext.h provided by the system
#define GL_GLEXT_LEGACY
#if defined(__APPLE__) && defined(SVTK_USE_COCOA)
#include <OpenGL/gl.h> // Include OpenGL API.
#else
#include "svtkWindows.h" // Needed to include OpenGL header on Windows.
#include <GL/gl.h>      // Include OpenGL API.
#endif

#endif
// SVTK-HeaderTest-Exclude: svtkOpenGL.h
