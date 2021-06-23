/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKWin32Header.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#ifndef QSVTKWin32Header_h
#define QSVTKWin32Header_h

#include "svtkABI.h"
#include "svtkSystemIncludes.h"

#if defined(SVTK_BUILD_SHARED_LIBS)
#if defined(QSVTK_EXPORTS)
#define QSVTK_EXPORT SVTK_ABI_EXPORT
#else
#define QSVTK_EXPORT SVTK_ABI_IMPORT
#endif
#else
#define QSVTK_EXPORT
#endif

#endif /*QSVTKWin32Header_h*/
