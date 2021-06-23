/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyDataTangents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the svtkPolyDataTangents filter

#include "svtkActor.h"
#include "svtkActorCollection.h"
#include "svtkArrowSource.h"
#include "svtkCamera.h"
#include "svtkDataSetAttributes.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkGlyph3DMapper.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkJPEGReader.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkPolyDataTangents.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"
#include "svtkTextureMapToCylinder.h"
#include "svtkTriangleFilter.h"
#include "svtkXMLPolyDataReader.h"

//----------------------------------------------------------------------------
int TestPolyDataTangents(int argc, char* argv[])
{
  svtkNew<svtkXMLPolyDataReader> reader;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
  reader->SetFileName(fname);
  delete[] fname;

  svtkNew<svtkPolyDataNormals> normals;
  normals->SetInputConnection(reader->GetOutputPort());
  normals->SplittingOff();

  svtkNew<svtkTriangleFilter> triangle;
  triangle->SetInputConnection(normals->GetOutputPort());

  svtkNew<svtkTextureMapToCylinder> textureMap;
  textureMap->SetInputConnection(triangle->GetOutputPort());

  svtkNew<svtkPolyDataTangents> tangents;
  tangents->SetInputConnection(textureMap->GetOutputPort());

  svtkNew<svtkArrowSource> arrow;
  arrow->SetTipResolution(20);
  arrow->SetShaftResolution(20);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(tangents->GetOutputPort());

  svtkNew<svtkGlyph3DMapper> tgtsMapper;
  tgtsMapper->SetInputConnection(tangents->GetOutputPort());
  tgtsMapper->SetOrientationArray(svtkDataSetAttributes::TANGENTS);
  tgtsMapper->SetSourceConnection(arrow->GetOutputPort());
  tgtsMapper->SetScaleFactor(0.5);

  svtkNew<svtkJPEGReader> image;
  char* texname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tex_debug.jpg");
  image->SetFileName(texname);
  delete[] texname;

  svtkNew<svtkTexture> texture;
  texture->SetInputConnection(image->GetOutputPort());

  svtkNew<svtkRenderer> renderer;

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->SetTexture(texture);

  svtkNew<svtkActor> actorTangents;
  actorTangents->SetMapper(tgtsMapper);
  actorTangents->GetProperty()->SetColor(1.0, 0.0, 0.0);

  renderer->AddActor(actor);
  renderer->AddActor(actorTangents);

  renWin->Render();

  renderer->GetActiveCamera()->Zoom(3.0);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
