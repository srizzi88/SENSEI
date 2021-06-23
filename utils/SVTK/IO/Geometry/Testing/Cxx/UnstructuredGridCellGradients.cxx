/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnstructuredGridCellGradients.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkActor.h"
#include "svtkArrowSource.h"
#include "svtkAssignAttribute.h"
#include "svtkCamera.h"
#include "svtkCellCenters.h"
#include "svtkExtractEdges.h"
#include "svtkGlyph3D.h"
#include "svtkGradientFilter.h"
#include "svtkPointDataToCellData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStdString.h"
#include "svtkTubeFilter.h"
#include "svtkUnstructuredGridReader.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

int UnstructuredGridCellGradients(int argc, char* argv[])
{
  int i;
  // Need to get the data root.
  const char* data_root = nullptr;
  for (i = 0; i < argc - 1; i++)
  {
    if (strcmp("-D", argv[i]) == 0)
    {
      data_root = argv[i + 1];
      break;
    }
  }
  if (!data_root)
  {
    cout << "Need to specify the directory to SVTK_DATA_ROOT with -D <dir>." << endl;
    return 1;
  }

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  svtkStdString filename;
  filename = data_root;
  filename += "/Data/uGridEx.svtk";
  cout << "Loading " << filename.c_str() << endl;
  SVTK_CREATE(svtkUnstructuredGridReader, reader);
  reader->SetFileName(filename.c_str());

  SVTK_CREATE(svtkExtractEdges, edges);
  edges->SetInputConnection(reader->GetOutputPort());

  SVTK_CREATE(svtkTubeFilter, tubes);
  tubes->SetInputConnection(edges->GetOutputPort());
  tubes->SetRadius(0.0625);
  tubes->SetVaryRadiusToVaryRadiusOff();
  tubes->SetNumberOfSides(32);

  SVTK_CREATE(svtkPolyDataMapper, tubesMapper);
  tubesMapper->SetInputConnection(tubes->GetOutputPort());
  tubesMapper->SetScalarRange(0.0, 26.0);

  SVTK_CREATE(svtkActor, tubesActor);
  tubesActor->SetMapper(tubesMapper);

  SVTK_CREATE(svtkPointDataToCellData, pd2cd);
  pd2cd->SetInputConnection(reader->GetOutputPort());

  SVTK_CREATE(svtkGradientFilter, gradients);
  gradients->SetInputConnection(pd2cd->GetOutputPort());
  gradients->SetInputScalars(svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);

  SVTK_CREATE(svtkCellCenters, cellCenters);
  cellCenters->SetInputConnection(gradients->GetOutputPort());

  SVTK_CREATE(svtkAssignAttribute, vectors);
  vectors->SetInputConnection(cellCenters->GetOutputPort());
  vectors->Assign("Gradients", svtkDataSetAttributes::VECTORS, svtkAssignAttribute::POINT_DATA);

  SVTK_CREATE(svtkArrowSource, arrow);

  SVTK_CREATE(svtkGlyph3D, glyphs);
  glyphs->SetInputConnection(0, vectors->GetOutputPort());
  glyphs->SetInputConnection(1, arrow->GetOutputPort());
  glyphs->ScalingOn();
  glyphs->SetScaleModeToScaleByVector();
  glyphs->SetScaleFactor(0.25);
  glyphs->OrientOn();
  glyphs->ClampingOff();
  glyphs->SetVectorModeToUseVector();
  glyphs->SetIndexModeToOff();

  SVTK_CREATE(svtkPolyDataMapper, glyphMapper);
  glyphMapper->SetInputConnection(glyphs->GetOutputPort());
  glyphMapper->ScalarVisibilityOff();

  SVTK_CREATE(svtkActor, glyphActor);
  glyphActor->SetMapper(glyphMapper);

  SVTK_CREATE(svtkRenderer, renderer);
  renderer->AddActor(tubesActor);
  renderer->AddActor(glyphActor);
  renderer->SetBackground(0.328125, 0.347656, 0.425781);

  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->SetMultiSamples(0);
  renwin->AddRenderer(renderer);
  renwin->SetSize(350, 500);

  renderer->ResetCamera();
  svtkCamera* camera = renderer->GetActiveCamera();
  camera->Elevation(-85.0);
  camera->OrthogonalizeViewUp();
  camera->Elevation(-5.0);
  camera->OrthogonalizeViewUp();
  camera->Elevation(-10.0);
  camera->Azimuth(55.0);

  int retVal = svtkTesting::Test(argc, argv, renwin, 5.0);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    SVTK_CREATE(svtkRenderWindowInteractor, iren);
    iren->SetRenderWindow(renwin);
    iren->Initialize();
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  if (retVal == svtkRegressionTester::PASSED)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
