/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkImageFlip.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkJPEGReader.h"
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkShaderProperty.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"

//----------------------------------------------------------------------------
int TestCubeMapRerender(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  svtkNew<svtkTexture> texture;
  texture->CubeMapOn();

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);

  delete[] fileName;

  svtkNew<svtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());

  const char* fpath[] = { "Data/skybox-px.jpg", "Data/skybox-nx.jpg", "Data/skybox-py.jpg",
    "Data/skybox-ny.jpg", "Data/skybox-pz.jpg", "Data/skybox-nz.jpg" };

  for (int i = 0; i < 6; i++)
  {
    svtkNew<svtkJPEGReader> imgReader;
    const char* fName = svtkTestUtilities::ExpandDataFileName(argc, argv, fpath[i]);
    imgReader->SetFileName(fName);
    delete[] fName;
    svtkNew<svtkImageFlip> flip;
    flip->SetInputConnection(imgReader->GetOutputPort());
    flip->SetFilteredAxis(1); // flip y axis
    texture->SetInputConnection(i, flip->GetOutputPort(0));
  }

  svtkNew<svtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(norms->GetOutputPort());

  svtkNew<svtkActor> actor;
  renderer->AddActor(actor);
  actor->SetTexture(texture);
  actor->SetMapper(mapper);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.4);
  renderWindow->Render();

  svtkShaderProperty* sp = actor->GetShaderProperty();

  // Add new code in default SVTK vertex shader
  sp->AddVertexShaderReplacement("//SVTK::PositionVC::Dec", // replace the normal block
    true,                                                  // before the standard replacements
    "//SVTK::PositionVC::Dec\n"                             // we still want the default
    "out vec3 TexCoords;\n",
    false // only do it once
  );
  sp->AddVertexShaderReplacement("//SVTK::PositionVC::Impl", // replace the normal block
    true,                                                   // before the standard replacements
    "//SVTK::PositionVC::Impl\n"                             // we still want the default
    "vec3 camPos = -MCVCMatrix[3].xyz * mat3(MCVCMatrix);\n"
    "TexCoords.xyz = reflect(vertexMC.xyz - camPos, normalize(normalMC));\n",
    false // only do it once
  );

  // Replace SVTK fragment shader
  sp->SetFragmentShaderCode("//SVTK::System::Dec\n" // always start with this line
                            "//SVTK::Output::Dec\n" // always have this line in your FS
                            "in vec3 TexCoords;\n"
                            "uniform samplerCube texture_0;\n"
                            "void main () {\n"
                            "  gl_FragData[0] = texture(texture_0, TexCoords);\n"
                            "}\n");

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.4);
  renderWindow->Render();

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  renderWindow->GetInteractor()->SetInteractorStyle(style);

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
