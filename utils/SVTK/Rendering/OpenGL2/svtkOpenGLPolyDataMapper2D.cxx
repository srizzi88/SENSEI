/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLPolyDataMapper2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLPolyDataMapper2D.h"

#include "svtkOpenGLHelper.h"

#include "svtkActor2D.h"
#include "svtkCellArray.h"
#include "svtkHardwareSelector.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLBufferObject.h"
#include "svtkOpenGLCellToSVTKCellMap.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLIndexBufferObject.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLResourceFreeCallback.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLTexture.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkOpenGLVertexBufferObjectCache.h"
#include "svtkOpenGLVertexBufferObjectGroup.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"
#include "svtkTransform.h"
#include "svtkUnsignedCharArray.h"
#include "svtkViewport.h"

// Bring in our shader symbols.
#include "svtkPolyData2DFS.h"
#include "svtkPolyData2DVS.h"
#include "svtkPolyDataWideLineGS.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLPolyDataMapper2D);

//-----------------------------------------------------------------------------
svtkOpenGLPolyDataMapper2D::svtkOpenGLPolyDataMapper2D()
{
  this->TransformedPoints = nullptr;
  this->CellScalarTexture = nullptr;
  this->CellScalarBuffer = nullptr;
  this->LastBoundBO = nullptr;
  this->HaveCellScalars = false;
  this->PrimitiveIDOffset = 0;
  this->LastPickState = 0;
  this->VBOs = svtkOpenGLVertexBufferObjectGroup::New();

  this->ResourceCallback = new svtkOpenGLResourceFreeCallback<svtkOpenGLPolyDataMapper2D>(
    this, &svtkOpenGLPolyDataMapper2D::ReleaseGraphicsResources);
}

//-----------------------------------------------------------------------------
svtkOpenGLPolyDataMapper2D::~svtkOpenGLPolyDataMapper2D()
{
  if (this->ResourceCallback)
  {
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = nullptr;
  }
  if (this->TransformedPoints)
  {
    this->TransformedPoints->UnRegister(this);
  }
  if (this->CellScalarTexture)
  { // Resources released previously.
    this->CellScalarTexture->Delete();
    this->CellScalarTexture = nullptr;
  }
  if (this->CellScalarBuffer)
  { // Resources released previously.
    this->CellScalarBuffer->Delete();
    this->CellScalarBuffer = nullptr;
  }
  this->HaveCellScalars = false;
  this->VBOs->Delete();
  this->VBOs = nullptr;
}

//-----------------------------------------------------------------------------
void svtkOpenGLPolyDataMapper2D::ReleaseGraphicsResources(svtkWindow* win)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  this->VBOs->ReleaseGraphicsResources(win);
  this->Points.ReleaseGraphicsResources(win);
  this->Lines.ReleaseGraphicsResources(win);
  this->Tris.ReleaseGraphicsResources(win);
  this->TriStrips.ReleaseGraphicsResources(win);
  if (this->CellScalarTexture)
  {
    this->CellScalarTexture->ReleaseGraphicsResources(win);
  }
  if (this->CellScalarBuffer)
  {
    this->CellScalarBuffer->ReleaseGraphicsResources();
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
bool svtkOpenGLPolyDataMapper2D::GetNeedToRebuildShaders(
  svtkOpenGLHelper& cellBO, svtkViewport* svtkNotUsed(viewport), svtkActor2D* actor)
{
  // has something changed that would require us to recreate the shader?
  // candidates are
  // property modified (representation interpolation and lighting)
  // input modified
  // light complexity changed
  if (cellBO.Program == nullptr || cellBO.ShaderSourceTime < this->GetMTime() ||
    cellBO.ShaderSourceTime < actor->GetMTime() ||
    cellBO.ShaderSourceTime < this->GetInput()->GetMTime() ||
    cellBO.ShaderSourceTime < this->PickStateChanged)
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void svtkOpenGLPolyDataMapper2D::BuildShaders(std::string& VSSource, std::string& FSSource,
  std::string& GSSource, svtkViewport* viewport, svtkActor2D* actor)
{
  VSSource = svtkPolyData2DVS;
  FSSource = svtkPolyData2DFS;
  if (this->HaveWideLines(viewport, actor))
  {
    GSSource = svtkPolyDataWideLineGS;
  }
  else
  {
    GSSource.clear();
  }

  // Build our shader if necessary.
  if (this->HaveCellScalars)
  {
    svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Dec", "uniform samplerBuffer textureC;");
    svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Impl",
      "gl_FragData[0] = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);");
  }
  else
  {
    if (this->Colors && this->Colors->GetNumberOfComponents())
    {
      svtkShaderProgram::Substitute(VSSource, "//SVTK::Color::Dec",
        "in vec4 diffuseColor;\n"
        "out vec4 fcolorVSOutput;");
      svtkShaderProgram::Substitute(
        VSSource, "//SVTK::Color::Impl", "fcolorVSOutput = diffuseColor;");
      svtkShaderProgram::Substitute(GSSource, "//SVTK::Color::Dec",
        "in vec4 fcolorVSOutput[];\n"
        "out vec4 fcolorGSOutput;");
      svtkShaderProgram::Substitute(
        GSSource, "//SVTK::Color::Impl", "fcolorGSOutput = fcolorVSOutput[i];");
      svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Dec", "in vec4 fcolorVSOutput;");
      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::Color::Impl", "gl_FragData[0] = fcolorVSOutput;");
    }
    else
    {
      svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Dec", "uniform vec4 diffuseColor;");
      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::Color::Impl", "gl_FragData[0] = diffuseColor;");
    }
  }

  int numTCoordComps = this->VBOs->GetNumberOfComponents("tcoordMC");
  if (numTCoordComps == 1 || numTCoordComps == 2)
  {
    if (numTCoordComps == 1)
    {
      svtkShaderProgram::Substitute(
        VSSource, "//SVTK::TCoord::Dec", "in float tcoordMC; out float tcoordVCVSOutput;");
      svtkShaderProgram::Substitute(VSSource, "//SVTK::TCoord::Impl", "tcoordVCVSOutput = tcoordMC;");
      svtkShaderProgram::Substitute(GSSource, "//SVTK::TCoord::Dec",
        "in float tcoordVCVSOutput[];\n"
        "out float tcoordVCGSOutput;");
      svtkShaderProgram::Substitute(
        GSSource, "//SVTK::TCoord::Impl", "tcoordVCGSOutput = tcoordVCVSOutput[i];");
      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::TCoord::Dec", "in float tcoordVCVSOutput; uniform sampler2D texture1;");
      svtkShaderProgram::Substitute(FSSource, "//SVTK::TCoord::Impl",
        "gl_FragData[0] = gl_FragData[0]*texture2D(texture1, vec2(tcoordVCVSOutput,0));");
    }
    else
    {
      svtkShaderProgram::Substitute(
        VSSource, "//SVTK::TCoord::Dec", "in vec2 tcoordMC; out vec2 tcoordVCVSOutput;");
      svtkShaderProgram::Substitute(VSSource, "//SVTK::TCoord::Impl", "tcoordVCVSOutput = tcoordMC;");
      svtkShaderProgram::Substitute(GSSource, "//SVTK::TCoord::Dec",
        "in vec2 tcoordVCVSOutput[];\n"
        "out vec2 tcoordVCGSOutput;");
      svtkShaderProgram::Substitute(
        GSSource, "//SVTK::TCoord::Impl", "tcoordVCGSOutput = tcoordVCVSOutput[i];");
      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::TCoord::Dec", "in vec2 tcoordVCVSOutput; uniform sampler2D texture1;");
      svtkShaderProgram::Substitute(FSSource, "//SVTK::TCoord::Impl",
        "gl_FragData[0] = gl_FragData[0]*texture2D(texture1, tcoordVCVSOutput.st);");
    }
  }

  if (this->HaveCellScalars)
  {
    svtkShaderProgram::Substitute(
      GSSource, "//SVTK::PrimID::Impl", "gl_PrimitiveID = gl_PrimitiveIDIn;");
  }

  svtkRenderer* ren = svtkRenderer::SafeDownCast(viewport);
  if (ren && ren->GetSelector())
  {
    this->ReplaceShaderPicking(FSSource, ren, actor);
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLPolyDataMapper2D::UpdateShaders(
  svtkOpenGLHelper& cellBO, svtkViewport* viewport, svtkActor2D* actor)
{
  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(viewport->GetSVTKWindow());

  cellBO.VAO->Bind();
  this->LastBoundBO = &cellBO;

  if (this->GetNeedToRebuildShaders(cellBO, viewport, actor))
  {
    std::string VSSource;
    std::string FSSource;
    std::string GSSource;
    this->BuildShaders(VSSource, FSSource, GSSource, viewport, actor);
    svtkShaderProgram* newShader = renWin->GetShaderCache()->ReadyShaderProgram(
      VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
    cellBO.ShaderSourceTime.Modified();
    // if the shader changed reinitialize the VAO
    if (newShader != cellBO.Program)
    {
      cellBO.Program = newShader;
      cellBO.VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
    }
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(cellBO.Program);
  }

  if (cellBO.Program)
  {
    this->SetMapperShaderParameters(cellBO, viewport, actor);
    this->SetPropertyShaderParameters(cellBO, viewport, actor);
    this->SetCameraShaderParameters(cellBO, viewport, actor);
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLPolyDataMapper2D::SetMapperShaderParameters(
  svtkOpenGLHelper& cellBO, svtkViewport* viewport, svtkActor2D* actor)
{
  // Now to update the VAO too, if necessary.
  if (this->VBOUpdateTime > cellBO.AttributeUpdateTime ||
    cellBO.ShaderSourceTime > cellBO.AttributeUpdateTime)
  {
    cellBO.VAO->Bind();

    this->VBOs->AddAllAttributesToVAO(cellBO.Program, cellBO.VAO);

    cellBO.AttributeUpdateTime.Modified();
  }

  if (this->HaveCellScalars)
  {
    int tunit = this->CellScalarTexture->GetTextureUnit();
    cellBO.Program->SetUniformi("textureC", tunit);
  }

  if (this->VBOs->GetNumberOfComponents("tcoordMC"))
  {
    svtkInformation* info = actor->GetPropertyKeys();
    if (info && info->Has(svtkProp::GeneralTextureUnit()))
    {
      int tunit = info->Get(svtkProp::GeneralTextureUnit());
      cellBO.Program->SetUniformi("texture1", tunit);
    }
  }

  // handle wide lines
  if (this->HaveWideLines(viewport, actor))
  {
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float lineWidth[2];
    lineWidth[0] = 2.0 * actor->GetProperty()->GetLineWidth() / vp[2];
    lineWidth[1] = 2.0 * actor->GetProperty()->GetLineWidth() / vp[3];
    cellBO.Program->SetUniform2f("lineWidthNVC", lineWidth);
  }

  svtkRenderer* ren = svtkRenderer::SafeDownCast(viewport);
  svtkHardwareSelector* selector = ren->GetSelector();
  if (selector && cellBO.Program->IsUniformUsed("mapperIndex"))
  {
    cellBO.Program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLPolyDataMapper2D::SetPropertyShaderParameters(
  svtkOpenGLHelper& cellBO, svtkViewport*, svtkActor2D* actor)
{
  if (!this->Colors || !this->Colors->GetNumberOfComponents())
  {
    svtkShaderProgram* program = cellBO.Program;

    // Query the actor for some of the properties that can be applied.
    float opacity = static_cast<float>(actor->GetProperty()->GetOpacity());
    double* dColor = actor->GetProperty()->GetColor();
    float diffuseColor[4] = { static_cast<float>(dColor[0]), static_cast<float>(dColor[1]),
      static_cast<float>(dColor[2]), static_cast<float>(opacity) };

    program->SetUniform4f("diffuseColor", diffuseColor);
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLPolyDataMapper2D::ReplaceShaderPicking(
  std::string& fssource, svtkRenderer*, svtkActor2D*)
{
  svtkShaderProgram::Substitute(fssource, "//SVTK::Picking::Dec", "uniform vec3 mapperIndex;");
  svtkShaderProgram::Substitute(
    fssource, "//SVTK::Picking::Impl", "gl_FragData[0] = vec4(mapperIndex,1.0);\n");
}

//-----------------------------------------------------------------------------
void svtkOpenGLPolyDataMapper2D::SetCameraShaderParameters(
  svtkOpenGLHelper& cellBO, svtkViewport* viewport, svtkActor2D* actor)
{
  svtkShaderProgram* program = cellBO.Program;

  if (!program)
  {
    svtkErrorWithObjectMacro(this, " got null shader program, cannot set parameters.");
    return;
  }

  // Get the position of the actor
  int size[2];
  size[0] = viewport->GetSize()[0];
  size[1] = viewport->GetSize()[1];

  double* vport = viewport->GetViewport();
  int* actorPos = actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);

  // get window info
  double* tileViewPort = viewport->GetSVTKWindow()->GetTileViewport();
  double visVP[4];
  visVP[0] = (vport[0] >= tileViewPort[0]) ? vport[0] : tileViewPort[0];
  visVP[1] = (vport[1] >= tileViewPort[1]) ? vport[1] : tileViewPort[1];
  visVP[2] = (vport[2] <= tileViewPort[2]) ? vport[2] : tileViewPort[2];
  visVP[3] = (vport[3] <= tileViewPort[3]) ? vport[3] : tileViewPort[3];
  if (visVP[0] >= visVP[2])
  {
    return;
  }
  if (visVP[1] >= visVP[3])
  {
    return;
  }
  size[0] = static_cast<int>(std::round(size[0] * (visVP[2] - visVP[0]) / (vport[2] - vport[0])));
  size[1] = static_cast<int>(std::round(size[1] * (visVP[3] - visVP[1]) / (vport[3] - vport[1])));

  const int* winSize = viewport->GetSVTKWindow()->GetSize();

  int xoff = static_cast<int>(actorPos[0] - (visVP[0] - vport[0]) * winSize[0]);
  int yoff = static_cast<int>(actorPos[1] - (visVP[1] - vport[1]) * winSize[1]);

  // set ortho projection
  float left = -xoff;
  float right = -xoff + size[0];
  float bottom = -yoff;
  float top = -yoff + size[1];

  // it's an error to call glOrtho with
  // either left==right or top==bottom
  if (left == right)
  {
    right = left + 1.0;
  }
  if (bottom == top)
  {
    top = bottom + 1.0;
  }

  float nearV = 0;
  float farV = SVTK_FLOAT_MAX;
  if (actor->GetProperty()->GetDisplayLocation() != SVTK_FOREGROUND_LOCATION)
  {
    nearV = -SVTK_FLOAT_MAX;
    farV = 0;
  }

  // compute the combined ModelView matrix and send it down to save time in the shader
  svtkMatrix4x4* tmpMat = svtkMatrix4x4::New();
  tmpMat->SetElement(0, 0, 2.0 / (right - left));
  tmpMat->SetElement(1, 1, 2.0 / (top - bottom));
  // XXX(cppcheck): possible division by zero
  tmpMat->SetElement(2, 2, -2.0 / (farV - nearV));
  tmpMat->SetElement(3, 3, 1.0);
  tmpMat->SetElement(0, 3, -1.0 * (right + left) / (right - left));
  tmpMat->SetElement(1, 3, -1.0 * (top + bottom) / (top - bottom));
  // XXX(cppcheck): possible division by zero
  tmpMat->SetElement(2, 3, -1.0 * (farV + nearV) / (farV - nearV));
  tmpMat->Transpose();
  /*
    if (this->VBO->GetCoordShiftAndScaleEnabled())
    {
      this->VBOTransformInverse->GetTranspose(this->VBOShiftScale);
      // Pre-multiply the inverse of the VBO's transform:
      svtkMatrix4x4::Multiply4x4(
        this->VBOShiftScale, tmpMat, tmpMat);
    }
  */
  program->SetUniformMatrix("WCVCMatrix", tmpMat);

  tmpMat->Delete();
}

//-------------------------------------------------------------------------
void svtkOpenGLPolyDataMapper2D::UpdateVBO(svtkActor2D* act, svtkViewport* viewport)
{
  svtkPolyData* poly = this->GetInput();
  if (poly == nullptr)
  {
    return;
  }

  this->MapScalars(act->GetProperty()->GetOpacity());
  this->HaveCellScalars = false;
  if (this->ScalarVisibility)
  {
    // We must figure out how the scalars should be mapped to the polydata.
    if ((this->ScalarMode == SVTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == SVTK_SCALAR_MODE_USE_FIELD_DATA ||
          !poly->GetPointData()->GetScalars()) &&
      this->ScalarMode != SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA && this->Colors)
    {
      this->HaveCellScalars = true;
    }
  }

  // if we have cell scalars then we have to
  // build the texture
  svtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();
  svtkDataArray* c = this->Colors;
  if (this->HaveCellScalars)
  {
    this->CellCellMap->Update(prims, SVTK_SURFACE, poly->GetPoints());

    if (!this->CellScalarTexture)
    {
      this->CellScalarTexture = svtkTextureObject::New();
      this->CellScalarBuffer = svtkOpenGLBufferObject::New();
      this->CellScalarBuffer->SetType(svtkOpenGLBufferObject::TextureBuffer);
    }
    this->CellScalarTexture->SetContext(
      static_cast<svtkOpenGLRenderWindow*>(viewport->GetSVTKWindow()));
    // create the cell scalar array adjusted for ogl Cells
    std::vector<unsigned char> newColors;
    unsigned char* colorPtr = this->Colors->GetPointer(0);
    int numComp = this->Colors->GetNumberOfComponents();
    assert(numComp == 4);
    for (size_t i = 0; i < this->CellCellMap->GetSize(); i++)
    {
      for (int j = 0; j < numComp; j++)
      {
        newColors.push_back(colorPtr[this->CellCellMap->GetValue(i) * numComp + j]);
      }
    }
    this->CellScalarBuffer->Upload(newColors, svtkOpenGLBufferObject::TextureBuffer);
    this->CellScalarTexture->CreateTextureBuffer(
      static_cast<unsigned int>(this->CellCellMap->GetSize()), numComp, SVTK_UNSIGNED_CHAR,
      this->CellScalarBuffer);
    c = nullptr;
  }

  // do we have texture maps?
  bool haveTextures = false;
  svtkInformation* info = act->GetPropertyKeys();
  if (info && info->Has(svtkProp::GeneralTextureUnit()))
  {
    haveTextures = true;
  }

  // Transform the points, if necessary
  svtkPoints* p = poly->GetPoints();
  if (this->TransformCoordinate)
  {
    svtkIdType numPts = p->GetNumberOfPoints();
    if (!this->TransformedPoints)
    {
      this->TransformedPoints = svtkPoints::New();
    }
    this->TransformedPoints->SetNumberOfPoints(numPts);
    for (svtkIdType j = 0; j < numPts; j++)
    {
      this->TransformCoordinate->SetValue(p->GetPoint(j));
      if (this->TransformCoordinateUseDouble)
      {
        double* dtmp = this->TransformCoordinate->GetComputedDoubleViewportValue(viewport);
        this->TransformedPoints->SetPoint(j, dtmp[0], dtmp[1], 0.0);
      }
      else
      {
        int* itmp = this->TransformCoordinate->GetComputedViewportValue(viewport);
        this->TransformedPoints->SetPoint(j, itmp[0], itmp[1], 0.0);
      }
    }
    p = this->TransformedPoints;
  }

  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(viewport->GetSVTKWindow());
  svtkOpenGLVertexBufferObjectCache* cache = renWin->GetVBOCache();

  this->VBOs->CacheDataArray("vertexWC", p->GetData(), cache, SVTK_FLOAT);
  this->VBOs->CacheDataArray(
    "tcoordMC", (haveTextures ? poly->GetPointData()->GetTCoords() : nullptr), cache, SVTK_FLOAT);
  this->VBOs->CacheDataArray("diffuseColor", c, cache, SVTK_UNSIGNED_CHAR);

  this->VBOs->BuildAllVBOs(cache);
  this->VBOUpdateTime
    .Modified(); // need to call all the time or GetNeedToRebuild will always return true;

  this->Points.IBO->IndexCount = this->Points.IBO->CreatePointIndexBuffer(prims[0]);
  this->Lines.IBO->IndexCount = this->Lines.IBO->CreateLineIndexBuffer(prims[1]);
  this->Tris.IBO->IndexCount =
    this->Tris.IBO->CreateTriangleIndexBuffer(prims[2], poly->GetPoints());
  this->TriStrips.IBO->IndexCount = this->TriStrips.IBO->CreateStripIndexBuffer(prims[3], false);
}

bool svtkOpenGLPolyDataMapper2D::HaveWideLines(svtkViewport* ren, svtkActor2D* actor)
{
  if (this->LastBoundBO == &this->Lines && actor->GetProperty()->GetLineWidth() > 1.0)
  {
    // we have wide lines, but the OpenGL implementation may
    // actually support them, check the range to see if we
    // really need have to implement our own wide lines
    svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(ren->GetSVTKWindow());
    return !(
      renWin && renWin->GetMaximumHardwareLineWidth() >= actor->GetProperty()->GetLineWidth());
  }
  return false;
}

void svtkOpenGLPolyDataMapper2D::RenderOverlay(svtkViewport* viewport, svtkActor2D* actor)
{
  svtkOpenGLClearErrorMacro();
  svtkPolyData* input = this->GetInput();

  svtkDebugMacro(<< "svtkOpenGLPolyDataMapper2D::Render");

  if (input == nullptr)
  {
    svtkErrorMacro(<< "No input!");
    return;
  }

  this->GetInputAlgorithm()->Update();
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts == 0)
  {
    svtkDebugMacro(<< "No points!");
    return;
  }

  if (this->LookupTable == nullptr)
  {
    this->CreateDefaultLookupTable();
  }

  svtkRenderWindow* renWin = svtkRenderWindow::SafeDownCast(viewport->GetSVTKWindow());

  this->ResourceCallback->RegisterGraphicsResources(static_cast<svtkOpenGLRenderWindow*>(renWin));

  svtkRenderer* ren = svtkRenderer::SafeDownCast(viewport);
  svtkHardwareSelector* selector = ren->GetSelector();
  if (selector)
  {
    selector->BeginRenderProp();
  }

  int picking = (selector ? 1 : 0);
  if (picking != this->LastPickState)
  {
    this->LastPickState = picking;
    this->PickStateChanged.Modified();
  }

  // Assume we want to do Zbuffering for now.
  // we may turn this off later
  static_cast<svtkOpenGLRenderWindow*>(renWin)->GetState()->svtkglDepthMask(GL_TRUE);

  // Update the VBO if needed.
  if (this->VBOUpdateTime < this->GetMTime() || this->VBOUpdateTime < actor->GetMTime() ||
    this->VBOUpdateTime < input->GetMTime() ||
    (this->TransformCoordinate &&
      (this->VBOUpdateTime < viewport->GetMTime() ||
        this->VBOUpdateTime < viewport->GetSVTKWindow()->GetMTime())))
  {
    this->UpdateVBO(actor, viewport);
    this->VBOUpdateTime.Modified();
  }

  // this->VBOs->Bind();
  this->LastBoundBO = nullptr;

  if (this->HaveCellScalars)
  {
    this->CellScalarTexture->Activate();
  }

  // Figure out and build the appropriate shader for the mapped geometry.
  this->PrimitiveIDOffset = 0;

  int numVerts = this->VBOs->GetNumberOfTuples("vertexWC");

  if (this->Points.IBO->IndexCount)
  {
    this->UpdateShaders(this->Points, viewport, actor);
    if (this->Points.Program)
    {
      this->Points.Program->SetUniformi("PrimitiveIDOffset", this->PrimitiveIDOffset);
    }

    // Set the PointSize
#ifndef GL_ES_VERSION_3_0
    glPointSize(actor->GetProperty()->GetPointSize()); // not on ES2
#endif
    this->Points.IBO->Bind();
    glDrawRangeElements(GL_POINTS, 0, static_cast<GLuint>(numVerts - 1),
      static_cast<GLsizei>(this->Points.IBO->IndexCount), GL_UNSIGNED_INT, nullptr);
    this->Points.IBO->Release();
    this->PrimitiveIDOffset += (int)this->Points.IBO->IndexCount;
  }

  if (this->Lines.IBO->IndexCount)
  {
    // Set the LineWidth
    this->UpdateShaders(this->Lines, viewport, actor);
    if (this->Lines.Program)
    {
      this->Lines.Program->SetUniformi("PrimitiveIDOffset", this->PrimitiveIDOffset);
      if (!this->HaveWideLines(viewport, actor))
      {
        glLineWidth(actor->GetProperty()->GetLineWidth());
      }
      this->Lines.IBO->Bind();
      glDrawRangeElements(GL_LINES, 0, static_cast<GLuint>(numVerts - 1),
        static_cast<GLsizei>(this->Lines.IBO->IndexCount), GL_UNSIGNED_INT, nullptr);
      this->Lines.IBO->Release();
    }
    this->PrimitiveIDOffset += (int)this->Lines.IBO->IndexCount / 2;
  }

  // now handle lit primitives
  if (this->Tris.IBO->IndexCount)
  {
    this->UpdateShaders(this->Tris, viewport, actor);
    if (this->Tris.Program)
    {
      this->Tris.Program->SetUniformi("PrimitiveIDOffset", this->PrimitiveIDOffset);
      this->Tris.IBO->Bind();
      glDrawRangeElements(GL_TRIANGLES, 0, static_cast<GLuint>(numVerts - 1),
        static_cast<GLsizei>(this->Tris.IBO->IndexCount), GL_UNSIGNED_INT, nullptr);
      this->Tris.IBO->Release();
      this->PrimitiveIDOffset += (int)this->Tris.IBO->IndexCount / 3;
    }
  }

  if (this->TriStrips.IBO->IndexCount)
  {
    this->UpdateShaders(this->TriStrips, viewport, actor);
    if (this->TriStrips.Program)
    {
      this->TriStrips.Program->SetUniformi("PrimitiveIDOffset", this->PrimitiveIDOffset);
      this->TriStrips.IBO->Bind();
      glDrawRangeElements(GL_TRIANGLES, 0, static_cast<GLuint>(numVerts - 1),
        static_cast<GLsizei>(this->TriStrips.IBO->IndexCount), GL_UNSIGNED_INT, nullptr);
      this->TriStrips.IBO->Release();
    }
  }

  if (this->HaveCellScalars)
  {
    this->CellScalarTexture->Deactivate();
  }

  if (this->LastBoundBO)
  {
    this->LastBoundBO->VAO->Release();
  }

  if (selector)
  {
    selector->EndRenderProp();
  }

  // this->VBOs->Release();

  svtkOpenGLCheckErrorMacro("failed after RenderOverlay");
}

//----------------------------------------------------------------------------
void svtkOpenGLPolyDataMapper2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
