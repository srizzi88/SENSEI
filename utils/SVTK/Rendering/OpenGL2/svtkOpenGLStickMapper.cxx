/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLStickMapper.h"

#include "svtkOpenGLHelper.h"

#include "svtkFloatArray.h"
#include "svtkHardwareSelector.h"
#include "svtkMatrix3x3.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLActor.h"
#include "svtkOpenGLCamera.h"
#include "svtkOpenGLIndexBufferObject.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkOpenGLVertexBufferObject.h"
#include "svtkOpenGLVertexBufferObjectGroup.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkUnsignedCharArray.h"

#include "svtkPointGaussianVS.h"
#include "svtkStickMapperGS.h"

#include "svtk_glew.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLStickMapper);

//-----------------------------------------------------------------------------
svtkOpenGLStickMapper::svtkOpenGLStickMapper()
{
  this->ScaleArray = nullptr;
  this->OrientationArray = nullptr;
  this->SelectionIdArray = nullptr;
}

//-----------------------------------------------------------------------------
void svtkOpenGLStickMapper::GetShaderTemplate(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  this->Superclass::GetShaderTemplate(shaders, ren, actor);
  shaders[svtkShader::Vertex]->SetSource(svtkPointGaussianVS);
  shaders[svtkShader::Geometry]->SetSource(svtkStickMapperGS);
}

void svtkOpenGLStickMapper::ReplaceShaderValues(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  std::string VSSource = shaders[svtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[svtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[svtkShader::Fragment]->GetSource();

  svtkShaderProgram::Substitute(VSSource, "//SVTK::Normal::Dec",
    "in vec3 orientMC;\n"
    "uniform mat3 normalMatrix;\n"
    "out float lengthVCVSOutput;\n"
    "out vec3 orientVCVSOutput;");

  svtkShaderProgram::Substitute(VSSource, "//SVTK::Normal::Impl",
    "  lengthVCVSOutput = length(orientMC);\n"
    "  orientVCVSOutput = normalMatrix * normalize(orientMC);\n"
    // make sure it is pointing out of the screen
    "if (orientVCVSOutput.z < 0.0) { \n"
    "  orientVCVSOutput = -orientVCVSOutput;\n }\n");

  svtkShaderProgram::Substitute(VSSource, "//SVTK::Camera::Dec",
    "uniform mat4 VCDCMatrix;\n"
    "uniform mat4 MCVCMatrix;");

  svtkShaderProgram::Substitute(FSSource, "//SVTK::PositionVC::Dec", "in vec4 vertexVCVSOutput;");

  // we create vertexVC below, so turn off the default
  // implementation
  svtkShaderProgram::Substitute(
    FSSource, "//SVTK::PositionVC::Impl", "  vec4 vertexVC = vertexVCVSOutput;\n");

  // for lights kit and positional the VCDC matrix is already defined
  // so don't redefine it
  std::string replacement = "in float radiusVCVSOutput;\n"
                            "in vec3 orientVCVSOutput;\n"
                            "in float lengthVCVSOutput;\n"
                            "in vec3 centerVCVSOutput;\n"
                            "uniform mat4 VCDCMatrix;\n";
  svtkShaderProgram::Substitute(FSSource, "//SVTK::Normal::Dec", replacement);

  // see https://www.cl.cam.ac.uk/teaching/1999/AGraphHCI/SMAG/node2.html
  svtkShaderProgram::Substitute(FSSource, "//SVTK::Depth::Impl",
    // compute the eye position and unit direction
    "  vec3 EyePos;\n"
    "  vec3 EyeDir;\n"
    "  if (cameraParallel != 0) {\n"
    "    EyePos = vec3(vertexVC.x, vertexVC.y, vertexVC.z + 3.0*radiusVCVSOutput);\n"
    "    EyeDir = vec3(0.0,0.0,-1.0); }\n"
    "  else {\n"
    "    EyeDir = vertexVC.xyz;\n"
    "    EyePos = vec3(0.0,0.0,0.0);\n"
    "    float lengthED = length(EyeDir);\n"
    "    EyeDir = normalize(EyeDir);\n"
    // we adjust the EyePos to be closer if it is too far away
    // to prevent floating point precision noise
    "    if (lengthED > radiusVCVSOutput*3.0) {\n"
    "      EyePos = vertexVC.xyz - EyeDir*3.0*radiusVCVSOutput; }\n"
    "    }\n"

    // translate to Cylinder center
    "  EyePos = EyePos - centerVCVSOutput;\n"

    // rotate to new basis
    // base1, base2, orientVC
    "  vec3 base1;\n"
    "  if (abs(orientVCVSOutput.z) < 0.99) {\n"
    "    base1 = normalize(cross(orientVCVSOutput,vec3(0.0,0.0,1.0))); }\n"
    "  else {\n"
    "    base1 = normalize(cross(orientVCVSOutput,vec3(0.0,1.0,0.0))); }\n"
    "  vec3 base2 = cross(orientVCVSOutput,base1);\n"
    "  EyePos = vec3(dot(EyePos,base1),dot(EyePos,base2),dot(EyePos,orientVCVSOutput));\n"
    "  EyeDir = vec3(dot(EyeDir,base1),dot(EyeDir,base2),dot(EyeDir,orientVCVSOutput));\n"

    // scale by radius
    "  EyePos = EyePos/radiusVCVSOutput;\n"

    // find the intersection
    "  float a = EyeDir.x*EyeDir.x + EyeDir.y*EyeDir.y;\n"
    "  float b = 2.0*(EyePos.x*EyeDir.x + EyePos.y*EyeDir.y);\n"
    "  float c = EyePos.x*EyePos.x + EyePos.y*EyePos.y - 1.0;\n"
    "  float d = b*b - 4.0*a*c;\n"
    "  vec3 normalVCVSOutput = vec3(0.0,0.0,1.0);\n"
    "  if (d < 0.0) { discard; }\n"
    "  else {\n"
    "    float t =  (-b - sqrt(d))/(2.0*a);\n"
    "    float tz = EyePos.z + t*EyeDir.z;\n"
    "    vec3 iPoint = EyePos + t*EyeDir;\n"
    "    if (abs(iPoint.z)*radiusVCVSOutput > lengthVCVSOutput*0.5) {\n"
    // test for end cap
    "      float t2 = (-b + sqrt(d))/(2.0*a);\n"
    "      float tz2 = EyePos.z + t2*EyeDir.z;\n"
    "      if (tz2*radiusVCVSOutput > lengthVCVSOutput*0.5 || tz*radiusVCVSOutput < "
    "-0.5*lengthVCVSOutput) { discard; }\n"
    "      else {\n"
    "        normalVCVSOutput = orientVCVSOutput;\n"
    "        float t3 = (lengthVCVSOutput*0.5/radiusVCVSOutput - EyePos.z)/EyeDir.z;\n"
    "        iPoint = EyePos + t3*EyeDir;\n"
    "        vertexVC.xyz = radiusVCVSOutput*(iPoint.x*base1 + iPoint.y*base2 + "
    "iPoint.z*orientVCVSOutput) + centerVCVSOutput;\n"
    "        }\n"
    "      }\n"
    "    else {\n"
    // The normal is the iPoint.xy rotated back into VC
    "      normalVCVSOutput = iPoint.x*base1 + iPoint.y*base2;\n"
    // rescale rerotate and translate
    "      vertexVC.xyz = radiusVCVSOutput*(normalVCVSOutput + iPoint.z*orientVCVSOutput) + "
    "centerVCVSOutput;\n"
    "      }\n"
    "    }\n"

    //    "  vec3 normalVC = vec3(0.0,0.0,1.0);\n"
    // compute the pixel's depth
    "  vec4 pos = VCDCMatrix * vertexVC;\n"
    "  gl_FragDepth = (pos.z / pos.w + 1.0) / 2.0;\n");

  // Strip out the normal line -- the normal is computed as part of the depth
  svtkShaderProgram::Substitute(FSSource, "//SVTK::Normal::Impl", "");

  svtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (selector != nullptr);
  if (picking)
  {
    if (!selector || (this->LastSelectionState >= svtkHardwareSelector::POINT_ID_LOW24))
    {
      svtkShaderProgram::Substitute(VSSource, "//SVTK::Picking::Dec",
        "in vec4 selectionId;\n"
        "out vec4 selectionIdVSOutput;");
      svtkShaderProgram::Substitute(
        VSSource, "//SVTK::Picking::Impl", "selectionIdVSOutput = selectionId;");
      svtkShaderProgram::Substitute(GSSource, "//SVTK::Picking::Dec",
        "in vec4 selectionIdVSOutput[];\n"
        "out vec4 selectionIdGSOutput;");
      svtkShaderProgram::Substitute(
        GSSource, "//SVTK::Picking::Impl", "selectionIdGSOutput = selectionIdVSOutput[0];");
      svtkShaderProgram::Substitute(FSSource, "//SVTK::Picking::Dec", "in vec4 selectionIdVSOutput;");
      svtkShaderProgram::Substitute(FSSource, "//SVTK::Picking::Impl",
        "    gl_FragData[0] = vec4(selectionIdVSOutput.rgb, 1.0);\n");
    }
    else
    {
      svtkShaderProgram::Substitute(FSSource, "//SVTK::Picking::Dec", "uniform vec3 mapperIndex;");
      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::Picking::Impl", "  gl_FragData[0] = vec4(mapperIndex,1.0);\n");
    }
  }

  shaders[svtkShader::Vertex]->SetSource(VSSource);
  shaders[svtkShader::Geometry]->SetSource(GSSource);
  shaders[svtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderValues(shaders, ren, actor);
}

//-----------------------------------------------------------------------------
svtkOpenGLStickMapper::~svtkOpenGLStickMapper()
{
  this->SetScaleArray(nullptr);
  this->SetOrientationArray(nullptr);
  this->SetSelectionIdArray(nullptr);
}

//-----------------------------------------------------------------------------
void svtkOpenGLStickMapper::SetCameraShaderParameters(
  svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* actor)
{
  svtkShaderProgram* program = cellBO.Program;

  svtkOpenGLCamera* cam = (svtkOpenGLCamera*)(ren->GetActiveCamera());

  svtkMatrix4x4* wcdc;
  svtkMatrix4x4* wcvc;
  svtkMatrix3x3* norms;
  svtkMatrix4x4* vcdc;
  cam->GetKeyMatrices(ren, wcvc, norms, vcdc, wcdc);

  if (program->IsUniformUsed("VCDCMatrix"))
  {
    program->SetUniformMatrix("VCDCMatrix", vcdc);
  }

  if (!actor->GetIsIdentity())
  {
    svtkMatrix4x4* mcwc;
    svtkMatrix3x3* anorms;
    ((svtkOpenGLActor*)actor)->GetKeyMatrices(mcwc, anorms);
    if (program->IsUniformUsed("MCVCMatrix"))
    {
      svtkMatrix4x4::Multiply4x4(mcwc, wcvc, this->TempMatrix4);
      program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
    }
    if (program->IsUniformUsed("normalMatrix"))
    {
      svtkMatrix3x3::Multiply3x3(anorms, norms, this->TempMatrix3);
      program->SetUniformMatrix("normalMatrix", this->TempMatrix3);
    }
  }
  else
  {
    if (program->IsUniformUsed("MCVCMatrix"))
    {
      program->SetUniformMatrix("MCVCMatrix", wcvc);
    }
    if (program->IsUniformUsed("normalMatrix"))
    {
      program->SetUniformMatrix("normalMatrix", norms);
    }
  }

  if (program->IsUniformUsed("cameraParallel"))
  {
    cellBO.Program->SetUniformi("cameraParallel", cam->GetParallelProjection());
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLStickMapper::SetMapperShaderParameters(
  svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* actor)
{
  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
}

//-----------------------------------------------------------------------------
void svtkOpenGLStickMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

namespace
{
// internal function called by CreateVBO
void svtkOpenGLStickMapperCreateVBO(svtkPolyData* poly, svtkIdType numPts, unsigned char* colors,
  int colorComponents, float* orients, float* sizes, svtkIdType* selectionIds,
  svtkOpenGLVertexBufferObjectGroup* VBOs, svtkViewport* ren)
{
  svtkFloatArray* orientDA = svtkFloatArray::New();
  orientDA->SetNumberOfComponents(3);
  orientDA->SetNumberOfTuples(numPts);
  float* orPtr = static_cast<float*>(orientDA->GetVoidPointer(0));

  svtkFloatArray* radiusDA = svtkFloatArray::New();
  radiusDA->SetNumberOfComponents(1);
  radiusDA->SetNumberOfTuples(numPts);
  float* radPtr = static_cast<float*>(radiusDA->GetVoidPointer(0));

  svtkUnsignedCharArray* ucolors = svtkUnsignedCharArray::New();
  ucolors->SetNumberOfComponents(4);
  ucolors->SetNumberOfTuples(numPts);
  unsigned char* cPtr = static_cast<unsigned char*>(ucolors->GetVoidPointer(0));

  float* orientPtr;
  unsigned char* colorPtr;

  for (svtkIdType i = 0; i < numPts; ++i)
  {
    // orientation
    float length = sizes[i * 3];
    orientPtr = orients + i * 3;
    orPtr[0] = orientPtr[0] * length;
    orPtr[1] = orientPtr[1] * length;
    orPtr[2] = orientPtr[2] * length;
    orPtr += 3;

    // colors or selection ids
    if (selectionIds)
    {
      svtkIdType thisId = selectionIds[i] + 1;
      cPtr[0] = thisId % 256;
      cPtr[1] = (thisId >> 8) % 256;
      cPtr[2] = (thisId >> 16) % 256;
      cPtr[3] = 0;
    }
    else
    {
      colorPtr = colors + i * colorComponents;
      cPtr[0] = colorPtr[0];
      cPtr[1] = colorPtr[1];
      cPtr[2] = colorPtr[2];
      cPtr[3] = (colorComponents == 4 ? colorPtr[3] : 255);
    }
    cPtr += 4;

    *(radPtr++) = sizes[i * 3 + 1];
  }

  VBOs->CacheDataArray("vertexMC", poly->GetPoints()->GetData(), ren, SVTK_FLOAT);
  VBOs->CacheDataArray("orientMC", orientDA, ren, SVTK_FLOAT);
  orientDA->Delete();
  VBOs->CacheDataArray("radiusMC", radiusDA, ren, SVTK_FLOAT);
  radiusDA->Delete();

  if (selectionIds)
  {
    VBOs->CacheDataArray("scalarColor", nullptr, ren, SVTK_UNSIGNED_CHAR);
    VBOs->CacheDataArray("selectionId", ucolors, ren, SVTK_UNSIGNED_CHAR);
  }
  else
  {
    VBOs->CacheDataArray("scalarColor", ucolors, ren, SVTK_UNSIGNED_CHAR);
    VBOs->CacheDataArray("selectionId", nullptr, ren, SVTK_UNSIGNED_CHAR);
  }
  ucolors->Delete();
  VBOs->BuildAllVBOs(ren);
}
}

//-------------------------------------------------------------------------
bool svtkOpenGLStickMapper::GetNeedToRebuildBufferObjects(svtkRenderer* ren, svtkActor* act)
{
  if (this->Superclass::GetNeedToRebuildBufferObjects(ren, act) ||
    this->VBOBuildTime < this->SelectionStateChanged)
  {
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------
void svtkOpenGLStickMapper::BuildBufferObjects(svtkRenderer* ren, svtkActor* svtkNotUsed(act))
{
  svtkPolyData* poly = this->CurrentInput;

  if (poly == nullptr) // || !poly->GetPointData()->GetNormals())
  {
    return;
  }

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(1.0);

  svtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (selector != nullptr);

  svtkOpenGLStickMapperCreateVBO(poly, poly->GetPoints()->GetNumberOfPoints(),
    this->Colors ? (unsigned char*)this->Colors->GetVoidPointer(0) : nullptr,
    this->Colors ? this->Colors->GetNumberOfComponents() : 0,
    static_cast<float*>(poly->GetPointData()->GetArray(this->OrientationArray)->GetVoidPointer(0)),
    static_cast<float*>(poly->GetPointData()->GetArray(this->ScaleArray)->GetVoidPointer(0)),
    picking ? static_cast<svtkIdType*>(
                poly->GetPointData()->GetArray(this->SelectionIdArray)->GetVoidPointer(0))
            : nullptr,
    this->VBOs, ren);

  // create the IBO (none)
  this->Primitives[PrimitivePoints].IBO->IndexCount = 0;
  this->Primitives[PrimitiveLines].IBO->IndexCount = 0;
  this->Primitives[PrimitiveTriStrips].IBO->IndexCount = 0;
  this->Primitives[PrimitiveTris].IBO->IndexCount = poly->GetPoints()->GetNumberOfPoints();

  this->VBOBuildTime.Modified();
}

//-----------------------------------------------------------------------------
void svtkOpenGLStickMapper::RenderPieceDraw(svtkRenderer* ren, svtkActor* actor)
{
  // draw polygons
  int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
  if (numVerts)
  {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(this->Primitives[PrimitiveTris], ren, actor);
    glDrawArrays(GL_POINTS, 0, static_cast<GLuint>(numVerts));
  }
}
