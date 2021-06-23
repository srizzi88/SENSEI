/*=========================================================================

  Program:   Visualization Toolkit
  Module:    task4.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TaskParallelismWithPorts.h"

#include "svtkImageData.h"
#include "svtkOutputPort.h"
#include "svtkPolyData.h"

// Task 4 for TaskParallelism.
// See TaskParallelismWithPorts.cxx for more information.
void task4(double data)
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

  // Gradient vector.
  svtkImageGradient* grad = svtkImageGradient::New();
  grad->SetDimensionality(3);
  grad->SetInputConnection(source1->GetOutputPort());

  svtkImageShrink3D* mask = svtkImageShrink3D::New();
  mask->SetInputConnection(grad->GetOutputPort());
  mask->SetShrinkFactors(5, 5, 5);

  // Label the scalar field as the active vectors.
  svtkAssignAttribute* aa = svtkAssignAttribute::New();
  aa->SetInputConnection(mask->GetOutputPort());
  aa->Assign(
    svtkDataSetAttributes::SCALARS, svtkDataSetAttributes::VECTORS, svtkAssignAttribute::POINT_DATA);

  svtkGlyphSource2D* arrow = svtkGlyphSource2D::New();
  arrow->SetGlyphTypeToArrow();
  arrow->SetScale(0.2);
  arrow->FilledOff();

  // Glyph the gradient vector (with arrows)
  svtkGlyph3D* glyph = svtkGlyph3D::New();
  glyph->SetInputConnection(aa->GetOutputPort());
  glyph->SetSource(arrow->GetOutput());
  glyph->ScalingOff();
  glyph->OrientOn();
  glyph->SetVectorModeToUseVector();
  glyph->SetColorModeToColorByVector();

  // Output port
  svtkOutputPort* op = svtkOutputPort::New();
  op->SetInputConnection(glyph->GetOutputPort());
  op->SetTag(11);

  // Process requests
  op->WaitForUpdate();

  // Cleanup
  op->Delete();
  source1->Delete();
  grad->Delete();
  aa->Delete();
  mask->Delete();
  glyph->Delete();
  arrow->Delete();
}
