/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRSliceFilterPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test svtkAMRSliceFilter filter.

#include <svtkAMRSliceFilter.h>
#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkCompositeDataDisplayAttributes.h>
#include <svtkCompositePolyDataMapper2.h>
#include <svtkDataObjectTreeIterator.h>
#include <svtkDataSetSurfaceFilter.h>
#include <svtkImageToAMR.h>
#include <svtkLookupTable.h>
#include <svtkNamedColors.h>
#include <svtkNew.h>
#include <svtkOverlappingAMR.h>
#include <svtkRTAnalyticSource.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkUniformGridAMRDataIterator.h>

#include <array>

int TestAMRSliceFilterPointData(int argc, char* argv[])
{
  svtkNew<svtkRTAnalyticSource> imgSrc;

  svtkNew<svtkImageToAMR> amr;
  amr->SetInputConnection(imgSrc->GetOutputPort());
  amr->SetNumberOfLevels(3);

  svtkNew<svtkAMRSliceFilter> slicer;
  slicer->SetInputConnection(amr->GetOutputPort());
  slicer->SetNormal(1);
  slicer->SetOffsetFromOrigin(10);
  slicer->SetMaxResolution(2);

  svtkNew<svtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(slicer->GetOutputPort());
  surface->Update();

  // color map
  svtkNew<svtkNamedColors> colors;

  svtkNew<svtkColorTransferFunction> colormap;
  colormap->SetColorSpaceToDiverging();
  colormap->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
  colormap->AddRGBPoint(1.0, 0.0, 0.0, 1.0);

  svtkNew<svtkLookupTable> lut;
  lut->SetNumberOfColors(256);
  for (int i = 0; i < lut->GetNumberOfColors(); ++i)
  {
    std::array<double, 4> color;
    colormap->GetColor(static_cast<double>(i) / lut->GetNumberOfColors(), color.data());
    color[3] = 1.0;
    lut->SetTableValue(i, color.data());
  }
  lut->Build();

  // Rendering
  svtkNew<svtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->SetLookupTable(lut);
  mapper->SetScalarRange(37.3531, 276.829);
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetInterpolateScalarsBeforeMapping(1);
  mapper->SelectColorArray("RTData");

  svtkNew<svtkCompositeDataDisplayAttributes> cdsa;
  mapper->SetCompositeDataDisplayAttributes(cdsa);

  int nonLeafNodes = 0;
  {
    svtkOverlappingAMR* oamr = svtkOverlappingAMR::SafeDownCast(slicer->GetOutputDataObject(0));
    svtkSmartPointer<svtkUniformGridAMRDataIterator> iter =
      svtkSmartPointer<svtkUniformGridAMRDataIterator>::New();
    iter->SetDataSet(oamr);
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (iter->GetCurrentLevel() < 2)
      {
        nonLeafNodes++;
      }
    }
  }

  // only show the leaf nodes
  svtkCompositeDataSet* input = svtkCompositeDataSet::SafeDownCast(surface->GetOutputDataObject(0));
  if (input)
  {
    svtkSmartPointer<svtkDataObjectTreeIterator> iter =
      svtkSmartPointer<svtkDataObjectTreeIterator>::New();
    iter->SetDataSet(input);
    iter->SkipEmptyNodesOn();
    iter->VisitOnlyLeavesOn();
    int count = 0;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      unsigned int flatIndex = iter->GetCurrentFlatIndex();
      mapper->SetBlockVisibility(flatIndex, count > nonLeafNodes);
      count++;
    }
  }

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> rwin;
  rwin->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(rwin);

  ren->AddActor(actor);
  ren->GetActiveCamera()->SetPosition(15, 0, 0);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  ren->ResetCamera();
  rwin->SetSize(300, 300);
  rwin->Render();

  int retVal = svtkRegressionTestImage(rwin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
