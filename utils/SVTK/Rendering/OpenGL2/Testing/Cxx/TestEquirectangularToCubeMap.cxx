/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestEquirectangularToCubeMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkEquirectangularToCubeMapTexture.h"
#include "svtkJPEGReader.h"
#include "svtkOpenGLTexture.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSkybox.h"

int TestEquirectangularToCubeMap(int argc, char* argv[])
{
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  svtkNew<svtkJPEGReader> reader;
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/autoshop.jpg");
  reader->SetFileName(fileName);
  delete[] fileName;

  svtkNew<svtkTexture> texture;
  texture->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkEquirectangularToCubeMapTexture> cubemap;
  cubemap->SetInputTexture(svtkOpenGLTexture::SafeDownCast(texture));

  svtkNew<svtkSkybox> world;
  world->SetTexture(cubemap);
  renderer->AddActor(world);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
