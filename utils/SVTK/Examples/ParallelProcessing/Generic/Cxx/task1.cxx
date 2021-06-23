/*=========================================================================

  Program:   Visualization Toolkit
  Module:    task1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TaskParallelism.h"

#include "svtkImageData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"

// Task 1 for TaskParallelism.
// See TaskParallelism.cxx for more information.
svtkPolyDataMapper* task1(svtkRenderWindow* renWin, double data, svtkCamera* cam)
{
  double extent = data;
  int iextent = static_cast<int>(data);
  // The pipeline

  // Synthetic image source.
  svtkRTAnalyticSource* source1 = svtkRTAnalyticSource::New();
  source1->SetWholeExtent(-1 * iextent, iextent, -1 * iextent, iextent, -1 * iextent, iextent);
  source1->SetCenter(0, 0, 0);
  source1->SetStandardDeviation(0.5);
  source1->SetMaximum(255.0);
  source1->SetXFreq(60);
  source1->SetXMag(10);
  source1->SetYFreq(30);
  source1->SetYMag(18);
  source1->SetZFreq(40);
  source1->SetZMag(5);
  source1->GetOutput()->SetSpacing(2.0 / extent, 2.0 / extent, 2.0 / extent);

  // Iso-surfacing.
  svtkContourFilter* contour = svtkContourFilter::New();
  contour->SetInputConnection(source1->GetOutputPort());
  contour->SetNumberOfContours(1);
  contour->SetValue(0, 220);

  // Magnitude of the gradient vector.
  svtkImageGradientMagnitude* magn = svtkImageGradientMagnitude::New();
  magn->SetDimensionality(3);
  magn->SetInputConnection(source1->GetOutputPort());

  // Probe magnitude with iso-surface.
  svtkProbeFilter* probe = svtkProbeFilter::New();
  probe->SetInputConnection(contour->GetOutputPort());
  probe->SetSourceConnection(magn->GetOutputPort());
  probe->SpatialMatchOn();

  // Rendering objects.
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputData(probe->GetPolyDataOutput());
  mapper->SetScalarRange(50, 180);

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

  svtkRenderer* ren = svtkRenderer::New();
  renWin->AddRenderer(ren);

  ren->AddActor(actor);
  ren->SetActiveCamera(cam);

  // Cleanup
  source1->Delete();
  contour->Delete();
  magn->Delete();
  probe->Delete();
  actor->Delete();
  ren->Delete();

  return mapper;
}
