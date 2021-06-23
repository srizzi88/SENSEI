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
#include "svtkDataObject.h"
#include "svtkDataSetAttributes.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkPointGaussianMapper.h"
#include "svtkProperty.h"
#include "svtkRandomAttributeGenerator.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTimerLog.h"

#include "svtkColorTransferFunction.h"
#include "svtkPointSource.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkPolyDataReader.h"

//#define TestPoints
//#define TestFile
#define TestSplats

int TestPointGaussianMapper(int argc, char* argv[])
{
  int desiredPoints = 1.0e4;

  svtkNew<svtkPointSource> points;
  points->SetNumberOfPoints(desiredPoints);
  points->SetRadius(pow(desiredPoints, 0.33) * 20.0);
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

#ifdef TestPoints
  randomAttr->SetDataTypeToUnsignedChar();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->SetMinimumComponentValue(0);
  randomAttr->SetMaximumComponentValue(255);
  randomAttr->Update();
  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetScaleFactor(0.0);
  mapper->EmissiveOff();
#endif

#ifdef TestFile
  svtkNew<svtkPolyDataReader> reader;
  reader->SetFileName("filename");
  reader->Update();

  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SelectColorArray("Color");
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetScalefactor(0.0);
  mapper->EmissiveOff();

  // actor->GetProperty()->SetPointSize(3.0);
#endif

#ifdef TestSplats
  randomAttr->SetDataTypeToFloat();
  randomAttr->GeneratePointScalarsOn();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->Update();

  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetInterpolateScalarsBeforeMapping(0);
  mapper->SetScaleArray("RandomPointVectors");
  mapper->SetScaleArrayComponent(3);

  // Note that LookupTable is 4x faster than
  // ColorTransferFunction. So if you have a choice
  // Usa a lut instead.
  //
  // svtkNew<svtkLookupTable> lut;
  // lut->SetHueRange(0.1,0.2);
  // lut->SetSaturationRange(1.0,0.5);
  // lut->SetValueRange(0.8,1.0);
  // mapper->SetLookupTable(lut);

  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddHSVPoint(0.0, 0.1, 1.0, 0.8);
  ctf->AddHSVPoint(1.0, 0.2, 0.5, 1.0);
  ctf->SetColorSpaceToRGB();
  mapper->SetLookupTable(ctf);
#endif

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
