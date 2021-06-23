/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSimplePointsReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenFOAMReader.h"

#include "svtkCellData.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositePolyDataMapper.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnstructuredGrid.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

int TestOpenFOAMReader64BitFloats(int argc, char* argv[])
{
  // Read file name.
  char* filename =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/64BitFloats/test.foam");

  // Read the file
  svtkNew<svtkOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;
  reader->Use64BitFloatsOn();

  // Visualize
  svtkNew<svtkCompositeDataGeometryFilter> polyFilter;
  polyFilter->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(polyFilter->GetOutputPort());
  mapper->SetScalarRange(1, 2);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(.2, .4, .6);

  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return EXIT_SUCCESS;
}
