/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProStarReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkMPASReader
// .SECTION Description
// Tests the svtkMPASReader.

#include "svtkMPASReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkExecutive.h"
#include "svtkGeometryFilter.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"

#include "svtkDataArray.h"
#include "svtkPointData.h"

int TestMPASReader(int argc, char* argv[])
{
  // Basic visualisation.
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Read file names.
  char* fName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/NetCDF/MPASReader.nc");
  std::string fileName(fName);
  delete[] fName;
  fName = nullptr;

  // make 2 loops for 2 actors since the reader can read in the file
  // as an sphere or as a plane
  for (int i = 0; i < 2; i++)
  {
    // Create the reader.
    svtkNew<svtkMPASReader> reader;
    reader->SetFileName(fileName.c_str());

    // Convert to PolyData.
    svtkNew<svtkGeometryFilter> geometryFilter;
    geometryFilter->SetInputConnection(reader->GetOutputPort());

    geometryFilter->UpdateInformation();
    svtkExecutive* executive = geometryFilter->GetExecutive();
    svtkInformationVector* inputVector = executive->GetInputInformation(0);
    double timeReq = 0;
    inputVector->GetInformationObject(0)->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
    reader->Update();
    reader->EnableAllCellArrays();
    reader->EnableAllPointArrays();
    reader->SetProjectLatLon((i != 0));
    reader->SetVerticalLevel(i);
    reader->Update();

    int* values = reader->GetVerticalLevelRange();
    if (values[0] != 0 || values[1] != 3)
    {
      svtkGenericWarningMacro("Vertical level range is incorrect.");
      return 1;
    }
    values = reader->GetLayerThicknessRange();
    if (values[0] != 0 || values[1] != 200000)
    {
      svtkGenericWarningMacro("Layer thickness range is incorrect.");
      return 1;
    }
    values = reader->GetCenterLonRange();
    if (values[0] != 0 || values[1] != 360)
    {
      svtkGenericWarningMacro("Center lon range is incorrect.");
      return 1;
    }

    // Create a mapper and LUT.
    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputConnection(geometryFilter->GetOutputPort());
    mapper->ScalarVisibilityOn();
    mapper->SetColorModeToMapScalars();
    mapper->SetScalarRange(0.0116, 199.9);
    mapper->SetScalarModeToUsePointFieldData();
    mapper->SelectColorArray("ke");

    // Create the actor.
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    if (i == 1)
    {
      actor->SetScale(30000);
      actor->AddPosition(4370000, 0, 0);
    }
    ren->AddActor(actor);
  }

  svtkNew<svtkCamera> camera;
  ren->ResetCamera(-4370000, 12370000, -6370000, 6370000, -6370000, 6370000);
  camera->Zoom(8);

  ren->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImageThreshold(renWin, 0.25);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  cerr << !retVal << " is the return val\n";
  return !retVal;
}
