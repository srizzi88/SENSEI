//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "svtkActor.h"
#include "svtkCellData.h"
#include "svtkCountVertices.h"
#include "svtkElevationFilter.h"
#include "svtkImageData.h"
#include "svtkImageMandelbrotSource.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkmContour.h"

namespace
{
template <typename T>
int RunSVTKPipeline(T* t, int argc, char* argv[])
{
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  svtkNew<svtkmContour> cubes;

  cubes->SetInputConnection(t->GetOutputPort());
  cubes->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Iterations");
  cubes->SetNumberOfContours(1);
  cubes->SetValue(0, 50.5f);
  cubes->ComputeScalarsOn();
  cubes->ComputeNormalsOn();

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cubes->GetOutputPort());
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

  svtkDataSet* output = cubes->GetOutput();

  if (!output->GetPointData()->GetNormals())
  {
    std::cerr << "Output normals not set.\n";
    return EXIT_FAILURE;
  }

  svtkDataArray* cellvar = output->GetCellData()->GetArray("Vertex Count");
  if (!cellvar)
  {
    std::cerr << "Cell data missing.\n";
    return EXIT_FAILURE;
  }

  if (cellvar->GetNumberOfTuples() != output->GetNumberOfCells())
  {
    std::cerr << "Mapped cell field does not match number of output cells.\n"
              << "Expected: " << output->GetNumberOfCells()
              << " Actual: " << cellvar->GetNumberOfTuples() << "\n";
    return EXIT_FAILURE;
  }

  return (!retVal);
}

} // Anonymous namespace

int TestSVTKMMarchingCubes(int argc, char* argv[])
{
  // create the sample grid
  svtkNew<svtkImageMandelbrotSource> src;
  src->SetWholeExtent(0, 250, 0, 250, 0, 250);

  // create a secondary field for interpolation
  svtkNew<svtkElevationFilter> elevation;
  elevation->SetInputConnection(src->GetOutputPort());
  elevation->SetScalarRange(0.0, 1.0);
  elevation->SetLowPoint(-1.75, 0.0, 1.0);
  elevation->SetHighPoint(0.75, 0.0, 1.0);

  svtkNew<svtkCountVertices> countVerts;
  countVerts->SetInputConnection(elevation->GetOutputPort());

  // run the pipeline
  return RunSVTKPipeline(countVerts.GetPointer(), argc, argv);
}
