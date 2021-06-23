/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSegY2DReaderZoom.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkSegYReader
// .SECTION Description
//

#include "svtkSegYReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkDataSetMapper.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkMathUtilities.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStructuredGrid.h"
#include "svtkTestUtilities.h"

int TestSegY2DReaderZoom(int argc, char* argv[])
{
  // Basic visualisation.
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Read file name.
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineA.sgy");

  svtkNew<svtkSegYReader> reader;
  svtkNew<svtkDataSetMapper> mapper;
  svtkNew<svtkActor> actor;

  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  int retVal = 0;

  double range[2];
  svtkDataSet* output = reader->GetOutput();

  output->GetScalarRange(range);

  // Test against expected scalar range
  if (!svtkMathUtilities::FuzzyCompare<float>(range[0], -6.421560287))
  {
    std::cerr << "Error: Range[0] != -6.42156" << std::endl
              << "Range[0] = " << range[0] << std::endl;
    retVal++;
  }
  if (!svtkMathUtilities::FuzzyCompare<float>(range[1], 6.616714478))
  {
    std::cerr << "Error: Range[1] != 6.61671" << std::endl
              << "Range[1] = " << range[1] << std::endl;
    retVal++;
  }

  // Test the Z-coordinate range for VerticalCRS
  double bounds[6];
  output->GetBounds(bounds);

  if (!svtkMathUtilities::FuzzyCompare<double>(bounds[4], -4000.00) || (bounds[5] > 1e-3))
  {
    std::cerr << "Error: Z bounds are incorrect: (" << bounds[4] << ", " << bounds[5] << ")"
              << std::endl
              << "Expected Z bounds: (-4000, 0)" << std::endl;
  }

  // Test some scalar values
  svtkFloatArray* scalars = svtkFloatArray::SafeDownCast(output->GetPointData()->GetScalars());
  float scalar = scalars->GetVariantValue(390 * 39).ToFloat();
  if (!svtkMathUtilities::FuzzyCompare<float>(scalar, 0.0676235f))
  {
    std::cerr << "Error: Trace value for 39th sample is wrong." << std::endl
              << "trace[390*39] = " << std::setprecision(10) << scalar << std::endl
              << "Expected trace[390*39] = 0.0676235f" << std::endl;
    retVal++;
  }

  scalar = scalars->GetVariantValue(390 * 390).ToFloat();
  if (!svtkMathUtilities::FuzzyCompare<float>(scalar, 0.6201947331f))
  {
    std::cerr << "Error: Trace value for 390th sample is wrong." << std::endl
              << "trace[390*390] = " << std::setprecision(10) << scalar << std::endl
              << "Expected trace[390*390] = 0.620195f" << std::endl;
    retVal++;
  }

  double midrange = 0.5 * (range[0] + range[1]);

  svtkNew<svtkColorTransferFunction> lut;
  lut->AddRGBPoint(range[0], 0.23, 0.30, 0.75);
  lut->AddRGBPoint(midrange, 0.86, 0.86, 0.86);
  lut->AddRGBPoint(range[1], 0.70, 0.02, 0.15);

  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  mapper->SetLookupTable(lut);

  actor->SetMapper(mapper);

  ren->AddActor(actor);
  ren->ResetCamera();

  ren->GetActiveCamera()->Azimuth(90);
  ren->GetActiveCamera()->Zoom(45.0);

  // interact with data
  renWin->Render();

  int regRetVal = svtkRegressionTestImage(renWin);

  if (regRetVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  retVal += !regRetVal;
  return retVal;
}
