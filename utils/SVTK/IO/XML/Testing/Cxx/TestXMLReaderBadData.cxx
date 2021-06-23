/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLReaderBadData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTestErrorObserver.h"
#include <svtkSmartPointer.h>
#include <svtkXMLDataParser.h>
#include <svtkXMLGenericDataObjectReader.h>

int TestXMLReaderBadData(int argc, char* argv[])
{
  // Verify input arguments
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " Filename" << std::endl;
    return EXIT_FAILURE;
  }

  std::string inputFilename = argv[1];

  // Observe errors
  svtkSmartPointer<svtkTest::ErrorObserver> errorObserver0 =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();
  svtkSmartPointer<svtkTest::ErrorObserver> errorObserver1 =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();
  svtkSmartPointer<svtkTest::ErrorObserver> errorObserver2 =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();

  // Read the file
  svtkSmartPointer<svtkXMLGenericDataObjectReader> reader =
    svtkSmartPointer<svtkXMLGenericDataObjectReader>::New();
  reader->SetFileName(inputFilename.c_str());
  reader->AddObserver(svtkCommand::ErrorEvent, errorObserver0);
  reader->SetReaderErrorObserver(errorObserver1);
  reader->SetParserErrorObserver(errorObserver2);
  reader->Update();
  int status = errorObserver2->CheckErrorMessage("svtkXMLDataParser");
  return status;
}
