/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PrmMagnify.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  Copyright 2005 Sandia Corporation. Under the terms of Contract
  DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
  or on behalf of the U.S. Government. Redistribution and use in source and
  binary forms, with or without modification, are permitted provided that this
  Notice and any statement of authorship are reproduced on all copies.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDummyController.h"
#include "svtkIdFilter.h"
#include "svtkObjectFactory.h"
#include "svtkParallelRenderManager.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMandelbrotSource.h"
#include "svtkImageMapper3D.h"
#include "svtkImageShiftScale.h"
#include "svtkPointData.h"
#include "svtkUnsignedCharArray.h"

#include "svtkSmartPointer.h"

#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

//-----------------------------------------------------------------------------

class svtkTestMagnifyRenderManager : public svtkParallelRenderManager
{
public:
  svtkTypeMacro(svtkTestMagnifyRenderManager, svtkParallelRenderManager);
  static svtkTestMagnifyRenderManager* New();

protected:
  svtkTestMagnifyRenderManager();
  ~svtkTestMagnifyRenderManager() override;

  virtual void PreRenderProcessing() override;
  virtual void PostRenderProcessing() override;

  virtual void ReadReducedImage() override;

  svtkImageMandelbrotSource* Mandelbrot;

private:
  svtkTestMagnifyRenderManager(const svtkTestMagnifyRenderManager&) = delete;
  void operator=(const svtkTestMagnifyRenderManager&) = delete;
};

svtkStandardNewMacro(svtkTestMagnifyRenderManager);

svtkTestMagnifyRenderManager::svtkTestMagnifyRenderManager()
{
  this->Mandelbrot = svtkImageMandelbrotSource::New();
}

svtkTestMagnifyRenderManager::~svtkTestMagnifyRenderManager()
{
  this->Mandelbrot->Delete();
}

void svtkTestMagnifyRenderManager::PreRenderProcessing()
{
  this->RenderWindowImageUpToDate = 0;
  this->RenderWindow->SwapBuffersOff();
}

void svtkTestMagnifyRenderManager::PostRenderProcessing()
{
  this->FullImage->SetNumberOfComponents(4);
  this->FullImage->SetNumberOfTuples(this->FullImageSize[0] * this->FullImageSize[1]);

  int fullImageViewport[4], reducedImageViewport[4];

  // Read in image as RGBA.
  this->UseRGBA = 1;
  this->ReducedImageUpToDate = 0;
  this->ReadReducedImage();

  fullImageViewport[0] = 0;
  fullImageViewport[1] = 0;
  fullImageViewport[2] = this->FullImageSize[0] / 2;
  fullImageViewport[3] = this->FullImageSize[1] / 2;
  reducedImageViewport[0] = 0;
  reducedImageViewport[1] = 0;
  reducedImageViewport[2] = this->ReducedImageSize[0] / 2;
  reducedImageViewport[3] = this->ReducedImageSize[1] / 2;
  this->MagnifyImageNearest(this->FullImage, this->FullImageSize, this->ReducedImage,
    this->ReducedImageSize, fullImageViewport, reducedImageViewport);

  fullImageViewport[0] = this->FullImageSize[0] / 2;
  fullImageViewport[1] = 0;
  fullImageViewport[2] = this->FullImageSize[0];
  fullImageViewport[3] = this->FullImageSize[1] / 2;
  reducedImageViewport[0] = this->ReducedImageSize[0] / 2;
  reducedImageViewport[1] = 0;
  reducedImageViewport[2] = this->ReducedImageSize[0];
  reducedImageViewport[3] = this->ReducedImageSize[1] / 2;
  this->MagnifyImageLinear(this->FullImage, this->FullImageSize, this->ReducedImage,
    this->ReducedImageSize, fullImageViewport, reducedImageViewport);

  // Read in image as RGB.
  this->UseRGBA = 0;
  this->ReducedImageUpToDate = 0;
  this->ReadReducedImage();

  fullImageViewport[0] = 0;
  fullImageViewport[1] = this->FullImageSize[1] / 2;
  fullImageViewport[2] = this->FullImageSize[0] / 2;
  fullImageViewport[3] = this->FullImageSize[1];
  reducedImageViewport[0] = 0;
  reducedImageViewport[1] = this->ReducedImageSize[1] / 2;
  reducedImageViewport[2] = this->ReducedImageSize[0] / 2;
  reducedImageViewport[3] = this->ReducedImageSize[1];
  this->MagnifyImageNearest(this->FullImage, this->FullImageSize, this->ReducedImage,
    this->ReducedImageSize, fullImageViewport, reducedImageViewport);

  fullImageViewport[0] = this->FullImageSize[0] / 2;
  fullImageViewport[1] = this->FullImageSize[1] / 2;
  fullImageViewport[2] = this->FullImageSize[0];
  fullImageViewport[3] = this->FullImageSize[1];
  reducedImageViewport[0] = this->ReducedImageSize[0] / 2;
  reducedImageViewport[1] = this->ReducedImageSize[1] / 2;
  reducedImageViewport[2] = this->ReducedImageSize[0];
  reducedImageViewport[3] = this->ReducedImageSize[1];
  this->MagnifyImageLinear(this->FullImage, this->FullImageSize, this->ReducedImage,
    this->ReducedImageSize, fullImageViewport, reducedImageViewport);

  this->FullImageUpToDate = 1;

  this->WriteFullImage();

  this->RenderWindow->SwapBuffersOn();
  this->RenderWindow->Frame();
}

void svtkTestMagnifyRenderManager::ReadReducedImage()
{
  if (this->ReducedImageUpToDate)
    return;

  this->Mandelbrot->SetWholeExtent(
    0, this->ReducedImageSize[0] - 1, 0, this->ReducedImageSize[1] - 1, 0, 0);
  this->Mandelbrot->SetMaximumNumberOfIterations(255);
  this->Mandelbrot->Update();

  svtkIdType numpixels = this->ReducedImageSize[0] * this->ReducedImageSize[1];

  svtkDataArray* src = this->Mandelbrot->GetOutput()->GetPointData()->GetScalars();
  if (src->GetNumberOfTuples() != numpixels)
  {
    svtkErrorMacro("Image is wrong size!");
    return;
  }

  if (this->UseRGBA)
  {
    this->ReducedImage->SetNumberOfComponents(4);
  }
  else
  {
    this->ReducedImage->SetNumberOfComponents(3);
  }
  this->ReducedImage->SetNumberOfTuples(numpixels);

  double color[4];
  color[3] = 255;

  for (svtkIdType i = 0; i < numpixels; i++)
  {
    double value = src->GetComponent(i, 0);
    color[0] = value;
    color[1] = (value < 128) ? value : (255 - value);
    color[2] = 255 - value;
    this->ReducedImage->SetTuple(i, color);
  }
}

//-----------------------------------------------------------------------------

int PrmMagnify(int argc, char* argv[])
{
  SVTK_CREATE(svtkDummyController, controller);
  controller->Initialize(&argc, &argv);

  SVTK_CREATE(svtkTestMagnifyRenderManager, prm);
  prm->SetController(controller);

  //   SVTK_CREATE(svtkSphereSource, sphere);
  //   sphere->SetEndPhi(90.0);
  //   sphere->SetPhiResolution(4);

  //   SVTK_CREATE(svtkIdFilter, colors);
  //   colors->SetInputConnection(sphere->GetOutputPort());
  //   colors->PointIdsOff();
  //   colors->CellIdsOn();
  //   colors->FieldDataOff();
  //   colors->Update();

  //   SVTK_CREATE(svtkPolyDataMapper, mapper);
  //   mapper->SetInputConnection(colors->GetOutputPort());
  //   mapper->UseLookupTableScalarRangeOff();
  //   mapper->SetScalarRange(colors->GetOutput()->GetCellData()
  //                          ->GetScalars()->GetRange());

  //   SVTK_CREATE(svtkActor, actor);
  //   actor->SetMapper(mapper);

  SVTK_CREATE(svtkImageMandelbrotSource, mandelbrot);
  mandelbrot->SetWholeExtent(0, 73, 0, 73, 0, 0);
  mandelbrot->SetMaximumNumberOfIterations(255);

  SVTK_CREATE(svtkImageShiftScale, charImage);
  charImage->SetInputConnection(mandelbrot->GetOutputPort());
  charImage->SetShift(0);
  charImage->SetScale(1);
  charImage->SetOutputScalarTypeToUnsignedChar();

  SVTK_CREATE(svtkImageActor, actor);
  actor->GetMapper()->SetInputConnection(charImage->GetOutputPort());
  actor->InterpolateOff();

  svtkSmartPointer<svtkRenderer> renderer = prm->MakeRenderer();
  renderer->Delete(); // Remove duplicate reference.
  renderer->AddActor(actor);
  renderer->SetBackground(1, 0, 0);

  svtkSmartPointer<svtkRenderWindow> renwin = prm->MakeRenderWindow();
  renwin->Delete(); // Remove duplicate reference.
  renwin->SetSize(256, 256);
  renwin->AddRenderer(renderer);
  prm->SetRenderWindow(renwin);

  prm->ResetAllCameras();
  prm->SetImageReductionFactor(8);

  // Run the regression test.
  renwin->Render();
  int retVal = svtkRegressionTestImage(renwin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    SVTK_CREATE(svtkRenderWindowInteractor, iren);
    iren->SetRenderWindow(renwin);
    renwin->Render();
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  controller->Finalize();

  return !retVal;
}
