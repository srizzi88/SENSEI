//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/Bounds.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>

#include <GL/glew.h>
#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <string.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/rendering/Actor.h>
#include <svtkm/rendering/CanvasGL.h>
#include <svtkm/rendering/MapperGL.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View.h>
#include <svtkm/rendering/testing/RenderTest.h>

namespace
{
static constexpr svtkm::Id WIDTH = 512, HEIGHT = 512;
static svtkm::Id windowID, which = 0, NUM_DATASETS = 4;
static bool done = false;
static bool batch = false;

static void keyboardCall(unsigned char key, int svtkmNotUsed(x), int svtkmNotUsed(y))
{
  if (key == 27)
    glutDestroyWindow(windowID);
  else
  {
    which = (which + 1) % NUM_DATASETS;
    glutPostRedisplay();
  }
}

static void displayCall()
{
  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::ColorTable colorTable("inferno");

  using M = svtkm::rendering::MapperGL;
  using C = svtkm::rendering::CanvasGL;
  using V3 = svtkm::rendering::View3D;
  using V2 = svtkm::rendering::View2D;

  if (which == 0)
    svtkm::rendering::testing::Render<M, C, V3>(
      maker.Make3DRegularDataSet0(), "pointvar", colorTable, "reg3D.pnm");
  else if (which == 1)
    svtkm::rendering::testing::Render<M, C, V3>(
      maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "rect3D.pnm");
  else if (which == 2)
    svtkm::rendering::testing::Render<M, C, V3>(
      maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "expl3D.pnm");
  else if (which == 3)
    svtkm::rendering::testing::Render<M, C, V2>(
      maker.Make2DRectilinearDataSet0(), "pointvar", colorTable, "rect2D.pnm");
  glutSwapBuffers();
}

void batchIdle()
{
  which++;
  if (which >= NUM_DATASETS)
    glutDestroyWindow(windowID);
  else
    glutPostRedisplay();
}

void RenderTests()
{
  if (!batch)
    std::cout << "Press any key to cycle through datasets. ESC to quit." << std::endl;

  int argc = 0;
  char* argv = nullptr;
  glutInit(&argc, &argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(WIDTH, HEIGHT);
  windowID = glutCreateWindow("GLUT test");
  glutDisplayFunc(displayCall);
  glutKeyboardFunc(keyboardCall);
  if (batch)
    glutIdleFunc(batchIdle);

  glutMainLoop();
}

} //namespace

int UnitTestMapperGLUT(int argc, char* argv[])
{
  if (argc > 1)
  {
    if (strcmp(argv[1], "-B") == 0)
    {
      batch = true;
    }
  }
  return svtkm::cont::testing::Testing::Run(RenderTests);
}
