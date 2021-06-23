/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSVTKMExtractVOI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkmPolyDataNormals.h"

#include "svtkActor.h"
#include "svtkArrowSource.h"
#include "svtkCamera.h"
#include "svtkCellCenters.h"
#include "svtkCellData.h"
#include "svtkCleanPolyData.h"
#include "svtkCylinderSource.h"
#include "svtkGlyph3D.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTriangleFilter.h"

namespace
{

void MakeInputDataSet(svtkPolyData* ds)
{
  svtkNew<svtkCylinderSource> cylinder;
  cylinder->SetRadius(1.0);
  cylinder->SetResolution(8);
  cylinder->CappingOn();

  svtkNew<svtkTriangleFilter> triangle;
  triangle->SetInputConnection(cylinder->GetOutputPort());

  svtkNew<svtkCleanPolyData> clean;
  clean->SetInputConnection(triangle->GetOutputPort());

  clean->Update();

  ds->ShallowCopy(clean->GetOutput());
  ds->GetPointData()->Initialize();
  ds->GetCellData()->Initialize();
}

}

int TestSVTKMPolyDataNormals(int argc, char* argv[])
{
  svtkNew<svtkPolyData> input;
  MakeInputDataSet(input);

  svtkNew<svtkmPolyDataNormals> normals;
  normals->SetInputData(input);
  normals->ComputePointNormalsOn();
  normals->ComputeCellNormalsOn();
  normals->AutoOrientNormalsOn();
  normals->FlipNormalsOn();
  normals->ConsistencyOn();

  // cylinder mapper and actor
  svtkNew<svtkPolyDataMapper> cylinderMapper;
  cylinderMapper->SetInputData(input);

  svtkNew<svtkActor> cylinderActor;
  cylinderActor->SetMapper(cylinderMapper);
  svtkSmartPointer<svtkProperty> cylinderProperty;
  cylinderProperty.TakeReference(cylinderActor->MakeProperty());
  cylinderProperty->SetRepresentationToWireframe();
  cylinderProperty->SetColor(0.3, 0.3, 0.3);
  cylinderActor->SetProperty(cylinderProperty);

  svtkNew<svtkArrowSource> arrow;

  // point normals
  svtkNew<svtkGlyph3D> pnGlyphs;
  pnGlyphs->SetInputConnection(normals->GetOutputPort());
  pnGlyphs->SetSourceConnection(arrow->GetOutputPort());
  pnGlyphs->SetScaleFactor(0.5);
  pnGlyphs->OrientOn();
  pnGlyphs->SetVectorModeToUseNormal();

  svtkNew<svtkPolyDataMapper> pnMapper;
  pnMapper->SetInputConnection(pnGlyphs->GetOutputPort());

  svtkNew<svtkActor> pnActor;
  pnActor->SetMapper(pnMapper);

  svtkNew<svtkRenderer> pnRenderer;
  pnRenderer->AddActor(cylinderActor);
  pnRenderer->AddActor(pnActor);
  pnRenderer->ResetCamera();
  pnRenderer->GetActiveCamera()->SetPosition(0.0, 4.5, 7.5);
  pnRenderer->ResetCameraClippingRange();

  // cell normals
  svtkNew<svtkCellCenters> cells;
  cells->SetInputConnection(normals->GetOutputPort());

  svtkNew<svtkGlyph3D> cnGlyphs;
  cnGlyphs->SetInputConnection(cells->GetOutputPort());
  cnGlyphs->SetSourceConnection(arrow->GetOutputPort());
  cnGlyphs->SetScaleFactor(0.5);
  cnGlyphs->OrientOn();
  cnGlyphs->SetVectorModeToUseNormal();

  svtkNew<svtkPolyDataMapper> cnMapper;
  cnMapper->SetInputConnection(cnGlyphs->GetOutputPort());

  svtkNew<svtkActor> cnActor;
  cnActor->SetMapper(cnMapper);

  svtkNew<svtkRenderer> cnRenderer;
  cnRenderer->AddActor(cylinderActor);
  cnRenderer->AddActor(cnActor);
  cnRenderer->ResetCamera();
  cnRenderer->GetActiveCamera()->SetPosition(0.0, 8.0, 0.1);
  cnRenderer->ResetCameraClippingRange();

  // render
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(600, 300);
  pnRenderer->SetViewport(0.0, 0.0, 0.5, 1.0);
  renWin->AddRenderer(pnRenderer);
  cnRenderer->SetViewport(0.5, 0.0, 1.0, 1.0);
  renWin->AddRenderer(cnRenderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
