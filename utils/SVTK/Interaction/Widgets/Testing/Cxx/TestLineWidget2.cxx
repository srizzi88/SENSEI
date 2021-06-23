/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLineWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkLineRepresentation.h"
#include "svtkLineWidget2.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiBlockPLOT3DReader.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRibbonFilter.h"
#include "svtkRungeKutta4.h"
#include "svtkStreamTracer.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridOutlineFilter.h"

#include "svtkTestUtilities.h"

#include "TestLineWidget2EventLog.h"
#include <string>

// This does the actual work: updates the probe.
// Callback for the interaction
class svtkLW2Callback : public svtkCommand
{
public:
  static svtkLW2Callback* New() { return new svtkLW2Callback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkLineWidget2* lineWidget = reinterpret_cast<svtkLineWidget2*>(caller);
    svtkLineRepresentation* rep =
      reinterpret_cast<svtkLineRepresentation*>(lineWidget->GetRepresentation());
    rep->GetPolyData(this->PolyData);
    this->Actor->VisibilityOn();
  }
  svtkLW2Callback()
    : PolyData(nullptr)
    , Actor(nullptr)
  {
  }
  svtkPolyData* PolyData;
  svtkActor* Actor;
};

int TestLineWidget2(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  char* fname2 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");

  // Start by loading some data.
  //
  svtkSmartPointer<svtkMultiBlockPLOT3DReader> pl3d =
    svtkSmartPointer<svtkMultiBlockPLOT3DReader>::New();
  pl3d->SetXYZFileName(fname);
  pl3d->SetQFileName(fname2);
  pl3d->SetScalarFunctionNumber(100);
  pl3d->SetVectorFunctionNumber(202);
  pl3d->Update();
  svtkDataSet* pl3d_block0 = svtkDataSet::SafeDownCast(pl3d->GetOutput()->GetBlock(0));

  delete[] fname;
  delete[] fname2;

  svtkSmartPointer<svtkPolyData> seeds = svtkSmartPointer<svtkPolyData>::New();

  // Create streamtues
  svtkSmartPointer<svtkRungeKutta4> rk4 = svtkSmartPointer<svtkRungeKutta4>::New();

  svtkSmartPointer<svtkStreamTracer> streamer = svtkSmartPointer<svtkStreamTracer>::New();
  streamer->SetInputData(pl3d_block0);
  streamer->SetSourceData(seeds);
  streamer->SetMaximumPropagation(100);
  streamer->SetInitialIntegrationStep(.2);
  streamer->SetIntegrationDirectionToForward();
  streamer->SetComputeVorticity(1);
  streamer->SetIntegrator(rk4);

  svtkSmartPointer<svtkRibbonFilter> rf = svtkSmartPointer<svtkRibbonFilter>::New();
  rf->SetInputConnection(streamer->GetOutputPort());
  rf->SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Normals");
  rf->SetWidth(0.1);
  rf->SetWidthFactor(5);

  svtkSmartPointer<svtkPolyDataMapper> streamMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  streamMapper->SetInputConnection(rf->GetOutputPort());
  double tmp[2];
  pl3d_block0->GetScalarRange(tmp);
  streamMapper->SetScalarRange(tmp[0], tmp[1]);

  svtkSmartPointer<svtkActor> streamline = svtkSmartPointer<svtkActor>::New();
  streamline->SetMapper(streamMapper);
  streamline->VisibilityOff();

  // An outline is shown for context.
  svtkSmartPointer<svtkStructuredGridOutlineFilter> outline =
    svtkSmartPointer<svtkStructuredGridOutlineFilter>::New();
  outline->SetInputData(pl3d_block0);

  svtkSmartPointer<svtkPolyDataMapper> outlineMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  svtkSmartPointer<svtkActor> outlineActor = svtkSmartPointer<svtkActor>::New();
  outlineActor->SetMapper(outlineMapper);

  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  svtkSmartPointer<svtkLW2Callback> myCallback = svtkSmartPointer<svtkLW2Callback>::New();
  myCallback->PolyData = seeds;
  myCallback->Actor = streamline;

  // The line widget is used probe the dataset.
  //
  double p[3];
  svtkSmartPointer<svtkLineRepresentation> rep = svtkSmartPointer<svtkLineRepresentation>::New();
  p[0] = 0.0;
  p[1] = -1.0;
  p[2] = 0.0;
  rep->SetPoint1WorldPosition(p);
  p[0] = 0.0;
  p[1] = 1.0;
  p[2] = 0.0;
  rep->SetPoint2WorldPosition(p);
  rep->PlaceWidget(pl3d_block0->GetBounds());
  rep->GetPolyData(seeds);
  rep->DistanceAnnotationVisibilityOn();

  svtkSmartPointer<svtkLineWidget2> lineWidget = svtkSmartPointer<svtkLineWidget2>::New();
  lineWidget->SetInteractor(iren);
  lineWidget->SetRepresentation(rep);
  lineWidget->AddObserver(svtkCommand::InteractionEvent, myCallback);

  ren1->AddActor(streamline);
  ren1->AddActor(outlineActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  svtkSmartPointer<svtkInteractorEventRecorder> recorder =
    svtkSmartPointer<svtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  //  recorder->SetFileName("c:/record.log");
  //  recorder->Record();
  recorder->ReadFromInputStringOn();
  std::string TestLineWidget2EventLog(TestLineWidget2EventLog_p1);
  TestLineWidget2EventLog += TestLineWidget2EventLog_p2;
  TestLineWidget2EventLog += TestLineWidget2EventLog_p3;
  recorder->SetInputString(TestLineWidget2EventLog.c_str());

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
