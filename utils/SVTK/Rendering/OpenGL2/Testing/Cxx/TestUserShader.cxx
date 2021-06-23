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
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkShaderProperty.h"
#include "svtkTriangleMeshPointNormals.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkRenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestUserShader(int argc, char* argv[])
{
  svtkNew<svtkActor> actor;
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkOpenGLPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  svtkNew<svtkTriangleMeshPointNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());
  norms->Update();

  mapper->SetInputConnection(norms->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);

  // Modify the shader to color based on model normal
  // To do this we have to modify the vertex shader
  // to pass the normal in model coordinates
  // through to the fragment shader. By default the normal
  // is converted to View coordinates and then passed on.
  // We keep that, but add a varying for the original normal.
  // Then we modify the fragment shader to set the diffuse color
  // based on that normal. First lets modify the vertex
  // shader
  svtkShaderProperty* sp = actor->GetShaderProperty();
  sp->AddVertexShaderReplacement("//SVTK::Normal::Dec", // replace the normal block
    true,                                              // before the standard replacements
    "//SVTK::Normal::Dec\n"                             // we still want the default
    "  out vec3 myNormalMCVSOutput;\n",                // but we add this
    false                                              // only do it once
  );
  sp->AddVertexShaderReplacement("//SVTK::Normal::Impl", // replace the normal block
    true,                                               // before the standard replacements
    "//SVTK::Normal::Impl\n"                             // we still want the default
    "  myNormalMCVSOutput = normalMC;\n",               // but we add this
    false                                               // only do it once
  );
  sp->AddVertexShaderReplacement("//SVTK::Color::Impl", // dummy replacement for testing clear method
    true, "SVTK::Color::Impl\n", false);
  sp->ClearVertexShaderReplacement("//SVTK::Color::Impl", true);

  // now modify the fragment shader
  sp->AddFragmentShaderReplacement("//SVTK::Normal::Dec", // replace the normal block
    true,                                                // before the standard replacements
    "//SVTK::Normal::Dec\n"                               // we still want the default
    "  in vec3 myNormalMCVSOutput;\n",                   // but we add this
    false                                                // only do it once
  );
  sp->AddFragmentShaderReplacement("//SVTK::Normal::Impl", // replace the normal block
    true,                                                 // before the standard replacements
    "//SVTK::Normal::Impl\n"                               // we still want the default calc
    "  diffuseColor = abs(myNormalMCVSOutput);\n",        // but we add this
    false                                                 // only do it once
  );

  // Test enumerating shader replacements
  int nbReplacements = sp->GetNumberOfShaderReplacements();
  if (nbReplacements != 4)
  {
    return EXIT_FAILURE;
  }
  if (sp->GetNthShaderReplacementTypeAsString(0) != std::string("Vertex") ||
    sp->GetNthShaderReplacementTypeAsString(1) != std::string("Fragment") ||
    sp->GetNthShaderReplacementTypeAsString(2) != std::string("Vertex") ||
    sp->GetNthShaderReplacementTypeAsString(3) != std::string("Fragment"))
  {
    return EXIT_FAILURE;
  }

  renderWindow->Render();
  renderer->GetActiveCamera()->SetPosition(-0.2, 0.4, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
