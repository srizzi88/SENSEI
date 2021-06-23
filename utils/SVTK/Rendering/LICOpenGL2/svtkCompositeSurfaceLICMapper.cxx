/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeSurfaceLICMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeSurfaceLICMapper.h"

#include "svtk_glew.h"

#include "svtkCellData.h"
#include "svtkColorTransferFunction.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkFloatArray.h"
#include "svtkHardwareSelector.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLIndexBufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLTexture.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkOpenGLVertexBufferObject.h"
#include "svtkOpenGLVertexBufferObjectGroup.h"
#include "svtkPainterCommunicator.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkScalarsToColors.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"

#include <algorithm>
#include <sstream>

#include "svtkSurfaceLICInterface.h"

#include "svtkCompositePolyDataMapper2Internal.h"

typedef std::map<svtkPolyData*, svtkCompositeMapperHelperData*>::iterator dataIter;

class svtkCompositeLICHelper : public svtkCompositeMapperHelper2
{
public:
  static svtkCompositeLICHelper* New();
  svtkTypeMacro(svtkCompositeLICHelper, svtkCompositeMapperHelper2);

protected:
  svtkCompositeLICHelper();
  ~svtkCompositeLICHelper() override;

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  void AppendOneBufferObject(svtkRenderer* ren, svtkActor* act, svtkCompositeMapperHelperData* hdata,
    svtkIdType& flat_index, std::vector<unsigned char>& colors, std::vector<float>& norms) override;

protected:
  /**
   * Set the shader parameteres related to the mapper/input data, called by UpdateShader
   */
  void SetMapperShaderParameters(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act) override;

  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderValues(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;

private:
  svtkCompositeLICHelper(const svtkCompositeLICHelper&) = delete;
  void operator=(const svtkCompositeLICHelper&) = delete;
};

//----------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkCompositeLICHelper);

//----------------------------------------------------------------------------
svtkCompositeLICHelper::svtkCompositeLICHelper()
{
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, svtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
svtkCompositeLICHelper::~svtkCompositeLICHelper() {}

void svtkCompositeLICHelper::ReplaceShaderValues(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  std::string VSSource = shaders[svtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[svtkShader::Fragment]->GetSource();

  // add some code to handle the LIC vectors and mask
  svtkShaderProgram::Substitute(VSSource, "//SVTK::TCoord::Dec",
    "in vec3 vecsMC;\n"
    "out vec3 tcoordVCVSOutput;\n");

  svtkShaderProgram::Substitute(VSSource, "//SVTK::TCoord::Impl", "tcoordVCVSOutput = vecsMC;");

  svtkShaderProgram::Substitute(FSSource, "//SVTK::TCoord::Dec",
    // 0/1, when 1 V is projected to surface for |V| computation.
    "uniform int uMaskOnSurface;\n"
    "uniform mat3 normalMatrix;\n"
    "in vec3 tcoordVCVSOutput;");

  svtkShaderProgram::Substitute(FSSource, "//SVTK::TCoord::Impl",
    // projected vectors
    "  vec3 tcoordLIC = normalMatrix * tcoordVCVSOutput;\n"
    "  vec3 normN = normalize(normalVCVSOutput);\n"
    "  float k = dot(tcoordLIC, normN);\n"
    "  tcoordLIC = (tcoordLIC - k*normN);\n"
    "  gl_FragData[1] = vec4(tcoordLIC.x, tcoordLIC.y, 0.0 , gl_FragCoord.z);\n"
    //   "  gl_FragData[1] = vec4(tcoordVC.xyz, gl_FragCoord.z);\n"
    // vectors for fragment masking
    "  if (uMaskOnSurface == 0)\n"
    "    {\n"
    "    gl_FragData[2] = vec4(tcoordVCVSOutput, gl_FragCoord.z);\n"
    "    }\n"
    "  else\n"
    "    {\n"
    "    gl_FragData[2] = vec4(tcoordLIC.x, tcoordLIC.y, 0.0 , gl_FragCoord.z);\n"
    "    }\n"
    //   "  gl_FragData[2] = vec4(19.0, 19.0, tcoordVC.x, gl_FragCoord.z);\n"
    ,
    false);

  shaders[svtkShader::Vertex]->SetSource(VSSource);
  shaders[svtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderValues(shaders, ren, actor);
}

void svtkCompositeLICHelper::SetMapperShaderParameters(
  svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* actor)
{
  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
  cellBO.Program->SetUniformi("uMaskOnSurface",
    static_cast<svtkCompositeSurfaceLICMapper*>(this->Parent)
      ->GetLICInterface()
      ->GetMaskOnSurface());
}

//-------------------------------------------------------------------------
void svtkCompositeLICHelper::AppendOneBufferObject(svtkRenderer* ren, svtkActor* act,
  svtkCompositeMapperHelperData* hdata, svtkIdType& voffset, std::vector<unsigned char>& newColors,
  std::vector<float>& newNorms)
{
  svtkPolyData* poly = hdata->Data;
  svtkDataArray* vectors = this->GetInputArrayToProcess(0, poly);
  if (vectors)
  {
    this->VBOs->AppendDataArray("vecsMC", vectors, SVTK_FLOAT);
  }

  this->Superclass::AppendOneBufferObject(ren, act, hdata, voffset, newColors, newNorms);
}

// #include <algorithm>

//===================================================================
// Now the main class methods

svtkStandardNewMacro(svtkCompositeSurfaceLICMapper);
//----------------------------------------------------------------------------
svtkCompositeSurfaceLICMapper::svtkCompositeSurfaceLICMapper() {}

//----------------------------------------------------------------------------
svtkCompositeSurfaceLICMapper::~svtkCompositeSurfaceLICMapper() {}

svtkCompositeMapperHelper2* svtkCompositeSurfaceLICMapper::CreateHelper()
{
  return svtkCompositeLICHelper::New();
}

//----------------------------------------------------------------------------
void svtkCompositeSurfaceLICMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkCompositeSurfaceLICMapper::CopyMapperValuesToHelper(svtkCompositeMapperHelper2* helper)
{
  this->Superclass::CopyMapperValuesToHelper(helper);
  // static_cast<svtkCompositeLICHelper *>(helper)->SetLICInterface(this->LICInterface);
  helper->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.

void svtkCompositeSurfaceLICMapper::Render(svtkRenderer* ren, svtkActor* actor)
{
  this->LICInterface->ValidateContext(ren);

  this->LICInterface->UpdateCommunicator(ren, actor, this->GetInputDataObject(0, 0));

  svtkPainterCommunicator* comm = this->LICInterface->GetCommunicator();

  if (comm->GetIsNull())
  {
    // other rank's may have some visible data but we
    // have none and should not participate further
    return;
  }

  // do we have vectors? Need a leaf node to know
  svtkCompositeDataSet* input = svtkCompositeDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
  bool haveVectors = true;
  if (input)
  {
    svtkSmartPointer<svtkDataObjectTreeIterator> iter =
      svtkSmartPointer<svtkDataObjectTreeIterator>::New();
    iter->SetDataSet(input);
    iter->SkipEmptyNodesOn();
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataObject* dso = iter->GetCurrentDataObject();
      svtkPolyData* pd = svtkPolyData::SafeDownCast(dso);
      if (pd && pd->GetPoints())
      {
        haveVectors = haveVectors && (this->GetInputArrayToProcess(0, pd) != nullptr);
      }
    }
  }
  else
  {
    svtkPolyData* pd = svtkPolyData::SafeDownCast(this->GetInputDataObject(0, 0));
    if (pd && pd->GetPoints())
    {
      haveVectors = (this->GetInputArrayToProcess(0, pd) != nullptr);
    }
  }

  this->LICInterface->SetHasVectors(haveVectors);

  if (!this->LICInterface->CanRenderSurfaceLIC(actor))
  {
    // we've determined that there's no work for us, or that the
    // requisite opengl extensions are not available. pass control on
    // to delegate renderer and return.
    this->Superclass::Render(ren, actor);
    return;
  }

  // Before start rendering LIC, capture some essential state so we can restore
  // it.
  svtkOpenGLRenderWindow* rw = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  svtkOpenGLState* ostate = rw->GetState();
  svtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);

  svtkNew<svtkOpenGLFramebufferObject> fbo;
  fbo->SetContext(svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
  ostate->PushFramebufferBindings();

  // allocate rendering resources, initialize or update
  // textures and shaders.
  this->LICInterface->InitializeResources();

  // draw the geometry
  this->LICInterface->PrepareForGeometry();

  this->Superclass::Render(ren, actor);

  this->LICInterface->CompletedGeometry();

  // --------------------------------------------- composite vectors for parallel LIC
  this->LICInterface->GatherVectors();

  // ------------------------------------------- LIC on screen
  this->LICInterface->ApplyLIC();

  // ------------------------------------------- combine scalar colors + LIC
  this->LICInterface->CombineColorsAndLIC();

  // ----------------------------------------------- depth test and copy to screen
  this->LICInterface->CopyToScreen();

  ostate->PopFramebufferBindings();
}
