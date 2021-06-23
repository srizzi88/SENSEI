/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGDALVectorReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGDALVectorReader.h"

// SVTK includes
#include <svtkActor.h>
#include <svtkCellData.h>
#include <svtkCompositePolyDataMapper.h>
#include <svtkDataSetAttributes.h>
#include <svtkDoubleArray.h>
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
#include <svtkTestUtilities.h>

// C++ includes
#include <sstream>

// Main program
int TestGDALVectorReader(int argc, char* argv[])
{
  const char* vectorFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GIS/countries.shp");

  // Create reader to read shape file.
  svtkNew<svtkGDALVectorReader> reader;
  reader->SetFileName(vectorFileName);
  reader->AddFeatureIdsOn();
  delete[] vectorFileName;

  // Test layer information helpers
  reader->UpdateInformation();
  int nl = reader->GetNumberOfLayers();
  for (int i = 0; i < nl; ++i)
  {
    reader->SetActiveLayer(i);
    cout << "Layer " << i << " Type " << reader->GetActiveLayerType() << " FeatureCount "
         << reader->GetActiveLayerFeatureCount() << "\n";
  }
  reader->SetActiveLayer(0); // Read only layer 0, which is the only layer.
  reader->Update();

  // We need a renderer
  svtkNew<svtkRenderer> renderer;

  // Get the data
  svtkSmartPointer<svtkMultiBlockDataSet> mbds = reader->GetOutput();

  // Verify that feature IDs exist as a scalar (assuming first block exists)
  if (mbds && mbds->GetNumberOfBlocks() > 0)
  {
    svtkPolyData* pd = svtkPolyData::SafeDownCast(mbds->GetBlock(0));
    svtkCellData* cd = pd ? pd->GetCellData() : nullptr;
    if (cd)
    {
      if (!cd->GetPedigreeIds())
      {
        cerr << "Unable to find pedigree IDs even though AddFeatureIds was ON\n";
        return 1;
      }
    }
  }

  // Create scene
  svtkNew<svtkActor> actor;
  svtkNew<svtkCompositePolyDataMapper> mapper;

  // Create an interesting lookup table
  svtkNew<svtkLookupTable> lut;
  lut->SetTableRange(1.0, 8.0);
  lut->SetValueRange(0.6, 0.9);
  lut->SetHueRange(0.0, 0.8);
  lut->SetSaturationRange(0.0, 0.7);
  lut->SetNumberOfColors(8);
  lut->Build();

  mapper->SetInputDataObject(mbds);
  mapper->SelectColorArray("mapcolor8");
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SetScalarVisibility(1);
  mapper->UseLookupTableScalarRangeOn();
  mapper->SetLookupTable(lut);
  mapper->SetColorModeToMapScalars();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetLineWidth(1.4f);
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
