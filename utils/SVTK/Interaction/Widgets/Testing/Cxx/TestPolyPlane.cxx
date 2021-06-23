/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistanceWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCommand.h"
#include "svtkContourWidget.h"
#include "svtkCutter.h"
#include "svtkDEMReader.h"
#include "svtkImageData.h"
#include "svtkImageDataGeometryFilter.h"
#include "svtkImageResample.h"
#include "svtkLinearContourLineInterpolator.h"
#include "svtkLookupTable.h"
#include "svtkOrientedGlyphContourRepresentation.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyLine.h"
#include "svtkPolyPlane.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"
#include "svtkTriangleFilter.h"
#include "svtkUnstructuredGrid.h"
#include "svtkWarpScalar.h"
#include "svtkXMLPolyDataWriter.h"
#include "svtkXYPlotActor.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

// --------------------------------------------------------------------------
// Callback for the widget interaction
class svtkTestPolyPlaneCallback : public svtkCommand
{
public:
  static svtkTestPolyPlaneCallback* New() { return new svtkTestPolyPlaneCallback; }

  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkContourWidget* widget = reinterpret_cast<svtkContourWidget*>(caller);
    svtkContourRepresentation* rep =
      svtkContourRepresentation::SafeDownCast(widget->GetRepresentation());

    svtkPolyData* pd = rep->GetContourRepresentationAsPolyData();

    // If less than 2 points, we can't define a polyplane..

    if (pd->GetPoints()->GetNumberOfPoints() >= 2)
    {
      svtkIdType npts;
      const svtkIdType* ptIds;
      pd->GetLines()->GetCellAtId(0, npts, ptIds);

      svtkPolyLine* polyline = svtkPolyLine::New();
      polyline->Initialize(static_cast<int>(npts), ptIds, pd->GetPoints());

      this->PolyPlane->SetPolyLine(polyline);
      polyline->Delete();

      this->Cutter->SetCutFunction(this->PolyPlane);
    }
  }

  svtkTestPolyPlaneCallback()
    : PolyPlane(nullptr)
    , Cutter(nullptr)
  {
  }
  svtkPolyPlane* PolyPlane;
  svtkCutter* Cutter;
};

// --------------------------------------------------------------------------
int TestPolyPlane(int argc, char* argv[])
{
  // Read height field.

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");

  svtkSmartPointer<svtkDEMReader> demReader = svtkSmartPointer<svtkDEMReader>::New();
  demReader->SetFileName(fname);

  delete[] fname;

  // Resample (left in case, we want to subsample, supersample)

  svtkSmartPointer<svtkImageResample> resample = svtkSmartPointer<svtkImageResample>::New();
  resample->SetInputConnection(demReader->GetOutputPort());
  resample->SetDimensionality(2);
  resample->SetAxisMagnificationFactor(0, 0.25);
  resample->SetAxisMagnificationFactor(1, 0.25);

  // Extract geometry

  svtkSmartPointer<svtkImageDataGeometryFilter> surface =
    svtkSmartPointer<svtkImageDataGeometryFilter>::New();
  surface->SetInputConnection(resample->GetOutputPort());

  // Convert to triangle mesh

  svtkSmartPointer<svtkTriangleFilter> triangleFilter = svtkSmartPointer<svtkTriangleFilter>::New();
  triangleFilter->SetInputConnection(surface->GetOutputPort());
  triangleFilter->Update();

  // Warp

  svtkSmartPointer<svtkWarpScalar> warp = svtkSmartPointer<svtkWarpScalar>::New();
  warp->SetInputConnection(triangleFilter->GetOutputPort());
  warp->SetScaleFactor(1);
  warp->UseNormalOn();
  warp->SetNormal(0, 0, 1);

  //  Update the pipeline until now.

  warp->Update();

  // Define a LUT mapping for the height field

  double lo = demReader->GetOutput()->GetScalarRange()[0];
  double hi = demReader->GetOutput()->GetScalarRange()[1];

  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  lut->SetHueRange(0.6, 0);
  lut->SetSaturationRange(1.0, 0);
  lut->SetValueRange(0.5, 1.0);

  // Create Renderer, Render window and interactor

  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderer> ren2 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Render the height field

  svtkSmartPointer<svtkPolyDataMapper> demMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  demMapper->SetInputConnection(warp->GetOutputPort());
  demMapper->SetScalarRange(lo, hi);
  demMapper->SetLookupTable(lut);

  svtkSmartPointer<svtkActor> demActor = svtkSmartPointer<svtkActor>::New();
  demActor->SetMapper(demMapper);

  ren1->AddActor(demActor);

  // Create a contour widget on ren1

  svtkSmartPointer<svtkContourWidget> contourWidget = svtkSmartPointer<svtkContourWidget>::New();
  contourWidget->SetInteractor(iren);
  svtkOrientedGlyphContourRepresentation* rep =
    svtkOrientedGlyphContourRepresentation::SafeDownCast(contourWidget->GetRepresentation());
  rep->GetLinesProperty()->SetColor(1, 0.2, 0);
  rep->GetLinesProperty()->SetLineWidth(3.0);

  // Use no interpolation (default is bezier).

  svtkSmartPointer<svtkLinearContourLineInterpolator> lineInterpolator =
    svtkSmartPointer<svtkLinearContourLineInterpolator>::New();
  rep->SetLineInterpolator(lineInterpolator);

  // Create a polyplane to cut with

  svtkSmartPointer<svtkPolyPlane> polyPlane = svtkSmartPointer<svtkPolyPlane>::New();

  // Create a cutter

  svtkSmartPointer<svtkCutter> cutter = svtkSmartPointer<svtkCutter>::New();
  cutter->SetInputConnection(warp->GetOutputPort());

  // Callback to update the polyplane when the contour is updated

  svtkSmartPointer<svtkTestPolyPlaneCallback> cb = svtkSmartPointer<svtkTestPolyPlaneCallback>::New();
  cb->PolyPlane = polyPlane;
  cb->Cutter = cutter;

  svtkPolyData* data = warp->GetPolyDataOutput();
  double* range = data->GetPointData()->GetScalars()->GetRange();

  //  plot the height field

  svtkSmartPointer<svtkXYPlotActor> profile = svtkSmartPointer<svtkXYPlotActor>::New();
  profile->AddDataSetInputConnection(cutter->GetOutputPort());
  profile->GetPositionCoordinate()->SetValue(0.05, 0.05, 0);
  profile->GetPosition2Coordinate()->SetValue(0.95, 0.95, 0);
  profile->SetXValuesToArcLength();
  profile->SetNumberOfXLabels(6);
  profile->SetTitle("Profile Data ");
  profile->SetXTitle("Arc length");
  profile->SetYTitle("Height");
  profile->SetYRange(range[0], range[1]);
  profile->GetProperty()->SetColor(0, 0, 0);
  profile->GetProperty()->SetLineWidth(2);
  profile->SetLabelFormat("%g");
  svtkTextProperty* tprop = profile->GetTitleTextProperty();
  tprop->SetColor(0.02, 0.06, 0.62);
  tprop->SetFontFamilyToArial();
  profile->SetAxisTitleTextProperty(tprop);
  profile->SetAxisLabelTextProperty(tprop);
  profile->SetTitleTextProperty(tprop);

  ren1->SetBackground(0.1, 0.2, 0.4);
  ren1->SetViewport(0, 0, 0.5, 1);

  ren2->SetBackground(1, 1, 1);
  ren2->SetViewport(0.5, 0, 1, 1);

  renWin->SetSize(800, 500);

  // Set up an interesting viewpoint

  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();
  svtkCamera* camera = ren1->GetActiveCamera();
  camera->SetViewUp(0.796081, -0.277969, 0.537576);
  camera->SetParallelScale(10726.6);
  camera->SetFocalPoint(562412, 5.11456e+06, 1955.44);
  camera->SetPosition(544402, 5.11984e+06, 31359.2);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();

  // Create some default points

  contourWidget->On();

  // First remove all nodes.
  rep->ClearAllNodes();
  rep->AddNodeAtWorldPosition(560846, 5.12018e+06, 2205.95);
  rep->AddNodeAtWorldPosition(562342, 5.11663e+06, 3630.72);
  rep->AddNodeAtWorldPosition(562421, 5.11321e+06, 3156.75);
  rep->AddNodeAtWorldPosition(565885, 5.11067e+06, 2885.73);
  contourWidget->SetWidgetState(svtkContourWidget::Manipulate);

  // Execute the cut
  cb->Execute(contourWidget, 0, nullptr);

  svtkXMLPolyDataWriter* pWriter = svtkXMLPolyDataWriter::New();
  pWriter->SetInputConnection(cutter->GetOutputPort());
  cutter->Update();
  pWriter->SetFileName("CutPolyPlane.vtp");
  pWriter->Write();
  pWriter->SetInputConnection(warp->GetOutputPort());
  pWriter->SetFileName("Dataset.vtp");
  pWriter->Write();
  pWriter->SetInputData(rep->GetContourRepresentationAsPolyData());
  pWriter->SetFileName("Contour.vtp");
  pWriter->Write();
  pWriter->Delete();

  // Observe and update profile when contour widget is interacted with
  contourWidget->AddObserver(svtkCommand::InteractionEvent, cb);

  // Render the image
  iren->Initialize();
  ren2->AddActor2D(profile);
  renWin->Render();
  ren1->ResetCamera();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
