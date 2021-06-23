/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestArrayCalculator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkArrayCalculator.h>
#include <svtkImageData.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkXMLImageDataReader.h>
#include <svtkXMLPolyDataReader.h>

int TestArrayCalculator(int argc, char* argv[])
{
  char* filename =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/disk_out_ref_surface.vtp");

  svtkNew<svtkXMLPolyDataReader> reader;
  reader->SetFileName(filename);
  delete[] filename;
  reader->Update();

  // first calculators job is to create a property whose name could clash
  // with a function
  svtkNew<svtkArrayCalculator> calc;
  calc->SetInputConnection(reader->GetOutputPort());
  calc->SetAttributeTypeToPointData();
  calc->AddScalarArrayName("Pres");
  calc->AddScalarArrayName("Temp");
  calc->SetFunction("Temp * Pres");
  calc->SetResultArrayName("norm");
  calc->Update();

  // now generate a vector with the second calculator
  svtkNew<svtkArrayCalculator> calc2;
  calc2->SetInputConnection(calc->GetOutputPort());
  calc2->SetAttributeTypeToPointData();
  calc2->AddScalarArrayName("Pres");
  calc2->AddScalarArrayName("Temp");
  calc2->AddScalarArrayName("norm");
  calc2->SetFunction("(2 * (Temp*iHat + Pres*jHat + norm*kHat))/2.0");
  calc2->SetResultArrayName("PresVector");
  calc2->Update();

  // now make sure the calculator can use the vector
  // confirm that we don't use "Pres" array, but the "PresVector"
  svtkNew<svtkArrayCalculator> calc3;
  calc3->SetInputConnection(calc2->GetOutputPort());
  calc3->SetAttributeTypeToPointData();
  calc3->AddScalarArrayName("Pres");
  calc3->AddVectorArrayName("PresVector");
  calc3->SetFunction("PresVector");
  calc3->SetResultArrayName("Result");
  calc3->Update();

  // verify the output is correct
  svtkPolyData* result = svtkPolyData::SafeDownCast(calc3->GetOutput());
  if (!result->GetPointData()->HasArray("Result"))
  {
    std::cerr << "Output from calc3 does not have an array named 'Result'" << std::endl;
    return EXIT_FAILURE;
  }

  // Test IgnoreMissingArrays option
  svtkNew<svtkArrayCalculator> calc4;
  calc4->SetInputConnection(calc2->GetOutputPort());
  calc4->SetAttributeTypeToPointData();
  calc4->IgnoreMissingArraysOn();
  calc4->AddScalarArrayName("NonExistant");
  calc4->SetFunction("2*NonExistant");
  calc4->SetResultArrayName("FromNonExistant");
  calc4->Update();

  // Output should have no array named "FromNonExistant"
  result = svtkPolyData::SafeDownCast(calc4->GetOutput());
  if (result->GetPointData()->HasArray("FromNonExistant"))
  {
    std::cerr << "Output from calc4 has an array named 'FromNonExistant'" << std::endl;
    return EXIT_FAILURE;
  }

  // Ensure that multiple variable names can be defined for the same array
  svtkNew<svtkArrayCalculator> calc5;
  calc5->SetInputConnection(calc2->GetOutputPort());
  calc5->SetAttributeTypeToPointData();
  calc5->AddScalarVariable("Pres", "Pres");
  calc5->AddScalarVariable("\"Pres\"", "Pres");
  calc5->SetFunction("Pres + \"Pres\"");
  calc5->SetResultArrayName("TwoPres");
  calc5->Update();

  result = svtkPolyData::SafeDownCast(calc5->GetOutput());
  if (!result->GetPointData()->HasArray("TwoPres"))
  {
    std::cerr << "Output from calc5 has no array named 'TwoPres'" << std::endl;
    return EXIT_FAILURE;
  }

  calc5->RemoveAllVariables();
  calc5->AddVectorVariable("PresVector", "PresVector");
  calc5->AddVectorVariable("\"PresVector\"", "PresVector");
  calc5->SetFunction("PresVector + \"PresVector\"");
  calc5->SetResultArrayName("TwoPresVector");
  calc5->Update();

  result = svtkPolyData::SafeDownCast(calc5->GetOutput());
  if (!result->GetPointData()->HasArray("TwoPresVector"))
  {
    std::cerr << "Output from calc5 has no array named 'TwoPresVector'" << std::endl;
    return EXIT_FAILURE;
  }

  char* filename2 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/wavelet300Arrays.vti");

  svtkNew<svtkXMLImageDataReader> reader2;
  reader2->SetFileName(filename2);
  delete[] filename2;
  reader2->Update();

  // finally, check that a dataset with a lot of arrays is supported
  svtkNew<svtkArrayCalculator> calc6;
  calc6->SetInputConnection(reader2->GetOutputPort());
  calc6->SetAttributeTypeToPointData();
  for (int i = 0; i < reader2->GetNumberOfPointArrays(); i++)
  {
    calc6->AddScalarArrayName(reader2->GetPointArrayName(i));
  }
  calc6->SetFunction("Result224");
  calc6->SetResultArrayName("Result");
  calc6->Update();

  svtkImageData* resultImage = svtkImageData::SafeDownCast(calc6->GetOutput());
  if (!resultImage->GetPointData()->HasArray("Result"))
  {
    std::cerr << "Output from calc6 has no array named 'Result'" << std::endl;
    return EXIT_FAILURE;
  }
  if (resultImage->GetPointData()->GetArray("Result")->GetTuple1(0) != 224)
  {
    std::cerr << "Output from calc6 has an unexpected value" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
