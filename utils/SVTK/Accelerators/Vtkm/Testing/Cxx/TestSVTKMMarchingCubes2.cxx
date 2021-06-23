/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCellData.h"
#include "svtkElevationFilter.h"
#include "svtkImageData.h"
#include "svtkImageMandelbrotSource.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkmContour.h"

const int EXTENT = 30;
int TestSVTKMMarchingCubes2(int argc, char* argv[])
{
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  svtkNew<svtkRTAnalyticSource> imageSource;
  imageSource->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT);

  svtkNew<svtkElevationFilter> ev;
  ev->SetInputConnection(imageSource->GetOutputPort());
  ev->SetLowPoint(-EXTENT, -EXTENT, -EXTENT);
  ev->SetHighPoint(EXTENT, EXTENT, EXTENT);

  svtkNew<svtkmContour> cg;
  cg->SetInputConnection(ev->GetOutputPort());
  cg->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");
  cg->SetValue(0, 200.0);
  cg->SetValue(1, 220.0);
  cg->ComputeScalarsOn();
  cg->ComputeNormalsOn();

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cg->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Elevation");
  mapper->SetScalarRange(0.0, 1.0);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  ren->AddActor(actor);
  ren->ResetCamera();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  if (!cg->GetOutput()->GetPointData()->GetNormals())
  {
    std::cerr << "Output normals not set.\n";
    return EXIT_FAILURE;
  }

  return (!retVal);
}
