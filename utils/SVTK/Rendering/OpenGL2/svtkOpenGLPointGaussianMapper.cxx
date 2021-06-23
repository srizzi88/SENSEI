/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLPointGaussianMapper.h"

#include "svtkOpenGLHelper.h"

#include "svtkBoundingBox.h"
#include "svtkCellArray.h"
#include "svtkCommand.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkFloatArray.h"
#include "svtkGarbageCollector.h"
#include "svtkHardwareSelector.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLActor.h"
#include "svtkOpenGLCamera.h"
#include "svtkOpenGLIndexBufferObject.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkOpenGLVertexBufferObject.h"
#include "svtkOpenGLVertexBufferObjectGroup.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkShaderProgram.h"
#include "svtkUnsignedCharArray.h"

#include "svtkPointGaussianGS.h"
#include "svtkPointGaussianVS.h"
#include "svtkPolyDataFS.h"

#include "svtk_glew.h"

class svtkOpenGLPointGaussianMapperHelper : public svtkOpenGLPolyDataMapper
{
public:
  static svtkOpenGLPointGaussianMapperHelper* New();
  svtkTypeMacro(svtkOpenGLPointGaussianMapperHelper, svtkOpenGLPolyDataMapper);

  svtkPointGaussianMapper* Owner;

  // set from parent
  float* OpacityTable;  // the table
  double OpacityScale;  // used for quick lookups
  double OpacityOffset; // used for quick lookups
  float* ScaleTable;    // the table
  double ScaleScale;    // used for quick lookups
  double ScaleOffset;   // used for quick lookups

  svtkIdType FlatIndex;

  bool UsingPoints;
  double TriangleScale;

  // called by our Owner skips some stuff
  void GaussianRender(svtkRenderer* ren, svtkActor* act);

protected:
  svtkOpenGLPointGaussianMapperHelper();
  ~svtkOpenGLPointGaussianMapperHelper() override;

  // Description:
  // Create the basic shaders before replacement
  void GetShaderTemplate(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer*, svtkActor*) override;

  // Description:
  // Perform string replacements on the shader templates
  void ReplaceShaderColor(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer*, svtkActor*) override;
  void ReplaceShaderPositionVC(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer*, svtkActor*) override;

  // Description:
  // Set the shader parameters related to the Camera
  void SetCameraShaderParameters(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act) override;

  // Description:
  // Set the shader parameters related to the actor/mapper
  void SetMapperShaderParameters(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act) override;

  // Description:
  // Does the VBO/IBO need to be rebuilt
  bool GetNeedToRebuildBufferObjects(svtkRenderer* ren, svtkActor* act) override;

  // Description:
  // Update the VBO to contain point based values
  void BuildBufferObjects(svtkRenderer* ren, svtkActor* act) override;

  void RenderPieceDraw(svtkRenderer* ren, svtkActor* act) override;

  // Description:
  // Does the shader source need to be recomputed
  bool GetNeedToRebuildShaders(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act) override;

private:
  svtkOpenGLPointGaussianMapperHelper(const svtkOpenGLPointGaussianMapperHelper&) = delete;
  void operator=(const svtkOpenGLPointGaussianMapperHelper&) = delete;
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLPointGaussianMapperHelper);

//-----------------------------------------------------------------------------
svtkOpenGLPointGaussianMapperHelper::svtkOpenGLPointGaussianMapperHelper()
{
  this->Owner = nullptr;
  this->UsingPoints = false;
  this->TriangleScale = 0.0;
  this->FlatIndex = 1;
  this->OpacityTable = nullptr;
  this->ScaleTable = nullptr;
  this->OpacityScale = 1.0;
  this->ScaleScale = 1.0;
  this->OpacityOffset = 0.0;
  this->ScaleOffset = 0.0;
}

//-----------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapperHelper::GetShaderTemplate(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  this->Superclass::GetShaderTemplate(shaders, ren, actor);

  if (this->Owner->GetScaleFactor() == 0.0)
  {
    this->UsingPoints = true;
  }
  else
  {
    this->UsingPoints = false;
    // for splats use a special shader that handles the offsets
    shaders[svtkShader::Vertex]->SetSource(svtkPointGaussianVS);
    shaders[svtkShader::Geometry]->SetSource(svtkPointGaussianGS);
  }
}

void svtkOpenGLPointGaussianMapperHelper::ReplaceShaderPositionVC(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  if (!this->UsingPoints)
  {
    std::string VSSource = shaders[svtkShader::Vertex]->GetSource();
    std::string FSSource = shaders[svtkShader::Fragment]->GetSource();

    svtkShaderProgram::Substitute(FSSource, "//SVTK::PositionVC::Dec", "in vec2 offsetVCVSOutput;");

    svtkShaderProgram::Substitute(VSSource, "//SVTK::Camera::Dec",
      "uniform mat4 VCDCMatrix;\n"
      "uniform mat4 MCVCMatrix;");

    shaders[svtkShader::Vertex]->SetSource(VSSource);
    shaders[svtkShader::Fragment]->SetSource(FSSource);
  }

  this->Superclass::ReplaceShaderPositionVC(shaders, ren, actor);
}

void svtkOpenGLPointGaussianMapperHelper::ReplaceShaderColor(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  if (!this->UsingPoints)
  {
    std::string FSSource = shaders[svtkShader::Fragment]->GetSource();

    if (this->Owner->GetSplatShaderCode() && strcmp(this->Owner->GetSplatShaderCode(), "") != 0)
    {
      svtkShaderProgram::Substitute(
        FSSource, "//SVTK::Color::Impl", this->Owner->GetSplatShaderCode(), false);
    }
    else
    {
      svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Impl",
        // compute the eye position and unit direction
        "//SVTK::Color::Impl\n"
        "  float dist2 = dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy);\n"
        "  float gaussian = exp(-0.5*dist2);\n"
        "  opacity = opacity*gaussian;"
        //  "  opacity = opacity*0.5;"
        ,
        false);
    }
    shaders[svtkShader::Fragment]->SetSource(FSSource);
  }

  this->Superclass::ReplaceShaderColor(shaders, ren, actor);
  // cerr << shaders[svtkShader::Fragment]->GetSource() << endl;
}

//-----------------------------------------------------------------------------
bool svtkOpenGLPointGaussianMapperHelper::GetNeedToRebuildShaders(
  svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* actor)
{
  this->LastLightComplexity[&cellBO] = 0;

  svtkHardwareSelector* selector = ren->GetSelector();
  int picking = selector ? selector->GetCurrentPass() : -1;
  if (this->LastSelectionState != picking)
  {
    this->SelectionStateChanged.Modified();
    this->LastSelectionState = picking;
  }

  svtkMTimeType renderPassMTime = this->GetRenderPassStageMTime(actor);

  // has something changed that would require us to recreate the shader?
  // candidates are
  // property modified (representation interpolation and lighting)
  // input modified
  // light complexity changed
  if (cellBO.Program == nullptr || cellBO.ShaderSourceTime < this->GetMTime() ||
    cellBO.ShaderSourceTime < actor->GetMTime() ||
    cellBO.ShaderSourceTime < this->CurrentInput->GetMTime() ||
    cellBO.ShaderSourceTime < this->SelectionStateChanged ||
    cellBO.ShaderSourceTime < renderPassMTime)
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
svtkOpenGLPointGaussianMapperHelper::~svtkOpenGLPointGaussianMapperHelper() {}

//-----------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapperHelper::SetCameraShaderParameters(
  svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* actor)
{
  if (this->UsingPoints)
  {
    this->Superclass::SetCameraShaderParameters(cellBO, ren, actor);
  }
  else
  {
    svtkShaderProgram* program = cellBO.Program;

    svtkOpenGLCamera* cam = (svtkOpenGLCamera*)(ren->GetActiveCamera());

    svtkMatrix4x4* wcdc;
    svtkMatrix4x4* wcvc;
    svtkMatrix3x3* norms;
    svtkMatrix4x4* vcdc;
    cam->GetKeyMatrices(ren, wcvc, norms, vcdc, wcdc);
    program->SetUniformMatrix("VCDCMatrix", vcdc);

    if (!actor->GetIsIdentity())
    {
      svtkMatrix4x4* mcwc;
      svtkMatrix3x3* anorms;
      ((svtkOpenGLActor*)actor)->GetKeyMatrices(mcwc, anorms);
      svtkMatrix4x4::Multiply4x4(mcwc, wcvc, this->TempMatrix4);
      program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
    }
    else
    {
      program->SetUniformMatrix("MCVCMatrix", wcvc);
    }

    // add in uniforms for parallel and distance
    cellBO.Program->SetUniformi("cameraParallel", cam->GetParallelProjection());
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapperHelper::SetMapperShaderParameters(
  svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* actor)
{
  if (!this->UsingPoints)
  {
    cellBO.Program->SetUniformf("triangleScale", this->TriangleScale);
  }
  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
}

namespace
{

template <typename PointDataType>
PointDataType svtkOpenGLPointGaussianMapperHelperGetComponent(
  PointDataType* tuple, int nComponent, int component)
{
  // If this is a single component array, make sure we do not compute
  // a useless magnitude
  if (nComponent == 1)
  {
    component = 0;
  }

  // If we request a non-existing componeent, return the magnitude of the tuple
  PointDataType compVal = 0.0;
  if (component < 0 || component >= nComponent)
  {
    for (int iComp = 0; iComp < nComponent; iComp++)
    {
      PointDataType tmp = tuple[iComp];
      compVal += tmp * tmp;
    }
    compVal = sqrt(compVal);
  }
  else
  {
    compVal = tuple[component];
  }
  return compVal;
}

void svtkOpenGLPointGaussianMapperHelperComputeColor(unsigned char* rcolor, unsigned char* colors,
  int colorComponents, svtkIdType index, svtkDataArray* opacities, int opacitiesComponent,
  svtkOpenGLPointGaussianMapperHelper* self)
{
  unsigned char white[4] = { 255, 255, 255, 255 };

  // if there are no per point sizes and the default size is zero
  // then just render points, saving memory and speed
  unsigned char* colorPtr = colors ? (colors + index * colorComponents) : white;
  rcolor[0] = *(colorPtr++);
  rcolor[1] = *(colorPtr++);
  rcolor[2] = *(colorPtr++);

  if (opacities)
  {
    double opacity = svtkOpenGLPointGaussianMapperHelperGetComponent<double>(
      opacities->GetTuple(index), opacities->GetNumberOfComponents(), opacitiesComponent);
    if (self->OpacityTable)
    {
      double tindex = (opacity - self->OpacityOffset) * self->OpacityScale;
      int itindex = static_cast<int>(tindex);
      if (itindex >= self->Owner->GetOpacityTableSize() - 1)
      {
        opacity = self->OpacityTable[self->Owner->GetOpacityTableSize() - 1];
      }
      else if (itindex < 0)
      {
        opacity = self->OpacityTable[0];
      }
      else
      {
        opacity = (1.0 - tindex + itindex) * self->OpacityTable[itindex] +
          (tindex - itindex) * self->OpacityTable[itindex + 1];
      }
    }
    rcolor[3] = static_cast<float>(opacity * 255.0);
  }
  else
  {
    rcolor[3] = (colorComponents == 4 ? *colorPtr : 255);
  }
}

void svtkOpenGLPointGaussianMapperHelperColors(svtkUnsignedCharArray* outColors, svtkIdType numPts,
  unsigned char* colors, int colorComponents, svtkDataArray* opacities, int opacitiesComponent,
  svtkOpenGLPointGaussianMapperHelper* self, svtkCellArray* verts)
{
  unsigned char* vPtr = static_cast<unsigned char*>(outColors->GetVoidPointer(0));

  // iterate over cells or not
  if (verts->GetNumberOfCells())
  {
    const svtkIdType* indices(nullptr);
    svtkIdType npts(0);
    for (verts->InitTraversal(); verts->GetNextCell(npts, indices);)
    {
      for (int i = 0; i < npts; ++i)
      {
        svtkOpenGLPointGaussianMapperHelperComputeColor(
          vPtr, colors, colorComponents, indices[i], opacities, opacitiesComponent, self);
        vPtr += 4;
      }
    }
  }
  else
  {
    for (svtkIdType i = 0; i < numPts; i++)
    {
      svtkOpenGLPointGaussianMapperHelperComputeColor(
        vPtr, colors, colorComponents, i, opacities, opacitiesComponent, self);
      vPtr += 4;
    }
  }
}

float svtkOpenGLPointGaussianMapperHelperGetRadius(
  double radius, svtkOpenGLPointGaussianMapperHelper* self)
{
  if (self->ScaleTable)
  {
    double tindex = (radius - self->ScaleOffset) * self->ScaleScale;
    int itindex = static_cast<int>(tindex);
    if (itindex >= self->Owner->GetScaleTableSize() - 1)
    {
      radius = self->ScaleTable[self->Owner->GetScaleTableSize() - 1];
    }
    else if (itindex < 0)
    {
      radius = self->ScaleTable[0];
    }
    else
    {
      radius = (1.0 - tindex + itindex) * self->ScaleTable[itindex] +
        (tindex - itindex) * self->ScaleTable[itindex + 1];
    }
  }
  radius *= self->Owner->GetScaleFactor();
  radius *= self->TriangleScale;

  return static_cast<float>(radius);
}

template <typename PointDataType>
void svtkOpenGLPointGaussianMapperHelperSizes(svtkFloatArray* scales, PointDataType* sizes,
  int nComponent, int component, svtkIdType numPts, svtkOpenGLPointGaussianMapperHelper* self,
  svtkCellArray* verts)
{
  float* it = static_cast<float*>(scales->GetVoidPointer(0));

  // iterate over cells or not
  if (verts->GetNumberOfCells())
  {
    const svtkIdType* indices(nullptr);
    svtkIdType npts(0);
    for (verts->InitTraversal(); verts->GetNextCell(npts, indices);)
    {
      for (svtkIdType i = 0; i < npts; ++i)
      {
        PointDataType size = 1.0;
        if (sizes)
        {
          size = svtkOpenGLPointGaussianMapperHelperGetComponent<PointDataType>(
            &sizes[indices[i] * nComponent], nComponent, component);
        }
        float radiusFloat = svtkOpenGLPointGaussianMapperHelperGetRadius(size, self);
        *(it++) = radiusFloat;
      }
    }
  }
  else
  {
    for (svtkIdType i = 0; i < numPts; i++)
    {
      PointDataType size = 1.0;
      if (sizes)
      {
        size = svtkOpenGLPointGaussianMapperHelperGetComponent<PointDataType>(
          &sizes[i * nComponent], nComponent, component);
      }
      float radiusFloat = svtkOpenGLPointGaussianMapperHelperGetRadius(size, self);
      *(it++) = radiusFloat;
    }
  }
}

template <typename PointDataType>
void svtkOpenGLPointGaussianMapperHelperPoints(
  svtkFloatArray* vcoords, PointDataType* points, svtkCellArray* verts)
{
  float* vPtr = static_cast<float*>(vcoords->GetVoidPointer(0));
  PointDataType* pointPtr;

  const svtkIdType* indices(nullptr);
  svtkIdType npts(0);
  for (verts->InitTraversal(); verts->GetNextCell(npts, indices);)
  {
    for (int i = 0; i < npts; ++i)
    {
      pointPtr = points + indices[i] * 3;

      // Vertices
      *(vPtr++) = pointPtr[0];
      *(vPtr++) = pointPtr[1];
      *(vPtr++) = pointPtr[2];
    }
  }
}

} // anonymous namespace

//-------------------------------------------------------------------------
bool svtkOpenGLPointGaussianMapperHelper::GetNeedToRebuildBufferObjects(
  svtkRenderer* svtkNotUsed(ren), svtkActor* act)
{
  if (this->VBOBuildTime < this->GetMTime() || this->VBOBuildTime < act->GetMTime() ||
    this->VBOBuildTime < this->CurrentInput->GetMTime() ||
    this->VBOBuildTime < this->Owner->GetMTime() ||
    (this->Owner->GetScalarOpacityFunction() &&
      this->VBOBuildTime < this->Owner->GetScalarOpacityFunction()->GetMTime()) ||
    (this->Owner->GetScaleFunction() &&
      this->VBOBuildTime < this->Owner->GetScaleFunction()->GetMTime()))
  {
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapperHelper::BuildBufferObjects(
  svtkRenderer* ren, svtkActor* svtkNotUsed(act))
{
  svtkPolyData* poly = this->CurrentInput;

  if (poly == nullptr)
  {
    return;
  }

  // set the triangle scale
  this->TriangleScale = this->Owner->GetTriangleScale();

  bool hasScaleArray = this->Owner->GetScaleArray() != nullptr &&
    poly->GetPointData()->HasArray(this->Owner->GetScaleArray());

  if (this->Owner->GetScaleFactor() == 0.0)
  {
    this->UsingPoints = true;
  }
  else
  {
    this->UsingPoints = false;
  }

  // if we have an opacity array then get it and if we have
  // a ScalarOpacityFunction map the array through it
  bool hasOpacityArray = this->Owner->GetOpacityArray() != nullptr &&
    poly->GetPointData()->HasArray(this->Owner->GetOpacityArray());

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(1.0);

  int splatCount = poly->GetPoints()->GetNumberOfPoints();
  if (poly->GetVerts()->GetNumberOfCells())
  {
    splatCount = poly->GetVerts()->GetNumberOfConnectivityIds();
  }

  // need to build points?
  if (poly->GetVerts()->GetNumberOfCells())
  {
    svtkFloatArray* pts = svtkFloatArray::New();
    pts->SetNumberOfComponents(3);
    pts->SetNumberOfTuples(splatCount);
    switch (poly->GetPoints()->GetDataType())
    {
      svtkTemplateMacro(svtkOpenGLPointGaussianMapperHelperPoints(
        pts, static_cast<SVTK_TT*>(poly->GetPoints()->GetVoidPointer(0)), poly->GetVerts()));
    }
    this->VBOs->CacheDataArray("vertexMC", pts, ren, SVTK_FLOAT);
    pts->Delete();
  }
  else // just pass the points
  {
    this->VBOs->CacheDataArray("vertexMC", poly->GetPoints()->GetData(), ren, SVTK_FLOAT);
  }

  if (!this->UsingPoints)
  {
    svtkFloatArray* offsets = svtkFloatArray::New();
    offsets->SetNumberOfComponents(1);
    offsets->SetNumberOfTuples(splatCount);

    if (hasScaleArray)
    {
      svtkDataArray* sizes = poly->GetPointData()->GetArray(this->Owner->GetScaleArray());
      switch (sizes->GetDataType())
      {
        svtkTemplateMacro(svtkOpenGLPointGaussianMapperHelperSizes(offsets,
          static_cast<SVTK_TT*>(sizes->GetVoidPointer(0)), sizes->GetNumberOfComponents(),
          this->Owner->GetScaleArrayComponent(), poly->GetPoints()->GetNumberOfPoints(), this,
          poly->GetVerts()));
      }
    }
    else
    {
      svtkOpenGLPointGaussianMapperHelperSizes(offsets, static_cast<float*>(nullptr), 0, 0,
        poly->GetPoints()->GetNumberOfPoints(), this, poly->GetVerts());
    }
    this->VBOs->CacheDataArray("radiusMC", offsets, ren, SVTK_FLOAT);
    offsets->Delete();
  }
  else
  {
    this->VBOs->CacheDataArray("radiusMC", nullptr, ren, SVTK_FLOAT);
  }

  if (this->Colors)
  {
    svtkUnsignedCharArray* clrs = svtkUnsignedCharArray::New();
    clrs->SetNumberOfComponents(4);
    clrs->SetNumberOfTuples(splatCount);

    svtkOpenGLPointGaussianMapperHelperColors(clrs, poly->GetPoints()->GetNumberOfPoints(),
      this->Colors ? (unsigned char*)this->Colors->GetVoidPointer(0) : (unsigned char*)nullptr,
      this->Colors ? this->Colors->GetNumberOfComponents() : 0,
      hasOpacityArray ? poly->GetPointData()->GetArray(this->Owner->GetOpacityArray())
                      : (svtkDataArray*)nullptr,
      this->Owner->GetOpacityArrayComponent(), this, poly->GetVerts());
    this->VBOs->CacheDataArray("scalarColor", clrs, ren, SVTK_UNSIGNED_CHAR);
    clrs->Delete();
  }

  this->VBOs->BuildAllVBOs(ren);

  // we use no IBO
  for (int i = PrimitiveStart; i < PrimitiveEnd; i++)
  {
    this->Primitives[i].IBO->IndexCount = 0;
  }
  this->Primitives[PrimitiveTris].IBO->IndexCount = splatCount;
  this->VBOBuildTime.Modified();
}

//-----------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapperHelper::RenderPieceDraw(svtkRenderer* ren, svtkActor* actor)
{
  // draw polygons
  int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
  if (numVerts)
  {
    this->UpdateShaders(this->Primitives[PrimitiveTris], ren, actor);
    glDrawArrays(GL_POINTS, 0, static_cast<GLuint>(numVerts));
  }
}

namespace
{
// helper to get the state of picking
int getPickState(svtkRenderer* ren)
{
  svtkHardwareSelector* selector = ren->GetSelector();
  if (selector)
  {
    return selector->GetCurrentPass();
  }

  return svtkHardwareSelector::MIN_KNOWN_PASS - 1;
}
}

//-----------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapperHelper::GaussianRender(svtkRenderer* ren, svtkActor* actor)
{
  int picking = getPickState(ren);
  if (this->LastSelectionState != picking)
  {
    this->SelectionStateChanged.Modified();
    this->LastSelectionState = picking;
  }

  this->LastBoundBO = nullptr;
  this->CurrentInput = this->GetInput();

  this->UpdateBufferObjects(ren, actor);
  this->RenderPieceDraw(ren, actor);

  if (this->LastBoundBO)
  {
    this->LastBoundBO->VAO->Release();
  }
}

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLPointGaussianMapper);

//-----------------------------------------------------------------------------
svtkOpenGLPointGaussianMapper::svtkOpenGLPointGaussianMapper()
{
  this->OpacityTable = nullptr;
  this->ScaleTable = nullptr;
  this->OpacityScale = 1.0;
  this->ScaleScale = 1.0;
  this->OpacityOffset = 0.0;
  this->ScaleOffset = 0.0;
}

svtkOpenGLPointGaussianMapper::~svtkOpenGLPointGaussianMapper()
{
  if (this->OpacityTable)
  {
    delete[] this->OpacityTable;
    this->OpacityTable = nullptr;
  }
  if (this->ScaleTable)
  {
    delete[] this->ScaleTable;
    this->ScaleTable = nullptr;
  }

  // clear old helpers carefully due to garbage collection loops
  for (auto hiter = this->Helpers.begin(); hiter != this->Helpers.end(); ++hiter)
  {
    // these pointers may be set to nullptr by the garbage collector
    // since we are passing them in using ReportReferences
    if (*hiter)
    {
      (*hiter)->Delete();
    }
  }
  this->Helpers.clear();
}

void svtkOpenGLPointGaussianMapper::ReportReferences(svtkGarbageCollector* collector)
{
  // Report references held by this object that may be in a loop.
  this->Superclass::ReportReferences(collector);

  // helpers is a vector
  for (auto hiter = this->Helpers.begin(); hiter != this->Helpers.end(); ++hiter)
  {
    svtkGarbageCollectorReport(collector, *hiter, "svtkOpenGLPointGaussianMapperHelper");
  }
}

void svtkOpenGLPointGaussianMapper::Render(svtkRenderer* ren, svtkActor* actor)
{
  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  if (this->GetInputAlgorithm() == nullptr)
  {
    return;
  }

  if (!this->Static)
  {
    this->InvokeEvent(svtkCommand::StartEvent, nullptr);
    this->GetInputAlgorithm()->Update();
    this->InvokeEvent(svtkCommand::EndEvent, nullptr);
  }

  if (this->GetInputDataObject(0, 0) == nullptr)
  {
    svtkErrorMacro(<< "No input!");
    return;
  }

  // update tables
  if (this->GetScaleFunction() && this->GetScaleArray())
  {
    if (this->ScaleTableUpdateTime < this->GetScaleFunction()->GetMTime() ||
      this->ScaleTableUpdateTime < this->GetMTime())
    {
      this->BuildScaleTable();
      this->ScaleTableUpdateTime.Modified();
    }
  }
  else
  {
    delete[] this->ScaleTable;
    this->ScaleTable = nullptr;
  }

  if (this->GetScalarOpacityFunction() && this->GetOpacityArray())
  {
    if (this->OpacityTableUpdateTime < this->GetScalarOpacityFunction()->GetMTime() ||
      this->OpacityTableUpdateTime < this->GetMTime())
    {
      this->BuildOpacityTable();
      this->OpacityTableUpdateTime.Modified();
    }
  }
  else
  {
    delete[] this->OpacityTable;
    this->OpacityTable = nullptr;
  }

  // the first step is to update the helpers if needed
  if (this->HelperUpdateTime < this->GetInputDataObject(0, 0)->GetMTime() ||
    this->HelperUpdateTime < this->GetInputAlgorithm()->GetMTime() ||
    this->HelperUpdateTime < this->GetMTime())
  {
    // clear old helpers
    for (auto hiter = this->Helpers.begin(); hiter != this->Helpers.end(); ++hiter)
    {
      (*hiter)->Delete();
    }
    this->Helpers.clear();

    // build new helpers
    svtkCompositeDataSet* input = svtkCompositeDataSet::SafeDownCast(this->GetInputDataObject(0, 0));

    if (input)
    {
      svtkSmartPointer<svtkDataObjectTreeIterator> iter =
        svtkSmartPointer<svtkDataObjectTreeIterator>::New();
      iter->SetDataSet(input);
      iter->SkipEmptyNodesOn();
      iter->VisitOnlyLeavesOn();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        unsigned int flatIndex = iter->GetCurrentFlatIndex();
        svtkDataObject* dso = iter->GetCurrentDataObject();
        svtkPolyData* pd = svtkPolyData::SafeDownCast(dso);

        if (!pd || !pd->GetPoints())
        {
          continue;
        }

        svtkOpenGLPointGaussianMapperHelper* helper = this->CreateHelper();
        this->CopyMapperValuesToHelper(helper);
        helper->SetInputData(pd);
        helper->FlatIndex = flatIndex;
        this->Helpers.push_back(helper);
      }
    }
    else
    {
      svtkPolyData* pd = svtkPolyData::SafeDownCast(this->GetInputDataObject(0, 0));
      if (pd && pd->GetPoints())
      {
        svtkOpenGLPointGaussianMapperHelper* helper = this->CreateHelper();
        this->CopyMapperValuesToHelper(helper);
        helper->SetInputData(pd);
        this->Helpers.push_back(helper);
      }
    }

    this->HelperUpdateTime.Modified();
  }

  if (this->Emissive != 0 && !ren->GetSelector())
  {
    svtkOpenGLState* ostate = static_cast<svtkOpenGLRenderer*>(ren)->GetState();
    svtkOpenGLState::ScopedglBlendFuncSeparate bfsaver(ostate);
    ostate->svtkglDepthMask(GL_FALSE);
    ostate->svtkglBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive for emissive sources
    this->RenderInternal(ren, actor);
  }
  else // intentional else due to scope
  {
    this->RenderInternal(ren, actor);
  }
}

// this could be made much faster for composite
// datasets that have lots of small blocks
// but for now we just want to add the functionality
void svtkOpenGLPointGaussianMapper::RenderInternal(svtkRenderer* ren, svtkActor* actor)
{
  // Set the PointSize
#ifndef GL_ES_VERSION_3_0
  glPointSize(actor->GetProperty()->GetPointSize()); // not on ES2
#endif

  // render points for point picking in a special way
  svtkHardwareSelector* selector = ren->GetSelector();
  if (selector && selector->GetFieldAssociation() == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    static_cast<svtkOpenGLRenderer*>(ren)->GetState()->svtkglDepthMask(GL_FALSE);
  }

  if (selector)
  {
    selector->BeginRenderProp();
  }

  for (auto hiter = this->Helpers.begin(); hiter != this->Helpers.end(); ++hiter)
  {
    // make sure the BOs are up to date
    svtkOpenGLPointGaussianMapperHelper* helper = *hiter;
    if (selector && selector->GetCurrentPass() == svtkHardwareSelector::COMPOSITE_INDEX_PASS)
    {
      selector->RenderCompositeIndex(helper->FlatIndex);
    }
    helper->GaussianRender(ren, actor);
  }

  // reset picking
  if (selector && selector->GetFieldAssociation() == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    static_cast<svtkOpenGLRenderer*>(ren)->GetState()->svtkglDepthMask(GL_TRUE);
  }
  if (selector)
  {
    selector->EndRenderProp();
  }

  this->UpdateProgress(1.0);
}

svtkOpenGLPointGaussianMapperHelper* svtkOpenGLPointGaussianMapper::CreateHelper()
{
  auto helper = svtkOpenGLPointGaussianMapperHelper::New();
  helper->Owner = this;
  return helper;
}

void svtkOpenGLPointGaussianMapper::CopyMapperValuesToHelper(
  svtkOpenGLPointGaussianMapperHelper* helper)
{
  helper->svtkPolyDataMapper::ShallowCopy(this);
  helper->OpacityTable = this->OpacityTable;
  helper->OpacityScale = this->OpacityScale;
  helper->OpacityOffset = this->OpacityOffset;
  helper->ScaleTable = this->ScaleTable;
  helper->ScaleScale = this->ScaleScale;
  helper->ScaleOffset = this->ScaleOffset;
  helper->Modified();
}

//-----------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapper::ReleaseGraphicsResources(svtkWindow* win)
{
  for (auto hiter = this->Helpers.begin(); hiter != this->Helpers.end(); ++hiter)
  {
    (*hiter)->ReleaseGraphicsResources(win);
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
bool svtkOpenGLPointGaussianMapper::HasTranslucentPolygonalGeometry()
{
  // emissive always needs to be opaque
  if (this->Emissive)
  {
    return false;
  }
  return this->Superclass::HasTranslucentPolygonalGeometry();
}

//-------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapper::BuildScaleTable()
{
  double range[2];

  // if a piecewise function was provided, use it to map the opacities
  svtkPiecewiseFunction* pwf = this->GetScaleFunction();
  int tableSize = this->GetScaleTableSize();

  delete[] this->ScaleTable;
  this->ScaleTable = new float[tableSize + 1];
  if (pwf)
  {
    // build the interpolation table
    pwf->GetRange(range);
    pwf->GetTable(range[0], range[1], tableSize, this->ScaleTable);
    // duplicate the last value for bilinear interp edge case
    this->ScaleTable[tableSize] = this->ScaleTable[tableSize - 1];
    this->ScaleScale = (tableSize - 1.0) / (range[1] - range[0]);
    this->ScaleOffset = range[0];
  }
  this->Modified();
}

//-------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapper::BuildOpacityTable()
{
  double range[2];

  // if a piecewise function was provided, use it to map the opacities
  svtkPiecewiseFunction* pwf = this->GetScalarOpacityFunction();
  int tableSize = this->GetOpacityTableSize();

  delete[] this->OpacityTable;
  this->OpacityTable = new float[tableSize + 1];
  if (pwf)
  {
    // build the interpolation table
    pwf->GetRange(range);
    pwf->GetTable(range[0], range[1], tableSize, this->OpacityTable);
    // duplicate the last value for bilinear interp edge case
    this->OpacityTable[tableSize] = this->OpacityTable[tableSize - 1];
    this->OpacityScale = (tableSize - 1.0) / (range[1] - range[0]);
    this->OpacityOffset = range[0];
  }
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkOpenGLPointGaussianMapper::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkOpenGLPointGaussianMapper::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
// Looks at each DataSet and finds the union of all the bounds
void svtkOpenGLPointGaussianMapper::ComputeBounds()
{
  svtkCompositeDataSet* input = svtkCompositeDataSet::SafeDownCast(this->GetInputDataObject(0, 0));

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if (!input)
  {
    this->Superclass::ComputeBounds();
    return;
  }

  svtkBoundingBox bbox;

  // for each data set build a svtkPolyDataMapper
  svtkCompositeDataIterator* iter = input->NewIterator();
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
  {
    svtkPolyData* pd = svtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
    if (pd)
    {
      double bounds[6];
      pd->GetBounds(bounds);
      bbox.AddBounds(bounds);
    }
    iter->GoToNextItem();
  }
  iter->Delete();

  bbox.GetBounds(this->Bounds);
}

//-----------------------------------------------------------------------------
void svtkOpenGLPointGaussianMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkOpenGLPointGaussianMapper::ProcessSelectorPixelBuffers(
  svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, svtkProp* prop)
{
  if (sel->GetCurrentPass() == svtkHardwareSelector::ACTOR_PASS)
  {
    this->PickPixels.clear();
    return;
  }

  if (PickPixels.empty() && !pixeloffsets.empty())
  {
    // preprocess the image to find matching pixels and
    // store them in a map of vectors based on flat index
    // this makes the block processing far faster as we just
    // loop over the pixels for our block
    unsigned char* compositedata =
      sel->GetRawPixelBuffer(svtkHardwareSelector::COMPOSITE_INDEX_PASS);

    if (!compositedata)
    {
      return;
    }

    int maxFlatIndex = 0;
    for (auto hiter = this->Helpers.begin(); hiter != this->Helpers.end(); ++hiter)
    {
      maxFlatIndex = ((*hiter)->FlatIndex > maxFlatIndex) ? (*hiter)->FlatIndex : maxFlatIndex;
    }

    this->PickPixels.resize(maxFlatIndex + 1);

    for (auto pos : pixeloffsets)
    {
      int compval = compositedata[pos + 2];
      compval = compval << 8;
      compval |= compositedata[pos + 1];
      compval = compval << 8;
      compval |= compositedata[pos];
      compval -= 1;
      if (compval <= maxFlatIndex)
      {
        this->PickPixels[compval].push_back(pos);
      }
    }
  }

  // for each block update the image
  for (auto hiter = this->Helpers.begin(); hiter != this->Helpers.end(); ++hiter)
  {
    if (!this->PickPixels[(*hiter)->FlatIndex].empty())
    {
      (*hiter)->ProcessSelectorPixelBuffers(sel, this->PickPixels[(*hiter)->FlatIndex], prop);
    }
  }
}
