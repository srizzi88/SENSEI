/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLGlyph3DHelper.h"

#include "svtkOpenGLHelper.h"

#include "svtkBitArray.h"
#include "svtkCamera.h"
#include "svtkDataObject.h"
#include "svtkHardwareSelector.h"
#include "svtkMath.h"
#include "svtkMatrix3x3.h"
#include "svtkMatrix4x4.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLActor.h"
#include "svtkOpenGLBufferObject.h"
#include "svtkOpenGLCamera.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLIndexBufferObject.h"
#include "svtkOpenGLInstanceCulling.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLResourceFreeCallback.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkOpenGLVertexBufferObject.h"
#include "svtkOpenGLVertexBufferObjectGroup.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkShader.h"
#include "svtkShaderProgram.h"
#include "svtkTransform.h"
#include "svtkTransformFeedback.h"

#include "svtkGlyph3DVS.h"

#include <algorithm>
#include <numeric>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLGlyph3DHelper);

//-----------------------------------------------------------------------------
svtkOpenGLGlyph3DHelper::svtkOpenGLGlyph3DHelper()
{
  this->UsingInstancing = false;
  this->PopulateSelectionSettings = 0;
}

// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
void svtkOpenGLGlyph3DHelper::ReleaseGraphicsResources(svtkWindow* window)
{
  this->NormalMatrixBuffer->ReleaseGraphicsResources();
  this->MatrixBuffer->ReleaseGraphicsResources();
  this->ColorBuffer->ReleaseGraphicsResources();
  this->Superclass::ReleaseGraphicsResources(window);
}

//-----------------------------------------------------------------------------
void svtkOpenGLGlyph3DHelper::GetShaderTemplate(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  this->Superclass::GetShaderTemplate(shaders, ren, actor);

  shaders[svtkShader::Vertex]->SetSource(svtkGlyph3DVS);
}

void svtkOpenGLGlyph3DHelper::ReplaceShaderPositionVC(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  std::string VSSource = shaders[svtkShader::Vertex]->GetSource();

  if (this->LastLightComplexity[this->LastBoundBO] > 0)
  {
    // we use vertex instead of vertexMC
    svtkShaderProgram::Substitute(VSSource, "//SVTK::PositionVC::Impl",
      "vertexVCVSOutput = MCVCMatrix * vertex;\n"
      "  gl_Position = MCDCMatrix * vertex;\n");
  }
  else
  {
    svtkShaderProgram::Substitute(
      VSSource, "//SVTK::PositionVC::Impl", "gl_Position = MCDCMatrix * vertex;\n");
  }

  shaders[svtkShader::Vertex]->SetSource(VSSource);

  this->Superclass::ReplaceShaderPositionVC(shaders, ren, actor);
}

void svtkOpenGLGlyph3DHelper::ReplaceShaderColor(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  std::string VSSource = shaders[svtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[svtkShader::Fragment]->GetSource();
  std::string GSSource = shaders[svtkShader::Geometry]->GetSource();

  // deal with color
  if (this->UsingInstancing)
  {
    svtkShaderProgram::Substitute(VSSource, "//SVTK::Color::Dec",
      "in vec4 glyphColor;\n"
      "out vec4 vertexColorVSOutput;");
    svtkShaderProgram::Substitute(GSSource, "//SVTK::Color::Dec",
      "in vec4 vertexColorVSOutput[];\n"
      "out vec4 vertexColorGSOutput;");
    svtkShaderProgram::Substitute(
      GSSource, "//SVTK::Color::Impl", "vertexColorGSOutput = vertexColorVSOutput[i];");
    svtkShaderProgram::Substitute(
      VSSource, "//SVTK::Color::Impl", "vertexColorVSOutput =  glyphColor;");
    svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Dec",
      "in vec4 vertexColorVSOutput;\n"
      "//SVTK::Color::Dec",
      false);
  }
  else
  {
    svtkShaderProgram::Substitute(VSSource, "//SVTK::Color::Dec", "");
    svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Dec",
      "uniform vec4 glyphColor;\n"
      "//SVTK::Color::Dec",
      false);
    svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Impl",
      "vec4 vertexColorVSOutput = glyphColor;\n"
      "//SVTK::Color::Impl",
      false);
  }

  // now handle scalar coloring
  if (!this->DrawingEdgesOrVertices)
  {
    svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Impl",
      "//SVTK::Color::Impl\n"
      "  diffuseColor = diffuseIntensity * vertexColorVSOutput.rgb;\n"
      "  ambientColor = ambientIntensity * vertexColorVSOutput.rgb;\n"
      "  opacity = opacity * vertexColorVSOutput.a;");
  }

  if (this->UsingInstancing)
  {
    svtkShaderProgram::Substitute(VSSource, "//SVTK::Glyph::Dec", "in mat4 GCMCMatrix;");
  }
  else
  {
    svtkShaderProgram::Substitute(VSSource, "//SVTK::Glyph::Dec", "uniform mat4 GCMCMatrix;");
  }
  svtkShaderProgram::Substitute(
    VSSource, "//SVTK::Glyph::Impl", "vec4 vertex = GCMCMatrix * vertexMC;\n");

  shaders[svtkShader::Vertex]->SetSource(VSSource);
  shaders[svtkShader::Fragment]->SetSource(FSSource);
  shaders[svtkShader::Geometry]->SetSource(GSSource);

  this->Superclass::ReplaceShaderColor(shaders, ren, actor);
}

void svtkOpenGLGlyph3DHelper::ReplaceShaderNormal(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  std::string VSSource = shaders[svtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[svtkShader::Fragment]->GetSource();

  // new code for normal matrix if we have normals
  if (this->VBOs->GetNumberOfComponents("normalMC") == 3)
  {
    if (this->UsingInstancing)
    {
      svtkShaderProgram::Substitute(VSSource, "//SVTK::Normal::Dec",
        "uniform mat3 normalMatrix;\n"
        "in vec3 normalMC;\n"
        "in mat3 glyphNormalMatrix;\n"
        "out vec3 normalVCVSOutput;");
    }
    else
    {
      svtkShaderProgram::Substitute(VSSource, "//SVTK::Normal::Dec",
        "uniform mat3 normalMatrix;\n"
        "in vec3 normalMC;\n"
        "uniform mat3 glyphNormalMatrix;\n"
        "out vec3 normalVCVSOutput;");
    }
    svtkShaderProgram::Substitute(VSSource, "//SVTK::Normal::Impl",
      "normalVCVSOutput = normalMatrix * glyphNormalMatrix * normalMC;");
  }

  shaders[svtkShader::Vertex]->SetSource(VSSource);
  shaders[svtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderNormal(shaders, ren, actor);
}

void svtkOpenGLGlyph3DHelper::ReplaceShaderClip(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  std::string VSSource = shaders[svtkShader::Vertex]->GetSource();

  // override one part of the clipping code
  if (this->GetNumberOfClippingPlanes())
  {
    // add all the clipping planes
    int numClipPlanes = this->GetNumberOfClippingPlanes();
    if (numClipPlanes > 6)
    {
      svtkErrorMacro("OpenGL has a limit of 6 clipping planes");
    }

    svtkShaderProgram::Substitute(VSSource, "//SVTK::Clip::Impl",
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    clipDistancesVSOutput[planeNum] = dot(clipPlanes[planeNum], vertex);\n"
      "    }\n");
  }

  shaders[svtkShader::Vertex]->SetSource(VSSource);

  this->Superclass::ReplaceShaderClip(shaders, ren, actor);
}

void svtkOpenGLGlyph3DHelper::ReplaceShaderPicking(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer*, svtkActor*)
{
  std::string FSSource = shaders[svtkShader::Fragment]->GetSource();

  if (this->LastSelectionState >= svtkHardwareSelector::MIN_KNOWN_PASS)
  {
    svtkShaderProgram::Substitute(FSSource, "//SVTK::Picking::Dec", "uniform vec3 mapperIndex;");
    svtkShaderProgram::Substitute(
      FSSource, "//SVTK::Picking::Impl", "  gl_FragData[0] = vec4(mapperIndex,1.0);\n");
  }
  shaders[svtkShader::Fragment]->SetSource(FSSource);
}

void svtkOpenGLGlyph3DHelper::GlyphRender(svtkRenderer* ren, svtkActor* actor, svtkIdType numPts,
  std::vector<unsigned char>& colors, std::vector<float>& matrices,
  std::vector<float>& normalMatrices, std::vector<svtkIdType>& pickIds, svtkMTimeType pointMTime,
  bool culling)
{
  this->ResourceCallback->RegisterGraphicsResources(
    static_cast<svtkOpenGLRenderWindow*>(ren->GetRenderWindow()));

  this->UsingInstancing = false;

  svtkHardwareSelector* selector = ren->GetSelector();

  if (!selector && GLEW_ARB_instanced_arrays)
  {
    // if there is no triangle, culling is useless.
    // GLEW_ARB_gpu_shader5 is needed by the culling shader.
#ifndef GL_ES_VERSION_3_0
    if (this->CurrentInput->GetNumberOfPolys() <= 0 || !GLEW_ARB_gpu_shader5 ||
      !GLEW_ARB_transform_feedback3)
    {
      culling = false;
    }
#else
    // disable culling on OpenGL ES
    culling = false;
#endif

    this->GlyphRenderInstances(
      ren, actor, numPts, colors, matrices, normalMatrices, pointMTime, culling);
    return;
  }

  bool selecting_points =
    selector && (selector->GetFieldAssociation() == svtkDataObject::FIELD_ASSOCIATION_POINTS);

  int representation = actor->GetProperty()->GetRepresentation();

  this->RenderPieceStart(ren, actor);

  if (selecting_points)
  {
#ifndef GL_ES_VERSION_3_0
    glPointSize(6.0);
#endif
    representation = GL_POINTS;
  }

  bool draw_surface_with_edges =
    (actor->GetProperty()->GetEdgeVisibility() && representation == SVTK_SURFACE) && !selector;
  int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
  for (int i = PrimitiveStart;
       i < (draw_surface_with_edges ? PrimitiveEnd : PrimitiveTriStrips + 1); i++)
  {
    this->DrawingEdgesOrVertices = (i > PrimitiveTriStrips ? true : false);
    if (this->Primitives[i].IBO->IndexCount)
    {
      this->UpdateShaders(this->Primitives[i], ren, actor);
      GLenum mode = this->GetOpenGLMode(representation, i);
      this->Primitives[i].IBO->Bind();
      for (svtkIdType inPtId = 0; inPtId < numPts; inPtId++)
      {
        // handle the middle
        svtkShaderProgram* program = this->Primitives[i].Program;

        if (!program)
        {
          return;
        }

        // Apply the extra transform
        program->SetUniformMatrix4x4("GCMCMatrix", &(matrices[inPtId * 16]));

        // for lit shaders set normal matrix
        if (this->LastLightComplexity[this->LastBoundBO] > 0 &&
          this->VBOs->GetNumberOfComponents("normalMC") == 3 && !this->UsingInstancing)
        {
          program->SetUniformMatrix3x3("glyphNormalMatrix", &(normalMatrices[inPtId * 9]));
        }

        program->SetUniform4uc("glyphColor", &(colors[inPtId * 4]));

        if (selector)
        {
          if (selector->GetCurrentPass() == svtkHardwareSelector::POINT_ID_LOW24 ||
            selector->GetCurrentPass() == svtkHardwareSelector::POINT_ID_HIGH24 ||
            selector->GetCurrentPass() == svtkHardwareSelector::CELL_ID_LOW24 ||
            selector->GetCurrentPass() == svtkHardwareSelector::CELL_ID_HIGH24)
          {
            selector->SetPropColorValue(pickIds[inPtId]);
          }
          program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
        }

        glDrawRangeElements(mode, 0, static_cast<GLuint>(numVerts - 1),
          static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount), GL_UNSIGNED_INT, nullptr);
      }
      this->Primitives[i].IBO->Release();
    }
  }
  this->RenderPieceFinish(ren, actor);
}

//-----------------------------------------------------------------------------
void svtkOpenGLGlyph3DHelper::SetMapperShaderParameters(
  svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* actor)
{
  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);

  svtkHardwareSelector* selector = ren->GetSelector();
  if (selector)
  {
    cellBO.Program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
  }
}

void svtkOpenGLGlyph3DHelper::GlyphRenderInstances(svtkRenderer* ren, svtkActor* actor,
  svtkIdType numPts, std::vector<unsigned char>& colors, std::vector<float>& matrices,
  std::vector<float>& normalMatrices, svtkMTimeType pointMTime, bool culling)
{
  this->UsingInstancing = true;
  this->RenderPieceStart(ren, actor);
  int representation = actor->GetProperty()->GetRepresentation();

  bool withNormals = (this->VBOs->GetNumberOfComponents("normalMC") == 3);

  // update the VBOs if needed
  if (pointMTime > this->InstanceBuffersBuildTime.GetMTime())
  {
    this->MatrixBuffer->Upload(matrices, svtkOpenGLBufferObject::ArrayBuffer);

    if (withNormals)
    {
      this->NormalMatrixBuffer->Upload(normalMatrices, svtkOpenGLBufferObject::ArrayBuffer);
    }

    this->ColorBuffer->Upload(colors, svtkOpenGLBufferObject::ArrayBuffer);
    this->InstanceBuffersBuildTime.Modified();
  }

  bool draw_surface_with_edges =
    (actor->GetProperty()->GetEdgeVisibility() && representation == SVTK_SURFACE);
  for (int i = PrimitiveStart;
       i < (draw_surface_with_edges ? PrimitiveEnd : PrimitiveTriStrips + 1); i++)
  {
    this->DrawingEdgesOrVertices = (i > PrimitiveTriStrips ? true : false);
    if (this->Primitives[i].IBO->IndexCount)
    {
      GLenum mode = this->GetOpenGLMode(representation, i);

      // culling
      if (culling)
      {
        this->BuildCullingShaders(ren, actor, numPts, withNormals);
        if (!this->InstanceCulling->GetHelper().Program)
        {
          return;
        }

        this->InstanceCulling->RunCullingShaders(
          numPts, this->MatrixBuffer, this->ColorBuffer, this->NormalMatrixBuffer);

        // draw each LOD

        this->UpdateShaders(this->Primitives[i], ren, actor);
        if (!this->Primitives[i].Program)
        {
          return;
        }

        size_t stride = (withNormals ? 29 : 20) * sizeof(float);

        this->Primitives[i].VAO->Bind();

        for (svtkIdType j = 0; j < this->InstanceCulling->GetNumberOfLOD(); j++)
        {
          if (this->InstanceCulling->GetLOD(j).NumberOfInstances == 0)
            continue;

          // add VBO of current instance in VAO
          if (!this->Primitives[i].VAO->AddAttributeArray(this->Primitives[i].Program,
                this->InstanceCulling->GetLOD(j).PositionVBO, "vertexMC", 0, 4 * sizeof(float),
                SVTK_FLOAT, 4, false))
          {
            svtkErrorMacro("Error setting 'vertexMC' in shader VAO.");
          }

          if (withNormals)
          {
            if (!this->Primitives[i].VAO->AddAttributeArray(this->Primitives[i].Program,
                  this->InstanceCulling->GetLOD(j).NormalVBO, "normalMC", 0, 3 * sizeof(float),
                  SVTK_FLOAT, 3, false))
            {
              svtkErrorMacro("Error setting 'normalMC' in shader VAO.");
            }
          }

          // add instances attributes based on transform feedback buffers
          if (!this->Primitives[i].VAO->AddAttributeArrayWithDivisor(this->Primitives[i].Program,
                this->InstanceCulling->GetLODBuffer(j), "glyphColor", 16 * sizeof(float), stride,
                SVTK_FLOAT, 4, false, 1, false))
          {
            svtkErrorMacro("Error setting 'diffuse color' in shader VAO.");
          }

          if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                this->InstanceCulling->GetLODBuffer(j), "GCMCMatrix", 0, stride, SVTK_FLOAT, 4,
                false, 1, 4 * sizeof(float)))
          {
            svtkErrorMacro("Error setting 'GCMCMatrix' in shader VAO.");
          }

          if (withNormals)
          {
            if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                  this->InstanceCulling->GetLODBuffer(j), "glyphNormalMatrix", 20 * sizeof(float),
                  stride, SVTK_FLOAT, 3, false, 1, 3 * sizeof(float)))
            {
              svtkErrorMacro("Error setting 'glyphNormalMatrix' in shader VAO.");
            }
          }

          if (this->InstanceCulling->GetLOD(j).IBO->IndexCount > 0)
          {
            this->InstanceCulling->GetLOD(j).IBO->Bind();

#ifdef GL_ES_VERSION_3_0
            glDrawElementsInstanced(mode,
              static_cast<GLsizei>(this->InstanceCulling->GetLOD(j).IBO->IndexCount),
              GL_UNSIGNED_INT, nullptr, this->InstanceCulling->GetLOD(j).NumberOfInstances);
#else
            glDrawElementsInstancedARB(mode,
              static_cast<GLsizei>(this->InstanceCulling->GetLOD(j).IBO->IndexCount),
              GL_UNSIGNED_INT, nullptr, this->InstanceCulling->GetLOD(j).NumberOfInstances);
#endif
            this->InstanceCulling->GetLOD(j).IBO->Release();
          }
          else
          {
#ifdef GL_ES_VERSION_3_0
            glDrawArraysInstanced(
              GL_POINTS, 0, 1, this->InstanceCulling->GetLOD(j).NumberOfInstances);
#else
            glDrawArraysInstancedARB(
              GL_POINTS, 0, 1, this->InstanceCulling->GetLOD(j).NumberOfInstances);
#endif
          }
        }
      }
      else
      {
        this->UpdateShaders(this->Primitives[i], ren, actor);
        if (!this->Primitives[i].Program)
        {
          return;
        }

        // do the superclass and then reset a couple values
        if ((this->InstanceBuffersBuildTime > this->InstanceBuffersLoadTime ||
              this->Primitives[i].ShaderSourceTime > this->InstanceBuffersLoadTime))
        {
          this->Primitives[i].VAO->Bind();

          this->MatrixBuffer->Bind();
          if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                this->MatrixBuffer, "GCMCMatrix", 0, 16 * sizeof(float), SVTK_FLOAT, 4, false, 1,
                4 * sizeof(float)))
          {
            svtkErrorMacro("Error setting 'GCMCMatrix' in shader VAO.");
          }
          this->MatrixBuffer->Release();

          if (withNormals && this->Primitives[i].Program->IsAttributeUsed("glyphNormalMatrix"))
          {
            this->NormalMatrixBuffer->Bind();
            if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                  this->NormalMatrixBuffer, "glyphNormalMatrix", 0, 9 * sizeof(float), SVTK_FLOAT, 3,
                  false, 1, 3 * sizeof(float)))
            {
              svtkErrorMacro("Error setting 'glyphNormalMatrix' in shader VAO.");
            }
            this->NormalMatrixBuffer->Release();
          }

          if (this->Primitives[i].Program->IsAttributeUsed("glyphColor"))
          {
            this->ColorBuffer->Bind();
            if (!this->Primitives[i].VAO->AddAttributeArrayWithDivisor(this->Primitives[i].Program,
                  this->ColorBuffer, "glyphColor", 0, 4 * sizeof(unsigned char), SVTK_UNSIGNED_CHAR,
                  4, true, 1, false))
            {
              svtkErrorMacro("Error setting 'diffuse color' in shader VAO.");
            }
            this->ColorBuffer->Release();
          }
          this->InstanceBuffersLoadTime.Modified();
        }

        this->Primitives[i].IBO->Bind();

#ifdef GL_ES_VERSION_3_0
        glDrawElementsInstanced(mode, static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount),
          GL_UNSIGNED_INT, nullptr, numPts);
#else
        glDrawElementsInstancedARB(mode, static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount),
          GL_UNSIGNED_INT, nullptr, numPts);
#endif

        this->Primitives[i].IBO->Release();
      }
    }
  }

  svtkOpenGLCheckErrorMacro("failed after Render");
  this->RenderPieceFinish(ren, actor);
}

//-----------------------------------------------------------------------------
void svtkOpenGLGlyph3DHelper::BuildCullingShaders(
  svtkRenderer* ren, svtkActor* actor, svtkIdType numPts, bool withNormals)
{
  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  if (!this->InstanceCulling->GetHelper().Program)
  {
    this->InstanceCulling->InitLOD(this->CurrentInput);

    for (auto& lod : this->LODs)
    {
      this->InstanceCulling->AddLOD(lod.first, lod.second);
    }
  }

  this->InstanceCulling->BuildCullingShaders(renWin->GetShaderCache(), numPts, withNormals);

  if (this->InstanceCulling->GetHelper().Program)
  {
    this->SetCameraShaderParameters(this->InstanceCulling->GetHelper(), ren, actor);

    double* bounds = this->CurrentInput->GetBounds();
    float BBoxSize[4] = { static_cast<float>(bounds[1] - bounds[0]),
      static_cast<float>(bounds[3] - bounds[2]), static_cast<float>(bounds[5] - bounds[4]), 0.f };

    this->InstanceCulling->GetHelper().Program->SetUniform4f("BBoxSize", BBoxSize);
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLGlyph3DHelper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void svtkOpenGLGlyph3DHelper::SetLODs(std::vector<std::pair<float, float> >& lods)
{
  this->LODs = lods;
}

//-----------------------------------------------------------------------------
void svtkOpenGLGlyph3DHelper::SetLODColoring(bool val)
{
  this->InstanceCulling->SetColorLOD(val);
}
