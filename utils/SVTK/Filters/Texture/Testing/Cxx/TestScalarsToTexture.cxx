/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarsToTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkColorTransferFunction.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkNew.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarsToTextureFilter.h"
#include "svtkTestUtilities.h"
#include "svtkXMLPolyDataReader.h"

//----------------------------------------------------------------------------
int TestScalarsToTexture(int argc, char* argv[])
{
  svtkNew<svtkXMLPolyDataReader> reader;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can_slice.vtp");
  reader->SetFileName(fname);
  delete[] fname;

  svtkNew<svtkColorTransferFunction> stc;
  stc->SetVectorModeToMagnitude();
  stc->SetColorSpaceToDiverging();
  stc->AddRGBPoint(0.0, 59. / 255., 76. / 255., 192. / 255.);
  stc->AddRGBPoint(7.0e6, 221. / 255., 221. / 255., 221. / 255.);
  stc->AddRGBPoint(1.4e7, 180. / 255., 4. / 255., 38. / 255.);
  stc->Build();

  svtkNew<svtkScalarsToTextureFilter> stt;
  stt->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "ACCL");
  stt->SetTextureDimensions(256, 256);
  stt->SetTransferFunction(stc);
  stt->UseTransferFunctionOn();
  stt->SetInputConnection(reader->GetOutputPort());

  // render texture
  svtkNew<svtkImageActor> actor;
  actor->GetMapper()->SetInputConnection(stt->GetOutputPort(1));

  // Standard rendering classes
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  renderer->AddActor(actor);
  renderer->ResetCamera();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
