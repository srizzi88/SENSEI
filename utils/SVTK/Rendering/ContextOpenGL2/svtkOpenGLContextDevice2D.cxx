
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLContextDevice2D.h"

#include "svtkAbstractContextBufferId.h"
#include "svtkBrush.h"
#include "svtkContext2D.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkImageResize.h"
#include "svtkMath.h"
#include "svtkMatrix3x3.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLGL2PSHelper.h"
#include "svtkOpenGLHelper.h"
#include "svtkOpenGLIndexBufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLTexture.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkOpenGLVertexBufferObject.h"
#include "svtkPath.h"
#include "svtkPen.h"
#include "svtkPointData.h"
#include "svtkPoints2D.h"
#include "svtkPolyData.h"
#include "svtkRect.h"
#include "svtkShaderProgram.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"
#include "svtkTexture.h"
#include "svtkTextureUnitManager.h"
#include "svtkTransform.h"
#include "svtkTransformFeedback.h"
#include "svtkVector.h"
#include "svtkViewport.h"
#include "svtkWindow.h"

#include "svtkObjectFactory.h"

#include "svtkOpenGLContextDevice2DPrivate.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <sstream>

#define BUFFER_OFFSET(i) (reinterpret_cast<char*>(i))

namespace
{
void copyColors(std::vector<unsigned char>& newColors, unsigned char* colors, int nc)
{
  for (int j = 0; j < nc; j++)
  {
    newColors.push_back(colors[j]);
  }
}

const char* myVertShader = "in vec2 vertexMC;\n"
                           "uniform mat4 WCDCMatrix;\n"
                           "uniform mat4 MCWCMatrix;\n"
                           "#ifdef haveColors\n"
                           "in vec4 vertexScalar;\n"
                           "out vec4 vertexColor;\n"
                           "#endif\n"
                           "#ifdef haveTCoords\n"
                           "in vec2 tcoordMC;\n"
                           "out vec2 tcoord;\n"
                           "#endif\n"
                           "#ifdef haveLines\n"
                           "in vec2 tcoordMC;\n"
                           "out float ldistance;\n"
                           "#endif\n"
                           "void main() {\n"
                           "#ifdef haveColors\n"
                           "vertexColor = vertexScalar;\n"
                           "#endif\n"
                           "#ifdef haveTCoords\n"
                           "tcoord = tcoordMC;\n"
                           "#endif\n"
                           "#ifdef haveLines\n"
                           "ldistance = tcoordMC.x;\n"
                           "#endif\n"
                           "vec4 vertex = vec4(vertexMC.xy, 0.0, 1.0);\n"
                           "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n";

const char* myFragShader = "//SVTK::Output::Dec\n"
                           "#ifdef haveColors\n"
                           "in vec4 vertexColor;\n"
                           "#else\n"
                           "uniform vec4 vertexColor;\n"
                           "#endif\n"
                           "#ifdef haveTCoords\n"
                           "in vec2 tcoord;\n"
                           "uniform sampler2D texture1;\n"
                           "#endif\n"
                           "#ifdef haveLines\n"
                           "in float ldistance;\n"
                           "uniform int stipple;\n"
                           "#endif\n"
                           "void main() {\n"
                           "#ifdef haveLines\n"
                           "if ((0x01 << int(mod(ldistance,16.0)) & stipple) == 0) { discard; }\n"
                           "#endif\n"
                           "#ifdef haveTCoords\n"
                           " gl_FragData[0] = texture2D(texture1, tcoord);\n"
                           "#else\n"
                           " gl_FragData[0] = vertexColor;\n"
                           "#endif\n"
                           "}\n";

//-----------------------------------------------------------------------------
// Returns true when rendering the GL2PS background raster image. Vectorizable
// primitives should not be drawn during these passes.
bool SkipDraw()
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Background)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// Releases the current shader program if it is inconsistent with the GL2PS
// capture state. Returns the current OpenGLGL2PSHelper instance if one exists.
svtkOpenGLGL2PSHelper* PrepProgramForGL2PS(svtkOpenGLHelper& helper)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
  {
    // Always recreate the program when doing GL2PS capture.
    if (helper.Program)
    {
      helper.Program->Delete();
      helper.Program = nullptr;
    }
  }
  else
  {
    // If there is a feedback transform capturer set on the current shader
    // program and we're not capturing, recreate the program.
    if (helper.Program && helper.Program->GetTransformFeedback())
    {
      helper.Program->Delete();
      helper.Program = nullptr;
    }
  }

  return gl2ps;
}

//-----------------------------------------------------------------------------
// Call before glDraw* commands to ensure that vertices are properly captured
// for GL2PS export.
void PreDraw(svtkOpenGLHelper& helper, int drawMode, size_t numVerts)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture && helper.Program)
  {
    if (svtkTransformFeedback* tfc = helper.Program->GetTransformFeedback())
    {
      tfc->SetNumberOfVertices(drawMode, numVerts);
      tfc->BindBuffer();
    }
  }
}

//-----------------------------------------------------------------------------
// Call after glDraw* commands to ensure that vertices are properly captured
// for GL2PS export.
void PostDraw(svtkOpenGLHelper& helper, svtkRenderer* ren, unsigned char col[4])
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture && helper.Program)
  {
    if (svtkTransformFeedback* tfc = helper.Program->GetTransformFeedback())
    {
      tfc->ReadBuffer();
      tfc->ReleaseGraphicsResources();
      gl2ps->ProcessTransformFeedback(tfc, ren, col);
      tfc->ReleaseBufferData();
    }
  }
}

//-----------------------------------------------------------------------------
// Returns true if the startAngle and stopAngle (as used in the ellipse drawing
// functions) describe a full circle.
inline bool IsFullCircle(float startAngle, float stopAngle)
{
  // A small number practical for rendering purposes.
  const float TOL = 1e-5f;

  return std::fabs(stopAngle - startAngle) + TOL >= 360.f;
}

} // end anon namespace

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLContextDevice2D);

//-----------------------------------------------------------------------------
svtkOpenGLContextDevice2D::svtkOpenGLContextDevice2D()
{
  this->Renderer = nullptr;
  this->InRender = false;
  this->Storage = new svtkOpenGLContextDevice2D::Private;
  this->PolyDataImpl = new svtkOpenGLContextDevice2D::CellArrayHelper(this);
  this->RenderWindow = nullptr;
  this->MaximumMarkerCacheSize = 20;
  this->ProjectionMatrix = svtkTransform::New();
  this->ModelMatrix = svtkTransform::New();
  this->VBO = new svtkOpenGLHelper;
  this->VCBO = new svtkOpenGLHelper;
  this->LinesBO = new svtkOpenGLHelper;
  this->LinesCBO = new svtkOpenGLHelper;
  this->VTBO = new svtkOpenGLHelper;
  this->SBO = new svtkOpenGLHelper;
  this->SCBO = new svtkOpenGLHelper;
  this->LinePattern = 0xFFFF;
}

//-----------------------------------------------------------------------------
svtkOpenGLContextDevice2D::~svtkOpenGLContextDevice2D()
{
  delete this->VBO;
  this->VBO = nullptr;
  delete this->VCBO;
  this->VCBO = nullptr;
  delete this->LinesBO;
  this->LinesBO = nullptr;
  delete this->LinesCBO;
  this->LinesCBO = nullptr;
  delete this->SBO;
  this->SBO = nullptr;
  delete this->SCBO;
  this->SCBO = nullptr;
  delete this->VTBO;
  this->VTBO = nullptr;

  while (!this->MarkerCache.empty())
  {
    this->MarkerCache.back().Value->Delete();
    this->MarkerCache.pop_back();
  }

  this->ProjectionMatrix->Delete();
  this->ModelMatrix->Delete();
  delete this->Storage;
  delete this->PolyDataImpl;
}

svtkMatrix4x4* svtkOpenGLContextDevice2D::GetProjectionMatrix()
{
  return this->ProjectionMatrix->GetMatrix();
}

svtkMatrix4x4* svtkOpenGLContextDevice2D::GetModelMatrix()
{
  return this->ModelMatrix->GetMatrix();
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::Begin(svtkViewport* viewport)
{
  svtkOpenGLClearErrorMacro();
  // Need the actual pixel size of the viewport - ask OpenGL.
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  this->Storage->Offset.Set(static_cast<int>(vp[0]), static_cast<int>(vp[1]));

  this->Storage->Dim.Set(static_cast<int>(vp[2]), static_cast<int>(vp[3]));

  // push a 2D matrix on the stack
  this->ProjectionMatrix->Push();
  this->ProjectionMatrix->Identity();
  this->PushMatrix();
  this->ModelMatrix->Identity();

  double offset = 0.5;
  double xmin = offset;
  double xmax = vp[2] + offset - 1.0;
  double ymin = offset;
  double ymax = vp[3] + offset - 1.0;
  double znear = -2000;
  double zfar = 2000;

  double matrix[4][4];
  svtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = 2 / (xmax - xmin);
  matrix[1][1] = 2 / (ymax - ymin);
  matrix[2][2] = -2 / (zfar - znear);

  matrix[0][3] = -(xmin + xmax) / (xmax - xmin);
  matrix[1][3] = -(ymin + ymax) / (ymax - ymin);
  matrix[2][3] = -(znear + zfar) / (zfar - znear);

  this->ProjectionMatrix->SetMatrix(*matrix);

  // Store the previous state before changing it
  this->Renderer = svtkRenderer::SafeDownCast(viewport);
  this->RenderWindow = svtkOpenGLRenderWindow::SafeDownCast(this->Renderer->GetRenderWindow());
  svtkOpenGLState* ostate = this->RenderWindow->GetState();

  this->Storage->SaveGLState(ostate);
  ostate->svtkglDisable(GL_DEPTH_TEST);
  ostate->svtkglEnable(GL_BLEND);

  this->RenderWindow->GetShaderCache()->ReleaseCurrentShader();

  // Enable simple line smoothing if multisampling is on.
  if (this->Renderer->GetRenderWindow()->GetMultiSamples())
  {
    glEnable(GL_LINE_SMOOTH);
  }

  this->InRender = true;
  svtkOpenGLCheckErrorMacro("failed after Begin");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::End()
{
  if (!this->InRender)
  {
    return;
  }

  this->ProjectionMatrix->Pop();
  this->PopMatrix();

  svtkOpenGLClearErrorMacro();

  // Restore the GL state that we changed
  svtkOpenGLState* ostate = this->RenderWindow->GetState();
  this->Storage->RestoreGLState(ostate);

  // Disable simple line smoothing if multisampling is on.
  if (this->Renderer->GetRenderWindow()->GetMultiSamples())
  {
    glDisable(GL_LINE_SMOOTH);
  }

  this->PolyDataImpl->HandleEndFrame();

  this->RenderWindow = nullptr;
  this->InRender = false;

  svtkOpenGLCheckErrorMacro("failed after End");
}

// ----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::BufferIdModeBegin(svtkAbstractContextBufferId* bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId != nullptr);

  svtkOpenGLClearErrorMacro();

  this->BufferId = bufferId;

  // Save OpenGL state.
  svtkOpenGLState* ostate = this->RenderWindow->GetState();
  this->Storage->SaveGLState(ostate, true);

  int lowerLeft[2];
  int usize, vsize;
  this->Renderer->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);

  // push a 2D matrix on the stack
  this->ProjectionMatrix->Push();
  this->ProjectionMatrix->Identity();
  this->PushMatrix();
  this->ModelMatrix->Identity();

  double xmin = 0.5;
  double xmax = usize + 0.5;
  double ymin = 0.5;
  double ymax = vsize + 0.5;
  double znear = -1;
  double zfar = 1;

  double matrix[4][4];
  svtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = 2 / (xmax - xmin);
  matrix[1][1] = 2 / (ymax - ymin);
  matrix[2][2] = -2 / (zfar - znear);

  matrix[0][3] = -(xmin + xmax) / (xmax - xmin);
  matrix[1][3] = -(ymin + ymax) / (ymax - ymin);
  matrix[2][3] = -(znear + zfar) / (zfar - znear);

  this->ProjectionMatrix->SetMatrix(*matrix);

  ostate->svtkglDrawBuffer(GL_BACK_LEFT);
  ostate->svtkglClearColor(0.0, 0.0, 0.0, 0.0); // id=0 means no hit, just background
  ostate->svtkglClear(GL_COLOR_BUFFER_BIT);
  ostate->svtkglDisable(GL_STENCIL_TEST);
  ostate->svtkglDisable(GL_DEPTH_TEST);
  ostate->svtkglDisable(GL_BLEND);

  svtkOpenGLCheckErrorMacro("failed after BufferIdModeBegin");

  assert("post: started" && this->GetBufferIdMode());
}

// ----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());

  svtkOpenGLClearErrorMacro();

  // Assume the renderer has been set previously during rendering (sse Begin())
  int lowerLeft[2];
  int usize, vsize;
  this->Renderer->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);
  this->BufferId->SetValues(lowerLeft[0], lowerLeft[1]);

  this->ProjectionMatrix->Pop();
  this->PopMatrix();

  this->Storage->RestoreGLState(this->RenderWindow->GetState(), true);

  this->BufferId = nullptr;

  svtkOpenGLCheckErrorMacro("failed after BufferIdModeEnd");

  assert("post: done" && !this->GetBufferIdMode());
}

void svtkOpenGLContextDevice2D::SetMatrices(svtkShaderProgram* prog)
{
  prog->SetUniformMatrix("WCDCMatrix", this->ProjectionMatrix->GetMatrix());
  prog->SetUniformMatrix("MCWCMatrix", this->ModelMatrix->GetMatrix());
}

void svtkOpenGLContextDevice2D::BuildVBO(
  svtkOpenGLHelper* cellBO, float* f, int nv, unsigned char* colors, int nc, float* tcoords)
{
  int stride = 2;
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
    va[i * stride] = f[i * 2];
    va[i * stride + 1] = f[i * 2 + 1];
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
        cellBO->Program, cellBO->IBO, "vertexMC", 0, sizeof(float) * stride, SVTK_FLOAT, 2, false))
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

void svtkOpenGLContextDevice2D::ReadyVBOProgram()
{
  svtkOpenGLGL2PSHelper* gl2ps = PrepProgramForGL2PS(*this->VBO);

  if (!this->VBO->Program)
  {
    svtkTransformFeedback* tf = nullptr;
    if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
    {
      tf = svtkTransformFeedback::New();
      tf->AddVarying(svtkTransformFeedback::Vertex_ClipCoordinate_F, "gl_Position");
    }
    std::string vs = "//SVTK::System::Dec\n";
    vs += myVertShader;
    std::string fs = "//SVTK::System::Dec\n";
    fs += myFragShader;
    this->VBO->Program =
      this->RenderWindow->GetShaderCache()->ReadyShaderProgram(vs.c_str(), fs.c_str(), "", tf);
    if (tf)
    {
      tf->Delete();
      tf = nullptr;
    }
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VBO->Program);
  }
}

void svtkOpenGLContextDevice2D::ReadyVCBOProgram()
{
  svtkOpenGLGL2PSHelper* gl2ps = PrepProgramForGL2PS(*this->VCBO);

  if (!this->VCBO->Program)
  {
    svtkTransformFeedback* tf = nullptr;
    if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
    {
      tf = svtkTransformFeedback::New();
      tf->AddVarying(svtkTransformFeedback::Vertex_ClipCoordinate_F, "gl_Position");
      tf->AddVarying(svtkTransformFeedback::Color_RGBA_F, "vertexColor");
    }
    std::string vs = "//SVTK::System::Dec\n#define haveColors\n";
    vs += myVertShader;
    std::string fs = "//SVTK::System::Dec\n#define haveColors\n";
    fs += myFragShader;
    this->VCBO->Program =
      this->RenderWindow->GetShaderCache()->ReadyShaderProgram(vs.c_str(), fs.c_str(), "", tf);
    if (tf)
    {
      tf->Delete();
      tf = nullptr;
    }
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VCBO->Program);
  }
}

void svtkOpenGLContextDevice2D::ReadyLinesBOProgram()
{
  svtkOpenGLGL2PSHelper* gl2ps = PrepProgramForGL2PS(*this->VCBO);

  if (!this->LinesBO->Program)
  {
    svtkTransformFeedback* tf = nullptr;
    if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
    {
      tf = svtkTransformFeedback::New();
      tf->AddVarying(svtkTransformFeedback::Vertex_ClipCoordinate_F, "gl_Position");
    }
    std::string vs = "//SVTK::System::Dec\n#define haveLines\n";
    vs += myVertShader;
    std::string fs = "//SVTK::System::Dec\n#define haveLines\n";
    fs += myFragShader;
    this->LinesBO->Program =
      this->RenderWindow->GetShaderCache()->ReadyShaderProgram(vs.c_str(), fs.c_str(), "", tf);
    if (tf)
    {
      tf->Delete();
      tf = nullptr;
    }
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->LinesBO->Program);
  }
}

void svtkOpenGLContextDevice2D::ReadyLinesCBOProgram()
{
  svtkOpenGLGL2PSHelper* gl2ps = PrepProgramForGL2PS(*this->VCBO);

  if (!this->LinesCBO->Program)
  {
    svtkTransformFeedback* tf = nullptr;
    if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
    {
      tf = svtkTransformFeedback::New();
      tf->AddVarying(svtkTransformFeedback::Vertex_ClipCoordinate_F, "gl_Position");
      tf->AddVarying(svtkTransformFeedback::Color_RGBA_F, "vertexColor");
    }
    std::string vs = "//SVTK::System::Dec\n#define haveColors\n#define haveLines\n";
    vs += myVertShader;
    std::string fs = "//SVTK::System::Dec\n#define haveColors\n#define haveLines\n";
    fs += myFragShader;
    this->LinesCBO->Program =
      this->RenderWindow->GetShaderCache()->ReadyShaderProgram(vs.c_str(), fs.c_str(), "", tf);
    if (tf)
    {
      tf->Delete();
      tf = nullptr;
    }
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->LinesCBO->Program);
  }
}

void svtkOpenGLContextDevice2D::ReadyVTBOProgram()
{
  if (!this->VTBO->Program)
  {
    std::string vs = "//SVTK::System::Dec\n#define haveTCoords\n";
    vs += myVertShader;
    std::string fs = "//SVTK::System::Dec\n#define haveTCoords\n";
    fs += myFragShader;
    this->VTBO->Program =
      this->RenderWindow->GetShaderCache()->ReadyShaderProgram(vs.c_str(), fs.c_str(), "");
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VTBO->Program);
  }
}

void svtkOpenGLContextDevice2D::ReadySBOProgram()
{
  if (!this->SBO->Program)
  {
    this->SBO->Program = this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
      // vertex shader
      "//SVTK::System::Dec\n"
      "in vec2 vertexMC;\n"
      "uniform mat4 WCDCMatrix;\n"
      "uniform mat4 MCWCMatrix;\n"
      "void main() {\n"
      "vec4 vertex = vec4(vertexMC.xy, 0.0, 1.0);\n"
      "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
      // fragment shader
      "//SVTK::System::Dec\n"
      "//SVTK::Output::Dec\n"
      "uniform vec4 vertexColor;\n"
      "uniform sampler2D texture1;\n"
      "void main() { gl_FragData[0] = vertexColor*texture2D(texture1, gl_PointCoord); }",
      // geometry shader
      "");
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->SBO->Program);
  }
}

void svtkOpenGLContextDevice2D::ReadySCBOProgram()
{
  if (!this->SCBO->Program)
  {
    this->SCBO->Program = this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
      // vertex shader
      "//SVTK::System::Dec\n"
      "in vec2 vertexMC;\n"
      "in vec4 vertexScalar;\n"
      "uniform mat4 WCDCMatrix;\n"
      "uniform mat4 MCWCMatrix;\n"
      "out vec4 vertexColor;\n"
      "void main() {\n"
      "vec4 vertex = vec4(vertexMC.xy, 0.0, 1.0);\n"
      "vertexColor = vertexScalar;\n"
      "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
      // fragment shader
      "//SVTK::System::Dec\n"
      "//SVTK::Output::Dec\n"
      "in vec4 vertexColor;\n"
      "uniform sampler2D texture1;\n"
      "void main() { gl_FragData[0] = vertexColor*texture2D(texture1, gl_PointCoord); }",
      // geometry shader
      "");
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->SCBO->Program);
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawPoly(float* f, int n, unsigned char* colors, int nc)
{
  assert("f must be non-null" && f != nullptr);
  assert("n must be greater than 0" && n > 0);

  if (SkipDraw())
  {
    return;
  }

  if (this->Pen->GetLineType() == svtkPen::NO_PEN)
  {
    return;
  }

  // Skip transparent elements.
  if (!colors && this->Pen->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  svtkOpenGLClearErrorMacro();
  this->SetLineType(this->Pen->GetLineType());

  svtkOpenGLHelper* cbo = nullptr;
  if (colors)
  {
    this->ReadyLinesCBOProgram();
    cbo = this->LinesCBO;
  }
  else
  {
    this->ReadyLinesBOProgram();
    cbo = this->LinesBO;
    if (cbo->Program)
    {
      cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
    }
  }
  if (!cbo->Program)
  {
    return;
  }

  cbo->Program->SetUniformi("stipple", this->LinePattern);

  this->SetMatrices(cbo->Program);

  // for line stipple we need to compute the scaled
  // cumulative linear distance
  double* scale = this->ModelMatrix->GetScale();
  std::vector<float> distances;
  distances.resize(n * 2);
  float totDist = 0.0;
  distances[0] = 0.0;
  for (int i = 1; i < n; i++)
  {
    float xDel = scale[0] * (f[i * 2] - f[i * 2 - 2]);
    float yDel = scale[1] * (f[i * 2 + 1] - f[i * 2 - 1]);
    // discarding infinite coordinates
    totDist += (std::abs(yDel) != std::numeric_limits<float>::infinity() &&
                 std::abs(xDel) != std::numeric_limits<float>::infinity())
      ? sqrt(xDel * xDel + yDel * yDel)
      : 0;
    distances[i * 2] = totDist;
  }

  // For GL2PS captures, use the path that draws lines instead of triangles --
  // GL2PS can handle stipples and linewidths just fine.
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();

  if (this->Pen->GetWidth() > 1.0 &&
    !(gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture))
  {
    // convert to triangles and draw, this is because
    // OpenGL no longer supports wide lines directly
    float hwidth = this->Pen->GetWidth() / 2.0;
    std::vector<float> newVerts;
    std::vector<unsigned char> newColors;
    std::vector<float> newDistances;
    newDistances.resize((n - 1) * 12);
    for (int i = 0; i < n - 1; i++)
    {
      // for each line segment draw two triangles
      // start by computing the direction
      svtkVector2f dir(
        (f[i * 2 + 2] - f[i * 2]) * scale[0], (f[i * 2 + 3] - f[i * 2 + 1]) * scale[1]);
      svtkVector2f norm(-dir.GetY(), dir.GetX());
      norm.Normalize();
      norm.SetX(hwidth * norm.GetX() / scale[0]);
      norm.SetY(hwidth * norm.GetY() / scale[1]);

      newVerts.push_back(f[i * 2] + norm.GetX());
      newVerts.push_back(f[i * 2 + 1] + norm.GetY());
      newVerts.push_back(f[i * 2] - norm.GetX());
      newVerts.push_back(f[i * 2 + 1] - norm.GetY());
      newVerts.push_back(f[i * 2 + 2] - norm.GetX());
      newVerts.push_back(f[i * 2 + 3] - norm.GetY());

      newVerts.push_back(f[i * 2] + norm.GetX());
      newVerts.push_back(f[i * 2 + 1] + norm.GetY());
      newVerts.push_back(f[i * 2 + 2] - norm.GetX());
      newVerts.push_back(f[i * 2 + 3] - norm.GetY());
      newVerts.push_back(f[i * 2 + 2] + norm.GetX());
      newVerts.push_back(f[i * 2 + 3] + norm.GetY());

      if (colors)
      {
        copyColors(newColors, colors + i * nc, nc);
        copyColors(newColors, colors + i * nc, nc);
        copyColors(newColors, colors + (i + 1) * nc, nc);
        copyColors(newColors, colors + i * nc, nc);
        copyColors(newColors, colors + (i + 1) * nc, nc);
        copyColors(newColors, colors + (i + 1) * nc, nc);
      }

      newDistances[i * 12] = distances[i * 2];
      newDistances[i * 12 + 2] = distances[i * 2];
      newDistances[i * 12 + 4] = distances[i * 2 + 2];
      newDistances[i * 12 + 6] = distances[i * 2];
      newDistances[i * 12 + 8] = distances[i * 2 + 2];
      newDistances[i * 12 + 10] = distances[i * 2 + 2];
    }

    this->BuildVBO(cbo, &(newVerts[0]), static_cast<int>(newVerts.size() / 2),
      colors ? &(newColors[0]) : nullptr, nc, &(newDistances[0]));

    PreDraw(*cbo, GL_TRIANGLES, newVerts.size() / 2);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(newVerts.size() / 2));
    PostDraw(*cbo, this->Renderer, this->Pen->GetColor());
  }
  else
  {
    this->SetLineWidth(this->Pen->GetWidth());
    this->BuildVBO(cbo, f, n, colors, nc, &(distances[0]));
    PreDraw(*cbo, GL_LINE_STRIP, n);
    glDrawArrays(GL_LINE_STRIP, 0, n);
    PostDraw(*cbo, this->Renderer, this->Pen->GetColor());
    this->SetLineWidth(1.0);
  }

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  svtkOpenGLCheckErrorMacro("failed after DrawPoly");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawLines(float* f, int n, unsigned char* colors, int nc)
{
  assert("f must be non-null" && f != nullptr);
  assert("n must be greater than 0" && n > 0);

  if (SkipDraw())
  {
    return;
  }

  if (this->Pen->GetLineType() == svtkPen::NO_PEN)
  {
    return;
  }

  // Skip transparent elements.
  if (!colors && this->Pen->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  svtkOpenGLClearErrorMacro();

  this->SetLineType(this->Pen->GetLineType());

  svtkOpenGLHelper* cbo = nullptr;
  if (colors)
  {
    this->ReadyLinesCBOProgram();
    cbo = this->LinesCBO;
  }
  else
  {
    this->ReadyLinesBOProgram();
    cbo = this->LinesBO;
    if (!cbo->Program)
    {
      return;
    }
    cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
  }
  if (!cbo->Program)
  {
    return;
  }

  cbo->Program->SetUniformi("stipple", this->LinePattern);

  this->SetMatrices(cbo->Program);

  // for line stipple we need to compute the scaled
  // cumulative linear distance
  double* scale = this->ModelMatrix->GetScale();
  std::vector<float> distances;
  distances.resize(n * 2);
  float totDist = 0.0;
  distances[0] = 0.0;
  for (int i = 1; i < n; i++)
  {
    float xDel = scale[0] * (f[i * 2] - f[i * 2 - 2]);
    float yDel = scale[1] * (f[i * 2 + 1] - f[i * 2 - 1]);
    totDist += sqrt(xDel * xDel + yDel * yDel);
    distances[i * 2] = totDist;
  }

  if (this->Pen->GetWidth() > 1.0)
  {
    // convert to triangles and draw, this is because
    // OpenGL no longer supports wide lines directly
    float hwidth = this->Pen->GetWidth() / 2.0;
    std::vector<float> newVerts;
    std::vector<unsigned char> newColors;
    std::vector<float> newDistances;
    newDistances.resize((n / 2) * 12);
    for (int i = 0; i < n - 1; i += 2)
    {
      // for each line segment draw two triangles
      // start by computing the direction
      svtkVector2f dir(
        (f[i * 2 + 2] - f[i * 2]) * scale[0], (f[i * 2 + 3] - f[i * 2 + 1]) * scale[1]);
      svtkVector2f norm(-dir.GetY(), dir.GetX());
      norm.Normalize();
      norm.SetX(hwidth * norm.GetX() / scale[0]);
      norm.SetY(hwidth * norm.GetY() / scale[1]);

      newVerts.push_back(f[i * 2] + norm.GetX());
      newVerts.push_back(f[i * 2 + 1] + norm.GetY());
      newVerts.push_back(f[i * 2] - norm.GetX());
      newVerts.push_back(f[i * 2 + 1] - norm.GetY());
      newVerts.push_back(f[i * 2 + 2] - norm.GetX());
      newVerts.push_back(f[i * 2 + 3] - norm.GetY());

      newVerts.push_back(f[i * 2] + norm.GetX());
      newVerts.push_back(f[i * 2 + 1] + norm.GetY());
      newVerts.push_back(f[i * 2 + 2] - norm.GetX());
      newVerts.push_back(f[i * 2 + 3] - norm.GetY());
      newVerts.push_back(f[i * 2 + 2] + norm.GetX());
      newVerts.push_back(f[i * 2 + 3] + norm.GetY());

      if (colors)
      {
        copyColors(newColors, colors + i * nc, nc);
        copyColors(newColors, colors + i * nc, nc);
        copyColors(newColors, colors + (i + 1) * nc, nc);
        copyColors(newColors, colors + i * nc, nc);
        copyColors(newColors, colors + (i + 1) * nc, nc);
        copyColors(newColors, colors + (i + 1) * nc, nc);
      }

      newDistances[i * 6] = distances[i * 2];
      newDistances[i * 6 + 2] = distances[i * 2];
      newDistances[i * 6 + 4] = distances[i * 2 + 2];
      newDistances[i * 6 + 6] = distances[i * 2];
      newDistances[i * 6 + 8] = distances[i * 2 + 2];
      newDistances[i * 6 + 10] = distances[i * 2 + 2];
    }

    this->BuildVBO(cbo, &(newVerts[0]), static_cast<int>(newVerts.size() / 2),
      colors ? &(newColors[0]) : nullptr, nc, &(newDistances[0]));
    PreDraw(*cbo, GL_TRIANGLES, newVerts.size() / 2);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(newVerts.size() / 2));
    PostDraw(*cbo, this->Renderer, this->Pen->GetColor());
  }
  else
  {
    this->SetLineWidth(this->Pen->GetWidth());
    this->BuildVBO(cbo, f, n, colors, nc, &(distances[0]));
    PreDraw(*cbo, GL_LINES, n);
    glDrawArrays(GL_LINES, 0, n);
    PostDraw(*cbo, this->Renderer, this->Pen->GetColor());
    this->SetLineWidth(1.0);
  }

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  svtkOpenGLCheckErrorMacro("failed after DrawLines");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawPoints(float* f, int n, unsigned char* c, int nc)
{
  if (SkipDraw())
  {
    return;
  }

  // Skip transparent elements.
  if (!c && this->Pen->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  svtkOpenGLClearErrorMacro();

  svtkOpenGLHelper* cbo = nullptr;
  if (c)
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

  this->SetPointSize(this->Pen->GetWidth());

  this->BuildVBO(cbo, f, n, c, nc, nullptr);
  this->SetMatrices(cbo->Program);

  PreDraw(*cbo, GL_POINTS, n);
  glDrawArrays(GL_POINTS, 0, n);
  PostDraw(*cbo, this->Renderer, this->Pen->GetColor());

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  svtkOpenGLCheckErrorMacro("failed after DrawPoints");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawPointSprites(
  svtkImageData* sprite, float* points, int n, unsigned char* colors, int nc_comps)
{
  //  // Draw these to the background -- we don't currently export them to GL2PS.
  //  if (SkipDraw())
  //    {
  //    return;
  //    }

  svtkOpenGLClearErrorMacro();
  if (points && n > 0)
  {
    this->SetPointSize(this->Pen->GetWidth());

    svtkOpenGLHelper* cbo = nullptr;
    if (colors)
    {
      this->ReadySCBOProgram();
      cbo = this->SCBO;
      if (!cbo->Program)
      {
        return;
      }
    }
    else
    {
      this->ReadySBOProgram();
      cbo = this->SBO;
      if (!cbo->Program)
      {
        return;
      }
      cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
    }

    this->BuildVBO(cbo, points, n, colors, nc_comps, nullptr);
    this->SetMatrices(cbo->Program);

    if (sprite)
    {
      if (!this->Storage->SpriteTexture)
      {
        this->Storage->SpriteTexture = svtkTexture::New();
      }
      int properties = this->Brush->GetTextureProperties();
      this->Storage->SpriteTexture->SetInputData(sprite);
      this->Storage->SpriteTexture->SetRepeat(properties & svtkContextDevice2D::Repeat);
      this->Storage->SpriteTexture->SetInterpolate(properties & svtkContextDevice2D::Linear);
      this->Storage->SpriteTexture->Render(this->Renderer);
      int tunit = svtkOpenGLTexture::SafeDownCast(this->Storage->SpriteTexture)->GetTextureUnit();
      cbo->Program->SetUniformi("texture1", tunit);
    }

    // We can actually use point sprites here
    if (this->RenderWindow->IsPointSpriteBugPresent())
    {
      glEnable(GL_POINT_SPRITE);
      glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
    }
    glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);

    glDrawArrays(GL_POINTS, 0, n);

    // free everything
    cbo->ReleaseGraphicsResources(this->RenderWindow);
    if (this->RenderWindow->IsPointSpriteBugPresent())
    {
      glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_FALSE);
      glDisable(GL_POINT_SPRITE);
    }

    if (sprite)
    {
      this->Storage->SpriteTexture->PostRender(this->Renderer);
    }
  }
  else
  {
    svtkWarningMacro(<< "Points supplied without a valid image or pointer.");
  }
  svtkOpenGLCheckErrorMacro("failed after DrawPointSprites");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawMarkers(
  int shape, bool highlight, float* points, int n, unsigned char* colors, int nc_comps)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case svtkOpenGLGL2PSHelper::Capture:
        this->DrawMarkersGL2PS(shape, highlight, points, n, colors, nc_comps);
        return;
      case svtkOpenGLGL2PSHelper::Background:
        return; // Do nothing.
      case svtkOpenGLGL2PSHelper::Inactive:
        break; // Render as normal.
    }
  }

  // Get a point sprite for the shape
  svtkImageData* sprite = this->GetMarker(shape, this->Pen->GetWidth(), highlight);
  this->DrawPointSprites(sprite, points, n, colors, nc_comps);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawQuad(float* f, int n)
{
  if (SkipDraw())
  {
    return;
  }

  if (!f || n <= 0)
  {
    svtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
  }

  // convert quads to triangles
  std::vector<float> tverts;
  int numTVerts = 6 * n / 4;
  tverts.resize(numTVerts * 2);
  int offset[6] = { 0, 1, 2, 0, 2, 3 };
  for (int i = 0; i < numTVerts; i++)
  {
    int index = 2 * (4 * (i / 6) + offset[i % 6]);
    tverts[i * 2] = f[index];
    tverts[i * 2 + 1] = f[index + 1];
  }

  this->CoreDrawTriangles(tverts);
}

void svtkOpenGLContextDevice2D::CoreDrawTriangles(
  std::vector<float>& tverts, unsigned char* colors, int numComp)
{
  if (SkipDraw())
  {
    return;
  }

  svtkOpenGLClearErrorMacro();

  float* texCoord = nullptr;
  svtkOpenGLHelper* cbo = nullptr;
  if (this->Brush->GetTexture())
  {
    this->ReadyVTBOProgram();
    cbo = this->VTBO;
    if (!cbo->Program)
    {
      return;
    }
    this->SetTexture(this->Brush->GetTexture(), this->Brush->GetTextureProperties());
    this->Storage->Texture->Render(this->Renderer);
    texCoord = this->Storage->TexCoords(&(tverts[0]), static_cast<int>(tverts.size() / 2));

    int tunit = svtkOpenGLTexture::SafeDownCast(this->Storage->Texture)->GetTextureUnit();
    cbo->Program->SetUniformi("texture1", tunit);
  }
  else if (colors && numComp > 0)
  {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
  }
  else
  {
    // Skip transparent elements.
    if (this->Brush->GetColorObject().GetAlpha() == 0)
    {
      return;
    }
    this->ReadyVBOProgram();
    cbo = this->VBO;
  }

  if (!cbo->Program)
  {
    return;
  }

  cbo->Program->SetUniform4uc("vertexColor", this->Brush->GetColor());

  this->BuildVBO(cbo, &(tverts[0]), static_cast<int>(tverts.size() / 2), colors, numComp, texCoord);

  this->SetMatrices(cbo->Program);

  PreDraw(*cbo, GL_TRIANGLES, tverts.size() / 2);
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(tverts.size() / 2));
  PostDraw(*cbo, this->Renderer, this->Brush->GetColor());

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  if (this->Storage->Texture)
  {
    this->Storage->Texture->PostRender(this->Renderer);
    delete[] texCoord;
  }
  svtkOpenGLCheckErrorMacro("failed after DrawQuad");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawQuadStrip(float* f, int n)
{
  if (SkipDraw())
  {
    return;
  }

  if (!f || n <= 0)
  {
    svtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
  }

  // convert quad strips to triangles
  std::vector<float> tverts;
  int numTVerts = 3 * (n - 2);
  tverts.resize(numTVerts * 2);
  int offset[6] = { 0, 1, 3, 0, 3, 2 };
  for (int i = 0; i < numTVerts; i++)
  {
    int index = 2 * (2 * (i / 6) + offset[i % 6]);
    tverts[i * 2] = f[index];
    tverts[i * 2 + 1] = f[index + 1];
  }

  this->CoreDrawTriangles(tverts);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawPolygon(float* f, int n)
{
  if (SkipDraw())
  {
    return;
  }

  if (!f || n <= 0)
  {
    svtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
  }

  // convert polygon to triangles
  std::vector<float> tverts;
  int numTVerts = 3 * (n - 2);
  tverts.reserve(numTVerts * 2);
  for (int i = 0; i < n - 2; i++)
  {
    tverts.push_back(f[0]);
    tverts.push_back(f[1]);
    tverts.push_back(f[i * 2 + 2]);
    tverts.push_back(f[i * 2 + 3]);
    tverts.push_back(f[i * 2 + 4]);
    tverts.push_back(f[i * 2 + 5]);
  }

  this->CoreDrawTriangles(tverts);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawColoredPolygon(
  float* f, int n, unsigned char* colors, int nc_comps)
{
  if (SkipDraw())
  {
    return;
  }

  if (!f || n <= 0)
  {
    svtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
  }

  // convert polygon to triangles
  int numTVerts = 3 * (n - 2);

  std::vector<float> tverts;
  tverts.reserve(numTVerts * 2);

  std::vector<unsigned char> tcolors;
  if (colors)
  {
    tcolors.resize(numTVerts * nc_comps);
  }
  std::vector<unsigned char>::iterator colIt = tcolors.begin();

  for (int i = 0; i < n - 2; i++)
  {
    tverts.push_back(f[0]);
    tverts.push_back(f[1]);
    tverts.push_back(f[i * 2 + 2]);
    tverts.push_back(f[i * 2 + 3]);
    tverts.push_back(f[i * 2 + 4]);
    tverts.push_back(f[i * 2 + 5]);
    if (colors)
    {
      std::copy(colors, colors + nc_comps, colIt);
      colIt += nc_comps;
      std::copy(colors + ((i + 1) * nc_comps), colors + ((i + 3) * nc_comps), colIt);
      colIt += 2 * nc_comps;
    }
  }

  this->CoreDrawTriangles(tverts, colors ? tcolors.data() : nullptr, nc_comps);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawEllipseWedge(float x, float y, float outRx, float outRy,
  float inRx, float inRy, float startAngle, float stopAngle)

{
  assert("pre: positive_outRx" && outRx >= 0.0f);
  assert("pre: positive_outRy" && outRy >= 0.0f);
  assert("pre: positive_inRx" && inRx >= 0.0f);
  assert("pre: positive_inRy" && inRy >= 0.0f);
  assert("pre: ordered_rx" && inRx <= outRx);
  assert("pre: ordered_ry" && inRy <= outRy);

  if (SkipDraw())
  {
    return;
  }

  if (outRy == 0.0f && outRx == 0.0f)
  {
    // we make sure maxRadius will never be null.
    return;
  }

  // If the 'wedge' is actually a full circle, gl2ps can just insert a circle
  // instead of using a polygonal approximation.
  if (IsFullCircle(startAngle, stopAngle))
  {
    svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
    if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
    {
      this->DrawWedgeGL2PS(x, y, outRx, outRy, inRx, inRy);
      return;
    }
  }

  int iterations = this->GetNumberOfArcIterations(outRx, outRy, startAngle, stopAngle);

  // step in radians.
  double step = svtkMath::RadiansFromDegrees(stopAngle - startAngle) / (iterations);

  // step have to be lesser or equal to maxStep computed inside
  // GetNumberOfIterations()

  double rstart = svtkMath::RadiansFromDegrees(startAngle);

  // the A vertices (0,2,4,..) are on the inner side
  // the B vertices (1,3,5,..) are on the outer side
  // (A and B vertices terms come from triangle strip definition in
  // OpenGL spec)
  // we are iterating counterclockwise

  // convert polygon to triangles
  std::vector<float> tverts;
  int numTVerts = 6 * iterations;
  tverts.resize(numTVerts * 2);
  int offset[6] = { 0, 1, 3, 0, 3, 2 };
  for (int i = 0; i < numTVerts; i++)
  {
    int index = i / 6 + offset[i % 6] / 2;
    double radiusX = (offset[i % 6] % 2) ? outRx : inRx;
    double radiusY = (offset[i % 6] % 2) ? outRy : inRy;
    double a = rstart + index * step;
    tverts.push_back(radiusX * cos(a) + x);
    tverts.push_back(radiusY * sin(a) + y);
  }

  this->CoreDrawTriangles(tverts);
}

// ----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawEllipticArc(
  float x, float y, float rX, float rY, float startAngle, float stopAngle)
{
  assert("pre: positive_rX" && rX >= 0);
  assert("pre: positive_rY" && rY >= 0);

  if (SkipDraw())
  {
    return;
  }

  if (rX == 0.0f && rY == 0.0f)
  {
    // we make sure maxRadius will never be null.
    return;
  }

  // If the 'arc' is actually a full circle, gl2ps can just insert a circle
  // instead of using a polygonal approximation.
  if (IsFullCircle(startAngle, stopAngle))
  {
    svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
    if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
    {
      this->DrawCircleGL2PS(x, y, rX, rY);
      return;
    }
  }

  svtkOpenGLClearErrorMacro();

  int iterations = this->GetNumberOfArcIterations(rX, rY, startAngle, stopAngle);

  float* p = new float[2 * (iterations + 1)];

  // step in radians.
  double step = svtkMath::RadiansFromDegrees(stopAngle - startAngle) / (iterations);

  // step have to be lesser or equal to maxStep computed inside
  // GetNumberOfIterations()
  double rstart = svtkMath::RadiansFromDegrees(startAngle);

  // we are iterating counterclockwise
  for (int i = 0; i <= iterations; ++i)
  {
    double a = rstart + i * step;
    p[2 * i] = rX * cos(a) + x;
    p[2 * i + 1] = rY * sin(a) + y;
  }

  this->DrawPolygon(p, iterations + 1);
  this->DrawPoly(p, iterations + 1);
  delete[] p;

  svtkOpenGLCheckErrorMacro("failed after DrawEllipseArc");
}

// ----------------------------------------------------------------------------
int svtkOpenGLContextDevice2D::GetNumberOfArcIterations(
  float rX, float rY, float startAngle, float stopAngle)
{
  assert("pre: positive_rX" && rX >= 0.0f);
  assert("pre: positive_rY" && rY >= 0.0f);
  assert("pre: not_both_null" && (rX > 0.0 || rY > 0.0));

  // 1.0: pixel precision. 0.5 (subpixel precision, useful with multisampling)
  double error = 4.0; // experience shows 4.0 is visually enough.

  // The tessellation is the most visible on the biggest radius.
  double maxRadius;
  if (rX >= rY)
  {
    maxRadius = rX;
  }
  else
  {
    maxRadius = rY;
  }

  if (error > maxRadius)
  {
    // to make sure the argument of asin() is in a valid range.
    error = maxRadius;
  }

  // Angle of a sector so that its chord is `error' pixels.
  // This is will be our maximum angle step.
  double maxStep = 2.0 * asin(error / (2.0 * maxRadius));

  // ceil because we want to make sure we don't underestimate the number of
  // iterations by 1.
  return static_cast<int>(ceil(svtkMath::RadiansFromDegrees(stopAngle - startAngle) / maxStep));
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawString(float* point, const svtkStdString& string)
{
  this->DrawString(point, svtkUnicodeString::from_utf8(string));
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::ComputeStringBounds(const svtkStdString& string, float bounds[4])
{
  this->ComputeStringBoundsInternal(svtkUnicodeString::from_utf8(string), bounds);
  bounds[0] = 0.f;
  bounds[1] = 0.f;
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::ComputeJustifiedStringBounds(const char* string, float bounds[4])
{
  this->ComputeStringBoundsInternal(svtkUnicodeString::from_utf8(string), bounds);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawString(float* point, const svtkUnicodeString& string)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case svtkOpenGLGL2PSHelper::Capture:
      {
        float tx = point[0];
        float ty = point[1];
        this->TransformPoint(tx, ty);
        double x[3] = { tx, ty, 0. };
        gl2ps->DrawString(string.utf8_str(), this->TextProp, x, 0., this->Renderer);
        return;
      }
      case svtkOpenGLGL2PSHelper::Background:
        return; // Do nothing.
      case svtkOpenGLGL2PSHelper::Inactive:
        break; // Render as normal.
    }
  }

  svtkTextRenderer* tren = svtkTextRenderer::GetInstance();
  if (!tren)
  {
    svtkErrorMacro("No text renderer available. Link to svtkRenderingFreeType "
                  "to get the default implementation.");
    return;
  }

  svtkOpenGLClearErrorMacro();

  double* mv = this->ModelMatrix->GetMatrix()->Element[0];
  float xScale = mv[0];
  float yScale = mv[5];

  float p[] = { std::floor(point[0] * xScale) / xScale, std::floor(point[1] * yScale) / yScale };

  // TODO this currently ignores svtkContextScene::ScaleTiles. Not sure how to
  // get at that from here, but this is better than ignoring scaling altogether.
  // TODO Also, FreeType supports anisotropic DPI. Might be needed if the
  // tileScale isn't homogeneous, but we'll need to update the textrenderer API
  // and see if MPL/mathtext can support it.
  int tileScale[2];
  this->RenderWindow->GetTileScale(tileScale);
  int dpi = this->RenderWindow->GetDPI() * std::max(tileScale[0], tileScale[1]);

  // Cache rendered text strings
  svtkTextureImageCache<UTF16TextPropertyKey>::CacheData& cache =
    this->Storage->TextTextureCache.GetCacheData(UTF16TextPropertyKey(this->TextProp, string, dpi));
  svtkImageData* image = cache.ImageData;
  if (image->GetNumberOfPoints() == 0 && image->GetNumberOfCells() == 0)
  {
    int textDims[2];
    if (!tren->RenderString(this->TextProp, string, image, textDims, dpi))
    {
      svtkErrorMacro("Error rendering string: " << string);
      return;
    }
    if (!tren->GetMetrics(this->TextProp, string, cache.Metrics, dpi))
    {
      svtkErrorMacro("Error computing bounding box for string: " << string);
      return;
    }
  }
  svtkTexture* texture = cache.Texture;
  texture->Render(this->Renderer);

  int imgDims[3];
  image->GetDimensions(imgDims);

  float textWidth =
    static_cast<float>(cache.Metrics.BoundingBox[1] - cache.Metrics.BoundingBox[0] + 1);
  float textHeight =
    static_cast<float>(cache.Metrics.BoundingBox[3] - cache.Metrics.BoundingBox[2] + 1);

  float width = textWidth / xScale;
  float height = textHeight / yScale;

  float xw = textWidth / static_cast<float>(imgDims[0]);
  float xh = textHeight / static_cast<float>(imgDims[1]);

  // Align the text (the 0 point of the bounding box is aligned to the
  // rotated and justified anchor point, so just translate by the bbox origin):
  p[0] += cache.Metrics.BoundingBox[0] / xScale;
  p[1] += cache.Metrics.BoundingBox[2] / yScale;

  float points[] = { p[0], p[1], p[0] + width, p[1], p[0] + width, p[1] + height, p[0], p[1],
    p[0] + width, p[1] + height, p[0], p[1] + height };

  float texCoord[] = { 0.0f, 0.0f, xw, 0.0f, xw, xh, 0.0f, 0.0f, xw, xh, 0.0f, xh };

  svtkOpenGLClearErrorMacro();

  this->ReadyVTBOProgram();
  svtkOpenGLHelper* cbo = this->VTBO;
  if (!cbo->Program)
  {
    return;
  }
  int tunit = svtkOpenGLTexture::SafeDownCast(texture)->GetTextureUnit();
  cbo->Program->SetUniformi("texture1", tunit);

  this->BuildVBO(cbo, points, 6, nullptr, 0, texCoord);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  texture->PostRender(this->Renderer);

  svtkOpenGLCheckErrorMacro("failed after DrawString");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::ComputeStringBounds(const svtkUnicodeString& string, float bounds[4])
{
  this->ComputeStringBoundsInternal(string, bounds);
  bounds[0] = 0.f;
  bounds[1] = 0.f;
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawMathTextString(float point[2], const svtkStdString& string)
{
  // The default text renderer detects and handles mathtext now. Just use the
  // regular implementation.
  this->DrawString(point, string);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawImage(float p[2], float scale, svtkImageData* image)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case svtkOpenGLGL2PSHelper::Capture:
        this->DrawImageGL2PS(p, scale, image);
        return;
      case svtkOpenGLGL2PSHelper::Background:
        return; // Do nothing.
      case svtkOpenGLGL2PSHelper::Inactive:
        break; // Draw as normal.
    }
  }

  svtkOpenGLClearErrorMacro();

  this->SetTexture(image);
  this->Storage->Texture->Render(this->Renderer);
  int* extent = image->GetExtent();
  float points[] = { p[0], p[1], p[0] + scale * extent[1] + 1.0f, p[1],
    p[0] + scale * extent[1] + 1.0f, p[1] + scale * extent[3] + 1.0f, p[0], p[1],
    p[0] + scale * extent[1] + 1.0f, p[1] + scale * extent[3] + 1.0f, p[0],
    p[1] + scale * extent[3] + 1.0f };

  float texCoord[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

  svtkOpenGLClearErrorMacro();

  this->ReadyVTBOProgram();
  svtkOpenGLHelper* cbo = this->VTBO;
  if (!cbo->Program)
  {
    return;
  }
  int tunit = svtkOpenGLTexture::SafeDownCast(this->Storage->Texture)->GetTextureUnit();
  cbo->Program->SetUniformi("texture1", tunit);

  this->BuildVBO(cbo, points, 6, nullptr, 0, texCoord);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  this->Storage->Texture->PostRender(this->Renderer);

  svtkOpenGLCheckErrorMacro("failed after DrawImage");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawPolyData(
  float p[2], float scale, svtkPolyData* polyData, svtkUnsignedCharArray* colors, int scalarMode)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case svtkOpenGLGL2PSHelper::Capture:
        // TODO Implement PolyDataGL2PS
        // this->DrawPolyDataGL2PS(pos, image);
        return;
      case svtkOpenGLGL2PSHelper::Background:
        return; // Do nothing.
      case svtkOpenGLGL2PSHelper::Inactive:
        break; // Draw as normal.
    }
  }

  if (SkipDraw())
  {
    return;
  }

  if (polyData->GetLines()->GetNumberOfCells() > 0)
  {
    this->PolyDataImpl->Draw(CellArrayHelper::LINE, polyData, polyData->GetPoints(), p[0], p[1],
      scale, scalarMode, colors);
  }

  if (polyData->GetPolys()->GetNumberOfCells() > 0)
  {
    this->PolyDataImpl->Draw(CellArrayHelper::POLYGON, polyData, polyData->GetPoints(), p[0], p[1],
      scale, scalarMode, colors);
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawImage(const svtkRectf& pos, svtkImageData* image)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case svtkOpenGLGL2PSHelper::Capture:
        this->DrawImageGL2PS(pos, image);
        return;
      case svtkOpenGLGL2PSHelper::Background:
        return; // Do nothing.
      case svtkOpenGLGL2PSHelper::Inactive:
        break; // Draw as normal.
    }
  }

  int tunit = this->RenderWindow->GetTextureUnitManager()->Allocate();
  if (tunit < 0)
  {
    svtkErrorMacro("Hardware does not support the number of textures defined.");
    return;
  }

  this->RenderWindow->GetState()->svtkglActiveTexture(GL_TEXTURE0 + tunit);

  svtkVector2f tex(1.0, 1.0);

  // Call this *after* calling svtkglActiveTexture() to ensure the texture
  // is bound to the correct texture unit.
  GLuint index = this->Storage->TextureFromImage(image, tex);

  float points[] = { pos.GetX(), pos.GetY(), pos.GetX() + pos.GetWidth(), pos.GetY(),
    pos.GetX() + pos.GetWidth(), pos.GetY() + pos.GetHeight(), pos.GetX(), pos.GetY(),
    pos.GetX() + pos.GetWidth(), pos.GetY() + pos.GetHeight(), pos.GetX(),
    pos.GetY() + pos.GetHeight() };

  float texCoord[] = { 0.0f, 0.0f, tex[0], 0.0f, tex[0], tex[1], 0.0f, 0.0f, tex[0], tex[1], 0.0f,
    tex[1] };

  this->ReadyVTBOProgram();
  svtkOpenGLHelper* cbo = this->VTBO;
  if (!cbo->Program)
  {
    return;
  }
  cbo->Program->SetUniformi("texture1", tunit);

  this->BuildVBO(cbo, points, 6, nullptr, 0, texCoord);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  this->RenderWindow->GetTextureUnitManager()->Free(tunit);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  glDeleteTextures(1, &index);

  svtkOpenGLCheckErrorMacro("failed after DrawImage");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::SetColor4(unsigned char*)
{
  svtkErrorMacro("color cannot be set this way\n");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::SetColor(unsigned char*)
{
  svtkErrorMacro("color cannot be set this way\n");
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::SetTexture(svtkImageData* image, int properties)
{
  if (image == nullptr)
  {
    if (this->Storage->Texture)
    {
      this->Storage->Texture->Delete();
      this->Storage->Texture = nullptr;
    }
    return;
  }
  if (this->Storage->Texture == nullptr)
  {
    this->Storage->Texture = svtkTexture::New();
  }
  this->Storage->Texture->SetInputData(image);
  this->Storage->TextureProperties = properties;
  this->Storage->Texture->SetRepeat(properties & svtkContextDevice2D::Repeat);
  this->Storage->Texture->SetInterpolate(properties & svtkContextDevice2D::Linear);
  this->Storage->Texture->EdgeClampOn();
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::SetPointSize(float size)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
  {
    gl2ps->SetPointSize(size);
  }
  glPointSize(size);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::SetLineWidth(float width)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
  {
    gl2ps->SetLineWidth(width);
  }
  glLineWidth(width);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::SetLineType(int type)
{
  this->LinePattern = 0x0000;
  switch (type)
  {
    case svtkPen::NO_PEN:
      this->LinePattern = 0x0000;
      break;
    case svtkPen::DASH_LINE:
      this->LinePattern = 0x00FF;
      break;
    case svtkPen::DOT_LINE:
      this->LinePattern = 0x0101;
      break;
    case svtkPen::DASH_DOT_LINE:
      this->LinePattern = 0x0C0F;
      break;
    case svtkPen::DASH_DOT_DOT_LINE:
      this->LinePattern = 0x1C47;
      break;
    case svtkPen::DENSE_DOT_LINE:
      this->LinePattern = 0x1111;
      break;
    default:
      this->LinePattern = 0xFFFF;
  }

  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps && gl2ps->GetActiveState() == svtkOpenGLGL2PSHelper::Capture)
  {
    gl2ps->SetLineStipple(this->LinePattern);
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::MultiplyMatrix(svtkMatrix3x3* m)
{
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
  double* M = m->GetData();
  double matrix[16];

  matrix[0] = M[0];
  matrix[1] = M[1];
  matrix[2] = 0.0;
  matrix[3] = M[2];
  matrix[4] = M[3];
  matrix[5] = M[4];
  matrix[6] = 0.0;
  matrix[7] = M[5];
  matrix[8] = 0.0;
  matrix[9] = 0.0;
  matrix[10] = 1.0;
  matrix[11] = 0.0;
  matrix[12] = M[6];
  matrix[13] = M[7];
  matrix[14] = 0.0;
  matrix[15] = M[8];

  this->ModelMatrix->Concatenate(matrix);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::SetMatrix(svtkMatrix3x3* m)
{
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
  double* M = m->GetData();
  double matrix[16];

  matrix[0] = M[0];
  matrix[1] = M[1];
  matrix[2] = 0.0;
  matrix[3] = M[2];
  matrix[4] = M[3];
  matrix[5] = M[4];
  matrix[6] = 0.0;
  matrix[7] = M[5];
  matrix[8] = 0.0;
  matrix[9] = 0.0;
  matrix[10] = 1.0;
  matrix[11] = 0.0;
  matrix[12] = M[6];
  matrix[13] = M[7];
  matrix[14] = 0.0;
  matrix[15] = M[8];

  this->ModelMatrix->SetMatrix(matrix);
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::GetMatrix(svtkMatrix3x3* m)
{
  assert("pre: non_null" && m != nullptr);
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
  double* M = m->GetData();
  double* matrix = this->ModelMatrix->GetMatrix()->Element[0];

  M[0] = matrix[0];
  M[1] = matrix[1];
  M[2] = matrix[3];
  M[3] = matrix[4];
  M[4] = matrix[5];
  M[5] = matrix[7];
  M[6] = matrix[12];
  M[7] = matrix[13];
  M[8] = matrix[15];

  m->Modified();
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::PushMatrix()
{
  this->ModelMatrix->Push();
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::PopMatrix()
{
  this->ModelMatrix->Pop();
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::SetClipping(int* dim)
{
  // If the window is using tile scaling, we need to update the clip coordinates
  // relative to the tile being rendered.
  // (see paraview/paraview#17308)
  double tileViewPort[4];
  this->Renderer->GetSVTKWindow()->GetTileViewport(tileViewPort);
  this->Renderer->NormalizedDisplayToDisplay(tileViewPort[0], tileViewPort[1]);
  this->Renderer->NormalizedDisplayToDisplay(tileViewPort[2], tileViewPort[3]);

  svtkRecti tileRect{ svtkContext2D::FloatToInt(tileViewPort[0]),
    svtkContext2D::FloatToInt(tileViewPort[1]), 0, 0 };
  tileRect.AddPoint(
    svtkContext2D::FloatToInt(tileViewPort[2]), svtkContext2D::FloatToInt(tileViewPort[3]));
  // tileRect is the tile being rendered in the current RenderWindow in pixels.

  double viewport[4];
  this->Renderer->GetViewport(viewport);
  this->Renderer->NormalizedDisplayToDisplay(viewport[0], viewport[1]);
  this->Renderer->NormalizedDisplayToDisplay(viewport[2], viewport[3]);
  svtkRecti rendererRect{ svtkContext2D::FloatToInt(viewport[0]),
    svtkContext2D::FloatToInt(viewport[1]), 0, 0 };
  rendererRect.AddPoint(
    svtkContext2D::FloatToInt(viewport[2]), svtkContext2D::FloatToInt(viewport[3]));
  // rendererRect is the viewport in pixels.

  // `dim` is specified as (x,y,width,height) relative to the viewport that this
  // prop is rendering in. So let's fit it in the viewport rect i.e.
  // rendererRect
  svtkRecti clipRect{ dim[0], dim[1], dim[2], dim[3] };
  clipRect.MoveTo(clipRect.GetX() + rendererRect.GetX(), clipRect.GetY() + rendererRect.GetY());
  clipRect.Intersect(rendererRect);

  // Now, clamp the clipRect to the region being shown on the current tile. This
  // generally has no effect since clipRect is wholly contained in tileRect
  // unless tile scaling was being used. In either case, this method will return
  // true as long as the rectangle intersection will produce a valid rectangle.
  if (clipRect.Intersect(tileRect))
  {
    // offset clipRect relative to current tile i.e. window.
    clipRect.MoveTo(clipRect.GetX() - tileRect.GetX(), clipRect.GetY() - tileRect.GetY());
  }
  else
  {
    // clipping region results in empty region, just set to empty.
    clipRect = svtkRecti{ 0, 0, 0, 0 };
  }

  assert(clipRect.GetWidth() >= 0 && clipRect.GetHeight() >= 0);

  this->RenderWindow->GetState()->svtkglScissor(
    clipRect.GetX(), clipRect.GetY(), clipRect.GetWidth(), clipRect.GetHeight());
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::EnableClipping(bool enable)
{
  this->RenderWindow->GetState()->SetEnumState(GL_SCISSOR_TEST, enable);
}

//-----------------------------------------------------------------------------
bool svtkOpenGLContextDevice2D::SetStringRendererToFreeType()
{
  // FreeType is the only choice - nothing to do here
  return true;
}

//-----------------------------------------------------------------------------
bool svtkOpenGLContextDevice2D::SetStringRendererToQt()
{
  // The Qt based strategy is not available
  return false;
}

//----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::ReleaseGraphicsResources(svtkWindow* window)
{
  this->VBO->ReleaseGraphicsResources(window);
  this->VCBO->ReleaseGraphicsResources(window);
  this->LinesBO->ReleaseGraphicsResources(window);
  this->LinesCBO->ReleaseGraphicsResources(window);
  this->SBO->ReleaseGraphicsResources(window);
  this->SCBO->ReleaseGraphicsResources(window);
  this->VTBO->ReleaseGraphicsResources(window);
  if (this->Storage->Texture)
  {
    this->Storage->Texture->ReleaseGraphicsResources(window);
  }
  if (this->Storage->SpriteTexture)
  {
    this->Storage->SpriteTexture->ReleaseGraphicsResources(window);
  }
  this->Storage->TextTextureCache.ReleaseGraphicsResources(window);
  this->Storage->MathTextTextureCache.ReleaseGraphicsResources(window);
}

//----------------------------------------------------------------------------
bool svtkOpenGLContextDevice2D::HasGLSL()
{
  return true;
}

//-----------------------------------------------------------------------------
svtkImageData* svtkOpenGLContextDevice2D::GetMarker(int shape, int size, bool highlight)
{
  // Generate the cache key for this marker
  svtkTypeUInt64 key = highlight ? (1U << 31) : 0U;
  key |= static_cast<svtkTypeUInt16>(shape);
  key <<= 32;
  key |= static_cast<svtkTypeUInt32>(size);

  // Try to find the marker in the cache
  std::list<svtkMarkerCacheObject>::iterator match =
    std::find(this->MarkerCache.begin(), this->MarkerCache.end(), key);

  // Was it in the cache?
  if (match != this->MarkerCache.end())
  {
    // Yep -- move it to the front and return the data.
    if (match == this->MarkerCache.begin())
    {
      return match->Value;
    }
    else
    {
      svtkMarkerCacheObject result = *match;
      this->MarkerCache.erase(match);
      this->MarkerCache.push_front(result);
      return result.Value;
    }
  }

  // Nope -- we'll need to generate it. Create the image data:
  svtkMarkerCacheObject result;
  result.Key = key;
  result.Value = this->GenerateMarker(shape, size, highlight);

  // If there was an issue generating the marker, just return nullptr.
  if (!result.Value)
  {
    svtkErrorMacro(<< "Error generating marker: shape,size: " << shape << "," << size);
    return nullptr;
  }

  // Check the current cache size.
  while (this->MarkerCache.size() > static_cast<size_t>(this->MaximumMarkerCacheSize - 1) &&
    !this->MarkerCache.empty())
  {
    this->MarkerCache.back().Value->Delete();
    this->MarkerCache.pop_back();
  }

  // Add to the cache
  this->MarkerCache.push_front(result);
  return result.Value;
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::ComputeStringBoundsInternal(
  const svtkUnicodeString& string, float bounds[4])
{
  svtkTextRenderer* tren = svtkTextRenderer::GetInstance();
  if (!tren)
  {
    svtkErrorMacro("No text renderer available. Link to svtkRenderingFreeType "
                  "to get the default implementation.");
    return;
  }

  // TODO this currently ignores svtkContextScene::ScaleTiles. Not sure how to
  // get at that from here, but this is better than ignoring scaling altogether.
  // TODO Also, FreeType supports anisotropic DPI. Might be needed if the
  // tileScale isn't homogeneous, but we'll need to update the textrenderer API
  // and see if MPL/mathtext can support it.
  int tileScale[2];
  this->RenderWindow->GetTileScale(tileScale);
  int dpi = this->RenderWindow->GetDPI() * std::max(tileScale[0], tileScale[1]);

  int bbox[4];
  if (!tren->GetBoundingBox(this->TextProp, string, bbox, dpi))
  {
    svtkErrorMacro("Error computing bounding box for string: " << string);
    return;
  }

  // Check for invalid bounding box
  if (bbox[0] >= bbox[1] || bbox[2] >= bbox[3])
  {
    bounds[0] = 0.f;
    bounds[1] = 0.f;
    bounds[2] = 0.f;
    bounds[3] = 0.f;
    return;
  }

  double* mv = this->ModelMatrix->GetMatrix()->Element[0];
  float xScale = mv[0];
  float yScale = mv[5];
  bounds[0] = static_cast<float>(bbox[0]) / xScale;
  bounds[1] = static_cast<float>(bbox[2]) / yScale;
  bounds[2] = static_cast<float>((bbox[1] - bbox[0] + 1) / xScale);
  bounds[3] = static_cast<float>((bbox[3] - bbox[2] + 1) / yScale);
}

//-----------------------------------------------------------------------------
svtkImageData* svtkOpenGLContextDevice2D::GenerateMarker(int shape, int width, bool highlight)
{
  // Set up the image data, if highlight then the mark shape is different
  svtkImageData* result = svtkImageData::New();

  result->SetExtent(0, width - 1, 0, width - 1, 0, 0);
  result->AllocateScalars(SVTK_UNSIGNED_CHAR, 4);

  unsigned char* image = static_cast<unsigned char*>(result->GetScalarPointer());
  memset(image, 0, width * width * 4);

  // Generate the marker image at the required size
  switch (shape)
  {
    case SVTK_MARKER_CROSS:
    {
      int center = (width + 1) / 2;
      for (int i = 0; i < center; ++i)
      {
        int j = width - i - 1;
        memset(image + (4 * (width * i + i)), 255, 4);
        memset(image + (4 * (width * i + j)), 255, 4);
        memset(image + (4 * (width * j + i)), 255, 4);
        memset(image + (4 * (width * j + j)), 255, 4);
        if (highlight)
        {
          memset(image + (4 * (width * (j - 1) + (i))), 255, 4);
          memset(image + (4 * (width * (i + 1) + (i))), 255, 4);
          memset(image + (4 * (width * (i) + (i + 1))), 255, 4);
          memset(image + (4 * (width * (i) + (j - 1))), 255, 4);
          memset(image + (4 * (width * (i + 1) + (j))), 255, 4);
          memset(image + (4 * (width * (j - 1) + (j))), 255, 4);
          memset(image + (4 * (width * (j) + (j - 1))), 255, 4);
          memset(image + (4 * (width * (j) + (i + 1))), 255, 4);
        }
      }
      break;
    }
    default: // Maintaining old behavior, which produces plus for unknown shape
      svtkWarningMacro(<< "Invalid marker shape: " << shape);
      SVTK_FALLTHROUGH;
    case SVTK_MARKER_PLUS:
    {
      int center = (width + 1) / 2;
      for (int i = 0; i < center; ++i)
      {
        int j = width - i - 1;
        int c = center - 1;
        memset(image + (4 * (width * c + i)), 255, 4);
        memset(image + (4 * (width * c + j)), 255, 4);
        memset(image + (4 * (width * i + c)), 255, 4);
        memset(image + (4 * (width * j + c)), 255, 4);
        if (highlight)
        {
          memset(image + (4 * (width * (c - 1) + i)), 255, 4);
          memset(image + (4 * (width * (c + 1) + i)), 255, 4);
          memset(image + (4 * (width * (c - 1) + j)), 255, 4);
          memset(image + (4 * (width * (c + 1) + j)), 255, 4);
          memset(image + (4 * (width * i + (c - 1))), 255, 4);
          memset(image + (4 * (width * i + (c + 1))), 255, 4);
          memset(image + (4 * (width * j + (c - 1))), 255, 4);
          memset(image + (4 * (width * j + (c + 1))), 255, 4);
        }
      }
      break;
    }
    case SVTK_MARKER_SQUARE:
    {
      memset(image, 255, width * width * 4);
      break;
    }
    case SVTK_MARKER_CIRCLE:
    {
      double r = width / 2.0;
      double r2 = r * r;
      for (int i = 0; i < width; ++i)
      {
        double dx2 = (i - r) * (i - r);
        for (int j = 0; j < width; ++j)
        {
          double dy2 = (j - r) * (j - r);
          if ((dx2 + dy2) < r2)
          {
            memset(image + (4 * width * i) + (4 * j), 255, 4);
          }
        }
      }
      break;
    }
    case SVTK_MARKER_DIAMOND:
    {
      int r = width / 2;
      for (int i = 0; i < width; ++i)
      {
        int dx = abs(i - r);
        for (int j = 0; j < width; ++j)
        {
          int dy = abs(j - r);
          if (r - dx >= dy)
          {
            memset(image + (4 * width * i) + (4 * j), 255, 4);
          }
        }
      }
      break;
    }
  }
  return result;
}

//-----------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << endl;
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "MaximumMarkerCacheSize: " << this->MaximumMarkerCacheSize << endl;
  os << indent << "MarkerCache: " << this->MarkerCache.size() << " entries." << endl;
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawMarkersGL2PS(
  int shape, bool highlight, float* points, int n, unsigned char* colors, int nc_comps)
{
  switch (shape)
  {
    case SVTK_MARKER_CROSS:
      this->DrawCrossMarkersGL2PS(highlight, points, n, colors, nc_comps);
      break;
    default:
      // default is here for consistency with old impl -- defaults to plus for
      // unrecognized shapes.
    case SVTK_MARKER_PLUS:
      this->DrawPlusMarkersGL2PS(highlight, points, n, colors, nc_comps);
      break;
    case SVTK_MARKER_SQUARE:
      this->DrawSquareMarkersGL2PS(highlight, points, n, colors, nc_comps);
      break;
    case SVTK_MARKER_CIRCLE:
      this->DrawCircleMarkersGL2PS(highlight, points, n, colors, nc_comps);
      break;
    case SVTK_MARKER_DIAMOND:
      this->DrawDiamondMarkersGL2PS(highlight, points, n, colors, nc_comps);
      break;
  }
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawCrossMarkersGL2PS(
  bool highlight, float* points, int n, unsigned char* colors, int nc_comps)
{
  float oldWidth = this->Pen->GetWidth();
  unsigned char oldColor[4];
  this->Pen->GetColor(oldColor);
  int oldLineType = this->Pen->GetLineType();

  float halfWidth = oldWidth * 0.5f;
  float deltaX = halfWidth;
  float deltaY = halfWidth;

  this->TransformSize(deltaX, deltaY);

  if (highlight)
  {
    this->Pen->SetWidth(1.5);
  }
  else
  {
    this->Pen->SetWidth(0.5);
  }
  this->Pen->SetLineType(svtkPen::SOLID_LINE);

  float curLine[4];
  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float* point = points + (i * 2);
    if (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          SVTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          svtkErrorMacro(<< "Invalid number of color components: " << nc_comps);
          break;
      }

      this->Pen->SetColor(color);
    }

    // The first line of the cross:
    curLine[0] = point[0] + deltaX;
    curLine[1] = point[1] + deltaY;
    curLine[2] = point[0] - deltaX;
    curLine[3] = point[1] - deltaY;
    this->DrawPoly(curLine, 2);

    // And the second:
    curLine[0] = point[0] + deltaX;
    curLine[1] = point[1] - deltaY;
    curLine[2] = point[0] - deltaX;
    curLine[3] = point[1] + deltaY;
    this->DrawPoly(curLine, 2);
  }

  this->Pen->SetWidth(oldWidth);
  this->Pen->SetColor(oldColor);
  this->Pen->SetLineType(oldLineType);
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawPlusMarkersGL2PS(
  bool highlight, float* points, int n, unsigned char* colors, int nc_comps)
{
  float oldWidth = this->Pen->GetWidth();
  unsigned char oldColor[4];
  this->Pen->GetColor(oldColor);
  int oldLineType = this->Pen->GetLineType();

  float halfWidth = oldWidth * 0.5f;
  float deltaX = halfWidth;
  float deltaY = halfWidth;

  this->TransformSize(deltaX, deltaY);

  if (highlight)
  {
    this->Pen->SetWidth(1.5);
  }
  else
  {
    this->Pen->SetWidth(0.5);
  }
  this->Pen->SetLineType(svtkPen::SOLID_LINE);

  float curLine[4];
  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float* point = points + (i * 2);
    if (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          SVTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          svtkErrorMacro(<< "Invalid number of color components: " << nc_comps);
          break;
      }

      this->Pen->SetColor(color);
    }

    // The first line of the plus:
    curLine[0] = point[0] - deltaX;
    curLine[1] = point[1];
    curLine[2] = point[0] + deltaX;
    curLine[3] = point[1];
    this->DrawPoly(curLine, 2);

    // And the second:
    curLine[0] = point[0];
    curLine[1] = point[1] - deltaY;
    curLine[2] = point[0];
    curLine[3] = point[1] + deltaY;
    this->DrawPoly(curLine, 2);
  }

  this->Pen->SetWidth(oldWidth);
  this->Pen->SetColor(oldColor);
  this->Pen->SetLineType(oldLineType);
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawSquareMarkersGL2PS(
  bool /*highlight*/, float* points, int n, unsigned char* colors, int nc_comps)
{
  unsigned char oldColor[4];
  this->Brush->GetColor(oldColor);

  this->Brush->SetColor(this->Pen->GetColor());

  float halfWidth = this->GetPen()->GetWidth() * 0.5f;
  float deltaX = halfWidth;
  float deltaY = halfWidth;

  this->TransformSize(deltaX, deltaY);

  float quad[8];
  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float* point = points + (i * 2);
    if (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          SVTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          svtkErrorMacro(<< "Invalid number of color components: " << nc_comps);
          break;
      }

      this->Brush->SetColor(color);
    }

    quad[0] = point[0] - deltaX;
    quad[1] = point[1] - deltaY;
    quad[2] = point[0] + deltaX;
    quad[3] = quad[1];
    quad[4] = quad[2];
    quad[5] = point[1] + deltaY;
    quad[6] = quad[0];
    quad[7] = quad[5];

    this->DrawQuad(quad, 4);
  }

  this->Brush->SetColor(oldColor);
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawCircleMarkersGL2PS(
  bool /*highlight*/, float* points, int n, unsigned char* colors, int nc_comps)
{
  float radius = this->GetPen()->GetWidth() * 0.475;

  unsigned char oldColor[4];
  this->Brush->GetColor(oldColor);

  this->Brush->SetColor(this->Pen->GetColor());

  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float* point = points + (i * 2);
    if (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          SVTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          svtkErrorMacro(<< "Invalid number of color components: " << nc_comps);
          break;
      }

      this->Brush->SetColor(color);
    }

    this->DrawEllipseWedge(point[0], point[1], radius, radius, 0, 0, 0, 360);
  }

  this->Brush->SetColor(oldColor);
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawDiamondMarkersGL2PS(
  bool /*highlight*/, float* points, int n, unsigned char* colors, int nc_comps)
{
  unsigned char oldColor[4];
  this->Brush->GetColor(oldColor);

  this->Brush->SetColor(this->Pen->GetColor());

  float halfWidth = this->GetPen()->GetWidth() * 0.5f;
  float deltaX = halfWidth;
  float deltaY = halfWidth;

  this->TransformSize(deltaX, deltaY);

  float quad[8];
  unsigned char color[4];
  for (int i = 0; i < n; ++i)
  {
    float* point = points + (i * 2);
    if (colors)
    {
      color[3] = 255;
      switch (nc_comps)
      {
        case 4:
        case 3:
          memcpy(color, colors + (i * nc_comps), nc_comps);
          break;
        case 2:
          color[3] = colors[i * nc_comps + 1];
          SVTK_FALLTHROUGH;
        case 1:
          memset(color, colors[i * nc_comps], 3);
          break;
        default:
          svtkErrorMacro(<< "Invalid number of color components: " << nc_comps);
          break;
      }

      this->Brush->SetColor(color);
    }

    quad[0] = point[0] - deltaX;
    quad[1] = point[1];
    quad[2] = point[0];
    quad[3] = point[1] - deltaY;
    quad[4] = point[0] + deltaX;
    quad[5] = point[1];
    quad[6] = point[0];
    quad[7] = point[1] + deltaY;

    this->DrawQuad(quad, 4);
  }

  this->Brush->SetColor(oldColor);
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawImageGL2PS(float p[2], svtkImageData* input)
{
  // Must be unsigned char -- otherwise OpenGL rendering behaves badly anyway.
  if (!svtkDataTypesCompare(input->GetScalarType(), SVTK_UNSIGNED_CHAR))
  {
    svtkErrorMacro("Invalid image format: Expected unsigned char scalars.");
    return;
  }

  // Convert to float for GL2PS
  svtkNew<svtkImageData> image;
  image->ShallowCopy(input);
  svtkDataArray* s = image->GetPointData()->GetScalars();
  size_t numVals = (s->GetNumberOfComponents() * s->GetNumberOfTuples());
  unsigned char* vals = static_cast<unsigned char*>(s->GetVoidPointer(0));
  svtkNew<svtkFloatArray> scalars;
  scalars->SetNumberOfComponents(s->GetNumberOfComponents());
  scalars->SetNumberOfTuples(s->GetNumberOfTuples());
  for (size_t i = 0; i < numVals; ++i)
  {
    scalars->SetValue(static_cast<svtkIdType>(i), vals[i] / 255.f);
  }
  image->GetPointData()->SetScalars(scalars);

  // Instance always exists when this method is called:
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();

  float tp[2] = { p[0], p[1] };
  this->TransformPoint(tp[0], tp[1]);
  double pos[3] = { static_cast<double>(tp[0]), static_cast<double>(tp[1]), 0. };
  gl2ps->DrawImage(image, pos);
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawImageGL2PS(float p[2], float scale, svtkImageData* image)
{
  if (std::fabs(scale - 1.f) < 1e-5f)
  {
    this->DrawImageGL2PS(p, image);
    return;
  }

  int dims[3];
  image->GetDimensions(dims);
  svtkRectf rect(p[0], p[1], dims[0] * scale, dims[1] * scale);
  this->DrawImageGL2PS(rect, image);
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawImageGL2PS(const svtkRectf& rect, svtkImageData* image)
{
  int dims[3];
  image->GetDimensions(dims);
  int width = static_cast<int>(std::round(rect.GetWidth()));
  int height = static_cast<int>(std::round(rect.GetHeight()));
  if (width == dims[0] && height == dims[1])
  {
    this->DrawImageGL2PS(rect.GetBottomLeft().GetData(), image);
    return;
  }

  svtkNew<svtkImageResize> resize;
  resize->SetInputData(image);
  resize->SetResizeMethod(svtkImageResize::OUTPUT_DIMENSIONS);
  resize->SetOutputDimensions(width, height, -1);
  resize->Update();
  this->DrawImageGL2PS(rect.GetBottomLeft().GetData(), resize->GetOutput());
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawCircleGL2PS(float x, float y, float rX, float rY)
{
  if (this->Brush->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  // We know this is valid if this method has been called:
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();

  svtkNew<svtkPath> path;
  this->AddEllipseToPath(path, 0.f, 0.f, rX, rY, false);
  this->TransformPath(path);

  double origin[3] = { x, y, 0.f };

  // Fill
  unsigned char fillColor[4];
  this->Brush->GetColor(fillColor);

  std::stringstream label;
  label << "svtkOpenGLContextDevice2D::DrawCircleGL2PS(" << x << ", " << y << ", " << rX << ", "
        << rY << ") fill:";

  gl2ps->DrawPath(path, origin, origin, fillColor, nullptr, 0.0, -1.f, label.str().c_str());

  // and stroke
  unsigned char strokeColor[4];
  this->Pen->GetColor(strokeColor);
  float strokeWidth = this->Pen->GetWidth();

  label.str("");
  label.clear();
  label << "svtkOpenGLContextDevice2D::DrawCircleGL2PS(" << x << ", " << y << ", " << rX << ", "
        << rY << ") stroke:";
  gl2ps->DrawPath(
    path, origin, origin, strokeColor, nullptr, 0.0, strokeWidth, label.str().c_str());
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::DrawWedgeGL2PS(
  float x, float y, float outRx, float outRy, float inRx, float inRy)
{
  if (this->Brush->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  svtkNew<svtkPath> path;
  this->AddEllipseToPath(path, 0.f, 0.f, outRx, outRy, false);
  this->AddEllipseToPath(path, 0.f, 0.f, inRx, inRy, true);

  std::stringstream label;
  label << "svtkOpenGLGL2PSContextDevice2D::DrawWedgeGL2PS(" << x << ", " << y << ", " << outRx
        << ", " << outRy << ", " << inRx << ", " << inRy << ") path:";

  unsigned char color[4];
  this->Brush->GetColor(color);

  double rasterPos[3] = { static_cast<double>(x), static_cast<double>(y), 0. };

  this->TransformPoint(x, y);
  double windowPos[3] = { static_cast<double>(x), static_cast<double>(y), 0. };

  // We know the helper exists and that we are capturing if this function has
  // been called.
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  gl2ps->DrawPath(path, rasterPos, windowPos, color, nullptr, 0.0, -1.f, label.str().c_str());
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::AddEllipseToPath(
  svtkPath* path, float x, float y, float rx, float ry, bool reverse)
{
  if (rx < 1e-5 || ry < 1e-5)
  {
    return;
  }

  // method based on http://www.tinaja.com/glib/ellipse4.pdf
  static const float MAGIC = (4.f / 3.f) * (sqrt(2.f) - 1.f);

  if (!reverse)
  {
    path->InsertNextPoint(x - rx, y, 0, svtkPath::MOVE_TO);
    path->InsertNextPoint(x - rx, ry * MAGIC, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(-rx * MAGIC, y + ry, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x, y + ry, 0, svtkPath::CUBIC_CURVE);

    path->InsertNextPoint(rx * MAGIC, y + ry, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x + rx, ry * MAGIC, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x + rx, y, 0, svtkPath::CUBIC_CURVE);

    path->InsertNextPoint(x + rx, -ry * MAGIC, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(rx * MAGIC, y - ry, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x, y - ry, 0, svtkPath::CUBIC_CURVE);

    path->InsertNextPoint(-rx * MAGIC, y - ry, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x - rx, -ry * MAGIC, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x - rx, y, 0, svtkPath::CUBIC_CURVE);
  }
  else
  {
    path->InsertNextPoint(x - rx, y, 0, svtkPath::MOVE_TO);
    path->InsertNextPoint(x - rx, -ry * MAGIC, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(-rx * MAGIC, y - ry, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x, y - ry, 0, svtkPath::CUBIC_CURVE);

    path->InsertNextPoint(rx * MAGIC, y - ry, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x + rx, -ry * MAGIC, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x + rx, y, 0, svtkPath::CUBIC_CURVE);

    path->InsertNextPoint(x + rx, ry * MAGIC, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(rx * MAGIC, y + ry, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x, y + ry, 0, svtkPath::CUBIC_CURVE);

    path->InsertNextPoint(-rx * MAGIC, y + ry, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x - rx, ry * MAGIC, 0, svtkPath::CUBIC_CURVE);
    path->InsertNextPoint(x - rx, y, 0, svtkPath::CUBIC_CURVE);
  }
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::TransformPath(svtkPath* path) const
{
  // Transform the path with the modelview matrix:
  double modelview[16];
  svtkMatrix4x4::DeepCopy(modelview, this->ModelMatrix->GetMatrix());

  // Transform the 2D path.
  float newPoint[3] = { 0, 0, 0 };
  svtkPoints* points = path->GetPoints();
  for (svtkIdType i = 0; i < path->GetNumberOfPoints(); ++i)
  {
    double* point = points->GetPoint(i);
    newPoint[0] = modelview[0] * point[0] + modelview[1] * point[1] + modelview[3];
    newPoint[1] = modelview[4] * point[0] + modelview[5] * point[1] + modelview[7];
    points->SetPoint(i, newPoint);
  }
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::TransformPoint(float& x, float& y) const
{
  double modelview[16];
  svtkMatrix4x4::DeepCopy(modelview, this->ModelMatrix->GetMatrix());

  float inX = x;
  float inY = y;
  x = modelview[0] * inX + modelview[1] * inY + modelview[3];
  y = modelview[4] * inX + modelview[5] * inY + modelview[7];
}

//------------------------------------------------------------------------------
void svtkOpenGLContextDevice2D::TransformSize(float& dx, float& dy) const
{
  double modelview[16];
  svtkMatrix4x4::DeepCopy(modelview, this->ModelMatrix->GetMatrix());

  dx /= modelview[0];
  dy /= modelview[5];
}
