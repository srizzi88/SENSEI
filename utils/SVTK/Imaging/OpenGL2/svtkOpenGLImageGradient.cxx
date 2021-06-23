/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLImageGradient.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLImageGradient.h"

#include "svtkOpenGLImageAlgorithmHelper.h"

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkShaderProgram.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <algorithm> // for std::nth_element

svtkStandardNewMacro(svtkOpenGLImageGradient);

//-----------------------------------------------------------------------------
// Construct an instance of svtkOpenGLImageGradient filter.
svtkOpenGLImageGradient::svtkOpenGLImageGradient()
{
  // for GPU we do not want threading
  this->NumberOfThreads = 1;
  this->EnableSMP = false;
  this->Helper = svtkOpenGLImageAlgorithmHelper::New();
}

//-----------------------------------------------------------------------------
svtkOpenGLImageGradient::~svtkOpenGLImageGradient()
{
  if (this->Helper)
  {
    this->Helper->Delete();
    this->Helper = 0;
  }
}

void svtkOpenGLImageGradient::SetRenderWindow(svtkRenderWindow* renWin)
{
  this->Helper->SetRenderWindow(renWin);
}

//-----------------------------------------------------------------------------
void svtkOpenGLImageGradient::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Helper: ";
  this->Helper->PrintSelf(os, indent);
}

// this is used as a callback by the helper to set shader parameters
// before running and to update them on each slice
class svtkOpenGLGradientCB : public svtkOpenGLImageAlgorithmCallback
{
public:
  // initialize the spacing
  virtual void InitializeShaderUniforms(svtkShaderProgram* program)
  {
    float sp[3];
    sp[0] = this->Spacing[0];
    sp[1] = this->Spacing[1];
    sp[2] = this->Spacing[2];
    program->SetUniform3f("spacing", sp);
  }

  // no uniforms change on a per slice basis so empty
  virtual void UpdateShaderUniforms(svtkShaderProgram* /* program */, int /* zExtent */) {}

  double* Spacing;
  svtkOpenGLGradientCB() {}
  virtual ~svtkOpenGLGradientCB() {}

private:
  svtkOpenGLGradientCB(const svtkOpenGLGradientCB&) = delete;
  void operator=(const svtkOpenGLGradientCB&) = delete;
};

//-----------------------------------------------------------------------------
// This method contains the first switch statement that calls the correct
// templated function for the input and output region types.
void svtkOpenGLImageGradient::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int outExt[6], int svtkNotUsed(id))
{
  svtkDataArray* inArray = this->GetInputArrayToProcess(0, inputVector);
  outData[0]->GetPointData()->GetScalars()->SetName(inArray->GetName());

  // The output scalar type must be double to store proper gradients.
  if (outData[0]->GetScalarType() != SVTK_DOUBLE)
  {
    svtkErrorMacro(
      "Execute: output ScalarType is " << outData[0]->GetScalarType() << "but must be double.");
    return;
  }

  // Gradient makes sense only with one input component.  This is not
  // a Jacobian filter.
  if (inArray->GetNumberOfComponents() != 1)
  {
    svtkErrorMacro("Execute: input has more than one component. "
                  "The input to gradient should be a single component image. "
                  "Think about it. If you insist on using a color image then "
                  "run it though RGBToHSV then ExtractComponents to get the V "
                  "components. That's probably what you want anyhow.");
    return;
  }

  svtkOpenGLGradientCB cb;
  cb.Spacing = inData[0][0]->GetSpacing();

  // build the fragment shader for 2D or 3D gradient
  std::string fragShader =
    "//SVTK::System::Dec\n"
    "varying vec2 tcoordVSOutput;\n"
    "uniform sampler3D inputTex1;\n"
    "uniform float zPos;\n"
    "uniform vec3 spacing;\n"
    "uniform float inputScale;\n"
    "uniform float inputShift;\n"
    "//SVTK::Output::Dec\n"
    "void main(void) {\n"
    "  float dx = textureOffset(inputTex1, vec3(tcoordVSOutput, zPos), ivec3(1,0,0)).r\n"
    "    - textureOffset(inputTex1, vec3(tcoordVSOutput, zPos), ivec3(-1,0,0)).r;\n"
    "  dx = inputScale*0.5*dx/spacing.x;\n"
    "  float dy = textureOffset(inputTex1, vec3(tcoordVSOutput, zPos), ivec3(0,1,0)).r\n"
    "    - textureOffset(inputTex1, vec3(tcoordVSOutput, zPos), ivec3(0,-1,0)).r;\n"
    "  dy = inputScale*0.5*dy/spacing.y;\n";

  if (this->Dimensionality == 3)
  {
    fragShader +=
      "  float dz = textureOffset(inputTex1, vec3(tcoordVSOutput, zPos), ivec3(0,0,1)).r\n"
      "    - textureOffset(inputTex1, vec3(tcoordVSOutput, zPos), ivec3(0,0,-1)).r;\n"
      "  dz = inputScale*0.5*dz/spacing.z;\n"
      "  gl_FragData[0] = vec4(dx, dy, dz, 1.0);\n"
      "}\n";
  }
  else
  {
    fragShader += "  gl_FragData[0] = vec4(dx, dy, 0.0, 1.0);\n"
                  "}\n";
  }

  // call the helper to execte this code
  this->Helper->Execute(&cb, inData[0][0], inArray, outData[0], outExt,

    "//SVTK::System::Dec\n"
    "attribute vec4 vertexMC;\n"
    "attribute vec2 tcoordMC;\n"
    "varying vec2 tcoordVSOutput;\n"
    "void main() {\n"
    "  tcoordVSOutput = tcoordMC;\n"
    "  gl_Position = vertexMC;\n"
    "}\n",

    fragShader.c_str(),

    "");
}
