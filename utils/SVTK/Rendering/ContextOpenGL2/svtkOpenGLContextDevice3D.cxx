/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLContextDevice3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLContextDevice3D.h"

#include "svtkBrush.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLCamera.h"
#include "svtkOpenGLContextDevice2D.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLHelper.h"
#include "svtkOpenGLIndexBufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkOpenGLVertexBufferObject.h"
#include "svtkPen.h"
#include "svtkShaderProgram.h"
#include "svtkTransform.h"

class svtkOpenGLContextDevice3D::Private
{
public:
  Private() = default;

  ~Private() = default;

  void Transpose(double* in, double* transposed)
  {
    transposed[0] = in[0];
    transposed[1] = in[4];
    transposed[2] = in[8];
    transposed[3] = in[12];

    transposed[4] = in[1];
    transposed[5] = in[5];
    transposed[6] = in[9];
    transposed[7] = in[13];

    transposed[8] = in[2];
    transposed[9] = in[6];
    transposed[10] = in[10];
    transposed[11] = in[14];

    transposed[12] = in[3];
    transposed[13] = in[7];
    transposed[14] = in[11];
    transposed[15] = in[15];
  }

  void SetLineType(int type)
  {
    if (type == svtkPen::SOLID_LINE || type == svtkPen::NO_PEN)
    {
      return;
    }
    svtkGenericWarningMacro(<< "Line Stipples are no longer supported");
  }

  svtkVector2i Dim;
  svtkVector2i Offset;
};

svtkStandardNewMacro(svtkOpenGLContextDevice3D);

svtkOpenGLContextDevice3D::svtkOpenGLContextDevice3D()
  : Storage(new Private)
{
  this->ModelMatrix = svtkTransform::New();
  this->ModelMatrix->Identity();
  this->VBO = new svtkOpenGLHelper;
  this->VCBO = new svtkOpenGLHelper;
  this->ClippingPlaneStates.resize(6, false);
  this->ClippingPlaneValues.resize(24);
}

svtkOpenGLContextDevice3D::~svtkOpenGLContextDevice3D()
{
  delete this->VBO;
  this->VBO = nullptr;
  delete this->VCBO;
  this->VCBO = nullptr;

  this->ModelMatrix->Delete();
  delete Storage;
}

void svtkOpenGLContextDevice3D::Initialize(svtkRenderer* ren, svtkOpenGLContextDevice2D* dev)
{
  this->Device2D = dev;
  this->Renderer = ren;
  this->RenderWindow = svtkOpenGLRenderWindow::SafeDownCast(ren->GetSVTKWindow());
}
//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice3D::Begin(svtkViewport* svtkNotUsed(viewport))
{
  this->ModelMatrix->Identity();
  for (int i = 0; i < 6; i++)
  {
    this->ClippingPlaneStates[i] = false;
  }
}

void svtkOpenGLContextDevice3D::SetMatrices(svtkShaderProgram* prog)
{
  svtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->svtkglDisable(GL_SCISSOR_TEST);
  prog->SetUniformMatrix("WCDCMatrix", this->Device2D->GetProjectionMatrix());

  svtkMatrix4x4* mvm = this->Device2D->GetModelMatrix();
  svtkMatrix4x4* tmp = svtkMatrix4x4::New();
  svtkMatrix4x4::Multiply4x4(mvm, this->ModelMatrix->GetMatrix(), tmp);

  prog->SetUniformMatrix("MCWCMatrix",
    //    this->ModelMatrix->GetMatrix());
    tmp);
  tmp->Delete();

  // add all the clipping planes
  int numClipPlanes = 0;
  float planeEquations[6][4];
  for (int i = 0; i < 6; i++)
  {
    if (this->ClippingPlaneStates[i])
    {
      planeEquations[numClipPlanes][0] = this->ClippingPlaneValues[i * 4];
      planeEquations[numClipPlanes][1] = this->ClippingPlaneValues[i * 4 + 1];
      planeEquations[numClipPlanes][2] = this->ClippingPlaneValues[i * 4 + 2];
      planeEquations[numClipPlanes][3] = this->ClippingPlaneValues[i * 4 + 3];
      numClipPlanes++;
    }
  }
  prog->SetUniformi("numClipPlanes", numClipPlanes);
  prog->SetUniform4fv("clipPlanes", 6, planeEquations);
}

void svtkOpenGLContextDevice3D::BuildVBO(svtkOpenGLHelper* cellBO, const float* f, int nv,
  const unsigned char* colors, int nc, float* tcoords)
{
  int stride = 3;
  int cOffset = 0;
  int tOffset = 0;
  if (colors)
  {
    cOffset = stride;
    stride++;
  }
  if (tcoords)
  {
    tOffset = stride;
    stride += 2;
  }

  std::vector<float> va;
  va.resize(nv * stride);
  svtkFourByteUnion c;
  for (int i = 0; i < nv; i++)
  {
    va[i * stride] = f[i * 3];
    va[i * stride + 1] = f[i * 3 + 1];
    va[i * stride + 2] = f[i * 3 + 2];
    if (colors)
    {
      c.c[0] = colors[nc * i];
      c.c[1] = colors[nc * i + 1];
      c.c[2] = colors[nc * i + 2];
      if (nc == 4)
      {
        c.c[3] = colors[nc * i + 3];
      }
      else
      {
        c.c[3] = 255;
      }
      va[i * stride + cOffset] = c.f;
    }
    if (tcoords)
    {
      va[i * stride + tOffset] = tcoords[i * 2];
      va[i * stride + tOffset + 1] = tcoords[i * 2 + 1];
    }
  }

  // upload the data
  cellBO->IBO->Upload(va, svtkOpenGLBufferObject::ArrayBuffer);
  cellBO->VAO->Bind();
  if (!cellBO->VAO->AddAttributeArray(
        cellBO->Program, cellBO->IBO, "vertexMC", 0, sizeof(float) * stride, SVTK_FLOAT, 3, false))
  {
    svtkErrorMacro(<< "Error setting vertexMC in shader VAO.");
  }
  if (colors)
  {
    if (!cellBO->VAO->AddAttributeArray(cellBO->Program, cellBO->IBO, "vertexScalar",
          sizeof(float) * cOffset, sizeof(float) * stride, SVTK_UNSIGNED_CHAR, 4, true))
    {
      svtkErrorMacro(<< "Error setting vertexScalar in shader VAO.");
    }
  }
  if (tcoords)
  {
    if (!cellBO->VAO->AddAttributeArray(cellBO->Program, cellBO->IBO, "tcoordMC",
          sizeof(float) * tOffset, sizeof(float) * stride, SVTK_FLOAT, 2, false))
    {
      svtkErrorMacro(<< "Error setting tcoordMC in shader VAO.");
    }
  }

  cellBO->VAO->Bind();
}

void svtkOpenGLContextDevice3D::ReadyVBOProgram()
{
  if (!this->VBO->Program)
  {
    this->VBO->Program = this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
      // vertex shader
      "//SVTK::System::Dec\n"
      "in vec3 vertexMC;\n"
      "uniform mat4 WCDCMatrix;\n"
      "uniform mat4 MCWCMatrix;\n"
      "uniform int numClipPlanes;\n"
      "uniform vec4 clipPlanes[6];\n"
      "out float clipDistances[6];\n"
      "void main() {\n"
      "vec4 vertex = vec4(vertexMC.xyz, 1.0);\n"
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "  {\n"
      "  clipDistances[planeNum] = dot(clipPlanes[planeNum], vertex*MCWCMatrix);\n"
      "  }\n"
      "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
      // fragment shader
      "//SVTK::System::Dec\n"
      "//SVTK::Output::Dec\n"
      "uniform vec4 vertexColor;\n"
      "uniform int numClipPlanes;\n"
      "in float clipDistances[6];\n"
      "void main() { \n"
      "  for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    if (clipDistances[planeNum] < 0.0) discard;\n"
      "    }\n"
      "  gl_FragData[0] = vertexColor; }",
      // geometry shader
      "");
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VBO->Program);
  }
}

void svtkOpenGLContextDevice3D::ReadyVCBOProgram()
{
  if (!this->VCBO->Program)
  {
    this->VCBO->Program = this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
      // vertex shader
      "//SVTK::System::Dec\n"
      "in vec3 vertexMC;\n"
      "in vec4 vertexScalar;\n"
      "uniform mat4 WCDCMatrix;\n"
      "uniform mat4 MCWCMatrix;\n"
      "out vec4 vertexColor;\n"
      "uniform int numClipPlanes;\n"
      "uniform vec4 clipPlanes[6];\n"
      "out float clipDistances[6];\n"
      "void main() {\n"
      "vec4 vertex = vec4(vertexMC.xyz, 1.0);\n"
      "vertexColor = vertexScalar;\n"
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "  {\n"
      "  clipDistances[planeNum] = dot(clipPlanes[planeNum], vertex*MCWCMatrix);\n"
      "  }\n"
      "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
      // fragment shader
      "//SVTK::System::Dec\n"
      "//SVTK::Output::Dec\n"
      "in vec4 vertexColor;\n"
      "uniform int numClipPlanes;\n"
      "in float clipDistances[6];\n"
      "void main() { \n"
      "  for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    if (clipDistances[planeNum] < 0.0) discard;\n"
      "    }\n"
      "  gl_FragData[0] = vertexColor; }",
      // geometry shader
      "");
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VCBO->Program);
  }
}

bool svtkOpenGLContextDevice3D::HaveWideLines()
{
  if (this->Pen->GetWidth() > 1.0)
  {
    // we have wide lines, but the OpenGL implementation may
    // actually support them, check the range to see if we
    // really need have to implement our own wide lines
    return !(this->RenderWindow &&
      this->RenderWindow->GetMaximumHardwareLineWidth() >= this->Pen->GetWidth());
  }
  return false;
}

void svtkOpenGLContextDevice3D::DrawPoly(
  const float* verts, int n, const unsigned char* colors, int nc)
{
  assert("verts must be non-null" && verts != nullptr);
  assert("n must be greater than 0" && n > 0);

  if (this->Pen->GetLineType() == svtkPen::NO_PEN)
  {
    return;
  }

  svtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  this->Storage->SetLineType(this->Pen->GetLineType());

  svtkOpenGLHelper* cbo = nullptr;
  if (colors)
  {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    if (!cbo->Program)
    {
      return;
    }
  }
  else
  {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    if (!cbo->Program)
    {
      return;
    }
    if (this->HaveWideLines())
    {
      svtkWarningMacro(
        << "a line width has been requested that is larger than your system supports");
    }
    else
    {
      glLineWidth(this->Pen->GetWidth());
    }
    cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
  }

  this->BuildVBO(cbo, verts, n, colors, nc, nullptr);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_LINE_STRIP, 0, n);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);
  glLineWidth(1.0);

  this->DisableDepthBuffer();

  svtkOpenGLCheckErrorMacro("failed after DrawPoly");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice3D::DrawLines(
  const float* verts, int n, const unsigned char* colors, int nc)
{
  assert("verts must be non-null" && verts != nullptr);
  assert("n must be greater than 0" && n > 0);

  if (this->Pen->GetLineType() == svtkPen::NO_PEN)
  {
    return;
  }

  svtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  this->Storage->SetLineType(this->Pen->GetLineType());

  if (this->Pen->GetWidth() > 1.0)
  {
    svtkErrorMacro(<< "lines wider than 1.0 are not supported\n");
  }
  glLineWidth(this->Pen->GetWidth());

  svtkOpenGLHelper* cbo = nullptr;
  if (colors)
  {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    if (!cbo->Program)
    {
      return;
    }
  }
  else
  {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    if (!cbo->Program)
    {
      return;
    }
    cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
  }

  this->BuildVBO(cbo, verts, n, colors, nc, nullptr);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_LINE, 0, n);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);
  glLineWidth(1.0);

  this->DisableDepthBuffer();

  svtkOpenGLCheckErrorMacro("failed after DrawLines");
}

void svtkOpenGLContextDevice3D::DrawPoints(
  const float* verts, int n, const unsigned char* colors, int nc)
{
  assert("verts must be non-null" && verts != nullptr);
  assert("n must be greater than 0" && n > 0);

  svtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  glPointSize(this->Pen->GetWidth());

  svtkOpenGLHelper* cbo = nullptr;
  if (colors)
  {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    if (!cbo->Program)
    {
      return;
    }
  }
  else
  {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    if (!cbo->Program)
    {
      return;
    }
    cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
  }

  this->BuildVBO(cbo, verts, n, colors, nc, nullptr);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_POINTS, 0, n);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  this->DisableDepthBuffer();

  svtkOpenGLCheckErrorMacro("failed DrawPoints");
}

void svtkOpenGLContextDevice3D::DrawTriangleMesh(
  const float* mesh, int n, const unsigned char* colors, int nc)
{
  assert("mesh must be non-null" && mesh != nullptr);
  assert("n must be greater than 0" && n > 0);

  svtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  svtkOpenGLHelper* cbo = nullptr;
  if (colors)
  {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    if (!cbo->Program)
    {
      return;
    }
  }
  else
  {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    if (!cbo->Program)
    {
      return;
    }
    cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
  }

  this->BuildVBO(cbo, mesh, n, colors, nc, nullptr);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_TRIANGLES, 0, n);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  this->DisableDepthBuffer();

  svtkOpenGLCheckErrorMacro("failed after DrawTriangleMesh");
}

void svtkOpenGLContextDevice3D::ApplyPen(svtkPen* pen)
{
  this->Pen->DeepCopy(pen);
}

void svtkOpenGLContextDevice3D::ApplyBrush(svtkBrush* brush)
{
  this->Brush->DeepCopy(brush);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice3D::PushMatrix()
{
  this->ModelMatrix->Push();
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice3D::PopMatrix()
{
  this->ModelMatrix->Pop();
}

void svtkOpenGLContextDevice3D::SetMatrix(svtkMatrix4x4* m)
{
  this->ModelMatrix->SetMatrix(m);
}

void svtkOpenGLContextDevice3D::GetMatrix(svtkMatrix4x4* m)
{
  m->DeepCopy(this->ModelMatrix->GetMatrix());
}

void svtkOpenGLContextDevice3D::MultiplyMatrix(svtkMatrix4x4* m)
{
  this->ModelMatrix->Concatenate(m);
}

void svtkOpenGLContextDevice3D::SetClipping(const svtkRecti& rect)
{
  // Check the bounds, and clamp if necessary
  GLint vp[4] = { this->Storage->Offset.GetX(), this->Storage->Offset.GetY(),
    this->Storage->Dim.GetX(), this->Storage->Dim.GetY() };

  if (rect.GetX() > 0 && rect.GetX() < vp[2])
  {
    vp[0] += rect.GetX();
  }
  if (rect.GetY() > 0 && rect.GetY() < vp[3])
  {
    vp[1] += rect.GetY();
  }
  if (rect.GetWidth() > 0 && rect.GetWidth() < vp[2])
  {
    vp[2] = rect.GetWidth();
  }
  if (rect.GetHeight() > 0 && rect.GetHeight() < vp[3])
  {
    vp[3] = rect.GetHeight();
  }

  svtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->svtkglScissor(vp[0], vp[1], vp[2], vp[3]);
}

void svtkOpenGLContextDevice3D::EnableClipping(bool enable)
{
  svtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->SetEnumState(GL_SCISSOR_TEST, enable);
}

void svtkOpenGLContextDevice3D::EnableClippingPlane(int i, double* planeEquation)
{
  if (i >= 6)
  {
    svtkOpenGLCheckErrorMacro("only 6 ClippingPlane allowed");
    return;
  }
  this->ClippingPlaneStates[i] = true;
  this->ClippingPlaneValues[i * 4] = planeEquation[0];
  this->ClippingPlaneValues[i * 4 + 1] = planeEquation[1];
  this->ClippingPlaneValues[i * 4 + 2] = planeEquation[2];
  this->ClippingPlaneValues[i * 4 + 3] = planeEquation[3];
}

void svtkOpenGLContextDevice3D::DisableClippingPlane(int i)
{
  if (i >= 6)
  {
    svtkOpenGLCheckErrorMacro("only 6 ClippingPlane allowed");
    return;
  }
  this->ClippingPlaneStates[i] = false;
}

void svtkOpenGLContextDevice3D::EnableDepthBuffer()
{
  svtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->svtkglEnable(GL_DEPTH_TEST);
}

void svtkOpenGLContextDevice3D::DisableDepthBuffer()
{
  svtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->svtkglDisable(GL_DEPTH_TEST);
}

void svtkOpenGLContextDevice3D::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
