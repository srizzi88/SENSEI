/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGLUTRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example tests the svtkRenderingExternal module by drawing a GLUT window
// and rendering a SVTK cube in it. It uses an ExternalSVTKWidget and sets a
// svtkExternalOpenGLRenderWindow to it.
//
// The test also demonstrates the use of
// PreserveColorBuffer and PreserveDepthBuffer flags on the
// svtkExternalOpenGLRenderer by drawing a GL_TRIANGLE in the scene before
// drawing the svtk sphere.

#include <svtk_glew.h>
// GLUT includes
#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED > 1090
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <GLUT/glut.h> // Include GLUT API.
#else
#if defined(_WIN32)
#include "svtkWindows.h" // Needed to include OpenGL header on Windows.
#endif                  // _WIN32
#include <GL/glut.h>    // Include GLUT API.
#endif

// STD includes
#include <iostream>

// SVTK includes
#include <ExternalSVTKWidget.h>
#include <svtkActor.h>
#include <svtkCallbackCommand.h>
#include <svtkCamera.h>
#include <svtkCubeSource.h>
#include <svtkExternalOpenGLRenderWindow.h>
#include <svtkLight.h>
#include <svtkLogger.h>
#include <svtkNew.h>
#include <svtkPolyDataMapper.h>
#include <svtkTesting.h>

namespace
{

// Global variables used by the glutDisplayFunc and glutIdleFunc
svtkNew<ExternalSVTKWidget> externalSVTKWidget;
static bool initialized = false;
static int NumArgs;
char** ArgV;
static bool tested = false;
static int retVal = 0;
static int windowId = -1;
static int windowH = 301;
static int windowW = 300;

static void MakeCurrentCallback(svtkObject* svtkNotUsed(caller),
  long unsigned int svtkNotUsed(eventId), void* svtkNotUsed(clientData), void* svtkNotUsed(callData))
{
  svtkLogScopeFunction(1);
  if (initialized)
  {
    glutSetWindow(windowId);
  }
}

/* Handler for window-repaint event. Call back when the window first appears and
   whenever the window needs to be re-painted. */
void display()
{
  svtkLogScopeFunction(INFO);
  if (!initialized)
  {
    svtkLogScopeF(INFO, "do-initialize");
    // since `handleResize` may get called before display, we may have already
    // created and resized the svtkExternalOpenGLRenderWindow, hence we don't
    // recreate it here.
    auto renWin = externalSVTKWidget->GetRenderWindow();

    // since our example here is not setting up the `glViewport`, we don't want
    // the svtkExternalOpenGLRenderWindow to update its size based on the
    // glViewport hence we must disable automatic position and size.
    renWin->AutomaticWindowPositionAndResizeOff();

    assert(renWin != nullptr);
    svtkNew<svtkCallbackCommand> callback;
    callback->SetCallback(MakeCurrentCallback);
    renWin->AddObserver(svtkCommand::WindowMakeCurrentEvent, callback);
    svtkNew<svtkPolyDataMapper> mapper;
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    svtkRenderer* ren = externalSVTKWidget->AddRenderer();
    ren->AddActor(actor);
    svtkNew<svtkCubeSource> cs;
    mapper->SetInputConnection(cs->GetOutputPort());
    actor->RotateX(45.0);
    actor->RotateY(45.0);
    ren->ResetCamera();

    initialized = true;
  }

  // Enable depth testing. Demonstrates OpenGL context being managed by external
  // application i.e. GLUT in this case.
  glEnable(GL_DEPTH_TEST);

  // Buffers being managed by external application i.e. GLUT in this case.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black and opaque
  glClearDepth(1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the color buffer

  glFlush(); // Render now
  glBegin(GL_TRIANGLES);
  glVertex3f(-1.5, -1.5, 0.0);
  glVertex3f(1.5, 0.0, 0.0);
  glVertex3f(0.0, 1.5, 1.0);
  glEnd();

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  GLfloat lightpos[] = { -0.5f, 1.0f, 1.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
  GLfloat diffuse[] = { 0.0f, 0.8f, 0.8f, 1.0f };
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  GLfloat specular[] = { 0.5f, 0.0f, 0.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
  GLfloat ambient[] = { 1.0f, 1.0f, 0.2f, 1.0f };
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

  svtkLogScopeF(INFO, "do-svtk-render");
  externalSVTKWidget->GetRenderWindow()->Render();
  glutSwapBuffers();
}

void test()
{
  svtkLogScopeFunction(INFO);
  bool interactiveMode = false;
  svtkTesting* t = svtkTesting::New();
  for (int cc = 1; cc < NumArgs; cc++)
  {
    t->AddArgument(ArgV[cc]);
    if (strcmp(ArgV[cc], "-I") == 0)
    {
      interactiveMode = true;
    }
  }
  t->SetRenderWindow(externalSVTKWidget->GetRenderWindow());
  if (!tested)
  {
    retVal = t->RegressionTest(0);
    tested = true;
  }
  t->Delete();
  if (!interactiveMode)
  {
    // Exit out of the infinitely running loop
    exit(!retVal);
  }
}

void handleResize(int w, int h)
{
  svtkLogScopeF(INFO, "handleResize: %d, %d", w, h);
  externalSVTKWidget->GetRenderWindow()->SetSize(w, h);
  glutPostRedisplay();
}

void onexit(void)
{
  initialized = false;
}

} // end anon namespace

/* Main function: GLUT runs as a console application starting at main()  */
int TestGLUTRenderWindow(int argc, char* argv[])
{
  NumArgs = argc;
  ArgV = argv;
  glutInit(&argc, argv); // Initialize GLUT
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
  svtkLog(INFO, "glutInitWindowSize: " << windowW << ", " << windowH);
  glutInitWindowSize(windowW, windowH); // Set the window's initial width & height
  glutInitWindowPosition(101, 201);     // Position the window's initial top-left corner
  windowId = glutCreateWindow("SVTK External Window Test"); // Create a window with the given title
  glutDisplayFunc(display);      // Register display callback handler for window re-paint
  glutIdleFunc(test);            // Register test callback handler for svtkTesting
  glutReshapeFunc(handleResize); // Register resize callback handler for window resize
  atexit(onexit);                // Register callback to uninitialize on exit
  glewInit();
  glutMainLoop(); // Enter the infinitely event-processing loop
  return 0;
}
