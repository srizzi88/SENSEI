/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKRenderWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QSVTKRenderWidget
 * @brief Generic QWidget for displaying a svtkRenderWindow in a Qt Application.
 *
 * QSVTKRenderWidget is intended as a generic widget for displaying SVTK rendering
 * results in a Qt application. It is only a wrapper around other specific
 * classes. For now, this only wraps the QSVTKOpenGLNativeWidget which is the
 * most versatile implementation. This may evolve in the future.
 * See QSVTKOpenGLNativeWidget for implementation details.
 *
 * For Quad-buffer stereo support, please use directly the QSVTKOpenGLStereoWidget.
 *
 * @note QSVTKRenderWidget requires Qt version 5.9 and above.
 * @sa QSVTKOpenGLStereoWidget QSVTKOpenGLNativeWidget
 */
#ifndef QSVTKRenderWidget_h
#define QSVTKRenderWidget_h

// For now, only wraps the QSVTKOpenGLNativeWidget
#include "QSVTKOpenGLNativeWidget.h"
using QSVTKRenderWidget = QSVTKOpenGLNativeWidget;

#endif
