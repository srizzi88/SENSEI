/*=========================================================================

 Program:   Visualization Toolkit
 Module:    TestSprites.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

// .SECTION Thanks
// <verbatim>
//
// This file is based loosely on the PointSprites plugin developed
// and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkDataObject.h"
#include "svtkDataSetAttributes.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPointGaussianMapper.h"
#include "svtkPointSource.h"
#include "svtkProperty.h"
#include "svtkRandomAttributeGenerator.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTimerLog.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

int TestPointGaussianMapperOpacity(int argc, char* argv[])
{
  int desiredPoints = 1.0e4;

  svtkNew<svtkPointSource> points;
  points->SetNumberOfPoints(desiredPoints);
  points->SetRadius(pow(desiredPoints, 0.33) * 10.0);
  points->Update();

  svtkNew<svtkRandomAttributeGenerator> randomAttr;
  randomAttr->SetInputConnection(points->GetOutputPort());

  svtkNew<svtkPointGaussianMapper> mapper;

  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  randomAttr->SetDataTypeToFloat();
  randomAttr->GeneratePointScalarsOn();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->GeneratePointArrayOn();
  randomAttr->Update();
  svtkDataSet* output = static_cast<svtkDataSet*>(randomAttr->GetOutput());
  output->GetPointData()->SetScalars(output->GetPointData()->GetArray("RandomPointArray"));

  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetInterpolateScalarsBeforeMapping(0);
  mapper->SetScaleArray("RandomPointScalars");
  mapper->SetScaleArrayComponent(1); // tests clamping to zero-th component
  mapper->SetOpacityArray("RandomPointArray");
  mapper->SetOpacityArrayComponent(0);
  mapper->EmissiveOff();

  // show other shader examples
  // the fragment that is rendered is that of a triangle
  // large enough to encompass a circle of radius 3
  mapper->SetSplatShaderCode(
    // this first line keeps the default color opacity calcs
    // which you can then modify with additional code below
    "//SVTK::Color::Impl\n"

    // example of a circle with black edges
    // "  float dist = sqrt(dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy));\n"
    // "  if (dist > 1.1) { discard; }\n"
    // "  if (dist < 0.5) { discard; }\n"
    // // apply a black edge around the circle
    // "  if (dist > 1.0 || dist < 0.6) { diffuseColor = vec3(0,0,0); ambientColor = vec3(0,0,0);
    // }\n"

    // example for a square
    "  if (abs(offsetVCVSOutput.x) > 1.0 || abs(offsetVCVSOutput.y) > 1.0) { discard; }\n"
    "  if (abs(offsetVCVSOutput.x) < 0.6 && abs(offsetVCVSOutput.y) < 0.6) { discard; }\n");

  // since this shader only uses a radus of sqrt(2) we will adjust the mapper
  // to render a smaller area than the default radius of 3.0
  mapper->SetTriangleScale(1.5);

  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddHSVPoint(0.0, 0.1, 0.7, 1.0);
  ctf->AddHSVPoint(1.0, 0.9, 0.7, 1.0);
  ctf->SetColorSpaceToHSV();
  ctf->HSVWrapOff();
  mapper->SetLookupTable(ctf);

  svtkNew<svtkPiecewiseFunction> otf;
  otf->AddPoint(0.0, 0.3);
  otf->AddPoint(1.0, 1.0);
  mapper->SetScalarOpacityFunction(otf);

  svtkNew<svtkTimerLog> timer;
  timer->StartTimer();
  renderWindow->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  timer->StartTimer();
  int numRenders = 85;
  for (int i = 0; i < numRenders; ++i)
  {
    renderer->GetActiveCamera()->Azimuth(1);
    renderer->GetActiveCamera()->Elevation(1);
    renderWindow->Render();
  }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();

  int numPts = mapper->GetInput()->GetPoints()->GetNumberOfPoints();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  cerr << "number of points: " << numPts << endl;
  cerr << "points per second: " << numPts * (numRenders / elapsed) << endl;

  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  //  renderer->GetActiveCamera()->Print(cerr);

  renderer->GetActiveCamera()->Zoom(10.0);
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
