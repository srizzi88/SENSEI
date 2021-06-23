/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSimplePointsReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkSimplePointsReader and svtkSimplePointsWriter
// .SECTION Description
//

#include "svtkSimplePointsReader.h"
#include "svtkSimplePointsWriter.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

int TestSimplePointsReaderWriter(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create a sphere.
  svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
  sphereSource->Update();

  // Write the data.
  svtkSmartPointer<svtkSimplePointsWriter> writer = svtkSmartPointer<svtkSimplePointsWriter>::New();
  writer->SetInputConnection(sphereSource->GetOutputPort());
  writer->SetFileName("SimplePoints.xyz");
  writer->Write();

  // Create the reader.
  svtkSmartPointer<svtkSimplePointsReader> reader = svtkSmartPointer<svtkSimplePointsReader>::New();
  reader->SetFileName("SimplePoints.xyz");
  reader->Update();

  if (reader->GetOutput()->GetNumberOfPoints() != sphereSource->GetOutput()->GetNumberOfPoints())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
