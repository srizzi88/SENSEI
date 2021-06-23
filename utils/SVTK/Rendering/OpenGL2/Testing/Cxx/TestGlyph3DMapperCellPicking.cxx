/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This tests svtkVisibleCellSelector, svtkExtractSelectedFrustum,
// svtkRenderedAreaPicker, and svtkInteractorStyleRubberBandPick.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkBitArray.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkElevationFilter.h"
#include "svtkGlyph3DMapper.h"
#include "svtkHardwareSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkImageActor.h"
#include "svtkInteractorStyleRubberBandPick.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedAreaPicker.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSphereSource.h"
#include <cassert>

static svtkRenderer* renderer = nullptr;

class MyEndPickCommand : public svtkCommand
{
public:
  MyEndPickCommand()
  {
    this->Renderer = nullptr; // no reference counting
    this->Mask = nullptr;     // no reference counting
    this->DataSet = nullptr;
  }

  ~MyEndPickCommand() override
  {
    // empty
  }

  void Execute(svtkObject* svtkNotUsed(caller), unsigned long svtkNotUsed(eventId),
    void* svtkNotUsed(callData)) override
  {
    assert("pre: renderer_exists" && this->Renderer != nullptr);

    svtkHardwareSelector* sel = svtkHardwareSelector::New();
    sel->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_CELLS);
    sel->SetRenderer(renderer);

    double x0 = renderer->GetPickX1();
    double y0 = renderer->GetPickY1();
    double x1 = renderer->GetPickX2();
    double y1 = renderer->GetPickY2();
    sel->SetArea(static_cast<unsigned int>(x0), static_cast<unsigned int>(y0),
      static_cast<unsigned int>(x1), static_cast<unsigned int>(y1));

    svtkSelection* res = sel->Select();

#if 1
    cerr << "x0 " << x0 << " y0 " << y0 << "\t";
    cerr << "x1 " << x1 << " y1 " << y1 << endl;
    res->Print(cout);
#endif

    // Reset the mask to false.
    svtkIdType numPoints = this->Mask->GetNumberOfTuples();
    for (svtkIdType i = 0; i < numPoints; i++)
    {
      this->Mask->SetValue(i, false);
    }

    svtkSelectionNode* glyphids = res->GetNode(0);
    if (glyphids != nullptr)
    {
      svtkAbstractArray* abs = glyphids->GetSelectionList();
      if (abs == nullptr)
      {
        cout << "abs is null" << endl;
      }
      svtkIdTypeArray* ids = svtkArrayDownCast<svtkIdTypeArray>(abs);
      if (ids == nullptr)
      {
        cout << "ids is null" << endl;
      }
      else
      {
        // modify mask array with selection.
        svtkIdType numSelPoints = ids->GetNumberOfTuples();
        for (svtkIdType i = 0; i < numSelPoints; i++)
        {
          svtkIdType value = ids->GetValue(i);
          if (value >= 0 && value < numPoints)
          {
            cout << "Turn On: " << value << endl;
            this->Mask->SetValue(value, true);
          }
          else
          {
            cout << "Ignoring: " << value << endl;
          }
        }
      }
    }
    this->DataSet->Modified();

    sel->Delete();
    res->Delete();
  }

  void SetRenderer(svtkRenderer* r) { this->Renderer = r; }

  svtkRenderer* GetRenderer() const { return this->Renderer; }

  void SetMask(svtkBitArray* m) { this->Mask = m; }
  void SetDataSet(svtkDataSet* ds) { this->DataSet = ds; }

protected:
  svtkRenderer* Renderer;
  svtkBitArray* Mask;
  svtkDataSet* DataSet;
};

int TestGlyph3DMapperCellPicking(int argc, char* argv[])
{
  int res = 1;
  svtkPlaneSource* plane = svtkPlaneSource::New();
  plane->SetResolution(res, res);
  svtkElevationFilter* colors = svtkElevationFilter::New();
  colors->SetInputConnection(plane->GetOutputPort());
  plane->Delete();
  colors->SetLowPoint(-1, -1, -1);
  colors->SetHighPoint(0.5, 0.5, 0.5);

  svtkSphereSource* squad = svtkSphereSource::New();
  squad->SetPhiResolution(4);
  squad->SetThetaResolution(6);

  svtkGlyph3DMapper* glypher = svtkGlyph3DMapper::New();
  glypher->SetInputConnection(colors->GetOutputPort());
  colors->Delete();
  glypher->SetScaleFactor(1.5);
  glypher->SetSourceConnection(squad->GetOutputPort());
  squad->Delete();

  // selection is performed on actor1
  svtkActor* glyphActor1 = svtkActor::New();
  glyphActor1->SetMapper(glypher);
  glypher->Delete();
  glyphActor1->PickableOn();

  // result of selection is on actor2
  svtkActor* glyphActor2 = svtkActor::New();
  glyphActor2->PickableOff();
  colors->Update(); // make sure output is valid.
  svtkDataSet* selection = colors->GetOutput()->NewInstance();
  selection->ShallowCopy(colors->GetOutput());

  svtkBitArray* selectionMask = svtkBitArray::New();
  selectionMask->SetName("mask");
  selectionMask->SetNumberOfComponents(1);
  selectionMask->SetNumberOfTuples(selection->GetNumberOfPoints());
  // Initially, everything is selected
  svtkIdType i = 0;
  svtkIdType c = selectionMask->GetNumberOfTuples();
  while (i < c)
  {
    selectionMask->SetValue(i, true);
    ++i;
  }
  selection->GetPointData()->AddArray(selectionMask);
  selectionMask->Delete();

  svtkGlyph3DMapper* glypher2 = svtkGlyph3DMapper::New();
  //  glypher->SetNestedDisplayLists(0);
  glypher2->SetMasking(1);
  glypher2->SetMaskArray("mask");

  glypher2->SetInputData(selection);
  glypher2->SetScaleFactor(1.5);
  glypher2->SetSourceConnection(squad->GetOutputPort());
  glyphActor2->SetMapper(glypher2);
  glypher2->Delete();

  // Standard rendering classes
  renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.2, 0.2, 0.2);
  renWin->SetSize(600, 300);

  // use the rubber band pick interactor style
  svtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  svtkInteractorStyleRubberBandPick* rbp = svtkInteractorStyleRubberBandPick::New();
  rwi->SetInteractorStyle(rbp);

  svtkRenderedAreaPicker* areaPicker = svtkRenderedAreaPicker::New();
  rwi->SetPicker(areaPicker);

  renderer->AddActor(glyphActor1);
  renderer->AddActor(glyphActor2);
  glyphActor2->SetPosition(2, 0, 0);
  glyphActor1->Delete();
  glyphActor2->Delete();

  // pass pick events to the VisibleGlyphSelector
  MyEndPickCommand* cbc = new MyEndPickCommand;
  cbc->SetRenderer(renderer);
  cbc->SetMask(selectionMask);
  cbc->SetDataSet(selection);
  rwi->AddObserver(svtkCommand::EndPickEvent, cbc);
  cbc->Delete();

  ////////////////////////////////////////////////////////////

  // run the test

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2.0);

  renWin->Render();
  // areaPicker->AreaPick(0, 0, 241, 160, renderer);
  areaPicker->AreaPick(233, 120, 241, 160, renderer);
  cbc->Execute(nullptr, 0, nullptr);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  rbp->Delete();
  areaPicker->Delete();
  selection->Delete();
  return !retVal;
}
