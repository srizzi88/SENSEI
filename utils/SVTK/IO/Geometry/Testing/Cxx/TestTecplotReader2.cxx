/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTecPlotReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkTecplotReader
// .SECTION Description
//

#include "svtkDebugLeaks.h"
#include "svtkTecplotReader.h"

#include "svtkActor.h"
#include "svtkArrayIterator.h"
#include "svtkArrayIteratorTemplate.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDirectory.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTestUtilities.h"
#include <string>
#include <svtksys/SystemTools.hxx>

using namespace std;

class svtkErrorObserver
{

public:
  static void Reset()
  {
    HasError = false;
    ErrorMessage.clear();
  }

  static void OnError(svtkObject* svtkNotUsed(caller), unsigned long int svtkNotUsed(eventId),
    void* svtkNotUsed(clientData), void* callData)
  {
    HasError = true;
    char* pString = (char*)callData;
    if (pString)
    {
      ErrorMessage = pString;
    }
  }

  static bool HasError;
  static string ErrorMessage;
};

bool svtkErrorObserver::HasError = false;
string svtkErrorObserver::ErrorMessage = string();

int TestTecplotReader2(int argc, char* argv[])
{
  char* dataRoot = svtkTestUtilities::GetDataRoot(argc, argv);
  const string tecplotDir = string(dataRoot) + "/Data/TecPlot/";

  if (argc < 2)
  {
    return EXIT_SUCCESS;
  }

  const char* filename = argv[1];

  svtkNew<svtkCallbackCommand> cmd;
  cmd->SetCallback(&(svtkErrorObserver::OnError));

  const string ext = svtksys::SystemTools::GetFilenameLastExtension(filename);
  if (ext != ".dat")
  {
    return EXIT_FAILURE;
  }

  svtkErrorObserver::Reset();
  svtkNew<svtkTecplotReader> r;
  r->AddObserver("ErrorEvent", cmd);
  r->SetFileName((tecplotDir + filename).c_str());
  r->Update();
  r->RemoveAllObservers();

  svtkMultiBlockDataSet* ds = r->GetOutput();
  if (ds == nullptr)
  {
    cerr << "Failed to read data set from " << filename << endl;
    return EXIT_FAILURE;
  }
  if (svtkErrorObserver::HasError)
  {
    cerr << "Failed to read from " << filename << endl;
    if (!svtkErrorObserver::ErrorMessage.empty())
    {
      cerr << "Error message: " << svtkErrorObserver::ErrorMessage << endl;
    }
    return EXIT_FAILURE;
  }

  cout << filename << " was read without errors." << endl;
  delete[] dataRoot;
  return EXIT_SUCCESS;
}
