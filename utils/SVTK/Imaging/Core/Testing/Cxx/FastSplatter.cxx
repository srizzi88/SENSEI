/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// Simple test of svtkFastSplatter

#include "svtkFastSplatter.h"
#include "svtkImageData.h"
#include "svtkImageShiftScale.h"
#include "svtkImageViewer2.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include <cmath>

const int SPLAT_IMAGE_SIZE = 100;

int FastSplatter(int, char*[])
{
  // For the purposes of this example we'll build the splat image by
  // hand.

  SVTK_CREATE(svtkImageData, SplatImage);
  SplatImage->SetDimensions(SPLAT_IMAGE_SIZE, SPLAT_IMAGE_SIZE, 1);
  SplatImage->AllocateScalars(SVTK_FLOAT, 1);

  for (int i = 0; i < SPLAT_IMAGE_SIZE; ++i)
  {
    for (int j = 0; j < SPLAT_IMAGE_SIZE; ++j)
    {
      double xCoord = 1 - fabs((i - SPLAT_IMAGE_SIZE / 2) / (SPLAT_IMAGE_SIZE / 2.0));
      double yCoord = 1 - fabs((j - SPLAT_IMAGE_SIZE / 2) / (SPLAT_IMAGE_SIZE / 2.0));

      SplatImage->SetScalarComponentFromDouble(i, j, 0, 0, xCoord * yCoord);
    }
  }

  SVTK_CREATE(svtkPolyData, SplatPoints);
  SVTK_CREATE(svtkPoints, Points);

  Points->SetNumberOfPoints(5);
  double point[3];

  point[0] = 0;
  point[1] = 0;
  point[2] = 0;
  Points->SetPoint(0, point);

  point[0] = 1;
  point[1] = 1;
  point[2] = 0;
  Points->SetPoint(1, point);

  point[0] = -1;
  point[1] = 1;
  point[2] = 0;
  Points->SetPoint(2, point);

  point[0] = 1;
  point[1] = -1;
  point[2] = 0;
  Points->SetPoint(3, point);

  point[0] = -1;
  point[1] = -1;
  point[2] = 0;
  Points->SetPoint(4, point);

  SplatPoints->SetPoints(Points);

  SVTK_CREATE(svtkFastSplatter, splatter);
  splatter->SetInputData(SplatPoints);
  splatter->SetOutputDimensions(2 * SPLAT_IMAGE_SIZE, 2 * SPLAT_IMAGE_SIZE, 1);
  splatter->SetInputData(1, SplatImage);

  // The image viewers and writers are only happy with unsigned char
  // images.  This will convert the floats into that format.
  SVTK_CREATE(svtkImageShiftScale, resultScale);
  resultScale->SetOutputScalarTypeToUnsignedChar();
  resultScale->SetShift(0);
  resultScale->SetScale(255);
  resultScale->SetInputConnection(splatter->GetOutputPort());

  splatter->Update();
  resultScale->Update();

  // Set up a viewer for the image.  svtkImageViewer and
  // svtkImageViewer2 are convenient wrappers around svtkActor2D,
  // svtkImageMapper, svtkRenderer, and svtkRenderWindow.  All you need
  // to supply is the interactor and hooray, Bob's your uncle.
  SVTK_CREATE(svtkImageViewer2, ImageViewer);
  ImageViewer->SetInputConnection(resultScale->GetOutputPort());
  ImageViewer->SetColorLevel(127);
  ImageViewer->SetColorWindow(255);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  ImageViewer->SetupInteractor(iren);

  ImageViewer->Render();
  ImageViewer->GetRenderer()->ResetCamera();

  iren->Initialize();
  ImageViewer->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
