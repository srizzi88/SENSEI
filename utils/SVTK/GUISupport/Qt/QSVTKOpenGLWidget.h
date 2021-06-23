/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKOpenGLWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QSVTKOpenGLWidget
 * @brief Wrapper for QSVTKOpenGLStereoWidget for legacy support purpose
 *
 * QSVTKOpenGLWidget is only a wrapper for QSVTKOpenGLStereoWidget
 * so old code can still be built.
 * Please use QSVTKOpenGLStereoWidget if Quad-buffer stereo
 * rendering capabilities or needed or QSVTKOpenGLNativeWidget
 * for a more versatile implementation.
 *
 * @sa QSVTKOpenGLStereoWidget QSVTKOpenGLNativeWidget QSVTKRenderWidget
 */
#ifndef QSVTKOpenGLWidget_h
#define QSVTKOpenGLWidget_h

#include "QSVTKOpenGLStereoWidget.h"

#ifndef SVTK_LEGACY_REMOVE
typedef QSVTKOpenGLStereoWidget SVTK_LEGACY(QSVTKOpenGLWidget);
#endif

#endif
