/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositePolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkColorTransferFunction.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositePolyDataMapper2.h"
#include "svtkCylinderSource.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

// Test for multiblock data sets with field data arrays defined on
// only a subset of the blocks. The expected behavior is to have
// coloring by scalars on the blocks with the data array and coloring
// as though scalar mapping is turned off in the blocks without the
// data array.
int TestMultiBlockPartialArrayFieldData(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindow> win = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  // Components of the multiblock data set
  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetRadius(2.0);

  svtkNew<svtkCylinderSource> cylinderSource;
  cylinderSource->SetRadius(1.5);
  cylinderSource->SetHeight(2.0);
  cylinderSource->SetResolution(32);

  // Set up the multiblock data set consisting of a ring of blocks
  svtkSmartPointer<svtkMultiBlockDataSet> data = svtkSmartPointer<svtkMultiBlockDataSet>::New();

  int numBlocks = 16;
  data->SetNumberOfBlocks(numBlocks);

  double radius = 10.0;
  double deltaTheta = 2.0 * svtkMath::Pi() / numBlocks;
  for (int i = 0; i < numBlocks; ++i)
  {
    double theta = i * deltaTheta;
    double x = radius * cos(theta);
    double y = radius * sin(theta);

    svtkPolyData* pd = svtkPolyData::New();

    // Every third block does not have the color array
    if (i % 3 == 0)
    {
      sphereSource->SetCenter(x, y, 0.0);
      sphereSource->Update();
      pd->DeepCopy(sphereSource->GetOutput());
    }
    else
    {
      cylinderSource->SetCenter(x, y, 0.0);
      cylinderSource->Update();
      pd->DeepCopy(cylinderSource->GetOutput());

      // Add a field data array
      svtkSmartPointer<svtkDoubleArray> dataArray = svtkSmartPointer<svtkDoubleArray>::New();
      dataArray->SetName("mydata");
      dataArray->SetNumberOfComponents(1);
      dataArray->SetNumberOfTuples(1);
      dataArray->InsertValue(0, static_cast<double>(i));

      pd->GetFieldData()->AddArray(dataArray);
    }
    data->SetBlock(i, pd);
    pd->Delete();
  }

  svtkNew<svtkColorTransferFunction> lookupTable;
  lookupTable->AddRGBPoint(0.0, 1.0, 1.0, 1.0);
  lookupTable->AddRGBPoint(static_cast<double>(numBlocks - 1), 0.0, 1.0, 0.0);

  svtkSmartPointer<svtkCompositePolyDataMapper2> mapper =
    svtkSmartPointer<svtkCompositePolyDataMapper2>::New();
  mapper->SetInputDataObject(data);

  // Tell mapper to use field data for rendering
  mapper->SetLookupTable(lookupTable);
  mapper->SetFieldDataTupleId(0);
  mapper->SelectColorArray("mydata");
  mapper->SetScalarModeToUseFieldData();
  mapper->UseLookupTableScalarRangeOn();
  mapper->ScalarVisibilityOn();

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.67, 1.0);

  ren->AddActor(actor);
  win->SetSize(400, 400);

  ren->ResetCamera();

  win->Render();

  int retVal = svtkRegressionTestImageThreshold(win, 15);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
