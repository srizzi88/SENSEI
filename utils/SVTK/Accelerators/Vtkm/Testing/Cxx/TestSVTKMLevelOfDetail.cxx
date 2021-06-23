/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Uncomment this to directly compare serial and TBB versions
// #define FORCE_SVTKM_DEVICE

// TODO: Make a way to force the SVTK-m device without actually loading SVTK-m
// headers (and all subsequent dependent headers).

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkContourFilter.h"
#include "svtkDataSetMapper.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkMaskPoints.h"
#include "svtkNew.h"
#include "svtkPLYReader.h"
#include "svtkPNGWriter.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkQuadricClustering.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkTimerLog.h"
#include "svtkTriangleFilter.h"
#include "svtkWindowToImageFilter.h"
#include "svtkXMLPolyDataReader.h"
#include "svtkmLevelOfDetail.h"

#ifdef FORCE_SVTKM_DEVICE

#include <svtkm/cont/RuntimeDeviceTracker.h>

#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/tbb/DeviceAdapterTBB.h>

#endif // FORCE_SVTKM_DEVICE

#include <iomanip>
#include <sstream>

/*
 * This test has benchmarking code as well as a unit test.
 *
 * To run the benchmarks, add a "Benchmark" argument when invoking this test.
 *
 * By default, a wavelet is generated and used to time the filter's execution.
 * By setting the LUCY_PATH define below to the path to lucy.ply (or any other
 * ply file), other datasets can be used during benchmarking.
 *
 * The benchmark will print out timing information comparing svtkmLevelOfDetail
 * to svtkQuadricClustering, and also generate side-by-side renderings of each
 * algorithm for various grid dimensions. These images are written to the
 * working directory can be combined into a summary image by running
 * imagemagick's convert utility:
 *
 * convert LOD_0* -append summary.png
 */

//#define LUCY_PATH "/prm/lucy.ply"

namespace
{

const static int NUM_SAMPLES = 1;
const static int FONT_SIZE = 30;

struct SVTKmFilterGenerator
{
  using FilterType = svtkmLevelOfDetail;

  int GridSize;

  SVTKmFilterGenerator(int gridSize)
    : GridSize(gridSize)
  {
  }

  FilterType* operator()() const
  {
    FilterType* filter = FilterType::New();
    filter->SetNumberOfDivisions(this->GridSize, this->GridSize, this->GridSize);
    return filter;
  }

  svtkSmartPointer<svtkPolyData> Result;
};

struct SVTKFilterGenerator
{
  using FilterType = svtkQuadricClustering;

  int GridSize;
  bool UseInputPoints;
  svtkSmartPointer<svtkPolyData> Result;

  SVTKFilterGenerator(int gridSize, bool useInputPoints)
    : GridSize(gridSize)
    , UseInputPoints(useInputPoints)
  {
  }

  FilterType* operator()() const
  {
    FilterType* filter = FilterType::New();
    filter->SetNumberOfDivisions(this->GridSize, this->GridSize, this->GridSize);

    // Mimic PV's GeometeryRepresentation decimator settings:
    filter->SetAutoAdjustNumberOfDivisions(0);
    filter->SetUseInternalTriangles(0);
    filter->SetCopyCellData(1);
    filter->SetUseInputPoints(this->UseInputPoints ? 1 : 0);

    return filter;
  }
};

template <typename FilterGenerator>
double BenchmarkFilter(FilterGenerator& filterGen, svtkPolyData* input)
{
  using FilterType = typename FilterGenerator::FilterType;

  svtkNew<svtkTimerLog> timer;
  double result = 0.f;

  for (int i = 0; i < NUM_SAMPLES; ++i)
  {
    FilterType* filter = filterGen();
    filter->SetInputData(input);

    timer->StartTimer();
    filter->Update();
    timer->StopTimer();

    result += timer->GetElapsedTime();
    filterGen.Result = filter->GetOutput();
    filter->Delete();
  }

  return result / static_cast<double>(NUM_SAMPLES);
}

void RenderResults(int gridSize, svtkPolyData* input, double svtkmTime, svtkPolyData* svtkmData,
  double svtkTime, svtkPolyData* svtkData)
{
  double modelColor[3] = { 1., 1., 1. };
  double bgColor[3] = { .75, .75, .75 };
  double textColor[3] = { 0., 0., 0. };

  svtkNew<svtkRenderer> svtkRen;
  {
    svtkRen->SetViewport(0., 0., 0.5, 1.);
    svtkRen->SetBackground(bgColor);

    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputData(svtkData);
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->SetColor(modelColor);
    svtkRen->AddActor(actor);

    std::ostringstream tmp;
    tmp << "SVTK: " << std::setprecision(3) << svtkTime << "s\n"
        << "NumPts: " << svtkData->GetNumberOfPoints() << "\n"
        << "NumTri: " << svtkData->GetNumberOfCells() << "\n";

    svtkNew<svtkTextActor> timeText;
    timeText->SetInput(tmp.str().c_str());
    timeText->GetTextProperty()->SetJustificationToCentered();
    timeText->GetTextProperty()->SetColor(textColor);
    timeText->GetTextProperty()->SetFontSize(FONT_SIZE);
    timeText->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    timeText->GetPositionCoordinate()->SetValue(0.5, 0.01);
    svtkRen->AddActor(timeText);
  }

  svtkNew<svtkRenderer> svtkmRen;
  {
    svtkmRen->SetViewport(0.5, 0., 1., 1.);
    svtkmRen->SetBackground(bgColor);

    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputData(svtkmData);
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->SetColor(modelColor);
    svtkmRen->AddActor(actor);

    std::ostringstream tmp;
    tmp << "SVTK-m: " << std::setprecision(3) << svtkmTime << "s\n"
        << "NumPts: " << svtkmData->GetNumberOfPoints() << "\n"
        << "NumTri: " << svtkmData->GetNumberOfCells() << "\n";

    svtkNew<svtkTextActor> timeText;
    timeText->SetInput(tmp.str().c_str());
    timeText->GetTextProperty()->SetJustificationToCentered();
    timeText->GetTextProperty()->SetColor(textColor);
    timeText->GetTextProperty()->SetFontSize(FONT_SIZE);
    timeText->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    timeText->GetPositionCoordinate()->SetValue(0.5, 0.01);
    svtkmRen->AddActor(timeText);
  }

  svtkNew<svtkRenderer> metaRen;
  {
    metaRen->SetPreserveColorBuffer(1);

    std::ostringstream tmp;
    tmp << gridSize << "x" << gridSize << "x" << gridSize << "\n"
        << "InPts: " << input->GetNumberOfPoints() << "\n"
        << "InTri: " << input->GetNumberOfCells() << "\n";

    svtkNew<svtkTextActor> gridText;
    gridText->SetInput(tmp.str().c_str());
    gridText->GetTextProperty()->SetJustificationToCentered();
    gridText->GetTextProperty()->SetVerticalJustificationToTop();
    gridText->GetTextProperty()->SetColor(textColor);
    gridText->GetTextProperty()->SetFontSize(FONT_SIZE);
    gridText->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    gridText->GetPositionCoordinate()->SetValue(0.5, 0.95);
    metaRen->AddActor(gridText);
  }

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(800, 400);
  renWin->AddRenderer(svtkRen);
  renWin->AddRenderer(svtkmRen);
  renWin->AddRenderer(metaRen);

  renWin->Render();

#ifdef LUCY_PATH
  svtkRen->GetActiveCamera()->SetPosition(0, 1, 0);
  svtkRen->GetActiveCamera()->SetViewUp(0, 0, 1);
  svtkRen->GetActiveCamera()->SetFocalPoint(0, 0, 0);
#endif

  svtkRen->ResetCamera();
  svtkRen->GetActiveCamera()->Zoom(2.0);
  svtkmRen->SetActiveCamera(svtkRen->GetActiveCamera());
  renWin->Render();

  svtkNew<svtkWindowToImageFilter> w2i;
  w2i->SetInput(renWin);

  std::ostringstream tmp;
  tmp << "LOD_" << std::setw(4) << std::setfill('0') << std::right << gridSize << ".png";

  svtkNew<svtkPNGWriter> png;
  png->SetInputConnection(w2i->GetOutputPort());
  png->SetFileName(tmp.str().c_str());
  png->Write();
}

void RunBenchmark(int gridSize)
{
  // Prepare input dataset:
  static svtkSmartPointer<svtkPolyData> input;
  if (!input)
  {
#ifndef LUCY_PATH
    svtkNew<svtkRTAnalyticSource> wavelet;
    wavelet->SetXFreq(60);
    wavelet->SetYFreq(30);
    wavelet->SetZFreq(40);
    wavelet->SetXMag(10);
    wavelet->SetYMag(18);
    wavelet->SetZMag(5);
    wavelet->SetWholeExtent(-255, 256, -255, 256, -127, 128);
    svtkNew<svtkContourFilter> contour;
    contour->SetInputConnection(wavelet->GetOutputPort());
    contour->SetNumberOfContours(1);
    contour->SetValue(0, 157.);
    contour->Update();
    input = contour->GetOutput();
#else
    svtkNew<svtkPLYReader> reader;
    reader->SetFileName(LUCY_PATH);
    reader->Update();
    input = reader->GetOutput();
#endif
  }

#ifdef FORCE_SVTKM_DEVICE

  svtkm::cont::RuntimeDeviceTracker tracker = svtkm::cont::GetRuntimeDeviceTracker();

  // Run SVTKm
  svtkSmartPointer<svtkPolyData> svtkmResultSerial;
  double svtkmTimeSerial = 0.;
  {
    tracker.ForceDevice(svtkm::cont::DeviceAdapterTagSerial());
    SVTKmFilterGenerator generator(gridSize);
    svtkmTimeSerial = BenchmarkFilter(generator, input);
    svtkmResultSerial = generator.Result;
    tracker.Reset();
  }

#ifdef SVTKM_ENABLE_TBB
  svtkSmartPointer<svtkPolyData> svtkmResultTBB;
  double svtkmTimeTBB = 0.;
  bool tbbDeviceValid = tracker.CanRunOn(svtkm::cont::DeviceAdapterTagTBB());
  if (tbbDeviceValid)
  {
    tracker.ForceDevice(svtkm::cont::DeviceAdapterTagTBB());
    SVTKmFilterGenerator generator(gridSize);
    svtkmTimeTBB = BenchmarkFilter(generator, input);
    svtkmResultTBB = generator.Result;
    tracker.Reset();
  }
#endif // SVTKM_ENABLE_TBB

#else // !FORCE_SVTKM_DEVICE

  // Run SVTKm
  svtkSmartPointer<svtkPolyData> svtkmResult;
  double svtkmTime = 0.;
  {
    SVTKmFilterGenerator generator(gridSize);
    svtkmTime = BenchmarkFilter(generator, input);
    svtkmResult = generator.Result;
  }

#endif

  // Run SVTK -- average clustered points
  svtkSmartPointer<svtkPolyData> svtkResultAvePts;
  double svtkTimeAvePts = 0.;
  {
    SVTKFilterGenerator generator(gridSize, false);
    svtkTimeAvePts = BenchmarkFilter(generator, input);
    svtkResultAvePts = generator.Result;
  }

  // Run SVTK -- reuse input points
  svtkSmartPointer<svtkPolyData> svtkResult;
  double svtkTime = 0.;
  {
    SVTKFilterGenerator generator(gridSize, true);
    svtkTime = BenchmarkFilter(generator, input);
    svtkResult = generator.Result;
  }

  std::cerr << "Results for a " << gridSize << "x" << gridSize << "x" << gridSize << " grid.\n"
            << "Input dataset has " << input->GetNumberOfPoints()
            << " points "
               "and "
            << input->GetNumberOfCells() << " cells.\n";

#ifdef FORCE_SVTKM_DEVICE

  std::cerr << "svtkmLevelOfDetail (serial, average clustered points): " << svtkmTimeSerial
            << " seconds, " << svtkmResultSerial->GetNumberOfPoints() << " points, "
            << svtkmResultSerial->GetNumberOfCells() << " cells.\n";

#ifdef SVTKM_ENABLE_TBB
  if (tbbDeviceValid)
  {
    std::cerr << "svtkmLevelOfDetail (tbb, average clustered points): " << svtkmTimeTBB
              << " seconds, " << svtkmResultTBB->GetNumberOfPoints() << " points, "
              << svtkmResultTBB->GetNumberOfCells() << " cells.\n";
  }
#endif // SVTKM_ENABLE_TBB

#else // !FORCE_SVTKM_DEVICE

  std::cerr << "svtkmLevelOfDetail (average clustered points): " << svtkmTime << " seconds, "
            << svtkmResult->GetNumberOfPoints() << " points, " << svtkmResult->GetNumberOfCells()
            << " cells.\n";

#endif // !FORCE_SVTKM_DEVICE

  std::cerr << "svtkQuadricClustering (average clustered points): " << svtkTimeAvePts << " seconds, "
            << svtkResultAvePts->GetNumberOfPoints() << " points, "
            << svtkResultAvePts->GetNumberOfCells() << " cells.\n"
            << "svtkQuadricClustering (reuse input points): " << svtkTime << " seconds, "
            << svtkResult->GetNumberOfPoints() << " points, " << svtkResult->GetNumberOfCells()
            << " cells.\n";

#ifdef FORCE_SVTKM_DEVICE
#ifdef SVTKM_ENABLE_TBB
  RenderResults(gridSize, input, svtkmTimeTBB, svtkmResultTBB, svtkTime, svtkResult);
#endif // SVTKM_ENABLE_TBB
#else  // !FORCE_SVTKM_DEVICE
  RenderResults(gridSize, input, svtkmTime, svtkmResult, svtkTime, svtkResult);
#endif // !FORCE_SVTKM_DEVICE
}

void RunBenchmarks()
{
  RunBenchmark(32);
  RunBenchmark(64);
  RunBenchmark(128);
  RunBenchmark(256);
  RunBenchmark(512);
}

} // end anon namespace

int TestSVTKMLevelOfDetail(int argc, char* argv[])
{
  bool doBenchmarks = false;

  for (int i = 1; i < argc; ++i)
  {
    if (std::string("Benchmark") == argv[i])
    {
      doBenchmarks = true;
      break;
    }
  }

  if (doBenchmarks)
  {
    RunBenchmarks();
    return 0;
  }

  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  //---------------------------------------------------
  // Load file and make only triangles
  //---------------------------------------------------
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
  svtkNew<svtkXMLPolyDataReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  svtkNew<svtkTriangleFilter> clean;
  clean->SetInputConnection(reader->GetOutputPort());
  clean->Update();

  //---------------------------------------------------
  // Test LOD filter 4 times
  // We will setup 4 instances of the filter at different
  // levels of subdivision to make sure it is working properly
  //---------------------------------------------------

  std::vector<svtkNew<svtkmLevelOfDetail> > levelOfDetails(4);
  std::vector<svtkNew<svtkDataSetSurfaceFilter> > surfaces(4);
  std::vector<svtkNew<svtkPolyDataMapper> > mappers(4);
  std::vector<svtkNew<svtkActor> > actors(4);

  for (int i = 0; i < 4; ++i)
  {
    levelOfDetails[i]->SetInputConnection(clean->GetOutputPort());
    // subdivision levels of 16, 32, 48, 64
    levelOfDetails[i]->SetNumberOfXDivisions(((i + 1) * 16));
    levelOfDetails[i]->SetNumberOfYDivisions(((i + 1) * 16));
    levelOfDetails[i]->SetNumberOfZDivisions(((i + 1) * 16));

    surfaces[i]->SetInputConnection(levelOfDetails[i]->GetOutputPort());

    mappers[i]->SetInputConnection(surfaces[i]->GetOutputPort());

    actors[i]->SetMapper(mappers[i]);
    actors[i]->SetPosition((i % 2) * 10, -(i / 2) * 10, 0);

    ren->AddActor(actors[i]);
  }

  ren->SetBackground(0.1, 0.2, 0.4);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.3);
  renWin->SetSize(600, 600);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return (!retVal);
}
