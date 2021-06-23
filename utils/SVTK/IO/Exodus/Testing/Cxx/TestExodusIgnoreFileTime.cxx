/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExodusIgnoreFileTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkExecutive.h"
#include "svtkExodusIIReader.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTestUtilities.h"

#include <cstdio>
#include <vector>

int TestExodusIgnoreFileTime(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  if (!fname)
  {
    cout << "Could not obtain filename for test data.\n";
    return 1;
  }

  svtkNew<svtkExodusIIReader> reader;
  if (!reader->CanReadFile(fname))
  {
    cout << "Cannot read \"" << fname << "\"\n";
    return 1;
  }
  reader->SetFileName(fname);
  delete[] fname;

  reader->UpdateInformation();

  // Check default time information
  svtkInformation* outInfo = reader->GetExecutive()->GetOutputInformation(0);
  int numSteps = (outInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    ? outInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS())
    : 0;
  std::vector<double> times(numSteps);
  outInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0]);
  if (fabs(times[1] - 0.000100074) > 1e-6)
  {
    std::cerr << "With IgnoreFileTime off, times[1] was " << times[1]
              << " but 0.000100074 was expected." << std::endl;
    return EXIT_FAILURE;
  }

  reader->SetIgnoreFileTime(true);
  reader->UpdateInformation();

  outInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0]);
  if (fabs(times[1] - 1) > 1e-6)
  {
    std::cerr << "With IgnoreFileTime on, times[1] was " << times[1] << " but 1 was expected."
              << std::endl;
    return EXIT_FAILURE;
  }

  // extend test to test for `UseLegacyBlockNames`
  if (reader->GetNumberOfElementBlockArrays() == 0 ||
    strcmp(reader->GetElementBlockArrayName(0), "Unnamed block ID: 1") != 0)
  {
    cerr << "Error! Invalid block names!" << endl;
    return EXIT_FAILURE;
  }

  svtkNew<svtkExodusIIReader> reader2;
  reader2->SetFileName(reader->GetFileName());
  reader2->SetUseLegacyBlockNames(true);
  reader2->UpdateInformation();
  if (reader2->GetNumberOfElementBlockArrays() == 0 ||
    strcmp(reader2->GetElementBlockArrayName(0), "Unnamed block ID: 1 Type: HEX") != 0)
  {
    cerr << "Error! Invalid block names. "
            "Expected 'Unnamed block ID: 1 Type: HEX', got '"
         << reader2->GetElementBlockArrayName(0) << "'" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
