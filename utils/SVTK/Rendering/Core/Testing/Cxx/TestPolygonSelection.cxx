/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolygonSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkExtractSelectedPolyDataIds.h"
#include "svtkHardwareSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkInteractorStyleDrawPolygon.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

const char eventLog[] = "# StreamVersion 1\n"
                        "RenderEvent 0 0 0 0 0 0 0\n"
                        "EnterEvent 278 0 0 0 0 0 0\n"
                        "MouseMoveEvent 278 0 0 0 0 0 0\n"
                        "MouseMoveEvent 274 8 0 0 0 0 0\n"
                        "MouseMoveEvent 144 44 0 0 0 0 0\n"
                        "MouseMoveEvent 144 43 0 0 0 0 0\n"
                        "LeftButtonPressEvent 144 43 0 0 0 0 0\n"
                        "StartInteractionEvent 144 43 0 0 0 0 0\n"
                        "MouseMoveEvent 143 43 0 0 0 0 0\n"
                        "MouseMoveEvent 29 43 0 0 0 0 0\n"
                        "MouseMoveEvent 29 278 0 0 0 0 0\n"
                        "MouseMoveEvent 146 278 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 146 278 0 0 0 0 0\n"
                        "EndInteractionEvent 146 278 0 0 0 0 0\n"
                        "MouseMoveEvent 146 278 0 0 0 0 0\n"
                        "MouseMoveEvent 146 279 0 0 0 0 0\n"
                        "MouseMoveEvent 146 280 0 0 0 0 0\n"
                        "MouseMoveEvent 294 207 0 0 0 0 0\n"
                        "LeaveEvent 294 207 0 0 0 0 0\n";

int TestPolygonSelection(int argc, char* argv[])
{
  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);
  sphere->SetRadius(0.5);

  svtkNew<svtkActor> sactor;
  sactor->PickableOn(); // lets the HardwareSelector select in it
  svtkNew<svtkPolyDataMapper> smapper;
  sactor->SetMapper(smapper);

  svtkNew<svtkRenderer> ren;
  ren->AddActor(sactor);
  // extracted part
  svtkNew<svtkPolyDataMapper> emapper;
  svtkNew<svtkActor> eactor;
  eactor->PickableOff();
  eactor->SetMapper(emapper);
  ren->AddActor(eactor);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // use the draw-polygon interactor style
  svtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  svtkNew<svtkInteractorStyleDrawPolygon> polyStyle;
  polyStyle->DrawPolygonPixelsOff();
  rwi->SetInteractorStyle(polyStyle);

  // record events
  svtkNew<svtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(rwi);

#ifdef RECORD
  recorder->SetFileName("record.log");
  recorder->On();
  recorder->Record();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
#endif

  smapper->SetInputConnection(sphere->GetOutputPort());

  iren->Initialize();
  renWin->Render();

#ifndef RECORD
  recorder->Play();
  recorder->Off();
#endif

  renWin->Render();

  std::vector<svtkVector2i> points = polyStyle->GetPolygonPoints();
  if (points.size() >= 3)
  {
    svtkNew<svtkIntArray> polygonPointsArray;
    polygonPointsArray->SetNumberOfComponents(2);
    polygonPointsArray->SetNumberOfTuples(static_cast<svtkIdType>(points.size()));
    for (unsigned int j = 0; j < points.size(); ++j)
    {
      const svtkVector2i& v = points[j];
      int pos[2] = { v[0], v[1] };
      polygonPointsArray->SetTypedTuple(j, pos);
    }

    svtkNew<svtkHardwareSelector> hardSel;
    hardSel->SetRenderer(ren);

    const int* wsize = ren->GetSize();
    const int* origin = ren->GetOrigin();
    hardSel->SetArea(origin[0], origin[1], origin[0] + wsize[0] - 1, origin[1] + wsize[1] - 1);
    hardSel->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_CELLS);

    if (hardSel->CaptureBuffers())
    {
      svtkSelection* psel = hardSel->GeneratePolygonSelection(
        polygonPointsArray->GetPointer(0), polygonPointsArray->GetNumberOfTuples() * 2);
      hardSel->ClearBuffers();

      svtkSmartPointer<svtkSelection> sel;
      sel.TakeReference(psel);
      svtkNew<svtkExtractSelectedPolyDataIds> selFilter;
      selFilter->SetInputConnection(0, sphere->GetOutputPort());
      selFilter->SetInputData(1, sel);
      selFilter->Update();

      emapper->SetInputConnection(selFilter->GetOutputPort());
      emapper->Update();

      sactor->SetVisibility(false);
      renWin->Render();
    }
  }
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
