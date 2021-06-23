/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleToImage2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This is just a simple test. svtkResampleToImage internally uses
// svtkProbeFilter, which is tested thoroughly in other tests.

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkPointData.h"
#include "svtkResampleToImage.h"
#include "svtkTestUtilities.h"
#include "svtkXMLUnstructuredGridReader.h"

int TestResampleToImage2D(int argc, char* argv[])
{
  svtkNew<svtkXMLUnstructuredGridReader> reader;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/delaunay3d.vtu");
  reader->SetFileName(fname);
  delete[] fname;

  svtkNew<svtkResampleToImage> resample;
  resample->UseInputBoundsOff();
  resample->SetInputConnection(reader->GetOutputPort());

  double* range = nullptr;

  // test on X
  resample->SetSamplingBounds(0.0, 0.0, -10.0, 10.0, -10.0, 10.0);
  resample->SetSamplingDimensions(1, 100, 100);
  resample->Update();
  range = resample->GetOutput()->GetPointData()->GetArray("BrownianVectors")->GetRange();
  if (range[1] - range[0] < 0.01)
  {
    cerr << "Error resampling along X" << endl;
  }

  // test on Y
  resample->SetSamplingBounds(-10.0, 10.0, 0.0, 0.0, -10.0, 10.0);
  resample->SetSamplingDimensions(100, 1, 100);
  resample->Update();
  range = resample->GetOutput()->GetPointData()->GetArray("BrownianVectors")->GetRange();
  if (range[1] - range[0] < 0.01)
  {
    cerr << "Error resampling along Y" << endl;
  }

  // test on Z
  resample->SetSamplingBounds(-10.0, 10.0, -10.0, 10.0, 0.0, 0.0);
  resample->SetSamplingDimensions(100, 100, 1);
  resample->Update();
  range = resample->GetOutput()->GetPointData()->GetArray("BrownianVectors")->GetRange();
  if (range[1] - range[0] < 0.01)
  {
    cerr << "Error resampling along Z" << endl;
  }

  return EXIT_SUCCESS;
}
