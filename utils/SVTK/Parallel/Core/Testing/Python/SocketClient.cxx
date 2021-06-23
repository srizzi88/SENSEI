/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SocketClient.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkContourFilter.h"
#include "svtkDataSetMapper.h"
#include "svtkDebugLeaks.h"
#include "svtkDoubleArray.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRectilinearGrid.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSocketCommunicator.h"
#include "svtkSocketController.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"

#include "svtkRenderWindowInteractor.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include "ExerciseMultiProcessController.h"

static const int scMsgLength = 10;

static void CleanUp(svtkSmartPointer<svtkSocketCommunicator> svtkNotUsed(comm),
  svtkSmartPointer<svtkSocketController> svtkNotUsed(contr))
{
  // This will close the connection as well as delete
  // the communicator
  // Deleting no longer necessary with smart pointers.
  //   comm->Delete();
  //   contr->Delete();
}

int main(int argc, char** argv)
{
  SVTK_CREATE(svtkSocketController, contr);
  contr->Initialize();

  SVTK_CREATE(svtkSocketCommunicator, comm);

  int i;

  // Get the host name from the command line arguments
  char* hostname = new char[30];
  strcpy(hostname, "localhost");
  int dataIndex = -1;
  for (i = 0; i < argc; i++)
  {
    if (strcmp("-H", argv[i]) == 0)
    {
      if (i < argc - 1)
      {
        dataIndex = i + 1;
      }
    }
  }
  if (dataIndex != -1)
  {
    delete[] hostname;
    hostname = new char[strlen(argv[dataIndex]) + 1];
    strcpy(hostname, argv[dataIndex]);
  }

  // Get the port from the command line arguments
  int port = 11111;

  dataIndex = -1;
  for (i = 0; i < argc; i++)
  {
    if (strcmp("-P", argv[i]) == 0)
    {
      if (i < argc - 1)
      {
        dataIndex = i + 1;
      }
    }
  }
  if (dataIndex != -1)
  {
    port = atoi(argv[dataIndex]);
  }

  // Establish connection
  if (!comm->ConnectTo(hostname, port))
  {
    cerr << "Client error: Could not connect to the server." << endl;
    delete[] hostname;
    return 1;
  }
  delete[] hostname;

  // Test sending all supported types of arrays
  int datai[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datai[i] = i;
  }
  if (!comm->Send(datai, scMsgLength, 1, 11))
  {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  unsigned long dataul[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    dataul[i] = static_cast<unsigned long>(i);
  }
  if (!comm->Send(dataul, scMsgLength, 1, 22))
  {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  char datac[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datac[i] = static_cast<char>(i);
  }
  if (!comm->Send(datac, scMsgLength, 1, 33))
  {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  unsigned char datauc[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datauc[i] = static_cast<unsigned char>(i);
  }
  if (!comm->Send(datauc, scMsgLength, 1, 44))
  {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  float dataf[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    dataf[i] = static_cast<float>(i);
  }
  if (!comm->Send(dataf, scMsgLength, 1, 7))
  {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  double datad[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datad[i] = static_cast<double>(i);
  }
  if (!comm->Send(datad, scMsgLength, 1, 7))
  {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  svtkIdType datait[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datait[i] = static_cast<svtkIdType>(i);
  }
  if (!comm->Send(datait, scMsgLength, 1, 7))
  {
    cerr << "Client error: Error sending data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  // Test receiving svtkDataObject
  SVTK_CREATE(svtkUnstructuredGrid, ugrid);

  if (!comm->Receive(ugrid, 1, 9))
  {
    cerr << "Client error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }

  SVTK_CREATE(svtkDataSetMapper, umapper);
  umapper->SetInputData(ugrid);

  SVTK_CREATE(svtkActor, uactor);
  uactor->SetMapper(umapper);
  uactor->SetPosition(5, 0, 0);
  uactor->SetScale(0.2, 0.2, 0.2);

  // Test receiving svtkDataArray
  SVTK_CREATE(svtkDoubleArray, da);
  if (!comm->Receive(da, 1, 9))
  {
    cerr << "Client error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }
  for (i = 0; i < 40; i++)
  {
    if (da->GetValue(i) != static_cast<double>(i))
    {
      cerr << "Server error: Corrupt svtkDoubleArray." << endl;
      CleanUp(comm, contr);
      return 1;
    }
  }

  // Test receiving null svtkDataArray
  SVTK_CREATE(svtkDoubleArray, da2);
  if (!comm->Receive(da2, 1, 9))
  {
    cerr << "Client error: Error receiving data." << endl;
    CleanUp(comm, contr);
    return 1;
  }
  if (da2->GetNumberOfTuples() == 0)
  {
    cout << "receive null data array successful" << endl;
  }
  else
  {
    cout << "receive null data array failed" << endl;
  }

  contr->SetCommunicator(comm);

  // The following lines were added for coverage
  // These methods have empty implementations
  contr->SingleMethodExecute();
  contr->MultipleMethodExecute();
  contr->CreateOutputWindow();
  contr->Barrier();
  contr->Finalize();

  // Run the socket through the standard controller tests.  We have to make a
  // compliant controller first.
  int retVal;
  svtkMultiProcessController* compliantController = contr->CreateCompliantController();
  retVal = ExerciseMultiProcessController(compliantController);
  compliantController->Delete();
  if (retVal)
  {
    CleanUp(comm, contr);
    return retVal;
  }

  SVTK_CREATE(svtkPolyDataMapper, pmapper);
  SVTK_CREATE(svtkPolyData, pd);
  comm->Receive(pd, 1, 11);
  pmapper->SetInputData(pd);

  SVTK_CREATE(svtkActor, pactor);
  pactor->SetMapper(pmapper);

  SVTK_CREATE(svtkDataSetMapper, rgmapper);
  SVTK_CREATE(svtkRectilinearGrid, rg);
  comm->Receive(rg, 1, 11);
  rgmapper->SetInputData(rg);

  SVTK_CREATE(svtkActor, rgactor);
  rgactor->SetMapper(rgmapper);
  rgactor->SetPosition(0, -5, 0);
  rgactor->SetScale(2, 2, 2);

  SVTK_CREATE(svtkContourFilter, iso2);
  SVTK_CREATE(svtkStructuredGrid, sg);
  comm->Receive(sg, 1, 11);
  iso2->SetInputData(sg);
  iso2->SetValue(0, .205);

  SVTK_CREATE(svtkPolyDataMapper, sgmapper);
  sgmapper->SetInputConnection(0, iso2->GetOutputPort());

  SVTK_CREATE(svtkActor, sgactor);
  sgactor->SetMapper(sgmapper);
  sgactor->SetPosition(10, -5, -40);

  SVTK_CREATE(svtkImageData, id);
  comm->Receive(id, 1, 11);

  SVTK_CREATE(svtkImageActor, imactor);
  imactor->SetInputData(id);
  imactor->SetPosition(10, 0, 10);
  imactor->SetScale(0.02, 0.02, 0.02);

  SVTK_CREATE(svtkRenderer, ren);
  ren->AddActor(uactor);
  ren->AddActor(pactor);
  ren->AddActor(rgactor);
  ren->AddActor(sgactor);
  ren->AddActor(imactor);

  SVTK_CREATE(svtkRenderWindow, renWin);
  renWin->SetSize(500, 400);
  renWin->AddRenderer(ren);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(2.2);

  renWin->Render();

  retVal = svtkRegressionTestImage(renWin);

  CleanUp(comm, contr);

  return !retVal;
}
