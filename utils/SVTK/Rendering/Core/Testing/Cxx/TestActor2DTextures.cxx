
/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include <svtkIconGlyphFilter.h>

#include <svtkDoubleArray.h>
#include <svtkImageData.h>
#include <svtkPNGReader.h>
#include <svtkPointData.h>
#include <svtkPointSet.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper2D.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkTexture.h>
#include <svtkTexturedActor2D.h>

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

int TestActor2DTextures(int argc, char* argv[])
{
  // svtkRegressionTester::Result result = svtkRegressionTester::Passed;
  // svtkRegressionTester *test = new svtkRegressionTester("IconGlyphFilter");

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Tango/TangoIcons.png");

  int imageDims[3];

  svtkPNGReader* imageReader = svtkPNGReader::New();

  imageReader->SetFileName(fname);
  delete[] fname;
  imageReader->Update();

  imageReader->GetOutput()->GetDimensions(imageDims);

  svtkPolyData* pointSet = svtkPolyData::New();
  svtkPoints* points = svtkPoints::New();
  svtkDoubleArray* pointData = svtkDoubleArray::New();
  pointData->SetNumberOfComponents(3);
  points->SetData(pointData);
  pointSet->SetPoints(points);

  svtkIntArray* iconIndex = svtkIntArray::New();
  iconIndex->SetNumberOfComponents(1);

  pointSet->GetPointData()->SetScalars(iconIndex);

  for (double i = 1.0; i < 8; i++)
  {
    for (double j = 1.0; j < 8; j++)
    {
      points->InsertNextPoint(i * 26.0, j * 26.0, 0.0);
    }
  }

  for (int i = 0; i < points->GetNumberOfPoints(); i++)
  {
    iconIndex->InsertNextTuple1(i);
  }

  int size[] = { 24, 24 };

  svtkIconGlyphFilter* iconFilter = svtkIconGlyphFilter::New();

  iconFilter->SetInputData(pointSet);
  iconFilter->SetIconSize(size);
  iconFilter->SetUseIconSize(true);
  iconFilter->SetIconSheetSize(imageDims);

  svtkPolyDataMapper2D* mapper = svtkPolyDataMapper2D::New();
  mapper->SetInputConnection(iconFilter->GetOutputPort());

  svtkTexturedActor2D* iconActor = svtkTexturedActor2D::New();
  iconActor->SetMapper(mapper);

  svtkTexture* texture = svtkTexture::New();
  texture->SetInputConnection(imageReader->GetOutputPort());
  iconActor->SetTexture(texture);

  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetSize(208, 208);
  renWin->AddRenderer(renderer);

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  renderer->AddActor(iconActor);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  mapper->Delete();
  texture->Delete();
  imageReader->Delete();
  iconIndex->Delete();
  iconFilter->Delete();
  iconActor->Delete();
  pointSet->Delete();
  points->Delete();
  pointData->Delete();

  return !retVal;
}
