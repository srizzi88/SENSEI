/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositePolyDataMapper2NaNPartial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCompositePolyDataMapper2.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkFloatArray.h"
#include "svtkLookupTable.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTrivialProducer.h"

int TestCompositePolyDataMapper2NaNPartial(int, char*[])
{
  svtkNew<svtkRenderer> renderer;

  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->Update();
  svtkPolyData* sphere = svtkPolyData::SafeDownCast(sphereSource->GetOutputDataObject(0));

  svtkSmartPointer<svtkPolyData> sphere1 = svtkSmartPointer<svtkPolyData>::Take(sphere->NewInstance());
  sphere1->DeepCopy(sphere);

  sphereSource->SetCenter(1., 0., 0.);
  sphereSource->Update();
  sphere = svtkPolyData::SafeDownCast(sphereSource->GetOutputDataObject(0));

  svtkSmartPointer<svtkPolyData> sphere2 = svtkSmartPointer<svtkPolyData>::Take(sphere->NewInstance());
  sphere2->DeepCopy(sphere);

  svtkNew<svtkFloatArray> scalars;
  scalars->SetName("Scalars");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(sphere1->GetNumberOfPoints());
  for (svtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
  {
    scalars->SetTypedComponent(i, 0, static_cast<float>(i));
  }

  // Only add scalars to sphere 1.
  sphere1->GetPointData()->SetScalars(scalars);

  svtkNew<svtkMultiBlockDataSet> mbds;
  mbds->SetNumberOfBlocks(2);
  mbds->SetBlock(0, sphere1);
  mbds->SetBlock(1, sphere2);

  svtkNew<svtkTrivialProducer> source;
  source->SetOutput(mbds);

  svtkNew<svtkLookupTable> lut;
  lut->SetValueRange(scalars->GetRange());
  lut->SetNanColor(1., 1., 0., 1.);
  lut->Build();

  svtkNew<svtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
  mapper->SetLookupTable(lut);
  mapper->SetScalarVisibility(1);
  mapper->SetScalarRange(scalars->GetRange());
  mapper->SetColorMissingArraysWithNanColor(true);
  mapper->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(0., 0., 1.);
  renderer->AddActor(actor);

  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);

  renWin->SetSize(500, 500);
  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
