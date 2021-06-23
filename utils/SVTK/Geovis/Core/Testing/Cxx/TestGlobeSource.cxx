/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoView.cxx

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

#include "svtkActor.h"
#include "svtkDoubleArray.h"
#include "svtkGlobeSource.h"
#include "svtkJPEGReader.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"
#include "svtkTransform.h"

#include <svtksys/SystemTools.hxx>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New();

int TestGlobeSource(int argc, char* argv[])
{
  char* image = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/usa_image.jpg");

  svtkStdString imageFile = image;

  svtkSmartPointer<svtkJPEGReader> reader = svtkSmartPointer<svtkJPEGReader>::New();
  reader->SetFileName(imageFile.c_str());
  reader->Update();

  double latRange[] = { 24, 50 };
  double longRange[] = { -126, -66 };

  SVTK_CREATE(svtkGlobeSource, globeSource);
  globeSource->SetStartLatitude(latRange[0]);
  globeSource->SetEndLatitude(latRange[1]);
  globeSource->SetStartLongitude(longRange[0]);
  globeSource->SetEndLongitude(longRange[1]);

  globeSource->Update();

  SVTK_CREATE(svtkActor, actor);
  SVTK_CREATE(svtkPolyDataMapper, mapper);

  SVTK_CREATE(svtkDoubleArray, textureCoords);
  textureCoords->SetNumberOfComponents(2);

  svtkDoubleArray* array = svtkArrayDownCast<svtkDoubleArray>(
    globeSource->GetOutput(0)->GetPointData()->GetAbstractArray("LatLong"));

  double range[] = { (latRange[0] - latRange[1]), (longRange[0] - longRange[1]) };

  double val[2];
  double newVal[2];

  // Lower values of lat / long will correspond to
  // texture coordinate = 0 (for both s & t).
  for (int i = 0; i < array->GetNumberOfTuples(); ++i)
  {

    array->GetTypedTuple(i, val);

    // Get the texture coordinates in [0,1] range.
    newVal[1] = (latRange[0] - val[0]) / range[0];
    newVal[0] = (longRange[0] - val[1]) / range[1];

    textureCoords->InsertNextTuple(newVal);
  }

  globeSource->GetOutput(0)->GetPointData()->SetTCoords(textureCoords);
  mapper->SetInputConnection(globeSource->GetOutputPort());
  actor->SetMapper(mapper);

  SVTK_CREATE(svtkTexture, texture);
  texture->SetInputConnection(reader->GetOutputPort());
  actor->SetTexture(texture);

  // Get the right view.
  SVTK_CREATE(svtkTransform, transform);
  transform->RotateY(-90.0);
  transform->RotateX(-90.0);
  actor->SetUserMatrix(transform->GetMatrix());

  SVTK_CREATE(svtkRenderWindow, renWin);
  SVTK_CREATE(svtkRenderWindowInteractor, renWinInt);
  SVTK_CREATE(svtkRenderer, ren);

  ren->AddActor(actor);

  renWin->AddRenderer(ren);
  renWinInt->SetRenderWindow(renWin);

  renWin->SetSize(400, 400);
  renWin->Render();
  renWinInt->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renWinInt->Start();
  }

  delete[] image;
  return !retVal;
}
