/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGDALRasterReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkGDALRasterReader.h>

// SVTK includes
#include <svtkCellData.h>
#include <svtkCellDataToPointData.h>
#include <svtkCompositePolyDataMapper.h>
#include <svtkDataSetAttributes.h>
#include <svtkDoubleArray.h>
#include <svtkImageActor.h>
#include <svtkInformation.h>
#include <svtkLookupTable.h>
#include <svtkMapper.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkStreamingDemandDrivenPipeline.h>
#include <svtkTestUtilities.h>
#include <svtkUniformGrid.h>

// C++ includes
#include <iterator>
#include <sstream>

// Main program
int TestGDALRasterReader(int argc, char** argv)
{
  const char* rasterFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GIS/raster.tif");

  // Create reader to read shape file.
  svtkNew<svtkGDALRasterReader> reader;
  reader->SetFileName(rasterFileName);
  reader->UpdateInformation();
  // extent in points
  int* extent =
    reader->GetOutputInformation(0)->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  std::ostream_iterator<int> out_it(std::cout, " ");
  std::cout << "Point extents: ";
  std::copy(extent, extent + 6, out_it);
  std::cout << "\n";
  // raster dimensions in cells (pixels)
  int* rasterdims = reader->GetRasterDimensions();
  std::cout << "Cell dimensions: ";
  std::copy(rasterdims, rasterdims + 2, out_it);
  std::cout << std::endl;
  if (extent[1] - extent[0] != rasterdims[0] || extent[3] - extent[2] != rasterdims[1])
  {
    std::cerr << "Error: Number of cells should be one less than the number of points\n";
    return 1;
  }

  // test if we read all 3 bands with CollateBands=0 (default is 1)
  reader->SetCollateBands(0);
  reader->Update();
  svtkUniformGrid* data = svtkUniformGrid::SafeDownCast(reader->GetOutput());
  if (data->GetCellData()->GetNumberOfArrays() != 3)
  {
    std::cerr << "Error: Expecting 3 scalar arrays\n";
    return 1;
  }

  // test if we read only 2 bands once we deselected the first band
  reader->SetCellArrayStatus(reader->GetCellArrayName(0), 0);
  reader->Update();
  data = svtkUniformGrid::SafeDownCast(reader->GetOutput());
  if (data->GetCellData()->GetNumberOfArrays() != 2)
  {
    std::cerr << "Error: Expecting two scalar arrays\n";
    return 1;
  }

  // collate bands
  reader->SetCollateBands(1);
  reader->SetCellArrayStatus(reader->GetCellArrayName(0), 1);
  reader->Update();
  delete[] rasterFileName;

  // We need a renderer
  svtkNew<svtkRenderer> renderer;

  // Get the data
  svtkNew<svtkCellDataToPointData> c2p;
  c2p->SetInputDataObject(reader->GetOutput());
  c2p->Update();

  svtkNew<svtkImageActor> actor;
  actor->SetInputData(svtkUniformGrid::SafeDownCast(c2p->GetOutput()));
  renderer->AddActor(actor);

  // Create a render window, and an interactor
  svtkNew<svtkRenderWindow> renderWindow;
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindow->AddRenderer(renderer);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actor to the scene
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderWindow->SetSize(400, 400);
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
