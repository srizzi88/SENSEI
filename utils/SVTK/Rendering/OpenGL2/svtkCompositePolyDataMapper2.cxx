/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositePolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositePolyDataMapper2.h"

#include "svtk_glew.h"

#include "svtkBoundingBox.h"
#include "svtkCellData.h"
#include "svtkColorTransferFunction.h"
#include "svtkCommand.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositeDataSetRange.h"
#include "svtkDataObjectTree.h"
#include "svtkDataObjectTreeRange.h"
#include "svtkFloatArray.h"
#include "svtkHardwareSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLCellToSVTKCellMap.h"
#include "svtkOpenGLIndexBufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLShaderProperty.h"
#include "svtkOpenGLTexture.h"
#include "svtkOpenGLVertexBufferObject.h"
#include "svtkOpenGLVertexBufferObjectGroup.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkScalarsToColors.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"
#include "svtkTransform.h"
#include "svtkUnsignedIntArray.h"

#include <algorithm>
#include <sstream>

#include "svtkCompositePolyDataMapper2Internal.h"

typedef std::map<svtkPolyData*, svtkCompositeMapperHelperData*>::iterator dataIter;
typedef std::map<const std::string, svtkCompositeMapperHelper2*>::iterator helpIter;

svtkStandardNewMacro(svtkCompositeMapperHelper2);

svtkCompositeMapperHelper2::~svtkCompositeMapperHelper2()
{
  for (dataIter it = this->Data.begin(); it != this->Data.end(); ++it)
  {
    delete it->second;
  }
  this->Data.clear();
  this->RenderedList.clear();
}

void svtkCompositeMapperHelper2::SetShaderValues(
  svtkShaderProgram* prog, svtkCompositeMapperHelperData* hdata, size_t primOffset)
{
  if (this->PrimIDUsed)
  {
    prog->SetUniformi("PrimitiveIDOffset", static_cast<int>(primOffset));
  }

  if (this->CurrentSelector)
  {
    if (this->CurrentSelector->GetCurrentPass() == svtkHardwareSelector::COMPOSITE_INDEX_PASS &&
      prog->IsUniformUsed("mapperIndex"))
    {
      this->CurrentSelector->RenderCompositeIndex(hdata->FlatIndex);
      prog->SetUniform3f("mapperIndex", this->CurrentSelector->GetPropColorValue());
    }
    return;
  }

  // If requested, color partial / missing arrays with NaN color.
  bool useNanColor = false;
  double nanColor[4] = { -1., -1., -1., -1 };
  if (this->Parent->GetColorMissingArraysWithNanColor() && this->GetScalarVisibility())
  {
    int cellFlag = 0;
    svtkAbstractArray* scalars = svtkAbstractMapper::GetAbstractScalars(hdata->Data, this->ScalarMode,
      this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);
    if (scalars == nullptr)
    {
      svtkLookupTable* lut = svtkLookupTable::SafeDownCast(this->GetLookupTable());
      svtkColorTransferFunction* ctf =
        lut ? nullptr : svtkColorTransferFunction::SafeDownCast(this->GetLookupTable());
      if (lut)
      {
        lut->GetNanColor(nanColor);
        useNanColor = true;
      }
      else if (ctf)
      {
        ctf->GetNanColor(nanColor);
        useNanColor = true;
      }
    }
  }

  // override the opacity and color
  prog->SetUniformf("opacityUniform", hdata->Opacity);

  if (useNanColor)
  {
    float fnancolor[3] = { static_cast<float>(nanColor[0]), static_cast<float>(nanColor[1]),
      static_cast<float>(nanColor[2]) };
    prog->SetUniform3f("ambientColorUniform", fnancolor);
    prog->SetUniform3f("diffuseColorUniform", fnancolor);
  }
  else
  {
    svtkColor3d& aColor = hdata->AmbientColor;
    float ambientColor[3] = { static_cast<float>(aColor[0]), static_cast<float>(aColor[1]),
      static_cast<float>(aColor[2]) };
    svtkColor3d& dColor = hdata->DiffuseColor;
    float diffuseColor[3] = { static_cast<float>(dColor[0]), static_cast<float>(dColor[1]),
      static_cast<float>(dColor[2]) };
    prog->SetUniform3f("ambientColorUniform", ambientColor);
    prog->SetUniform3f("diffuseColorUniform", diffuseColor);
    if (this->OverideColorUsed)
    {
      prog->SetUniformi("OverridesColor", hdata->OverridesColor);
    }
  }
}

void svtkCompositeMapperHelper2::UpdateShaders(
  svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act)
{
#ifndef SVTK_LEGACY_REMOVE
  // in cases where LegacyShaderProperty is not nullptr, it means someone has used
  // legacy shader replacement functions, so we make sure the actor uses the same
  // shader property. NOTE: this implies that it is not possible to use both legacy
  // and new functionality on the same actor/mapper.
  if (this->Parent->LegacyShaderProperty &&
    act->GetShaderProperty() != this->Parent->LegacyShaderProperty)
  {
    act->SetShaderProperty(this->Parent->LegacyShaderProperty);
  }
#endif

  Superclass::UpdateShaders(cellBO, ren, act);
  if (cellBO.Program && this->Parent)
  {
    // allow the program to set what it wants
    this->Parent->InvokeEvent(svtkCommand::UpdateShaderEvent, cellBO.Program);
  }
}

void svtkCompositeMapperHelper2::ReplaceShaderColor(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  if (!this->CurrentSelector)
  {
    std::string FSSource = shaders[svtkShader::Fragment]->GetSource();

    svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Dec",
      "uniform bool OverridesColor;\n"
      "//SVTK::Color::Dec",
      false);

    svtkShaderProgram::Substitute(FSSource, "//SVTK::Color::Impl",
      "//SVTK::Color::Impl\n"
      "  if (OverridesColor) {\n"
      "    ambientColor = ambientColorUniform * ambientIntensity;\n"
      "    diffuseColor = diffuseColorUniform * diffuseIntensity; }\n",
      false);

    shaders[svtkShader::Fragment]->SetSource(FSSource);
  }

  this->Superclass::ReplaceShaderColor(shaders, ren, actor);
}

void svtkCompositeMapperHelper2::ClearMark()
{
  for (dataIter it = this->Data.begin(); it != this->Data.end(); ++it)
  {
    it->second->Marked = false;
  }
  this->Marked = false;
}

void svtkCompositeMapperHelper2::RemoveUnused()
{
  for (dataIter it = this->Data.begin(); it != this->Data.end();)
  {
    if (!it->second->Marked)
    {
      delete it->second;
      this->Data.erase(it++);
      this->Modified();
    }
    else
    {
      ++it;
    }
  }
}

//-----------------------------------------------------------------------------
// Returns if we can use texture maps for scalar coloring. Note this doesn't say
// we "will" use scalar coloring. It says, if we do use scalar coloring, we will
// use a texture.
// When rendering multiblock datasets, if any 2 blocks provide different
// lookup tables for the scalars, then also we cannot use textures. This case can
// be handled if required.
int svtkCompositeMapperHelper2::CanUseTextureMapForColoring(svtkDataObject*)
{
  if (!this->InterpolateScalarsBeforeMapping)
  {
    return 0; // user doesn't want us to use texture maps at all.
  }

  int cellFlag = 0;
  svtkScalarsToColors* scalarsLookupTable = nullptr;
  for (dataIter it = this->Data.begin(); it != this->Data.end(); ++it)
  {
    svtkPolyData* pd = it->second->Data;
    svtkDataArray* scalars = svtkAbstractMapper::GetScalars(
      pd, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);

    if (scalars)
    {
      if (cellFlag)
      {
        return 0;
      }
      if ((this->ColorMode == SVTK_COLOR_MODE_DEFAULT &&
            svtkArrayDownCast<svtkUnsignedCharArray>(scalars)) ||
        this->ColorMode == SVTK_COLOR_MODE_DIRECT_SCALARS)
      {
        // Don't use texture if direct coloring using RGB unsigned chars is
        // requested.
        return 0;
      }

      if (scalarsLookupTable && scalars->GetLookupTable() &&
        (scalarsLookupTable != scalars->GetLookupTable()))
      {
        // Two datasets are requesting different lookup tables to color with.
        // We don't handle this case right now for composite datasets.
        return 0;
      }
      if (scalars->GetLookupTable())
      {
        scalarsLookupTable = scalars->GetLookupTable();
      }
    }
  }

  if ((scalarsLookupTable && scalarsLookupTable->GetIndexedLookup()) ||
    (!scalarsLookupTable && this->LookupTable && this->LookupTable->GetIndexedLookup()))
  {
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkCompositeMapperHelper2::RenderPiece(svtkRenderer* ren, svtkActor* actor)
{
  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  this->CurrentInput = this->Data.begin()->first;

  this->RenderPieceStart(ren, actor);
  this->RenderPieceDraw(ren, actor);
  this->RenderPieceFinish(ren, actor);
}

void svtkCompositeMapperHelper2::DrawIBO(svtkRenderer* ren, svtkActor* actor, int primType,
  svtkOpenGLHelper& CellBO, GLenum mode, int pointSize)
{
  if (CellBO.IBO->IndexCount)
  {
    if (pointSize > 0)
    {
#ifndef GL_ES_VERSION_3_0
      glPointSize(pointSize); // need to use shader value
#endif
    }
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(CellBO, ren, actor);
    svtkShaderProgram* prog = CellBO.Program;
    if (!prog)
    {
      return;
    }
    this->PrimIDUsed = prog->IsUniformUsed("PrimitiveIDOffset");
    this->OverideColorUsed = prog->IsUniformUsed("OverridesColor");
    CellBO.IBO->Bind();

    if (!this->HaveWideLines(ren, actor) && mode == GL_LINES)
    {
      glLineWidth(actor->GetProperty()->GetLineWidth());
    }

    // if (this->DrawingEdgesOrVetices && !this->DrawingTubes(CellBO, actor))
    // {
    //   svtkProperty *ppty = actor->GetProperty();
    //   float diffuseColor[3] = {0.0, 0.0, 0.0};
    //   float ambientColor[3];
    //   double *acol = ppty->GetEdgeColor();
    //   ambientColor[0] = acol[0];
    //   ambientColor[1] = acol[1];
    //   ambientColor[2] = acol[2];
    //   prog->SetUniform3f("diffuseColorUniform", diffuseColor);
    //   prog->SetUniform3f("ambientColorUniform", ambientColor);
    // }

    bool selecting = (this->CurrentSelector ? true : false);
    for (dataIter it = this->Data.begin(); it != this->Data.end(); ++it)
    {
      svtkCompositeMapperHelperData* starthdata = it->second;
      if (starthdata->Visibility &&
        ((selecting || starthdata->IsOpaque) != actor->IsRenderingTranslucentPolygonalGeometry()) &&
        ((selecting && starthdata->Pickability) || !selecting) &&
        starthdata->NextIndex[primType] > starthdata->StartIndex[primType])
      {
        // compilers think this can exceed the bounds so we also
        // test against primType even though we should not need to
        if (primType <= PrimitiveTriStrips)
        {
          this->SetShaderValues(
            prog, starthdata, starthdata->CellCellMap->GetPrimitiveOffsets()[primType]);
        }
        glDrawRangeElements(mode, static_cast<GLuint>(starthdata->StartVertex),
          static_cast<GLuint>(starthdata->NextVertex > 0 ? starthdata->NextVertex - 1 : 0),
          static_cast<GLsizei>(starthdata->NextIndex[primType] - starthdata->StartIndex[primType]),
          GL_UNSIGNED_INT,
          reinterpret_cast<const GLvoid*>(starthdata->StartIndex[primType] * sizeof(GLuint)));
      }
    }
    CellBO.IBO->Release();
  }
}

//-----------------------------------------------------------------------------
void svtkCompositeMapperHelper2::RenderPieceDraw(svtkRenderer* ren, svtkActor* actor)
{
  int representation = actor->GetProperty()->GetRepresentation();

  // render points for point picking in a special way
  // all cell types should be rendered as points
  this->CurrentSelector = ren->GetSelector();
  bool pointPicking = false;
  if (this->CurrentSelector && this->PopulateSelectionSettings &&
    this->CurrentSelector->GetFieldAssociation() == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = SVTK_POINTS;
    pointPicking = true;
  }

  this->PrimitiveIDOffset = 0;

  // draw IBOs
  for (int i = PrimitiveStart; i < (this->CurrentSelector ? PrimitiveTriStrips + 1 : PrimitiveEnd);
       i++)
  {
    this->DrawingEdgesOrVertices = (i > PrimitiveTriStrips ? true : false);
    GLenum mode = this->GetOpenGLMode(representation, i);
    this->DrawIBO(ren, actor, i, this->Primitives[i], mode,
      pointPicking ? this->GetPointPickingPrimitiveSize(i) : 0);
  }

  if (this->CurrentSelector &&
    (this->CurrentSelector->GetCurrentPass() == svtkHardwareSelector::CELL_ID_LOW24 ||
      this->CurrentSelector->GetCurrentPass() == svtkHardwareSelector::CELL_ID_HIGH24))
  {
    this->CurrentSelector->SetPropColorValue(this->PrimitiveIDOffset);
  }
}

svtkCompositeMapperHelperData* svtkCompositeMapperHelper2::AddData(
  svtkPolyData* pd, unsigned int flatIndex)
{
  dataIter found = this->Data.find(pd);
  if (found == this->Data.end())
  {
    svtkCompositeMapperHelperData* hdata = new svtkCompositeMapperHelperData();
    hdata->FlatIndex = flatIndex;
    hdata->Data = pd;
    hdata->Marked = true;
    this->Data.insert(std::make_pair(pd, hdata));
    this->Modified();
    this->RenderedList.push_back(pd);
    return hdata;
  }
  found->second->FlatIndex = flatIndex;
  found->second->Marked = true;
  return found->second;
}

//-------------------------------------------------------------------------
void svtkCompositeMapperHelper2::BuildBufferObjects(svtkRenderer* ren, svtkActor* act)
{
  // render using the composite data attributes

  // create the cell scalar array adjusted for ogl Cells
  std::vector<unsigned char> newColors;
  std::vector<float> newNorms;

  dataIter iter;
  this->VBOs->ClearAllVBOs();

  if (this->Data.begin() == this->Data.end())
  {
    this->VBOBuildTime.Modified();
    return;
  }

  svtkBoundingBox bbox;
  double bounds[6];
  this->Data.begin()->second->Data->GetPoints()->GetBounds(bounds);
  bbox.SetBounds(bounds);
  svtkCompositeMapperHelperData* prevhdata = nullptr;
  for (iter = this->Data.begin(); iter != this->Data.end(); ++iter)
  {
    svtkCompositeMapperHelperData* hdata = iter->second;

    hdata->Data->GetPoints()->GetBounds(bounds);
    bbox.AddBounds(bounds);

    for (int i = 0; i < PrimitiveEnd; i++)
    {
      hdata->StartIndex[i] = static_cast<unsigned int>(this->IndexArray[i].size());
    }

    svtkIdType voffset = 0;
    // vert cell offset starts at the end of the last block
    hdata->CellCellMap->SetStartOffset(prevhdata ? prevhdata->CellCellMap->GetFinalOffset() : 0);
    this->AppendOneBufferObject(ren, act, hdata, voffset, newColors, newNorms);
    hdata->StartVertex = static_cast<unsigned int>(voffset);
    hdata->NextVertex = hdata->StartVertex + hdata->Data->GetPoints()->GetNumberOfPoints();
    for (int i = 0; i < PrimitiveEnd; i++)
    {
      hdata->NextIndex[i] = static_cast<unsigned int>(this->IndexArray[i].size());
    }
    prevhdata = hdata;
  }

  // clear color cache
  for (auto& c : this->ColorArrayMap)
  {
    c.second->Delete();
  }
  this->ColorArrayMap.clear();

  svtkOpenGLVertexBufferObject* posVBO = this->VBOs->GetVBO("vertexMC");
  if (posVBO && this->ShiftScaleMethod == svtkOpenGLVertexBufferObject::AUTO_SHIFT_SCALE)
  {
    posVBO->SetCoordShiftAndScaleMethod(svtkOpenGLVertexBufferObject::MANUAL_SHIFT_SCALE);
    bbox.GetBounds(bounds);
    std::vector<double> shift;
    std::vector<double> scale;
    for (int i = 0; i < 3; i++)
    {
      shift.push_back(0.5 * (bounds[i * 2] + bounds[i * 2 + 1]));
      scale.push_back(
        (bounds[i * 2 + 1] - bounds[i * 2]) ? 1.0 / (bounds[i * 2 + 1] - bounds[i * 2]) : 1.0);
    }
    posVBO->SetShift(shift);
    posVBO->SetScale(scale);
    // If the VBO coordinates were shifted and scaled, prepare the inverse transform
    // for application to the model->view matrix:
    if (posVBO->GetCoordShiftAndScaleEnabled())
    {
      this->VBOInverseTransform->Identity();
      this->VBOInverseTransform->Translate(shift[0], shift[1], shift[2]);
      this->VBOInverseTransform->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
      this->VBOInverseTransform->GetTranspose(this->VBOShiftScale);
    }
  }

  this->VBOs->BuildAllVBOs(ren);

  for (int i = PrimitiveStart; i < PrimitiveEnd; i++)
  {
    this->Primitives[i].IBO->IndexCount = this->IndexArray[i].size();
    if (this->Primitives[i].IBO->IndexCount)
    {
      this->Primitives[i].IBO->Upload(
        this->IndexArray[i], svtkOpenGLBufferObject::ElementArrayBuffer);
      this->IndexArray[i].resize(0);
    }
  }

  // allocate as needed
  if (this->HaveCellScalars)
  {
    if (!this->CellScalarTexture)
    {
      this->CellScalarTexture = svtkTextureObject::New();
      this->CellScalarBuffer = svtkOpenGLBufferObject::New();
    }
    this->CellScalarTexture->SetContext(static_cast<svtkOpenGLRenderWindow*>(ren->GetSVTKWindow()));
    this->CellScalarBuffer->Upload(newColors, svtkOpenGLBufferObject::TextureBuffer);
    this->CellScalarTexture->CreateTextureBuffer(static_cast<unsigned int>(newColors.size() / 4), 4,
      SVTK_UNSIGNED_CHAR, this->CellScalarBuffer);
  }

  if (this->HaveCellNormals)
  {
    if (!this->CellNormalTexture)
    {
      this->CellNormalTexture = svtkTextureObject::New();
      this->CellNormalBuffer = svtkOpenGLBufferObject::New();
      this->CellNormalBuffer->SetType(svtkOpenGLBufferObject::TextureBuffer);
    }
    this->CellNormalTexture->SetContext(static_cast<svtkOpenGLRenderWindow*>(ren->GetSVTKWindow()));

    // do we have float texture support ?
    int ftex = static_cast<svtkOpenGLRenderWindow*>(ren->GetRenderWindow())
                 ->GetDefaultTextureInternalFormat(SVTK_FLOAT, 4, false, true, false);

    if (ftex)
    {
      this->CellNormalBuffer->Upload(newNorms, svtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(
        static_cast<unsigned int>(newNorms.size() / 4), 4, SVTK_FLOAT, this->CellNormalBuffer);
    }
    else
    {
      // have to convert to unsigned char if no float support
      std::vector<unsigned char> ucNewNorms;
      ucNewNorms.resize(newNorms.size());
      for (size_t i = 0; i < newNorms.size(); i++)
      {
        ucNewNorms[i] = 127.0 * (newNorms[i] + 1.0);
      }
      this->CellNormalBuffer->Upload(ucNewNorms, svtkOpenGLBufferObject::TextureBuffer);
      this->CellNormalTexture->CreateTextureBuffer(static_cast<unsigned int>(newNorms.size() / 4),
        4, SVTK_UNSIGNED_CHAR, this->CellNormalBuffer);
    }
  }

  this->VBOBuildTime.Modified();
}

//-------------------------------------------------------------------------
void svtkCompositeMapperHelper2::AppendOneBufferObject(svtkRenderer* ren, svtkActor* act,
  svtkCompositeMapperHelperData* hdata, svtkIdType& voffset, std::vector<unsigned char>& newColors,
  std::vector<float>& newNorms)
{
  svtkPolyData* poly = hdata->Data;

  // if there are no points then skip this piece
  if (!poly->GetPoints() || poly->GetPoints()->GetNumberOfPoints() == 0)
  {
    return;
  }

  // Get rid of old texture color coordinates if any
  if (this->ColorCoordinates)
  {
    this->ColorCoordinates->UnRegister(this);
    this->ColorCoordinates = nullptr;
  }
  // Get rid of old texture color coordinates if any
  if (this->Colors)
  {
    this->Colors->UnRegister(this);
    this->Colors = nullptr;
  }

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(poly, 1.0);

  // If we are coloring by texture, then load the texture map.
  if (this->ColorTextureMap)
  {
    if (this->InternalColorTexture == nullptr)
    {
      this->InternalColorTexture = svtkOpenGLTexture::New();
      this->InternalColorTexture->RepeatOff();
    }
    this->InternalColorTexture->SetInputData(this->ColorTextureMap);
  }

  this->HaveCellScalars = false;
  svtkDataArray* c = this->Colors;
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
      c = nullptr;
    }
  }

  this->HaveCellNormals = false;
  // Do we have cell normals?
  svtkDataArray* n = (act->GetProperty()->GetInterpolation() != SVTK_FLAT)
    ? poly->GetPointData()->GetNormals()
    : nullptr;
  if (n == nullptr && poly->GetCellData()->GetNormals())
  {
    this->HaveCellNormals = true;
    n = nullptr;
  }

  int representation = act->GetProperty()->GetRepresentation();
  svtkHardwareSelector* selector = ren->GetSelector();

  if (selector && this->PopulateSelectionSettings &&
    selector->GetFieldAssociation() == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    representation = SVTK_POINTS;
  }

  // if we have cell scalars then we have to
  // explode the data
  svtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();

  // needs to get a cell call map passed in
  this->AppendCellTextures(
    ren, act, prims, representation, newColors, newNorms, poly, hdata->CellCellMap);

  hdata->CellCellMap->BuildPrimitiveOffsetsIfNeeded(prims, representation, poly->GetPoints());

  // do we have texture maps?
  bool haveTextures =
    (this->ColorTextureMap || act->GetTexture() || act->GetProperty()->GetNumberOfTextures());

  // Set the texture if we are going to use texture
  // for coloring with a point attribute.
  // fixme ... make the existence of the coordinate array the signal.
  svtkDataArray* tcoords = nullptr;
  if (haveTextures)
  {
    if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates)
    {
      tcoords = this->ColorCoordinates;
    }
    else
    {
      tcoords = poly->GetPointData()->GetTCoords();
    }
  }

  // Check if color array is already computed for the current array.
  // This step is mandatory otherwise the test ArrayExists will fail for "scalarColor" even if
  // the array used to map the color has already been added.
  if (c)
  {
    int cellFlag = 0; // not used
    svtkAbstractArray* abstractArray = this->GetAbstractScalars(
      poly, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);

    auto iter = this->ColorArrayMap.find(abstractArray);
    if (iter != this->ColorArrayMap.end())
    {
      c = iter->second;
    }
    else
    {
      this->ColorArrayMap[abstractArray] = c;
      c->Register(this);
    }
  }

  svtkFloatArray* tangents = svtkFloatArray::SafeDownCast(poly->GetPointData()->GetTangents());

  // Build the VBO
  svtkIdType offsetPos = 0;
  svtkIdType offsetNorm = 0;
  svtkIdType offsetColor = 0;
  svtkIdType offsetTex = 0;
  svtkIdType offsetTangents = 0;
  svtkIdType totalOffset = 0;
  svtkIdType dummy = 0;
  bool exists =
    this->VBOs->ArrayExists("vertexMC", poly->GetPoints()->GetData(), offsetPos, totalOffset) &&
    this->VBOs->ArrayExists("normalMC", n, offsetNorm, dummy) &&
    this->VBOs->ArrayExists("scalarColor", c, offsetColor, dummy) &&
    this->VBOs->ArrayExists("tcoord", tcoords, offsetTex, dummy) &&
    this->VBOs->ArrayExists("tangentMC", tangents, offsetTangents, dummy);

  // if all used arrays have the same offset and have already been added,
  // we can reuse them and save memory
  if (exists && (offsetNorm == 0 || offsetPos == offsetNorm) &&
    (offsetColor == 0 || offsetPos == offsetColor) && (offsetTex == 0 || offsetPos == offsetTex) &&
    (offsetTangents == 0 || offsetPos == offsetTangents))
  {
    voffset = offsetPos;
  }
  else
  {
    this->VBOs->AppendDataArray("vertexMC", poly->GetPoints()->GetData(), SVTK_FLOAT);
    this->VBOs->AppendDataArray("normalMC", n, SVTK_FLOAT);
    this->VBOs->AppendDataArray("scalarColor", c, SVTK_UNSIGNED_CHAR);
    this->VBOs->AppendDataArray("tcoord", tcoords, SVTK_FLOAT);
    this->VBOs->AppendDataArray("tangentMC", tangents, SVTK_FLOAT);

    voffset = totalOffset;
  }

  // now create the IBOs
  svtkOpenGLIndexBufferObject::AppendPointIndexBuffer(this->IndexArray[0], prims[0], voffset);

  svtkDataArray* ef = poly->GetPointData()->GetAttribute(svtkDataSetAttributes::EDGEFLAG);

  if (representation == SVTK_POINTS)
  {
    svtkOpenGLIndexBufferObject::AppendPointIndexBuffer(this->IndexArray[1], prims[1], voffset);

    svtkOpenGLIndexBufferObject::AppendPointIndexBuffer(this->IndexArray[2], prims[2], voffset);

    svtkOpenGLIndexBufferObject::AppendPointIndexBuffer(this->IndexArray[3], prims[3], voffset);
  }
  else // WIREFRAME OR SURFACE
  {
    svtkOpenGLIndexBufferObject::AppendLineIndexBuffer(this->IndexArray[1], prims[1], voffset);

    if (representation == SVTK_WIREFRAME)
    {
      if (ef)
      {
        if (ef->GetNumberOfComponents() != 1)
        {
          svtkDebugMacro(<< "Currently only 1d edge flags are supported.");
          ef = nullptr;
        }
        if (ef && !ef->IsA("svtkUnsignedCharArray"))
        {
          svtkDebugMacro(<< "Currently only unsigned char edge flags are supported.");
          ef = nullptr;
        }
      }
      if (ef)
      {
        svtkOpenGLIndexBufferObject::AppendEdgeFlagIndexBuffer(
          this->IndexArray[2], prims[2], voffset, ef);
      }
      else
      {
        svtkOpenGLIndexBufferObject::AppendTriangleLineIndexBuffer(
          this->IndexArray[2], prims[2], voffset);
      }
      svtkOpenGLIndexBufferObject::AppendStripIndexBuffer(
        this->IndexArray[3], prims[3], voffset, true);
    }
    else // SURFACE
    {
      svtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer(
        this->IndexArray[2], prims[2], poly->GetPoints(), voffset);
      svtkOpenGLIndexBufferObject::AppendStripIndexBuffer(
        this->IndexArray[3], prims[3], voffset, false);
    }
  }

  // when drawing edges also build the edge IBOs
  svtkProperty* prop = act->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == SVTK_SURFACE);
  if (draw_surface_with_edges)
  {
    if (ef)
    {
      if (ef->GetNumberOfComponents() != 1)
      {
        svtkDebugMacro(<< "Currently only 1d edge flags are supported.");
        ef = nullptr;
      }
      if (!ef->IsA("svtkUnsignedCharArray"))
      {
        svtkDebugMacro(<< "Currently only unsigned char edge flags are supported.");
        ef = nullptr;
      }
    }
    if (ef)
    {
      svtkOpenGLIndexBufferObject::AppendEdgeFlagIndexBuffer(
        this->IndexArray[4], prims[2], voffset, ef);
    }
    else
    {
      svtkOpenGLIndexBufferObject::AppendTriangleLineIndexBuffer(
        this->IndexArray[4], prims[2], voffset);
    }
    svtkOpenGLIndexBufferObject::AppendStripIndexBuffer(
      this->IndexArray[5], prims[3], voffset, false);
  }

  if (prop->GetVertexVisibility())
  {
    svtkOpenGLIndexBufferObject::AppendVertexIndexBuffer(
      this->IndexArray[PrimitiveVertices], prims, voffset);
  }
}

void svtkCompositeMapperHelper2::ProcessSelectorPixelBuffers(
  svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, svtkProp* prop)
{
  if (!this->PopulateSelectionSettings)
  {
    return;
  }

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

    size_t maxFlatIndex = 0;
    for (dataIter it = this->Data.begin(); it != this->Data.end(); ++it)
    {
      maxFlatIndex = (it->second->FlatIndex > maxFlatIndex) ? it->second->FlatIndex : maxFlatIndex;
    }

    this->PickPixels.resize(maxFlatIndex + 1);

    for (auto pos : pixeloffsets)
    {
      unsigned int compval = compositedata[pos + 2];
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
  for (dataIter it = this->Data.begin(); it != this->Data.end(); ++it)
  {
    if (!this->PickPixels[it->second->FlatIndex].empty())
    {
      this->ProcessCompositePixelBuffers(
        sel, prop, it->second, this->PickPixels[it->second->FlatIndex]);
    }
  }
}

void svtkCompositeMapperHelper2::ProcessCompositePixelBuffers(svtkHardwareSelector* sel,
  svtkProp* prop, svtkCompositeMapperHelperData* hdata, std::vector<unsigned int>& pixeloffsets)
{
  svtkPolyData* poly = hdata->Data;

  if (!poly)
  {
    return;
  }

  // which pass are we processing ?
  int currPass = sel->GetCurrentPass();

  // get some common useful values
  bool pointPicking = sel->GetFieldAssociation() == svtkDataObject::FIELD_ASSOCIATION_POINTS;
  svtkPointData* pd = poly->GetPointData();
  svtkCellData* cd = poly->GetCellData();

  // get some values
  unsigned char* rawplowdata = sel->GetRawPixelBuffer(svtkHardwareSelector::POINT_ID_LOW24);
  unsigned char* rawphighdata = sel->GetRawPixelBuffer(svtkHardwareSelector::POINT_ID_HIGH24);

  // do we need to do anything to the process pass data?
  if (currPass == svtkHardwareSelector::PROCESS_PASS)
  {
    unsigned char* processdata = sel->GetPixelBuffer(svtkHardwareSelector::PROCESS_PASS);
    svtkUnsignedIntArray* processArray = nullptr;

    if (sel->GetUseProcessIdFromData())
    {
      processArray = this->ProcessIdArrayName
        ? svtkArrayDownCast<svtkUnsignedIntArray>(pd->GetArray(this->ProcessIdArrayName))
        : nullptr;
    }

    if (processArray && processdata && rawplowdata)
    {
      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawphighdata)
        {
          inval = rawphighdata[pos];
          inval = inval << 8;
        }
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        inval -= 1;
        inval -= hdata->StartVertex;
        unsigned int outval = processArray->GetValue(inval) + 1;
        processdata[pos] = outval & 0xff;
        processdata[pos + 1] = (outval & 0xff00) >> 8;
        processdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  // do we need to do anything to the point id data?
  if (currPass == svtkHardwareSelector::POINT_ID_LOW24)
  {
    svtkIdTypeArray* pointArrayId = this->PointIdArrayName
      ? svtkArrayDownCast<svtkIdTypeArray>(pd->GetArray(this->PointIdArrayName))
      : nullptr;

    // do we need to do anything to the point id data?
    if (rawplowdata)
    {
      unsigned char* plowdata = sel->GetPixelBuffer(svtkHardwareSelector::POINT_ID_LOW24);

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawphighdata)
        {
          inval = rawphighdata[pos];
          inval = inval << 8;
        }
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        inval -= 1;
        inval -= hdata->StartVertex;
        svtkIdType outval = inval + 1;
        if (pointArrayId)
        {
          outval = pointArrayId->GetValue(inval) + 1;
        }
        plowdata[pos] = outval & 0xff;
        plowdata[pos + 1] = (outval & 0xff00) >> 8;
        plowdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == svtkHardwareSelector::POINT_ID_HIGH24)
  {
    svtkIdTypeArray* pointArrayId = this->PointIdArrayName
      ? svtkArrayDownCast<svtkIdTypeArray>(pd->GetArray(this->PointIdArrayName))
      : nullptr;

    // do we need to do anything to the point id data?
    if (rawphighdata)
    {
      unsigned char* phighdata = sel->GetPixelBuffer(svtkHardwareSelector::POINT_ID_HIGH24);

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        inval = rawphighdata[pos];
        inval = inval << 8;
        inval |= rawplowdata[pos + 2];
        inval = inval << 8;
        inval |= rawplowdata[pos + 1];
        inval = inval << 8;
        inval |= rawplowdata[pos];
        inval -= 1;
        inval -= hdata->StartVertex;
        svtkIdType outval = inval + 1;
        if (pointArrayId)
        {
          outval = pointArrayId->GetValue(inval) + 1;
        }
        phighdata[pos] = (outval & 0xff000000) >> 24;
        phighdata[pos + 1] = (outval & 0xff00000000) >> 32;
        phighdata[pos + 2] = (outval & 0xff0000000000) >> 40;
      }
    }
  }

  // vars for cell based indexing
  svtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();

  int representation = static_cast<svtkActor*>(prop)->GetProperty()->GetRepresentation();

  unsigned char* rawclowdata = sel->GetRawPixelBuffer(svtkHardwareSelector::CELL_ID_LOW24);
  unsigned char* rawchighdata = sel->GetRawPixelBuffer(svtkHardwareSelector::CELL_ID_HIGH24);

  // do we need to do anything to the composite pass data?
  if (currPass == svtkHardwareSelector::COMPOSITE_INDEX_PASS)
  {
    unsigned char* compositedata = sel->GetPixelBuffer(svtkHardwareSelector::COMPOSITE_INDEX_PASS);

    svtkUnsignedIntArray* compositeArray = this->CompositeIdArrayName
      ? svtkArrayDownCast<svtkUnsignedIntArray>(cd->GetArray(this->CompositeIdArrayName))
      : nullptr;

    if (compositedata && compositeArray && rawclowdata)
    {
      hdata->CellCellMap->Update(prims, representation, poly->GetPoints());

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawchighdata)
        {
          inval = rawchighdata[pos];
          inval = inval << 8;
        }
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        inval -= 1;
        svtkIdType svtkCellId =
          hdata->CellCellMap->ConvertOpenGLCellIdToSVTKCellId(pointPicking, inval);
        unsigned int outval = compositeArray->GetValue(svtkCellId) + 1;
        compositedata[pos] = outval & 0xff;
        compositedata[pos + 1] = (outval & 0xff00) >> 8;
        compositedata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == svtkHardwareSelector::CELL_ID_LOW24)
  {
    svtkIdTypeArray* cellArrayId = this->CellIdArrayName
      ? svtkArrayDownCast<svtkIdTypeArray>(cd->GetArray(this->CellIdArrayName))
      : nullptr;
    unsigned char* clowdata = sel->GetPixelBuffer(svtkHardwareSelector::CELL_ID_LOW24);

    if (rawclowdata)
    {
      hdata->CellCellMap->Update(prims, representation, poly->GetPoints());

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        if (rawchighdata)
        {
          inval = rawchighdata[pos];
          inval = inval << 8;
        }
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        inval -= 1;
        svtkIdType outval = hdata->CellCellMap->ConvertOpenGLCellIdToSVTKCellId(pointPicking, inval);
        if (cellArrayId)
        {
          outval = cellArrayId->GetValue(outval);
        }
        outval++;
        clowdata[pos] = outval & 0xff;
        clowdata[pos + 1] = (outval & 0xff00) >> 8;
        clowdata[pos + 2] = (outval & 0xff0000) >> 16;
      }
    }
  }

  if (currPass == svtkHardwareSelector::CELL_ID_HIGH24)
  {
    svtkIdTypeArray* cellArrayId = this->CellIdArrayName
      ? svtkArrayDownCast<svtkIdTypeArray>(cd->GetArray(this->CellIdArrayName))
      : nullptr;
    unsigned char* chighdata = sel->GetPixelBuffer(svtkHardwareSelector::CELL_ID_HIGH24);

    if (rawchighdata)
    {
      hdata->CellCellMap->Update(prims, representation, poly->GetPoints());

      for (auto pos : pixeloffsets)
      {
        unsigned int inval = 0;
        inval = rawchighdata[pos];
        inval = inval << 8;
        inval |= rawclowdata[pos + 2];
        inval = inval << 8;
        inval |= rawclowdata[pos + 1];
        inval = inval << 8;
        inval |= rawclowdata[pos];
        inval -= 1;
        svtkIdType outval = hdata->CellCellMap->ConvertOpenGLCellIdToSVTKCellId(pointPicking, inval);
        if (cellArrayId)
        {
          outval = cellArrayId->GetValue(outval);
        }
        outval++;
        chighdata[pos] = (outval & 0xff000000) >> 24;
        chighdata[pos + 1] = (outval & 0xff00000000) >> 32;
        chighdata[pos + 2] = (outval & 0xff0000000000) >> 40;
      }
    }
  }
}

//===================================================================
// Now the main class methods

svtkStandardNewMacro(svtkCompositePolyDataMapper2);
//----------------------------------------------------------------------------
svtkCompositePolyDataMapper2::svtkCompositePolyDataMapper2()
{
  this->CurrentFlatIndex = 0;
  this->ColorMissingArraysWithNanColor = false;
}

//----------------------------------------------------------------------------
svtkCompositePolyDataMapper2::~svtkCompositePolyDataMapper2()
{
  helpIter miter = this->Helpers.begin();
  for (; miter != this->Helpers.end(); ++miter)
  {
    miter->second->Delete();
  }
  this->Helpers.clear();
}

//----------------------------------------------------------------------------
int svtkCompositePolyDataMapper2::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkCompositePolyDataMapper2::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
// Looks at each DataSet and finds the union of all the bounds
void svtkCompositePolyDataMapper2::ComputeBounds()
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

  if (input->GetMTime() < this->BoundsMTime && this->GetMTime() < this->BoundsMTime)
  {
    return;
  }

  // computing bounds with only visible blocks
  svtkCompositeDataDisplayAttributes::ComputeVisibleBounds(
    this->CompositeAttributes, input, this->Bounds);
  this->BoundsMTime.Modified();
}

//-----------------------------------------------------------------------------
// simple tests, the mapper is tolerant of being
// called both on opaque and translucent
bool svtkCompositePolyDataMapper2::HasOpaqueGeometry()
{
  return true;
}

bool svtkCompositePolyDataMapper2::RecursiveHasTranslucentGeometry(
  svtkDataObject* dobj, unsigned int& flat_index)
{
  svtkCompositeDataDisplayAttributes* cda = this->GetCompositeDataDisplayAttributes();
  bool overrides_visibility = (cda && cda->HasBlockVisibility(dobj));
  if (overrides_visibility)
  {
    if (!cda->GetBlockVisibility(dobj))
    {
      return false;
    }
  }
  bool overrides_opacity = (cda && cda->HasBlockOpacity(dobj));
  if (overrides_opacity)
  {
    if (cda->GetBlockOpacity(dobj) < 1.0)
    {
      return true;
    }
  }

  // Advance flat-index. After this point, flat_index no longer points to this
  // block.
  flat_index++;

  auto dObjTree = svtkDataObjectTree::SafeDownCast(dobj);
  if (dObjTree)
  {
    using Opts = svtk::DataObjectTreeOptions;
    for (svtkDataObject* child : svtk::Range(dObjTree, Opts::None))
    {
      if (!child)
      {
        ++flat_index;
      }
      else
      {
        if (this->RecursiveHasTranslucentGeometry(child, flat_index))
        {
          return true;
        }
      }
    }
    return false;
  }

  svtkPolyData* pd = svtkPolyData::SafeDownCast(dobj);
  // if we think it is opaque check the scalars
  if (this->ScalarVisibility)
  {
    svtkScalarsToColors* lut = this->GetLookupTable();
    int cellFlag;
    svtkDataArray* scalars = this->GetScalars(
      pd, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);
    if (lut->IsOpaque(scalars, this->ColorMode, this->ArrayComponent) == 0)
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
// simple tests, the mapper is tolerant of being
// called both on opaque and translucent
bool svtkCompositePolyDataMapper2::HasTranslucentPolygonalGeometry()
{
  // Make sure that we have been properly initialized.
  if (this->GetInputAlgorithm() == nullptr)
  {
    return false;
  }

  if (!this->Static)
  {
    this->InvokeEvent(svtkCommand::StartEvent, nullptr);
    this->GetInputAlgorithm()->Update();
    this->InvokeEvent(svtkCommand::EndEvent, nullptr);
  }

  if (this->GetInputDataObject(0, 0) == nullptr)
  {
    return false;
  }

  // rebuild the render values if needed
  svtkCompositeDataDisplayAttributes* cda = this->GetCompositeDataDisplayAttributes();
  svtkScalarsToColors* lut = this->ScalarVisibility ? this->GetLookupTable() : nullptr;

  this->TempState.Clear();
  this->TempState.Append(cda ? cda->GetMTime() : 0, "cda mtime");
  this->TempState.Append(lut ? lut->GetMTime() : 0, "lut mtime");
  this->TempState.Append(this->GetInputDataObject(0, 0)->GetMTime(), "input mtime");
  if (this->TranslucentState != this->TempState)
  {
    this->TranslucentState = this->TempState;
    if (lut)
    {
      // Ensure that the lookup table is built
      lut->Build();
    }

    // Push base-values on the state stack.
    unsigned int flat_index = 0;
    this->HasTranslucentGeometry =
      this->RecursiveHasTranslucentGeometry(this->GetInputDataObject(0, 0), flat_index);
  }

  return this->HasTranslucentGeometry;
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::SetBlockVisibility(unsigned int index, bool visible)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->SetBlockVisibility(dataObj, visible);
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
bool svtkCompositePolyDataMapper2::GetBlockVisibility(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      return this->CompositeAttributes->GetBlockVisibility(dataObj);
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::RemoveBlockVisibility(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->RemoveBlockVisibility(dataObj);
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::RemoveBlockVisibilities()
{
  if (this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockVisibilities();
    this->Modified();
  }
}

#ifndef SVTK_LEGACY_REMOVE
void svtkCompositePolyDataMapper2::RemoveBlockVisibilites()
{
  this->RemoveBlockVisibilities();
}
#endif

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::SetBlockColor(unsigned int index, double color[3])
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);

    if (dataObj)
    {
      this->CompositeAttributes->SetBlockColor(dataObj, color);
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
double* svtkCompositePolyDataMapper2::GetBlockColor(unsigned int index)
{
  static double white[3] = { 1.0, 1.0, 1.0 };

  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->GetBlockColor(dataObj, this->ColorResult);
    }

    return this->ColorResult;
  }
  else
  {
    return white;
  }
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::RemoveBlockColor(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->RemoveBlockColor(dataObj);
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::RemoveBlockColors()
{
  if (this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockColors();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::SetBlockOpacity(unsigned int index, double opacity)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->SetBlockOpacity(dataObj, opacity);
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
double svtkCompositePolyDataMapper2::GetBlockOpacity(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      return this->CompositeAttributes->GetBlockOpacity(dataObj);
    }
  }
  return 1.;
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::RemoveBlockOpacity(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->RemoveBlockOpacity(dataObj);
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::RemoveBlockOpacities()
{
  if (this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockOpacities();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::SetCompositeDataDisplayAttributes(
  svtkCompositeDataDisplayAttributes* attributes)
{
  if (this->CompositeAttributes != attributes)
  {
    this->CompositeAttributes = attributes;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
svtkCompositeDataDisplayAttributes* svtkCompositePolyDataMapper2::GetCompositeDataDisplayAttributes()
{
  return this->CompositeAttributes;
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkCompositePolyDataMapper2::CopyMapperValuesToHelper(svtkCompositeMapperHelper2* helper)
{
  // We avoid PolyDataMapper::ShallowCopy because it copies the input
  helper->svtkMapper::ShallowCopy(this);
  helper->SetPointIdArrayName(this->GetPointIdArrayName());
  helper->SetCompositeIdArrayName(this->GetCompositeIdArrayName());
  helper->SetProcessIdArrayName(this->GetProcessIdArrayName());
  helper->SetCellIdArrayName(this->GetCellIdArrayName());
  helper->SetSeamlessU(this->SeamlessU);
  helper->SetSeamlessV(this->SeamlessV);
  helper->SetStatic(1);
}

//-----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::ReleaseGraphicsResources(svtkWindow* win)
{
  helpIter miter = this->Helpers.begin();
  for (; miter != this->Helpers.end(); ++miter)
  {
    miter->second->ReleaseGraphicsResources(win);
  }
  miter = this->Helpers.begin();
  for (; miter != this->Helpers.end(); ++miter)
  {
    miter->second->Delete();
  }
  this->Helpers.clear();
  this->Modified();
  this->Superclass::ReleaseGraphicsResources(win);
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.
void svtkCompositePolyDataMapper2::Render(svtkRenderer* ren, svtkActor* actor)
{
  this->RenderedList.clear();

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

  // the first step is to gather up the polydata based on their
  // signatures (aka have normals, have scalars etc)
  if (this->HelperMTime < this->GetInputDataObject(0, 0)->GetMTime() ||
    this->HelperMTime < this->GetMTime())
  {
    // clear old helpers
    for (helpIter hiter = this->Helpers.begin(); hiter != this->Helpers.end(); ++hiter)
    {
      hiter->second->ClearMark();
    }
    this->HelperDataMap.clear();

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
        int cellFlag = 0;
        bool hasScalars = this->ScalarVisibility &&
          (svtkAbstractMapper::GetAbstractScalars(pd, this->ScalarMode, this->ArrayAccessMode,
             this->ArrayId, this->ArrayName, cellFlag) != nullptr);

        bool hasNormals = (pd->GetPointData()->GetNormals() || pd->GetCellData()->GetNormals());

        bool hasTCoords = (pd->GetPointData()->GetTCoords() != nullptr);

        std::ostringstream toString;
        toString.str("");
        toString.clear();
        toString << 'A' << (hasScalars ? 1 : 0) << 'B' << (hasNormals ? 1 : 0) << 'C'
                 << (hasTCoords ? 1 : 0);

        svtkCompositeMapperHelper2* helper = nullptr;
        helpIter found = this->Helpers.find(toString.str());
        if (found == this->Helpers.end())
        {
          helper = this->CreateHelper();
          helper->SetParent(this);
          this->Helpers.insert(std::make_pair(toString.str(), helper));
        }
        else
        {
          helper = found->second;
        }
        this->CopyMapperValuesToHelper(helper);
        helper->SetMarked(true);
        this->HelperDataMap[pd] = helper->AddData(pd, flatIndex);
      }
    }
    else
    {
      svtkPolyData* pd = svtkPolyData::SafeDownCast(this->GetInputDataObject(0, 0));
      if (pd && pd->GetPoints())
      {
        int cellFlag = 0;
        bool hasScalars = this->ScalarVisibility &&
          (svtkAbstractMapper::GetAbstractScalars(pd, this->ScalarMode, this->ArrayAccessMode,
             this->ArrayId, this->ArrayName, cellFlag) != nullptr);

        bool hasNormals = (pd->GetPointData()->GetNormals() || pd->GetCellData()->GetNormals());

        bool hasTCoords = (pd->GetPointData()->GetTCoords() != nullptr);

        std::ostringstream toString;
        toString.str("");
        toString.clear();
        toString << 'A' << (hasScalars ? 1 : 0) << 'B' << (hasNormals ? 1 : 0) << 'C'
                 << (hasTCoords ? 1 : 0);

        svtkCompositeMapperHelper2* helper = nullptr;
        helpIter found = this->Helpers.find(toString.str());
        if (found == this->Helpers.end())
        {
          helper = this->CreateHelper();
          helper->SetParent(this);
          this->Helpers.insert(std::make_pair(toString.str(), helper));
        }
        else
        {
          helper = found->second;
        }
        this->CopyMapperValuesToHelper(helper);
        helper->SetMarked(true);
        this->HelperDataMap[pd] = helper->AddData(pd, 0);
      }
    }

    // delete unused old helpers/data
    for (helpIter hiter = this->Helpers.begin(); hiter != this->Helpers.end();)
    {
      hiter->second->RemoveUnused();
      if (!hiter->second->GetMarked())
      {
        hiter->second->ReleaseGraphicsResources(ren->GetSVTKWindow());
        hiter->second->Delete();
        this->Helpers.erase(hiter++);
      }
      else
      {
        ++hiter;
      }
    }
    this->HelperMTime.Modified();
  }

  // rebuild the render values if needed
  this->TempState.Clear();
  this->TempState.Append(actor->GetProperty()->GetMTime(), "actor mtime");
  this->TempState.Append(this->GetMTime(), "this mtime");
  this->TempState.Append(this->HelperMTime, "helper mtime");
  this->TempState.Append(
    actor->GetTexture() ? actor->GetTexture()->GetMTime() : 0, "texture mtime");
  if (this->RenderValuesState != this->TempState)
  {
    this->RenderValuesState = this->TempState;
    svtkProperty* prop = actor->GetProperty();
    svtkScalarsToColors* lut = this->GetLookupTable();
    if (lut)
    {
      // Ensure that the lookup table is built
      lut->Build();
    }

    // Push base-values on the state stack.
    this->BlockState.Visibility.push(true);
    this->BlockState.Pickability.push(true);
    this->BlockState.Opacity.push(prop->GetOpacity());
    this->BlockState.AmbientColor.push(svtkColor3d(prop->GetAmbientColor()));
    this->BlockState.DiffuseColor.push(svtkColor3d(prop->GetDiffuseColor()));
    this->BlockState.SpecularColor.push(svtkColor3d(prop->GetSpecularColor()));

    unsigned int flat_index = 0;
    this->BuildRenderValues(ren, actor, this->GetInputDataObject(0, 0), flat_index);

    this->BlockState.Visibility.pop();
    this->BlockState.Pickability.pop();
    this->BlockState.Opacity.pop();
    this->BlockState.AmbientColor.pop();
    this->BlockState.DiffuseColor.pop();
    this->BlockState.SpecularColor.pop();
  }

  this->InitializeHelpersBeforeRendering(ren, actor);

  for (helpIter hiter = this->Helpers.begin(); hiter != this->Helpers.end(); ++hiter)
  {
    svtkCompositeMapperHelper2* helper = hiter->second;
    helper->RenderPiece(ren, actor);

    // update the list of rendered polydata that svtkValuePass relies on
    std::vector<svtkPolyData*> pdl = helper->GetRenderedList();
    for (unsigned int i = 0; i < pdl.size(); ++i)
    {
      this->RenderedList.push_back(pdl[i]);
    }
  }
}

svtkCompositeMapperHelper2* svtkCompositePolyDataMapper2::CreateHelper()
{
  return svtkCompositeMapperHelper2::New();
}

//-----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::BuildRenderValues(
  svtkRenderer* renderer, svtkActor* actor, svtkDataObject* dobj, unsigned int& flat_index)
{
  svtkCompositeDataDisplayAttributes* cda = this->GetCompositeDataDisplayAttributes();
  bool overrides_visibility = (cda && cda->HasBlockVisibility(dobj));
  if (overrides_visibility)
  {
    this->BlockState.Visibility.push(cda->GetBlockVisibility(dobj));
  }
  bool overrides_pickability = (cda && cda->HasBlockPickability(dobj));
  if (overrides_pickability)
  {
    this->BlockState.Pickability.push(cda->GetBlockPickability(dobj));
  }

  bool overrides_opacity = (cda && cda->HasBlockOpacity(dobj));
  if (overrides_opacity)
  {
    this->BlockState.Opacity.push(cda->GetBlockOpacity(dobj));
  }

  bool overrides_color = (cda && cda->HasBlockColor(dobj));
  if (overrides_color)
  {
    svtkColor3d color = cda->GetBlockColor(dobj);
    this->BlockState.AmbientColor.push(color);
    this->BlockState.DiffuseColor.push(color);
    this->BlockState.SpecularColor.push(color);
  }

  // Advance flat-index. After this point, flat_index no longer points to this
  // block.
  flat_index++;

  bool textureOpaque = true;
  if (actor->GetTexture() != nullptr && actor->GetTexture()->IsTranslucent())
  {
    textureOpaque = false;
  }

  auto dObjTree = svtkDataObjectTree::SafeDownCast(dobj);
  if (dObjTree)
  {
    using Opts = svtk::DataObjectTreeOptions;
    for (svtkDataObject* child : svtk::Range(dObjTree, Opts::None))
    {
      if (!child)
      {
        ++flat_index;
      }
      else
      {
        this->BuildRenderValues(renderer, actor, child, flat_index);
      }
    }
  }
  else
  {
    svtkPolyData* pd = svtkPolyData::SafeDownCast(dobj);
    dataIter dit = this->HelperDataMap.find(pd);
    if (dit != this->HelperDataMap.end())
    {
      svtkCompositeMapperHelperData* helperData = dit->second;
      helperData->Opacity = this->BlockState.Opacity.top();
      helperData->Visibility = this->BlockState.Visibility.top();
      helperData->Pickability = this->BlockState.Pickability.top();
      helperData->AmbientColor = this->BlockState.AmbientColor.top();
      helperData->DiffuseColor = this->BlockState.DiffuseColor.top();
      helperData->OverridesColor = (this->BlockState.AmbientColor.size() > 1);
      helperData->IsOpaque = (helperData->Opacity >= 1.0) ? textureOpaque : false;
      // if we think it is opaque check the scalars
      if (helperData->IsOpaque && this->ScalarVisibility)
      {
        svtkScalarsToColors* lut = this->GetLookupTable();
        int cellFlag;
        svtkDataArray* scalars = this->GetScalars(
          pd, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);
        if (lut->IsOpaque(scalars, this->ColorMode, this->ArrayComponent) == 0)
        {
          helperData->IsOpaque = false;
        }
      }
    }
  }
  if (overrides_color)
  {
    this->BlockState.AmbientColor.pop();
    this->BlockState.DiffuseColor.pop();
    this->BlockState.SpecularColor.pop();
  }
  if (overrides_opacity)
  {
    this->BlockState.Opacity.pop();
  }
  if (overrides_pickability)
  {
    this->BlockState.Pickability.pop();
  }
  if (overrides_visibility)
  {
    this->BlockState.Visibility.pop();
  }
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::SetInputArrayToProcess(int idx, svtkInformation* inInfo)
{
  this->Superclass::SetInputArrayToProcess(idx, inInfo);

  // set inputs to helpers
  for (auto& helper : this->Helpers)
  {
    helper.second->SetInputArrayToProcess(idx, inInfo);
  }
}

//----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, int attributeType)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, attributeType);

  // set inputs to helpers
  for (auto& helper : this->Helpers)
  {
    helper.second->SetInputArrayToProcess(idx, port, connection, fieldAssociation, attributeType);
  }
}

//-----------------------------------------------------------------------------
void svtkCompositePolyDataMapper2::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);

  // set inputs to helpers
  for (auto& helper : this->Helpers)
  {
    helper.second->SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);
  }
}

void svtkCompositePolyDataMapper2::ProcessSelectorPixelBuffers(
  svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, svtkProp* prop)
{
  // forward to helper
  for (auto& helper : this->Helpers)
  {
    helper.second->ProcessSelectorPixelBuffers(sel, pixeloffsets, prop);
  }
}
