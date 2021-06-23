/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Win32Cone.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example is a windows application (instead of a console application)
// version of Examples/Tutorial/Step1/Cxx/Cone.cxx. It is organized in a more
// object oriented manner and shows a fairly minimal windows SVTK application.
//

#include "windows.h"

// first include the required header files for the svtk classes we are using
#include "svtkConeSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

static HANDLE hinst;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
// define the svtk part as a simple c++ class
class mySVTKApp
{
public:
  mySVTKApp(HWND parent);
  ~mySVTKApp();

private:
  svtkRenderWindow* renWin;
  svtkRenderer* renderer;
  svtkRenderWindowInteractor* iren;
  svtkConeSource* cone;
  svtkPolyDataMapper* coneMapper;
  svtkActor* coneActor;
};

int PASCAL WinMain(
  HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR /* lpszCmdParam */, int nCmdShow)
{
  static char szAppName[] = "Win32Cone";
  HWND hwnd;
  MSG msg;
  WNDCLASS wndclass;

  if (!hPrevInstance)
  {
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndclass.lpszMenuName = nullptr;
    wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndclass.lpszClassName = szAppName;
    RegisterClass(&wndclass);
  }

  hinst = hInstance;
  hwnd = CreateWindow(szAppName, "Draw Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
    400, 480, nullptr, nullptr, hInstance, nullptr);
  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static HWND ewin;
  static mySVTKApp* theSVTKApp;

  switch (message)
  {
    case WM_CREATE:
    {
      ewin = CreateWindow("button", "Exit", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 400, 400, 60,
        hwnd, (HMENU)2, (HINSTANCE)svtkGetWindowLong(hwnd, svtkGWL_HINSTANCE), nullptr);
      theSVTKApp = new mySVTKApp(hwnd);
      return 0;
    }

    case WM_COMMAND:
      switch (wParam)
      {
        case 2:
          PostQuitMessage(0);
          delete theSVTKApp;
          theSVTKApp = nullptr;
          break;
      }
      return 0;

    case WM_DESTROY:
      PostQuitMessage(0);
      delete theSVTKApp;
      theSVTKApp = nullptr;
      return 0;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

mySVTKApp::mySVTKApp(HWND hwnd)
{
  // Similar to Examples/Tutorial/Step1/Cxx/Cone.cxx
  // We create the basic parts of a pipeline and connect them
  this->renderer = svtkRenderer::New();
  this->renWin = svtkRenderWindow::New();
  this->renWin->AddRenderer(this->renderer);

  // setup the parent window
  this->renWin->SetParentId(hwnd);
  this->iren = svtkRenderWindowInteractor::New();
  this->iren->SetRenderWindow(this->renWin);

  this->cone = svtkConeSource::New();
  this->cone->SetHeight(3.0);
  this->cone->SetRadius(1.0);
  this->cone->SetResolution(10);
  this->coneMapper = svtkPolyDataMapper::New();
  this->coneMapper->SetInputConnection(this->cone->GetOutputPort());
  this->coneActor = svtkActor::New();
  this->coneActor->SetMapper(this->coneMapper);

  this->renderer->AddActor(this->coneActor);
  this->renderer->SetBackground(0.2, 0.4, 0.3);
  this->renWin->SetSize(400, 400);

  // Finally we start the interactor so that event will be handled
  this->renWin->Render();
}

mySVTKApp::~mySVTKApp()
{
  renWin->Delete();
  renderer->Delete();
  iren->Delete();
  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();
}
