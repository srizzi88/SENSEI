/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiBlockMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * \brief Tests svtkMultiBlockDataSet rendering. Uses the exposed VectorMode
 * API of svtkSmartVolumeMapper to render component X of the array data.
 *
 */

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkCompositeDataSet.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkMultiBlockVolumeMapper.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStructuredPointsReader.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"
#include "svtkXMLMultiBlockDataReader.h"
#include "svtkXMLPUnstructuredGridReader.h"

int TestMultiBlockMapper(int argc, char* argv[])
{
  // cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  svtkNew<svtkXMLMultiBlockDataReader> reader;
  const char* fileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headmr3blocks/headmr3blocks.vtm");
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  svtkNew<svtkMultiBlockVolumeMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SelectScalarArray("MetaImage");
  mapper->SetScalarMode(SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA);

  svtkNew<svtkColorTransferFunction> color;
  color->AddHSVPoint(1.0, 0.095, 0.33, 0.82);
  color->AddHSVPoint(53.3, 0.04, 0.7, 0.63);
  color->AddHSVPoint(256, 0.095, 0.33, 0.82);

  svtkNew<svtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.0);
  opacity->AddPoint(4.48, 0.0);
  opacity->AddPoint(43.116, 0.35);
  opacity->AddPoint(641.0, 1.0);

  svtkNew<svtkVolumeProperty> property;
  property->SetColor(color);
  property->SetScalarOpacity(opacity);
  property->SetInterpolationTypeToLinear();
  property->ShadeOn();

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(401, 400);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);

  ren->AddVolume(volume);
  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(0);
  ren->GetActiveCamera()->Roll(-65);
  ren->GetActiveCamera()->Elevation(-45);
  ren->GetActiveCamera()->Zoom(1.2);
  renWin->Render();

  // initialize render loop
  int const retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();
  }

  return !retVal;
}
