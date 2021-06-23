/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCornerAnnotation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMandelbrotSource.h"
#include "svtkImageMapToWindowLevelColors.h"
#include "svtkImageMapper3D.h"
#include "svtkImageShiftScale.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"

#include "svtkActor.h"
#include "svtkCornerAnnotation.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"

int TestCornerAnnotation(int argc, char* argv[])
{
  svtkSmartPointer<svtkImageMandelbrotSource> imageSource =
    svtkSmartPointer<svtkImageMandelbrotSource>::New();
  svtkSmartPointer<svtkImageShiftScale> imageCast = svtkSmartPointer<svtkImageShiftScale>::New();
  imageCast->SetInputConnection(imageSource->GetOutputPort());
  imageCast->SetScale(100);
  imageCast->SetShift(0);
  imageCast->SetOutputScalarTypeToShort();
  imageCast->Update();

  svtkSmartPointer<svtkImageMapToWindowLevelColors> imageWL =
    svtkSmartPointer<svtkImageMapToWindowLevelColors>::New();
  imageWL->SetInputConnection(imageCast->GetOutputPort());
  imageWL->SetWindow(10000);
  imageWL->SetLevel(5000);
  svtkSmartPointer<svtkImageActor> imageActor = svtkSmartPointer<svtkImageActor>::New();
  imageActor->GetMapper()->SetInputConnection(imageWL->GetOutputPort());

  // Visualize
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  renderWindow->SetSize(800, 600);
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderer->AddActor(imageActor);

  // Annotate the image with window/level and mouse over pixel information
  svtkSmartPointer<svtkCornerAnnotation> cornerAnnotation =
    svtkSmartPointer<svtkCornerAnnotation>::New();

  cornerAnnotation->SetImageActor(imageActor);
  cornerAnnotation->SetWindowLevel(imageWL);

  cornerAnnotation->SetLinearFontScaleFactor(2);
  cornerAnnotation->SetNonlinearFontScaleFactor(1);
  cornerAnnotation->SetMaximumFontSize(20);

  cornerAnnotation->SetText(svtkCornerAnnotation::LowerLeft, "LL (<image>)");
  cornerAnnotation->SetText(svtkCornerAnnotation::LowerRight, "LR (<image_and_max>)");
  cornerAnnotation->SetText(svtkCornerAnnotation::UpperLeft, "UL (<slice>)");
  cornerAnnotation->SetText(svtkCornerAnnotation::UpperRight, "UR (<slice_and_max>)");

  cornerAnnotation->SetText(svtkCornerAnnotation::UpperEdge, "T (<window_level>)");
  cornerAnnotation->SetText(svtkCornerAnnotation::LowerEdge, "B (<slice_pos>)");
  cornerAnnotation->SetText(svtkCornerAnnotation::LeftEdge, "L (<window>)");
  cornerAnnotation->SetText(svtkCornerAnnotation::RightEdge, "R (<level>)");

  cornerAnnotation->GetTextProperty()->SetColor(1, 0, 0);

  renderer->AddViewProp(cornerAnnotation);

  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
