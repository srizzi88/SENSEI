/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSVGExporterContextPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAbstractMapper.h"
#include "svtkAxis.h"
#include "svtkBandedPolyDataContourFilter.h"
#include "svtkBoundingBox.h"
#include "svtkCellData.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFeatureEdges.h"
#include "svtkFloatArray.h"
#include "svtkInteractiveArea.h"
#include "svtkInteractorStyle.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkPen.h"
#include "svtkPointData.h"
#include "svtkPolyDataConnectivityFilter.h"
#include "svtkPolyDataItem.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSVGExporter.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTestingInteractor.h"
#include "svtkTextProperty.h"
#include "svtkXMLPolyDataReader.h"

//------------------------------------------------------------------------------
namespace
{
svtkSmartPointer<svtkXMLPolyDataReader> ReadUVCDATPolyData(int argc, char* argv[])
{
  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/isofill_0.vtp");
  svtkSmartPointer<svtkXMLPolyDataReader> reader = svtkSmartPointer<svtkXMLPolyDataReader>::New();
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  return reader;
}

//------------------------------------------------------------------------------
svtkSmartPointer<svtkPolyDataItem> CreateMapItem(int argc, char* argv[])
{
  svtkSmartPointer<svtkXMLPolyDataReader> reader = ReadUVCDATPolyData(argc, argv);
  svtkPolyData* poly = reader->GetOutput();

  // Select point/cell data
  double range[2];
  int scalarMode = SVTK_SCALAR_MODE_USE_POINT_DATA;

  svtkDataArray* activeData = scalarMode == SVTK_SCALAR_MODE_USE_POINT_DATA
    ? poly->GetPointData()->GetScalars()
    : poly->GetCellData()->GetScalars();
  activeData->GetRange(range, 0);

  // Map scalars
  svtkLookupTable* colorLut = activeData->GetLookupTable();
  if (!colorLut)
  {
    activeData->CreateDefaultLookupTable();
    colorLut = activeData->GetLookupTable();
    colorLut->SetAlpha(1.0);
    colorLut->SetRange(range[0], range[1]);
  }
  svtkUnsignedCharArray* mappedColors = colorLut->MapScalars(activeData, SVTK_COLOR_MODE_DEFAULT, 0);

  // Setup item
  svtkSmartPointer<svtkPolyDataItem> polyItem = svtkSmartPointer<svtkPolyDataItem>::New();
  polyItem->SetPolyData(poly);
  polyItem->SetScalarMode(scalarMode);
  polyItem->SetMappedColors(mappedColors);
  mappedColors->Delete();

  return polyItem;
}

//------------------------------------------------------------------------------
svtkSmartPointer<svtkPolyDataItem> CreateContourItem(int argc, char* argv[])
{
  svtkSmartPointer<svtkXMLPolyDataReader> reader = ReadUVCDATPolyData(argc, argv);
  svtkNew<svtkBandedPolyDataContourFilter> contour;
  contour->SetInputConnection(reader->GetOutputPort());
  contour->GenerateValues(20, 6, 40);
  contour->ClippingOn();
  contour->SetClipTolerance(0.);
  contour->Update();

  svtkNew<svtkPolyDataConnectivityFilter> connectivity;
  connectivity->SetInputConnection(contour->GetOutputPort());
  connectivity->SetExtractionModeToAllRegions();
  connectivity->ColorRegionsOn();
  connectivity->Update();

  svtkNew<svtkPolyDataConnectivityFilter> extract;
  extract->SetInputConnection(connectivity->GetOutputPort());
  extract->ScalarConnectivityOn();
  extract->SetScalarRange(6, 58);

  svtkNew<svtkFeatureEdges> edge;
  edge->SetInputConnection(extract->GetOutputPort());
  edge->BoundaryEdgesOn();
  edge->FeatureEdgesOff();
  edge->ManifoldEdgesOff();
  edge->NonManifoldEdgesOff();
  edge->Update();

  // Select point/cell data
  double range[2];
  int scalarMode = SVTK_SCALAR_MODE_USE_CELL_DATA;

  svtkPolyData* poly = edge->GetOutput();
  svtkDataArray* activeData = scalarMode == SVTK_SCALAR_MODE_USE_POINT_DATA
    ? poly->GetPointData()->GetScalars()
    : poly->GetCellData()->GetScalars();
  activeData->GetRange(range, 0);

  // Map scalars
  svtkLookupTable* colorLut = activeData->GetLookupTable();
  if (!colorLut)
  {
    activeData->CreateDefaultLookupTable();
    colorLut = activeData->GetLookupTable();
    colorLut->SetAlpha(1.0);
    colorLut->SetRange(range[0], range[1]);
  }
  svtkUnsignedCharArray* mappedColors = colorLut->MapScalars(activeData, SVTK_COLOR_MODE_DEFAULT, 0);

  // Setup item
  svtkSmartPointer<svtkPolyDataItem> polyItem = svtkSmartPointer<svtkPolyDataItem>::New();
  polyItem->SetPolyData(poly);
  polyItem->SetScalarMode(scalarMode);
  polyItem->SetMappedColors(mappedColors);
  mappedColors->Delete();

  return polyItem;
}

} // end anon namespace

/**
 * Tests svtkPolyDataItem and shows its usage with an example. svtkPolyDataItem
 * renders svtkPolyData primitives into a svtkContextScene directly (without the
 * need of a svtkMapper).
 */

///////////////////////////////////////////////////////////////////////////////
int TestSVGExporterContextPolyData(int argc, char* argv[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(0.3, 0.3, 0.3);
  view->GetRenderWindow()->SetSize(600, 400);
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->GetInteractorStyle()->SetCurrentRenderer(view->GetRenderer());

  // Create the container item that handles view transform (aspect, interaction,
  // etc.)
  svtkNew<svtkInteractiveArea> area;

  svtkSmartPointer<svtkPolyDataItem> mapItem = CreateMapItem(argc, argv);
  svtkSmartPointer<svtkPolyDataItem> contourItem = CreateContourItem(argc, argv);
  area->GetDrawAreaItem()->AddItem(mapItem);
  area->GetDrawAreaItem()->AddItem(contourItem);

  svtkBoundingBox bounds(mapItem->GetPolyData()->GetBounds());
  area->SetDrawAreaBounds(
    svtkRectd(bounds.GetBound(0), bounds.GetBound(2), bounds.GetLength(0), bounds.GetLength(1)));
  area->SetFixedAspect(bounds.GetLength(0) / bounds.GetLength(1));

  area->GetAxis(svtkAxis::BOTTOM)->SetTitle("X Axis");
  area->GetAxis(svtkAxis::LEFT)->SetTitle("Y Axis");
  area->GetAxis(svtkAxis::TOP)->SetVisible(false);
  area->GetAxis(svtkAxis::RIGHT)->SetVisible(false);
  for (int i = 0; i < 4; ++i)
  {
    svtkAxis* axis = area->GetAxis(static_cast<svtkAxis::Location>(i));
    axis->GetLabelProperties()->SetColor(.6, .6, .9);
    axis->GetTitleProperties()->SetColor(.6, .6, .9);
    axis->GetPen()->SetColor(.6 * 255, .6 * 255, .9 * 255, 255);
    axis->GetGridPen()->SetColor(.6 * 255, .6 * 255, .9 * 255, 128);
  }

  // Turn off the color buffer
  view->GetScene()->SetUseBufferId(false);
  view->GetScene()->AddItem(area);
  view->Render();

  std::string filename =
    svtkTestingInteractor::TempDirectory + std::string("/TestSVGExporterContextPolyData.svg");

  svtkNew<svtkSVGExporter> exp;
  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileName(filename.c_str());
  // This polydata is quite large -- limit the number of triangles emitted
  // during gradient subdivision.
  exp->SetSubdivisionThreshold(10.f);
  exp->Write();

#if 0
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(view->GetRenderWindow());
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->GetInteractor()->Initialize();
  view->GetRenderWindow()->GetInteractor()->Start();
#endif

  return EXIT_SUCCESS;
}
