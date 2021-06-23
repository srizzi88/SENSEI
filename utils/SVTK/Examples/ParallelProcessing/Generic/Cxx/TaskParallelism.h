/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TaskParallelism.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __TASKPARA_H
#define __TASKPARA_H

#include "svtkActor.h"
#include "svtkAssignAttribute.h"
#include "svtkAttributeDataToFieldDataFilter.h"
#include "svtkCamera.h"
#include "svtkContourFilter.h"
#include "svtkDataSetMapper.h"
#include "svtkFieldDataToAttributeDataFilter.h"
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkImageGaussianSmooth.h"
#include "svtkImageGradient.h"
#include "svtkImageGradientMagnitude.h"
#include "svtkImageShrink3D.h"
#include "svtkMPIController.h"
#include "svtkProbeFilter.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

typedef svtkPolyDataMapper* (*taskFunction)(svtkRenderWindow* renWin, double data, svtkCamera* cam);

svtkPolyDataMapper* task1(svtkRenderWindow* renWin, double data, svtkCamera* cam);
svtkPolyDataMapper* task2(svtkRenderWindow* renWin, double data, svtkCamera* cam);

static const double EXTENT = 20;

static const int WINDOW_WIDTH = 400;
static const int WINDOW_HEIGHT = 300;

#endif
