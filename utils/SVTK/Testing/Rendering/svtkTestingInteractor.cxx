/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTestingInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTestingInteractor.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkSmartPointer.h"
#include "svtkTesting.h"

svtkStandardNewMacro(svtkTestingInteractor);

int svtkTestingInteractor::TestReturnStatus = -1;
double svtkTestingInteractor::ErrorThreshold = 10.0;
std::string svtkTestingInteractor::ValidBaseline;
std::string svtkTestingInteractor::TempDirectory;
std::string svtkTestingInteractor::DataDirectory;

//----------------------------------------------------------------------------------
// Start normally starts an event loop. This iterator uses svtkTesting
// to grab the render window and compare the results to a baseline image
void svtkTestingInteractor::Start()
{
  svtkSmartPointer<svtkTesting> testing = svtkSmartPointer<svtkTesting>::New();
  testing->SetRenderWindow(this->GetRenderWindow());

  // Location of the temp directory for testing
  testing->AddArgument("-T");
  testing->AddArgument(svtkTestingInteractor::TempDirectory.c_str());

  // Location of the Data directory. If NOTFOUND, suppress regression
  // testing
  if (svtkTestingInteractor::DataDirectory != "SVTK_DATA_ROOT-NOTFOUND")
  {
    testing->AddArgument("-D");
    testing->AddArgument(svtkTestingInteractor::DataDirectory.c_str());

    // The name of the valid baseline image
    testing->AddArgument("-V");
    std::string valid = svtkTestingInteractor::ValidBaseline;
    testing->AddArgument(valid.c_str());

    // Regression test the image
    svtkTestingInteractor::TestReturnStatus =
      testing->RegressionTest(svtkTestingInteractor::ErrorThreshold);
  }
}

//----------------------------------------------------------------------------------
void svtkTestingInteractor::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
