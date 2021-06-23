/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the ImageConnectivityFilter class
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkCamera.h"
#include "svtkIdTypeArray.h"
#include "svtkImageConnectivityFilter.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageReader2.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkIntArray.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVersion.h"

#include "svtkTestUtilities.h"

#include <string>

int TestImageConnectivityFilter(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkInteractorStyleImage> style = svtkSmartPointer<svtkInteractorStyleImage>::New();
  style->SetInteractionModeToImageSlicing();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Use a 3D image for the test
  char* temp = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  std::string fname = temp;
  delete[] temp;

  svtkSmartPointer<svtkImageReader2> reader = svtkSmartPointer<svtkImageReader2>::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 2, 4);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname.c_str());

  // Create two seed points
  svtkSmartPointer<svtkPoints> seedPoints = svtkSmartPointer<svtkPoints>::New();
  seedPoints->InsertNextPoint(25.6, 100.8, 2.25);
  seedPoints->InsertNextPoint(100.8, 100.8, 2.25);
  svtkSmartPointer<svtkUnsignedCharArray> seedScalars = svtkSmartPointer<svtkUnsignedCharArray>::New();
  seedScalars->InsertNextValue(2);
  seedScalars->InsertNextValue(5);
  svtkSmartPointer<svtkPolyData> seedData = svtkSmartPointer<svtkPolyData>::New();
  seedData->SetPoints(seedPoints);
  seedData->GetPointData()->SetScalars(seedScalars);

  // Generate a grid of renderers for the various tests
  for (int i = 0; i < 9; i++)
  {
    int j = 2 - i / 3;
    int k = i % 3;
    svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.0, 0.0, 0.0);
    renderer->SetViewport(k / 3.0, j / 3.0, (k + 1) / 3.0, (j + 1) / 3.0);
    renWin->AddRenderer(renderer);

    svtkSmartPointer<svtkImageConnectivityFilter> connectivity =
      svtkSmartPointer<svtkImageConnectivityFilter>::New();
    connectivity->SetInputConnection(reader->GetOutputPort());

    if (i == 0)
    {
      connectivity->GenerateRegionExtentsOn();
      connectivity->SetScalarRange(800, 1200);
      // No seeds
      // Default extraction mode
      // Default label mode
    }
    else if (i == 1)
    {
      connectivity->SetScalarRange(800, 1200);
      // No seeds
      connectivity->SetExtractionModeToLargestRegion();
      // Default label mode
    }
    else if (i == 2)
    {
      connectivity->SetScalarRange(800, 1200);
      // No seeds
      connectivity->SetSizeRange(10, 99);
      // Default label mode
    }
    else if (i == 3)
    {
      connectivity->SetScalarRange(800, 1200);
      connectivity->SetSeedData(seedData);
      // Default extraction mode
      // Default label mode (use seed scalars)
    }
    else if (i == 4)
    {
      connectivity->SetScalarRange(800, 1200);
      connectivity->SetSeedData(seedData);
      connectivity->SetExtractionModeToAllRegions();
      connectivity->SetLabelModeToSizeRank();
    }
    else if (i == 5)
    {
      // Seeds with no scalars
      connectivity->SetScalarRange(800, 1200);
      seedData->GetPointData()->SetScalars(nullptr);
      connectivity->SetSeedData(seedData);
    }
    else if (i == 6)
    {
      connectivity->SetScalarRange(1200, 4095);
    }
    else if (i == 7)
    {
      connectivity->SetScalarRange(0, 800);
    }
    else if (i == 8)
    {
      // use default scalar range
    }

    if (i == 0)
    {
      // Test OutputExtent != InputExtent
      int extent[6] = { 0, 63, 0, 63, 3, 3 };
      connectivity->UpdateExtent(extent);
    }
    else
    {
      // Test updating whole extent
      connectivity->Update();
    }

    // Test getting info about the output regions
    svtkIdTypeArray* sizeArray = connectivity->GetExtractedRegionSizes();
    svtkIdTypeArray* idArray = connectivity->GetExtractedRegionSeedIds();
    svtkIdTypeArray* labelArray = connectivity->GetExtractedRegionLabels();
    svtkIntArray* extentArray = connectivity->GetExtractedRegionExtents();
    svtkIdType rn = connectivity->GetNumberOfExtractedRegions();
    std::cout << "\nTest Case: " << i << std::endl;
    std::cout << "number of regions: " << rn << std::endl;
    for (svtkIdType r = 0; r < rn; r++)
    {
      std::cout << "region: " << r << ","
                << " seed: " << idArray->GetValue(r) << ","
                << " label: " << labelArray->GetValue(r) << ","
                << " size: " << sizeArray->GetValue(r) << ","
                << " extent: [";
      if (connectivity->GetGenerateRegionExtents())
      {
        std::cout << extentArray->GetValue(6 * r) << "," << extentArray->GetValue(6 * r + 1) << ","
                  << extentArray->GetValue(6 * r + 2) << "," << extentArray->GetValue(6 * r + 3)
                  << "," << extentArray->GetValue(6 * r + 4) << ","
                  << extentArray->GetValue(6 * r + 5);
      }
      std::cout << "]" << std::endl;
    }

    svtkSmartPointer<svtkImageSliceMapper> imageMapper = svtkSmartPointer<svtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(connectivity->GetOutputPort());
    imageMapper->BorderOn();
    imageMapper->SliceFacesCameraOn();
    imageMapper->SliceAtFocalPointOn();

    double point[3] = { 100.8, 100.8, 5.25 };
    camera->SetFocalPoint(point);
    point[2] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(3.2 * 32);

    svtkSmartPointer<svtkImageSlice> image = svtkSmartPointer<svtkImageSlice>::New();
    image->SetMapper(imageMapper);
    image->GetProperty()->SetColorWindow(6);
    image->GetProperty()->SetColorLevel(3);
    renderer->AddViewProp(image);
  }

  renWin->SetSize(192, 256);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
