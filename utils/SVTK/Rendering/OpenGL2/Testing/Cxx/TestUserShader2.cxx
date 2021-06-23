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
#include "svtkCommand.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkPLYReader.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkShaderProperty.h"
#include "svtkTestUtilities.h"
#include "svtkTimerLog.h"
#include "svtkTriangleMeshPointNormals.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

// -----------------------------------------------------------------------
// Update a uniform in the shader for each render. We do this with a
// callback for the UpdateShaderEvent
class svtkShaderCallback : public svtkCommand
{
public:
  static svtkShaderCallback* New() { return new svtkShaderCallback; }
  svtkRenderer* Renderer;
  void Execute(svtkObject*, unsigned long, void* calldata) override
  {
    svtkShaderProgram* program = reinterpret_cast<svtkShaderProgram*>(calldata);

    float diffuseColor[3];

#if 0 // trippy mode
    float inputHSV[3];
    double theTime = svtkTimerLog::GetUniversalTime();
    double twopi = 2.0*svtkMath::Pi();

    inputHSV[0] = sin(twopi*fmod(theTime,3.0)/3.0)/4.0 + 0.25;
    inputHSV[1] = sin(twopi*fmod(theTime,4.0)/4.0)/2.0 + 0.5;
    inputHSV[2] = 0.7*(sin(twopi*fmod(theTime,19.0)/19.0)/2.0 + 0.5);
    svtkMath::HSVToRGB(inputHSV,diffuseColor);
    cellBO->Program->SetUniform3f("diffuseColorUniform", diffuseColor);

    if (this->Renderer)
    {
      inputHSV[0] = sin(twopi*fmod(theTime,5.0)/5.0)/4.0 + 0.75;
      inputHSV[1] = sin(twopi*fmod(theTime,7.0)/7.0)/2.0 + 0.5;
      inputHSV[2] = 0.5*(sin(twopi*fmod(theTime,17.0)/17.0)/2.0 + 0.5);
      svtkMath::HSVToRGB(inputHSV,diffuseColor);
      this->Renderer->SetBackground(diffuseColor[0], diffuseColor[1], diffuseColor[2]);

      inputHSV[0] = sin(twopi*fmod(theTime,11.0)/11.0)/2.0+0.5;
      inputHSV[1] = sin(twopi*fmod(theTime,13.0)/13.0)/2.0 + 0.5;
      inputHSV[2] = 0.5*(sin(twopi*fmod(theTime,17.0)/17.0)/2.0 + 0.5);
      svtkMath::HSVToRGB(inputHSV,diffuseColor);
      this->Renderer->SetBackground2(diffuseColor[0], diffuseColor[1], diffuseColor[2]);
    }
#else
    diffuseColor[0] = 0.4;
    diffuseColor[1] = 0.7;
    diffuseColor[2] = 0.6;
    program->SetUniform3f("diffuseColorUniform", diffuseColor);
#endif
  }

  svtkShaderCallback() { this->Renderer = nullptr; }
};

//----------------------------------------------------------------------------
int TestUserShader2(int argc, char* argv[])
{
  svtkNew<svtkActor> actor;
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkOpenGLPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  renderer->GradientBackgroundOn();
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

  svtkShaderProperty* sp = actor->GetShaderProperty();

  // Clear all custom shader tag replacements
  // The following code is mainly for regression testing as we do not have any
  // custom shader replacements.
  sp->ClearAllVertexShaderReplacements();
  sp->ClearAllFragmentShaderReplacements();
  sp->ClearAllGeometryShaderReplacements();
  sp->ClearAllShaderReplacements();

  // Use our own hardcoded shader code. Generally this is a bad idea in a
  // general purpose program as there are so many things SVTK supports that
  // hardcoded shaders will not handle depth peeling, picking, etc, but if you
  // know what your data will be like it can be very useful. The mapper will set
  // a bunch of uniforms regardless of if you are using them. But feel free to
  // use them :-)
  sp->SetVertexShaderCode(
    "//SVTK::System::Dec\n" // always start with this line
    "in vec4 vertexMC;\n"
    // use the default normal decl as the mapper
    // will then provide the normalMatrix uniform
    // which we use later on
    "//SVTK::Normal::Dec\n"
    "uniform mat4 MCDCMatrix;\n"
    "void main () {\n"
    "  normalVCVSOutput = normalMatrix * normalMC;\n"
    // do something weird with the vertex positions
    // this will mess up your head if you keep
    // rotating and looking at it, very trippy
    "  vec4 tmpPos = MCDCMatrix * vertexMC;\n"
    "  gl_Position = tmpPos*vec4(0.2+0.8*abs(tmpPos.x),0.2+0.8*abs(tmpPos.y),1.0,1.0);\n"
    "}\n");
  sp->SetFragmentShaderCode(
    "//SVTK::System::Dec\n" // always start with this line
    "//SVTK::Output::Dec\n" // always have this line in your FS
    "in vec3 normalVCVSOutput;\n"
    "uniform vec3 diffuseColorUniform;\n"
    "void main () {\n"
    "  float df = max(0.0, normalVCVSOutput.z);\n"
    "  float sf = pow(df, 20.0);\n"
    "  vec3 diffuse = df * diffuseColorUniform;\n"
    "  vec3 specular = sf * vec3(0.4,0.4,0.4);\n"
    "  gl_FragData[0] = vec4(0.3*abs(normalVCVSOutput) + 0.7*diffuse + specular, 1.0);\n"
    "}\n");

  // Setup a callback to change some uniforms
  SVTK_CREATE(svtkShaderCallback, myCallback);
  myCallback->Renderer = renderer;
  mapper->AddObserver(svtkCommand::UpdateShaderEvent, myCallback);

  renderWindow->Render();
  renderer->GetActiveCamera()->SetPosition(-0.2, 0.4, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2.0);
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
