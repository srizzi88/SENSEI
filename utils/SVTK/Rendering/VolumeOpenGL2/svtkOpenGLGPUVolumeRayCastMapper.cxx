/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLGPUVolumeRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLGPUVolumeRayCastMapper.h"

#include <svtk_glew.h>

#include "svtkVolumeShaderComposer.h"
#include "svtkVolumeStateRAII.h"

// Include compiled shader code
#include <raycasterfs.h>
#include <raycastervs.h>

// SVTK includes
#include "svtkInformation.h"
#include "svtkOpenGLActor.h"
#include "svtkOpenGLResourceFreeCallback.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLUniforms.h"
#include <svtkBoundingBox.h>
#include <svtkCamera.h>
#include <svtkCellArray.h>
#include <svtkCellData.h>
#include <svtkClipConvexPolyData.h>
#include <svtkColorTransferFunction.h>
#include <svtkCommand.h>
#include <svtkContourFilter.h>
#include <svtkDataArray.h>
#include <svtkDensifyPolyData.h>
#include <svtkFloatArray.h>
#include <svtkImageData.h>
#include <svtkLight.h>
#include <svtkLightCollection.h>
#include <svtkMath.h>
#include <svtkMatrix4x4.h>
#include <svtkMultiVolume.h>
#include <svtkNew.h>
#include <svtkObjectFactory.h>
#include <svtkOpenGLBufferObject.h>
#include <svtkOpenGLCamera.h>
#include <svtkOpenGLError.h>
#include <svtkOpenGLFramebufferObject.h>
#include <svtkOpenGLRenderPass.h>
#include <svtkOpenGLRenderUtilities.h>
#include <svtkOpenGLRenderWindow.h>
#include <svtkOpenGLShaderCache.h>
#include <svtkOpenGLShaderProperty.h>
#include <svtkOpenGLVertexArrayObject.h>
#include <svtkPixelBufferObject.h>
#include <svtkPixelExtent.h>
#include <svtkPixelTransfer.h>
#include <svtkPlaneCollection.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderer.h>
#include <svtkShader.h>
#include <svtkShaderProgram.h>
#include <svtkSmartPointer.h>
#include <svtkTessellatedBoxSource.h>
#include <svtkTextureObject.h>
#include <svtkTimerLog.h>
#include <svtkTransform.h>
#include <svtkUnsignedCharArray.h>
#include <svtkUnsignedIntArray.h>

#include <svtkVolumeInputHelper.h>

#include "svtkOpenGLVolumeGradientOpacityTable.h"
#include "svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.h"
#include "svtkOpenGLVolumeMaskTransferFunction2D.h"
#include "svtkOpenGLVolumeOpacityTable.h"
#include "svtkOpenGLVolumeRGBTable.h"
#include "svtkOpenGLVolumeTransferFunction2D.h"

#include <svtkHardwareSelector.h>
#include <svtkVolumeMask.h>
#include <svtkVolumeProperty.h>
#include <svtkVolumeTexture.h>
#include <svtkWeakPointer.h>

// C/C++ includes
#include <cassert>
#include <limits>
#include <map>
#include <sstream>
#include <string>

svtkStandardNewMacro(svtkOpenGLGPUVolumeRayCastMapper);

//----------------------------------------------------------------------------
class svtkOpenGLGPUVolumeRayCastMapper::svtkInternal
{
public:
  // Constructor
  //--------------------------------------------------------------------------
  svtkInternal(svtkOpenGLGPUVolumeRayCastMapper* parent)
  {
    this->Parent = parent;
    this->ValidTransferFunction = false;
    this->LoadDepthTextureExtensionsSucceeded = false;
    this->CameraWasInsideInLastUpdate = false;
    this->CubeVBOId = 0;
    this->CubeVAOId = 0;
    this->CubeIndicesId = 0;
    this->DepthTextureObject = nullptr;
    this->SharedDepthTextureObject = false;
    this->TextureWidth = 1024;
    this->ActualSampleDistance = 1.0;
    this->CurrentMask = nullptr;
    this->TextureSize[0] = this->TextureSize[1] = this->TextureSize[2] = -1;
    this->WindowLowerLeft[0] = this->WindowLowerLeft[1] = 0;
    this->WindowSize[0] = this->WindowSize[1] = 0;
    this->LastDepthPassWindowSize[0] = this->LastDepthPassWindowSize[1] = 0;
    this->LastRenderToImageWindowSize[0] = 0;
    this->LastRenderToImageWindowSize[1] = 0;
    this->CurrentSelectionPass = svtkHardwareSelector::MIN_KNOWN_PASS - 1;

    this->NumberOfLights = 0;
    this->LightComplexity = 0;

    this->NeedToInitializeResources = false;
    this->ShaderCache = nullptr;

    this->FBO = nullptr;
    this->RTTDepthBufferTextureObject = nullptr;
    this->RTTDepthTextureObject = nullptr;
    this->RTTColorTextureObject = nullptr;
    this->RTTDepthTextureType = -1;

    this->DPFBO = nullptr;
    this->DPDepthBufferTextureObject = nullptr;
    this->DPColorTextureObject = nullptr;
    this->PreserveViewport = false;
    this->PreserveGLState = false;

    this->Partitions[0] = this->Partitions[1] = this->Partitions[2] = 1;
  }

  // Destructor
  //--------------------------------------------------------------------------
  ~svtkInternal()
  {
    if (this->DepthTextureObject)
    {
      this->DepthTextureObject->Delete();
      this->DepthTextureObject = nullptr;
    }

    if (this->FBO)
    {
      this->FBO->Delete();
      this->FBO = nullptr;
    }

    if (this->RTTDepthBufferTextureObject)
    {
      this->RTTDepthBufferTextureObject->Delete();
      this->RTTDepthBufferTextureObject = nullptr;
    }

    if (this->RTTDepthTextureObject)
    {
      this->RTTDepthTextureObject->Delete();
      this->RTTDepthTextureObject = nullptr;
    }

    if (this->RTTColorTextureObject)
    {
      this->RTTColorTextureObject->Delete();
      this->RTTColorTextureObject = nullptr;
    }

    if (this->ImageSampleFBO)
    {
      this->ImageSampleFBO->Delete();
      this->ImageSampleFBO = nullptr;
    }

    for (auto& tex : this->ImageSampleTexture)
    {
      tex = nullptr;
    }
    this->ImageSampleTexture.clear();
    this->ImageSampleTexNames.clear();

    if (this->ImageSampleVAO)
    {
      this->ImageSampleVAO->Delete();
      this->ImageSampleVAO = nullptr;
    }
    this->DeleteMaskTransfer();

    // Do not delete the shader programs - Let the cache clean them up.
    this->ImageSampleProg = nullptr;
  }

  // Helper methods
  //--------------------------------------------------------------------------
  template <typename T>
  static void ToFloat(const T& in1, const T& in2, float (&out)[2]);
  template <typename T>
  static void ToFloat(const T& in1, const T& in2, const T& in3, float (&out)[3]);
  template <typename T>
  static void ToFloat(T* in, float* out, int noOfComponents);
  template <typename T>
  static void ToFloat(T (&in)[3], float (&out)[3]);
  template <typename T>
  static void ToFloat(T (&in)[2], float (&out)[2]);
  template <typename T>
  static void ToFloat(T& in, float& out);
  template <typename T>
  static void ToFloat(T (&in)[4][2], float (&out)[4][2]);
  template <typename T, int SizeX, int SizeY>
  static void CopyMatrixToVector(T* matrix, float* matrixVec, int offset);
  template <typename T, int SizeSrc>
  static void CopyVector(T* srcVec, T* dstVec, int offset);

  ///@{
  /**
   * \brief Setup and clean-up transfer functions for each svtkVolumeInputHelper
   * and masks.
   */
  void UpdateTransferFunctions(svtkRenderer* ren);

  void RefreshMaskTransfer(svtkRenderer* ren, VolumeInput& input);
  int UpdateMaskTransfer(svtkRenderer* ren, svtkVolume* vol, unsigned int component);
  void SetupMaskTransfer(svtkRenderer* ren);
  void ReleaseGraphicsMaskTransfer(svtkWindow* window);
  void DeleteMaskTransfer();
  ///@}

  bool LoadMask(svtkRenderer* ren);

  // Update the depth sampler with the current state of the z-buffer. The
  // sampler is used for z-buffer compositing with opaque geometry during
  // ray-casting (rays are early-terminated if hidden begin opaque geometry).
  void CaptureDepthTexture(svtkRenderer* ren);

  // Test if camera is inside the volume geometry
  bool IsCameraInside(svtkRenderer* ren, svtkVolume* vol, double geometry[24]);

  //@{
  /**
   * Update volume's proxy-geometry and draw it
   */
  bool IsGeometryUpdateRequired(svtkRenderer* ren, svtkVolume* vol, double geometry[24]);
  void RenderVolumeGeometry(
    svtkRenderer* ren, svtkShaderProgram* prog, svtkVolume* vol, double geometry[24]);
  //@}

  // Update cropping params to shader
  void SetCroppingRegions(svtkShaderProgram* prog, double loadedBounds[6]);

  // Update clipping params to shader
  void SetClippingPlanes(svtkRenderer* ren, svtkShaderProgram* prog, svtkVolume* vol);

  // Update the ray sampling distance. Sampling distance should be updated
  // before updating opacity transfer functions.
  void UpdateSamplingDistance(svtkRenderer* ren);

  // Check if the mapper should enter picking mode.
  void CheckPickingState(svtkRenderer* ren);

  // Look for property keys used to control the mapper's state.
  // This is necessary for some render passes which need to ensure
  // a specific OpenGL state when rendering through this mapper.
  void CheckPropertyKeys(svtkVolume* vol);

  // Configure the svtkHardwareSelector to begin a picking pass. This call
  // changes GL_BLEND, so it needs to be called before constructing
  // svtkVolumeStateRAII.
  void BeginPicking(svtkRenderer* ren);

  // Update the prop Id if hardware selection is enabled.
  void SetPickingId(svtkRenderer* ren);

  // Configure the svtkHardwareSelector to end a picking pass.
  void EndPicking(svtkRenderer* ren);

  // Load OpenGL extensiosn required to grab depth sampler buffer
  void LoadRequireDepthTextureExtensions(svtkRenderWindow* renWin);

  // Create GL buffers
  void CreateBufferObjects();

  // Dispose / free GL buffers
  void DeleteBufferObjects();

  // Convert svtkTextureObject to svtkImageData
  void ConvertTextureToImageData(svtkTextureObject* texture, svtkImageData* output);

  // Render to texture for final rendering
  void SetupRenderToTexture(svtkRenderer* ren);
  void ExitRenderToTexture(svtkRenderer* ren);

  // Render to texture for depth pass
  void SetupDepthPass(svtkRenderer* ren);
  void RenderContourPass(svtkRenderer* ren);
  void ExitDepthPass(svtkRenderer* ren);
  void RenderWithDepthPass(svtkRenderer* ren, svtkOpenGLCamera* cam, svtkMTimeType renderPassTime);

  void RenderSingleInput(svtkRenderer* ren, svtkOpenGLCamera* cam, svtkShaderProgram* prog);

  void RenderMultipleInputs(svtkRenderer* ren, svtkOpenGLCamera* cam, svtkShaderProgram* prog);

  //@{
  /**
   * Update shader parameters.
   */
  void SetLightingShaderParameters(
    svtkRenderer* ren, svtkShaderProgram* prog, svtkVolume* vol, int numberOfSamplers);

  /**
   * Global parameters.
   */
  void SetMapperShaderParameters(
    svtkShaderProgram* prog, svtkRenderer* ren, int independent, int numComponents);

  /**
   * Per input data/ per component parameters.
   */
  void SetVolumeShaderParameters(
    svtkShaderProgram* prog, int independent, int numComponents, svtkMatrix4x4* modelViewMat);
  void BindTransformations(svtkShaderProgram* prog, svtkMatrix4x4* modelViewMat);

  /**
   * Transformation parameters.
   */
  void SetCameraShaderParameters(svtkShaderProgram* prog, svtkRenderer* ren, svtkOpenGLCamera* cam);

  /**
   * Feature specific.
   */
  void SetMaskShaderParameters(svtkShaderProgram* prog, svtkVolumeProperty* prop, int noOfComponents);
  void SetRenderToImageParameters(svtkShaderProgram* prog);
  void SetAdvancedShaderParameters(svtkRenderer* ren, svtkShaderProgram* prog, svtkVolume* vol,
    svtkVolumeTexture::VolumeBlock* block, int numComp);
  //@}

  void FinishRendering(int numComponents);

  inline bool ShaderRebuildNeeded(svtkCamera* cam, svtkVolume* vol, svtkMTimeType renderPassTime);
  bool VolumePropertyChanged = true;

  //@{
  /**
   * Image XY-Sampling
   * Render to an internal framebuffer with lower resolution than the currently
   * bound one (hence casting less rays and improving performance). The rendered
   * image is subsequently rendered as a texture-mapped quad (linearly
   * interpolated) to the default (or previously attached) framebuffer. If a
   * svtkOpenGLRenderPass is attached, a variable number of render targets are
   * supported (as specified by the RenderPass). The render targets are assumed
   * to be ordered from GL_COLOR_ATTACHMENT0 to GL_COLOR_ATTACHMENT$N$, where
   * $N$ is the number of targets specified (targets of the previously bound
   * framebuffer as activated through ActivateDrawBuffers(int)). Without a
   * RenderPass attached, it relies on FramebufferObject to re-activate the
   * appropriate previous DrawBuffer.
   *
   * \sa svtkOpenGLRenderPass svtkOpenGLFramebufferObject
   */
  void BeginImageSample(svtkRenderer* ren);
  bool InitializeImageSampleFBO(svtkRenderer* ren);
  void EndImageSample(svtkRenderer* ren);
  size_t GetNumImageSampleDrawBuffers(svtkVolume* vol);
  //@}

  //@{
  /**
   * Allocate and update input data. A list of active ports is maintained
   * by the parent class. This list is traversed to update internal structures
   * used during rendering.
   */
  bool UpdateInputs(svtkRenderer* ren, svtkVolume* vol);

  /**
   * Cleanup resources of inputs that have been removed.
   */
  void ClearRemovedInputs(svtkWindow* win);

  /**
   * Forces transfer functions in all of the active svtkVolumeInputHelpers to
   * re-initialize in the next update. This is essential if the order in
   * AssembledInputs changes (inputs are added or removed), given that variable
   * names cached in svtkVolumeInputHelper instances are indexed.
   */
  void ForceTransferInit();
  //@}

  svtkVolume* GetActiveVolume()
  {
    return this->MultiVolume ? this->MultiVolume : this->Parent->AssembledInputs[0].Volume;
  }
  int GetComponentMode(svtkVolumeProperty* prop, svtkDataArray* array) const;

  void ReleaseRenderToTextureGraphicsResources(svtkWindow* win);
  void ReleaseImageSampleGraphicsResources(svtkWindow* win);
  void ReleaseDepthPassGraphicsResources(svtkWindow* win);

  // Private member variables
  //--------------------------------------------------------------------------
  svtkOpenGLGPUVolumeRayCastMapper* Parent;

  bool ValidTransferFunction;
  bool LoadDepthTextureExtensionsSucceeded;
  bool CameraWasInsideInLastUpdate;

  GLuint CubeVBOId;
  GLuint CubeVAOId;
  GLuint CubeIndicesId;

  svtkTextureObject* DepthTextureObject;
  bool SharedDepthTextureObject;

  int TextureWidth;

  float ActualSampleDistance;

  int LastProjectionParallel;
  int TextureSize[3];
  int WindowLowerLeft[2];
  int WindowSize[2];
  int LastDepthPassWindowSize[2];
  int LastRenderToImageWindowSize[2];

  int NumberOfLights;
  int LightComplexity;

  std::ostringstream ExtensionsStringStream;

  svtkSmartPointer<svtkOpenGLVolumeMaskTransferFunction2D> LabelMapTransfer2D;
  svtkSmartPointer<svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D> LabelMapGradientOpacity;

  svtkTimeStamp ShaderBuildTime;

  svtkNew<svtkMatrix4x4> InverseProjectionMat;
  svtkNew<svtkMatrix4x4> InverseModelViewMat;
  svtkNew<svtkMatrix4x4> InverseVolumeMat;

  svtkSmartPointer<svtkPolyData> BBoxPolyData;
  svtkSmartPointer<svtkVolumeTexture> CurrentMask;

  svtkTimeStamp InitializationTime;
  svtkTimeStamp MaskUpdateTime;
  svtkTimeStamp ReleaseResourcesTime;
  svtkTimeStamp DepthPassTime;
  svtkTimeStamp DepthPassSetupTime;
  svtkTimeStamp SelectionStateTime;
  int CurrentSelectionPass;
  bool IsPicking;

  bool NeedToInitializeResources;
  bool PreserveViewport;
  bool PreserveGLState;

  svtkShaderProgram* ShaderProgram;
  svtkOpenGLShaderCache* ShaderCache;

  svtkOpenGLFramebufferObject* FBO;
  svtkTextureObject* RTTDepthBufferTextureObject;
  svtkTextureObject* RTTDepthTextureObject;
  svtkTextureObject* RTTColorTextureObject;
  int RTTDepthTextureType;

  svtkOpenGLFramebufferObject* DPFBO;
  svtkTextureObject* DPDepthBufferTextureObject;
  svtkTextureObject* DPColorTextureObject;

  svtkOpenGLFramebufferObject* ImageSampleFBO = nullptr;
  std::vector<svtkSmartPointer<svtkTextureObject> > ImageSampleTexture;
  std::vector<std::string> ImageSampleTexNames;
  svtkShaderProgram* ImageSampleProg = nullptr;
  svtkOpenGLVertexArrayObject* ImageSampleVAO = nullptr;
  size_t NumImageSampleDrawBuffers = 0;
  bool RebuildImageSampleProg = false;
  bool RenderPassAttached = false;

  svtkNew<svtkContourFilter> ContourFilter;
  svtkNew<svtkPolyDataMapper> ContourMapper;
  svtkNew<svtkActor> ContourActor;

  unsigned short Partitions[3];
  svtkMultiVolume* MultiVolume = nullptr;

  std::vector<float> VolMatVec, InvMatVec, TexMatVec, InvTexMatVec, TexEyeMatVec, CellToPointVec,
    TexMinVec, TexMaxVec, ScaleVec, BiasVec, StepVec, SpacingVec, RangeVec;
};

//----------------------------------------------------------------------------
template <typename T>
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ToFloat(
  const T& in1, const T& in2, float (&out)[2])
{
  out[0] = static_cast<float>(in1);
  out[1] = static_cast<float>(in2);
}

template <typename T>
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ToFloat(
  const T& in1, const T& in2, const T& in3, float (&out)[3])
{
  out[0] = static_cast<float>(in1);
  out[1] = static_cast<float>(in2);
  out[2] = static_cast<float>(in3);
}

//----------------------------------------------------------------------------
template <typename T>
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ToFloat(T* in, float* out, int noOfComponents)
{
  for (int i = 0; i < noOfComponents; ++i)
  {
    out[i] = static_cast<float>(in[i]);
  }
}

//----------------------------------------------------------------------------
template <typename T>
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ToFloat(T (&in)[3], float (&out)[3])
{
  out[0] = static_cast<float>(in[0]);
  out[1] = static_cast<float>(in[1]);
  out[2] = static_cast<float>(in[2]);
}

//----------------------------------------------------------------------------
template <typename T>
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ToFloat(T (&in)[2], float (&out)[2])
{
  out[0] = static_cast<float>(in[0]);
  out[1] = static_cast<float>(in[1]);
}

//----------------------------------------------------------------------------
template <typename T>
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ToFloat(T& in, float& out)
{
  out = static_cast<float>(in);
}

//----------------------------------------------------------------------------
template <typename T>
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ToFloat(T (&in)[4][2], float (&out)[4][2])
{
  out[0][0] = static_cast<float>(in[0][0]);
  out[0][1] = static_cast<float>(in[0][1]);
  out[1][0] = static_cast<float>(in[1][0]);
  out[1][1] = static_cast<float>(in[1][1]);
  out[2][0] = static_cast<float>(in[2][0]);
  out[2][1] = static_cast<float>(in[2][1]);
  out[3][0] = static_cast<float>(in[3][0]);
  out[3][1] = static_cast<float>(in[3][1]);
}

//----------------------------------------------------------------------------
template <typename T, int SizeX, int SizeY>
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::CopyMatrixToVector(
  T* matrix, float* matrixVec, int offset)
{
  const int MatSize = SizeX * SizeY;
  for (int j = 0; j < MatSize; j++)
  {
    matrixVec[offset + j] = matrix->Element[j / SizeX][j % SizeY];
  }
}

//----------------------------------------------------------------------------
template <typename T, int SizeSrc>
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::CopyVector(T* srcVec, T* dstVec, int offset)
{
  for (int j = 0; j < SizeSrc; j++)
  {
    dstVec[offset + j] = srcVec[j];
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetupMaskTransfer(svtkRenderer* ren)
{
  this->ReleaseGraphicsMaskTransfer(ren->GetRenderWindow());
  this->DeleteMaskTransfer();

  if (this->Parent->MaskInput != nullptr && this->Parent->MaskType == LabelMapMaskType &&
    !this->LabelMapTransfer2D)
  {
    this->LabelMapTransfer2D = svtkSmartPointer<svtkOpenGLVolumeMaskTransferFunction2D>::New();
    this->LabelMapGradientOpacity =
      svtkSmartPointer<svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D>::New();
  }

  this->InitializationTime.Modified();
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::RefreshMaskTransfer(
  svtkRenderer* ren, VolumeInput& input)
{
  auto vol = input.Volume;
  if (this->NeedToInitializeResources ||
    input.Volume->GetProperty()->GetMTime() > this->InitializationTime.GetMTime())
  {
    this->SetupMaskTransfer(ren);
  }
  this->UpdateMaskTransfer(ren, vol, 0);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::UpdateTransferFunctions(svtkRenderer* ren)
{
  int uniformIndex = 0;
  for (const auto& port : this->Parent->Ports)
  {
    auto& input = this->Parent->AssembledInputs[port];
    input.ColorRangeType = this->Parent->GetColorRangeType();
    input.ScalarOpacityRangeType = this->Parent->GetScalarOpacityRangeType();
    input.GradientOpacityRangeType = this->Parent->GetGradientOpacityRangeType();
    input.RefreshTransferFunction(
      ren, uniformIndex, this->Parent->BlendMode, this->ActualSampleDistance);

    uniformIndex++;
  }

  if (!this->MultiVolume)
  {
    this->RefreshMaskTransfer(ren, this->Parent->AssembledInputs[0]);
  }
}

//-----------------------------------------------------------------------------
bool svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::LoadMask(svtkRenderer* ren)
{
  bool result = true;
  auto maskInput = this->Parent->MaskInput;
  if (maskInput)
  {
    if (!this->CurrentMask)
    {
      this->CurrentMask = svtkSmartPointer<svtkVolumeTexture>::New();

      const auto part = this->Partitions;
      this->CurrentMask->SetPartitions(part[0], part[1], part[2]);
    }

    int isCellData;
    svtkDataArray* arr = this->Parent->GetScalars(maskInput, this->Parent->ScalarMode,
      this->Parent->ArrayAccessMode, this->Parent->ArrayId, this->Parent->ArrayName, isCellData);
    if (maskInput->GetMTime() > this->MaskUpdateTime ||
      this->CurrentMask->GetLoadedScalars() != arr ||
      (arr && arr->GetMTime() > this->MaskUpdateTime))
    {
      result =
        this->CurrentMask->LoadVolume(ren, maskInput, arr, isCellData, SVTK_NEAREST_INTERPOLATION);

      this->MaskUpdateTime.Modified();
    }
  }

  return result;
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ReleaseGraphicsMaskTransfer(svtkWindow* window)
{
  if (this->LabelMapTransfer2D)
  {
    this->LabelMapTransfer2D->ReleaseGraphicsResources(window);
  }
  if (this->LabelMapGradientOpacity)
  {
    this->LabelMapGradientOpacity->ReleaseGraphicsResources(window);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::DeleteMaskTransfer()
{
  this->LabelMapTransfer2D = nullptr;
  this->LabelMapGradientOpacity = nullptr;
}

//----------------------------------------------------------------------------
int svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::UpdateMaskTransfer(
  svtkRenderer* ren, svtkVolume* vol, unsigned int component)
{
  svtkVolumeProperty* volumeProperty = vol->GetProperty();

  auto volumeTex = this->Parent->AssembledInputs[0].Texture.GetPointer();
  double componentRange[2];
  for (int i = 0; i < 2; ++i)
  {
    componentRange[i] = volumeTex->ScalarRange[component][i];
  }

  if (this->Parent->MaskInput != nullptr && this->Parent->MaskType == LabelMapMaskType)
  {
    this->LabelMapTransfer2D->Update(volumeProperty, componentRange, 0, 0, 0,
      svtkTextureObject::Nearest, svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

    if (volumeProperty->HasLabelGradientOpacity())
    {
      this->LabelMapGradientOpacity->Update(volumeProperty, componentRange, 0, 0, 0,
        svtkTextureObject::Nearest, svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::CaptureDepthTexture(svtkRenderer* ren)
{
  // Make sure our render window is the current OpenGL context
  ren->GetRenderWindow()->MakeCurrent();

  // Load required extensions for grabbing depth sampler buffer
  if (!this->LoadDepthTextureExtensionsSucceeded)
  {
    this->LoadRequireDepthTextureExtensions(ren->GetRenderWindow());
  }

  // If we can't load the necessary extensions, provide
  // feedback on why it failed.
  if (!this->LoadDepthTextureExtensionsSucceeded)
  {
    std::cerr << this->ExtensionsStringStream.str() << std::endl;
    return;
  }

  if (!this->DepthTextureObject)
  {
    this->DepthTextureObject = svtkTextureObject::New();
  }

  this->DepthTextureObject->SetContext(svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  //  this->DepthTextureObject->Activate();
  if (!this->DepthTextureObject->GetHandle())
  {
    // First set the parameters
    this->DepthTextureObject->SetWrapS(svtkTextureObject::ClampToEdge);
    this->DepthTextureObject->SetWrapT(svtkTextureObject::ClampToEdge);
    this->DepthTextureObject->SetMagnificationFilter(svtkTextureObject::Linear);
    this->DepthTextureObject->SetMinificationFilter(svtkTextureObject::Linear);
    this->DepthTextureObject->AllocateDepth(this->WindowSize[0], this->WindowSize[1], 4);
  }

#ifndef GL_ES_VERSION_3_0
  // currently broken on ES
  this->DepthTextureObject->CopyFromFrameBuffer(this->WindowLowerLeft[0], this->WindowLowerLeft[1],
    0, 0, this->WindowSize[0], this->WindowSize[1]);
#endif
  //  this->DepthTextureObject->Deactivate();
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetLightingShaderParameters(
  svtkRenderer* ren, svtkShaderProgram* prog, svtkVolume* vol, int numberOfSamplers)
{
  // Set basic lighting parameters (per component)
  if (!ren || !prog || !vol)
  {
    return;
  }

  auto volumeProperty = vol->GetProperty();
  float ambient[4][3];
  float diffuse[4][3];
  float specular[4][3];
  float specularPower[4];

  for (int i = 0; i < numberOfSamplers; ++i)
  {
    ambient[i][0] = ambient[i][1] = ambient[i][2] = volumeProperty->GetAmbient(i);
    diffuse[i][0] = diffuse[i][1] = diffuse[i][2] = volumeProperty->GetDiffuse(i);
    specular[i][0] = specular[i][1] = specular[i][2] = volumeProperty->GetSpecular(i);
    specularPower[i] = volumeProperty->GetSpecularPower(i);
  }

  prog->SetUniform3fv("in_ambient", numberOfSamplers, ambient);
  prog->SetUniform3fv("in_diffuse", numberOfSamplers, diffuse);
  prog->SetUniform3fv("in_specular", numberOfSamplers, specular);
  prog->SetUniform1fv("in_shininess", numberOfSamplers, specularPower);

  // Set advanced lighting features
  if (vol && !vol->GetProperty()->GetShade())
  {
    return;
  }

  prog->SetUniformi("in_twoSidedLighting", ren->GetTwoSidedLighting());

  // for lightkit case there are some parameters to set
  svtkCamera* cam = ren->GetActiveCamera();
  svtkTransform* viewTF = cam->GetModelViewTransformObject();

  // Bind some light settings
  int numberOfLights = 0;
  svtkLightCollection* lc = ren->GetLights();
  svtkLight* light;

  svtkCollectionSimpleIterator sit;
  float lightAmbientColor[6][3];
  float lightDiffuseColor[6][3];
  float lightSpecularColor[6][3];
  float lightDirection[6][3];
  for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit));)
  {
    float status = light->GetSwitch();
    if (status > 0.0)
    {
      double* aColor = light->GetAmbientColor();
      double* dColor = light->GetDiffuseColor();
      double* sColor = light->GetDiffuseColor();
      double intensity = light->GetIntensity();
      lightAmbientColor[numberOfLights][0] = aColor[0] * intensity;
      lightAmbientColor[numberOfLights][1] = aColor[1] * intensity;
      lightAmbientColor[numberOfLights][2] = aColor[2] * intensity;
      lightDiffuseColor[numberOfLights][0] = dColor[0] * intensity;
      lightDiffuseColor[numberOfLights][1] = dColor[1] * intensity;
      lightDiffuseColor[numberOfLights][2] = dColor[2] * intensity;
      lightSpecularColor[numberOfLights][0] = sColor[0] * intensity;
      lightSpecularColor[numberOfLights][1] = sColor[1] * intensity;
      lightSpecularColor[numberOfLights][2] = sColor[2] * intensity;
      // Get required info from light
      double* lfp = light->GetTransformedFocalPoint();
      double* lp = light->GetTransformedPosition();
      double lightDir[3];
      svtkMath::Subtract(lfp, lp, lightDir);
      svtkMath::Normalize(lightDir);
      double* tDir = viewTF->TransformNormal(lightDir);
      lightDirection[numberOfLights][0] = tDir[0];
      lightDirection[numberOfLights][1] = tDir[1];
      lightDirection[numberOfLights][2] = tDir[2];
      numberOfLights++;
    }
  }

  prog->SetUniform3fv("in_lightAmbientColor", numberOfLights, lightAmbientColor);
  prog->SetUniform3fv("in_lightDiffuseColor", numberOfLights, lightDiffuseColor);
  prog->SetUniform3fv("in_lightSpecularColor", numberOfLights, lightSpecularColor);
  prog->SetUniform3fv("in_lightDirection", numberOfLights, lightDirection);
  prog->SetUniformi("in_numberOfLights", numberOfLights);

  // we are done unless we have positional lights
  if (this->LightComplexity < 3)
  {
    return;
  }

  // if positional lights pass down more parameters
  float lightAttenuation[6][3];
  float lightPosition[6][3];
  float lightConeAngle[6];
  float lightExponent[6];
  int lightPositional[6];
  numberOfLights = 0;
  for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit));)
  {
    float status = light->GetSwitch();
    if (status > 0.0)
    {
      double* attn = light->GetAttenuationValues();
      lightAttenuation[numberOfLights][0] = attn[0];
      lightAttenuation[numberOfLights][1] = attn[1];
      lightAttenuation[numberOfLights][2] = attn[2];
      lightExponent[numberOfLights] = light->GetExponent();
      lightConeAngle[numberOfLights] = light->GetConeAngle();
      double* lp = light->GetTransformedPosition();
      double* tlp = viewTF->TransformPoint(lp);
      lightPosition[numberOfLights][0] = tlp[0];
      lightPosition[numberOfLights][1] = tlp[1];
      lightPosition[numberOfLights][2] = tlp[2];
      lightPositional[numberOfLights] = light->GetPositional();
      numberOfLights++;
    }
  }
  prog->SetUniform3fv("in_lightAttenuation", numberOfLights, lightAttenuation);
  prog->SetUniform1iv("in_lightPositional", numberOfLights, lightPositional);
  prog->SetUniform3fv("in_lightPosition", numberOfLights, lightPosition);
  prog->SetUniform1fv("in_lightExponent", numberOfLights, lightExponent);
  prog->SetUniform1fv("in_lightConeAngle", numberOfLights, lightConeAngle);
}

//----------------------------------------------------------------------------
bool svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::IsCameraInside(
  svtkRenderer* ren, svtkVolume* vol, double geometry[24])
{
  svtkNew<svtkMatrix4x4> dataToWorld;
  dataToWorld->DeepCopy(vol->GetMatrix());

  svtkCamera* cam = ren->GetActiveCamera();

  double planes[24];
  cam->GetFrustumPlanes(ren->GetTiledAspectRatio(), planes);

  // convert geometry to world then compare to frustum planes
  double in[4];
  in[3] = 1.0;
  double out[4];
  double worldGeometry[24];
  for (int i = 0; i < 8; ++i)
  {
    in[0] = geometry[i * 3];
    in[1] = geometry[i * 3 + 1];
    in[2] = geometry[i * 3 + 2];
    dataToWorld->MultiplyPoint(in, out);
    worldGeometry[i * 3] = out[0] / out[3];
    worldGeometry[i * 3 + 1] = out[1] / out[3];
    worldGeometry[i * 3 + 2] = out[2] / out[3];
  }

  // does the front clipping plane intersect the volume?
  // true if points are on both sides of the plane
  bool hasPositive = false;
  bool hasNegative = false;
  bool hasZero = false;
  for (int i = 0; i < 8; ++i)
  {
    double val = planes[4 * 4] * worldGeometry[i * 3] +
      planes[4 * 4 + 1] * worldGeometry[i * 3 + 1] + planes[4 * 4 + 2] * worldGeometry[i * 3 + 2] +
      planes[4 * 4 + 3];
    if (val < 0)
    {
      hasNegative = true;
    }
    else if (val > 0)
    {
      hasPositive = true;
    }
    else
    {
      hasZero = true;
    }
  }

  return hasZero || (hasNegative && hasPositive);
}

//----------------------------------------------------------------------------
bool svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::IsGeometryUpdateRequired(
  svtkRenderer* ren, svtkVolume* vol, double geometry[24])
{
  if (!this->BBoxPolyData)
  {
    return true;
  }

  using namespace std;
  const auto GeomTime = this->BBoxPolyData->GetMTime();
  const bool uploadTimeChanged = any_of(this->Parent->AssembledInputs.begin(),
    this->Parent->AssembledInputs.end(), [&GeomTime](const pair<int, svtkVolumeInputHelper>& item) {
      return item.second.Texture->UploadTime > GeomTime;
    });

  return (this->NeedToInitializeResources || uploadTimeChanged ||
    this->IsCameraInside(ren, vol, geometry) || this->CameraWasInsideInLastUpdate ||
    (this->MultiVolume && this->MultiVolume->GetBoundsTime() > this->BBoxPolyData->GetMTime()));
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::RenderVolumeGeometry(
  svtkRenderer* ren, svtkShaderProgram* prog, svtkVolume* vol, double geometry[24])
{
  if (this->IsGeometryUpdateRequired(ren, vol, geometry))
  {
    svtkNew<svtkPolyData> boxSource;

    {
      svtkNew<svtkCellArray> cells;
      svtkNew<svtkPoints> points;
      points->SetDataTypeToDouble();
      for (int i = 0; i < 8; ++i)
      {
        points->InsertNextPoint(geometry + i * 3);
      }
      // 6 faces 12 triangles
      int tris[36] = {
        0, 1, 2, //
        1, 3, 2, //
        1, 5, 3, //
        5, 7, 3, //
        5, 4, 7, //
        4, 6, 7, //
        4, 0, 6, //
        0, 2, 6, //
        2, 3, 6, //
        3, 7, 6, //
        0, 4, 1, //
        1, 4, 5  //
      };
      for (int i = 0; i < 12; ++i)
      {
        cells->InsertNextCell(3);
        // this code uses a clockwise convention for some reason
        // no clue why but the ClipConvexPolyData assumes the same
        // so we add verts as 0 2 1 instead of 0 1 2
        cells->InsertCellPoint(tris[i * 3]);
        cells->InsertCellPoint(tris[i * 3 + 2]);
        cells->InsertCellPoint(tris[i * 3 + 1]);
      }
      boxSource->SetPoints(points);
      boxSource->SetPolys(cells);
    }

    svtkNew<svtkDensifyPolyData> densifyPolyData;
    if (this->IsCameraInside(ren, vol, geometry))
    {
      svtkNew<svtkMatrix4x4> dataToWorld;
      dataToWorld->DeepCopy(vol->GetMatrix());

      svtkCamera* cam = ren->GetActiveCamera();

      double fplanes[24];
      cam->GetFrustumPlanes(ren->GetTiledAspectRatio(), fplanes);

      // have to convert the 5th plane to volume coordinates
      double pOrigin[4];
      pOrigin[3] = 1.0;
      double pNormal[3];
      for (int i = 0; i < 3; ++i)
      {
        pNormal[i] = fplanes[16 + i];
        pOrigin[i] = -fplanes[16 + 3] * fplanes[16 + i];
      }

      // convert the normal
      double* dmat = dataToWorld->GetData();
      dataToWorld->Transpose();
      double pNormalV[3];
      pNormalV[0] = pNormal[0] * dmat[0] + pNormal[1] * dmat[1] + pNormal[2] * dmat[2];
      pNormalV[1] = pNormal[0] * dmat[4] + pNormal[1] * dmat[5] + pNormal[2] * dmat[6];
      pNormalV[2] = pNormal[0] * dmat[8] + pNormal[1] * dmat[9] + pNormal[2] * dmat[10];
      svtkMath::Normalize(pNormalV);

      // convert the point
      dataToWorld->Transpose();
      dataToWorld->Invert();
      dataToWorld->MultiplyPoint(pOrigin, pOrigin);

      svtkNew<svtkPlane> nearPlane;

      // We add an offset to the near plane to avoid hardware clipping of the
      // near plane due to floating-point precision.
      // camPlaneNormal is a unit vector, if the offset is larger than the
      // distance between near and far point, it will not work. Hence, we choose
      // a fraction of the near-far distance. However, care should be taken
      // to avoid hardware clipping in volumes with very small spacing where the
      // distance between near and far plane is also very small. In that case,
      // a minimum offset is chosen. This is chosen based on the typical
      // epsilon values on x86 systems.
      double offset = (cam->GetClippingRange()[1] - cam->GetClippingRange()[0]) * 0.001;
      // Minimum offset to avoid floating point precision issues for volumes
      // with very small spacing
      double minOffset = static_cast<double>(std::numeric_limits<float>::epsilon()) * 1000.0;
      offset = offset < minOffset ? minOffset : offset;

      for (int i = 0; i < 3; ++i)
      {
        pOrigin[i] += (pNormalV[i] * offset);
      }

      nearPlane->SetOrigin(pOrigin);
      nearPlane->SetNormal(pNormalV);

      svtkNew<svtkPlaneCollection> planes;
      planes->RemoveAllItems();
      planes->AddItem(nearPlane);

      svtkNew<svtkClipConvexPolyData> clip;
      clip->SetInputData(boxSource);
      clip->SetPlanes(planes);

      densifyPolyData->SetInputConnection(clip->GetOutputPort());

      this->CameraWasInsideInLastUpdate = true;
    }
    else
    {
      densifyPolyData->SetInputData(boxSource);
      this->CameraWasInsideInLastUpdate = false;
    }

    densifyPolyData->SetNumberOfSubdivisions(2);
    densifyPolyData->Update();

    this->BBoxPolyData = svtkSmartPointer<svtkPolyData>::New();
    this->BBoxPolyData->ShallowCopy(densifyPolyData->GetOutput());
    svtkPoints* points = this->BBoxPolyData->GetPoints();
    svtkCellArray* cells = this->BBoxPolyData->GetPolys();

    svtkNew<svtkUnsignedIntArray> polys;
    polys->SetNumberOfComponents(3);
    svtkIdType npts;
    const svtkIdType* pts;

    // See if the volume transform is orientation-preserving
    // and orient polygons accordingly
    svtkMatrix4x4* volMat = vol->GetMatrix();
    double det = svtkMath::Determinant3x3(volMat->GetElement(0, 0), volMat->GetElement(0, 1),
      volMat->GetElement(0, 2), volMat->GetElement(1, 0), volMat->GetElement(1, 1),
      volMat->GetElement(1, 2), volMat->GetElement(2, 0), volMat->GetElement(2, 1),
      volMat->GetElement(2, 2));
    bool preservesOrientation = det > 0.0;

    const svtkIdType indexMap[3] = { preservesOrientation ? 0 : 2, 1, preservesOrientation ? 2 : 0 };

    while (cells->GetNextCell(npts, pts))
    {
      polys->InsertNextTuple3(pts[indexMap[0]], pts[indexMap[1]], pts[indexMap[2]]);
    }

    // Dispose any previously created buffers
    this->DeleteBufferObjects();

    // Now create new ones
    this->CreateBufferObjects();

    // TODO: should really use the built in VAO class
    glBindVertexArray(this->CubeVAOId);

    // Pass cube vertices to buffer object memory
    glBindBuffer(GL_ARRAY_BUFFER, this->CubeVBOId);
    glBufferData(GL_ARRAY_BUFFER,
      points->GetData()->GetDataSize() * points->GetData()->GetDataTypeSize(),
      points->GetData()->GetVoidPointer(0), GL_STATIC_DRAW);

    prog->EnableAttributeArray("in_vertexPos");
    prog->UseAttributeArray("in_vertexPos", 0, 0, SVTK_FLOAT, 3, svtkShaderProgram::NoNormalize);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->CubeIndicesId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, polys->GetDataSize() * polys->GetDataTypeSize(),
      polys->GetVoidPointer(0), GL_STATIC_DRAW);
  }
  else
  {
    glBindVertexArray(this->CubeVAOId);
  }

  glDrawElements(
    GL_TRIANGLES, this->BBoxPolyData->GetNumberOfCells() * 3, GL_UNSIGNED_INT, nullptr);

  svtkOpenGLStaticCheckErrorMacro("Error after glDrawElements in"
                                 " RenderVolumeGeometry!");
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetCroppingRegions(
  svtkShaderProgram* prog, double loadedBounds[6])
{
  if (this->Parent->GetCropping())
  {
    int cropFlags = this->Parent->GetCroppingRegionFlags();
    double croppingRegionPlanes[6];
    this->Parent->GetCroppingRegionPlanes(croppingRegionPlanes);

    // Clamp it
    croppingRegionPlanes[0] =
      croppingRegionPlanes[0] < loadedBounds[0] ? loadedBounds[0] : croppingRegionPlanes[0];
    croppingRegionPlanes[0] =
      croppingRegionPlanes[0] > loadedBounds[1] ? loadedBounds[1] : croppingRegionPlanes[0];
    croppingRegionPlanes[1] =
      croppingRegionPlanes[1] < loadedBounds[0] ? loadedBounds[0] : croppingRegionPlanes[1];
    croppingRegionPlanes[1] =
      croppingRegionPlanes[1] > loadedBounds[1] ? loadedBounds[1] : croppingRegionPlanes[1];

    croppingRegionPlanes[2] =
      croppingRegionPlanes[2] < loadedBounds[2] ? loadedBounds[2] : croppingRegionPlanes[2];
    croppingRegionPlanes[2] =
      croppingRegionPlanes[2] > loadedBounds[3] ? loadedBounds[3] : croppingRegionPlanes[2];
    croppingRegionPlanes[3] =
      croppingRegionPlanes[3] < loadedBounds[2] ? loadedBounds[2] : croppingRegionPlanes[3];
    croppingRegionPlanes[3] =
      croppingRegionPlanes[3] > loadedBounds[3] ? loadedBounds[3] : croppingRegionPlanes[3];

    croppingRegionPlanes[4] =
      croppingRegionPlanes[4] < loadedBounds[4] ? loadedBounds[4] : croppingRegionPlanes[4];
    croppingRegionPlanes[4] =
      croppingRegionPlanes[4] > loadedBounds[5] ? loadedBounds[5] : croppingRegionPlanes[4];
    croppingRegionPlanes[5] =
      croppingRegionPlanes[5] < loadedBounds[4] ? loadedBounds[4] : croppingRegionPlanes[5];
    croppingRegionPlanes[5] =
      croppingRegionPlanes[5] > loadedBounds[5] ? loadedBounds[5] : croppingRegionPlanes[5];

    float cropPlanes[6] = { static_cast<float>(croppingRegionPlanes[0]),
      static_cast<float>(croppingRegionPlanes[1]), static_cast<float>(croppingRegionPlanes[2]),
      static_cast<float>(croppingRegionPlanes[3]), static_cast<float>(croppingRegionPlanes[4]),
      static_cast<float>(croppingRegionPlanes[5]) };

    prog->SetUniform1fv("in_croppingPlanes", 6, cropPlanes);
    const int numberOfRegions = 32;
    int cropFlagsArray[numberOfRegions];
    cropFlagsArray[0] = 0;
    int i = 1;
    while (cropFlags && i < 32)
    {
      cropFlagsArray[i] = cropFlags & 1;
      cropFlags = cropFlags >> 1;
      ++i;
    }
    for (; i < 32; ++i)
    {
      cropFlagsArray[i] = 0;
    }

    prog->SetUniform1iv("in_croppingFlags", numberOfRegions, cropFlagsArray);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetClippingPlanes(
  svtkRenderer* svtkNotUsed(ren), svtkShaderProgram* prog, svtkVolume* vol)
{
  if (this->Parent->GetClippingPlanes())
  {
    std::vector<float> clippingPlanes;
    // Currently we don't have any clipping plane
    clippingPlanes.push_back(0);

    this->Parent->ClippingPlanes->InitTraversal();
    svtkPlane* plane;
    while ((plane = this->Parent->ClippingPlanes->GetNextItem()))
    {
      // Planes are in world coordinates
      double planeOrigin[3], planeNormal[3];
      plane->GetOrigin(planeOrigin);
      plane->GetNormal(planeNormal);

      clippingPlanes.push_back(planeOrigin[0]);
      clippingPlanes.push_back(planeOrigin[1]);
      clippingPlanes.push_back(planeOrigin[2]);
      clippingPlanes.push_back(planeNormal[0]);
      clippingPlanes.push_back(planeNormal[1]);
      clippingPlanes.push_back(planeNormal[2]);
    }

    clippingPlanes[0] = clippingPlanes.size() > 1 ? static_cast<int>(clippingPlanes.size() - 1) : 0;

    prog->SetUniform1fv(
      "in_clippingPlanes", static_cast<int>(clippingPlanes.size()), &clippingPlanes[0]);
    float clippedVoxelIntensity =
      static_cast<float>(vol->GetProperty()->GetClippedVoxelIntensity());
    prog->SetUniformf("in_clippedVoxelIntensity", clippedVoxelIntensity);
  }
}

// -----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::CheckPropertyKeys(svtkVolume* vol)
{
  // Check the property keys to see if we should modify the blend/etc state:
  // Otherwise this breaks volume/translucent geo depth peeling.
  svtkInformation* volumeKeys = vol->GetPropertyKeys();
  this->PreserveGLState = false;
  if (volumeKeys && volumeKeys->Has(svtkOpenGLActor::GLDepthMaskOverride()))
  {
    int override = volumeKeys->Get(svtkOpenGLActor::GLDepthMaskOverride());
    if (override != 0 && override != 1)
    {
      this->PreserveGLState = true;
    }
  }

  // Some render passes (e.g. DualDepthPeeling) adjust the viewport for
  // intermediate passes so it is necessary to preserve it. This is a
  // temporary fix for svtkDualDepthPeelingPass to work when various viewports
  // are defined.  The correct way of fixing this would be to avoid setting the
  // viewport within the mapper.  It is enough for now to check for the
  // RenderPasses() svtkInfo given that svtkDualDepthPeelingPass is the only pass
  // currently supported by this mapper, the viewport will have to be adjusted
  // externally before adding support for other passes.
  svtkInformation* info = vol->GetPropertyKeys();
  this->PreserveViewport = info && info->Has(svtkOpenGLRenderPass::RenderPasses());
}

// -----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::CheckPickingState(svtkRenderer* ren)
{
  svtkHardwareSelector* selector = ren->GetSelector();
  bool selectorPicking = selector != nullptr;
  if (selector)
  {
    // this mapper currently only supports cell picking
    selectorPicking &= selector->GetFieldAssociation() == svtkDataObject::FIELD_ASSOCIATION_CELLS;
  }

  this->IsPicking = selectorPicking;
  if (this->IsPicking)
  {
    // rebuild the shader on every pass
    this->SelectionStateTime.Modified();
    this->CurrentSelectionPass =
      selector ? selector->GetCurrentPass() : svtkHardwareSelector::ACTOR_PASS;
  }
  else if (this->CurrentSelectionPass != svtkHardwareSelector::MIN_KNOWN_PASS - 1)
  {
    // return to the regular rendering state
    this->SelectionStateTime.Modified();
    this->CurrentSelectionPass = svtkHardwareSelector::MIN_KNOWN_PASS - 1;
  }
}

// -----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::BeginPicking(svtkRenderer* ren)
{
  svtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->IsPicking)
  {
    selector->BeginRenderProp();
  }
}

//------------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetPickingId(svtkRenderer* ren)
{
  float propIdColor[3] = { 0.0, 0.0, 0.0 };
  svtkHardwareSelector* selector = ren->GetSelector();

  if (selector && this->IsPicking)
  {
    // query the selector for the appropriate id
    selector->GetPropColorValue(propIdColor);
  }

  this->ShaderProgram->SetUniform3f("in_propId", propIdColor);
}

// ---------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::EndPicking(svtkRenderer* ren)
{
  svtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->IsPicking)
  {
    if (this->CurrentSelectionPass >= svtkHardwareSelector::POINT_ID_LOW24)
    {
      // Only supported on single-input
      int extents[6];
      this->Parent->GetTransformedInput(0)->GetExtent(extents);

      // Tell the selector the maximum number of cells that the mapper could render
      unsigned int const numVoxels = (extents[1] - extents[0] + 1) * (extents[3] - extents[2] + 1) *
        (extents[5] - extents[4] + 1);
      selector->UpdateMaximumPointId(numVoxels);
      selector->UpdateMaximumCellId(numVoxels);
    }
    selector->EndRenderProp();
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::UpdateSamplingDistance(
  svtkRenderer* svtkNotUsed(ren))
{
  auto input = this->Parent->GetTransformedInput(0);
  auto vol = this->Parent->AssembledInputs[0].Volume;
  double cellSpacing[3];
  input->GetSpacing(cellSpacing);

  if (!this->Parent->AutoAdjustSampleDistances)
  {
    if (this->Parent->LockSampleDistanceToInputSpacing)
    {
      int extents[6];
      input->GetExtent(extents);

      float const d =
        static_cast<float>(this->Parent->SpacingAdjustedSampleDistance(cellSpacing, extents));
      float const sample = this->Parent->SampleDistance;

      // ActualSampleDistance will grow proportionally to numVoxels^(1/3) (see
      // svtkVolumeMapper.cxx). Until it reaches 1/2 average voxel size when number of
      // voxels is 1E6.
      this->ActualSampleDistance =
        (sample / d < 0.999f || sample / d > 1.001f) ? d : this->Parent->SampleDistance;

      return;
    }

    this->ActualSampleDistance = this->Parent->SampleDistance;
  }
  else
  {
    input->GetSpacing(cellSpacing);
    svtkMatrix4x4* worldToDataset = vol->GetMatrix();
    double minWorldSpacing = SVTK_DOUBLE_MAX;
    int i = 0;
    while (i < 3)
    {
      double tmp = worldToDataset->GetElement(0, i);
      double tmp2 = tmp * tmp;
      tmp = worldToDataset->GetElement(1, i);
      tmp2 += tmp * tmp;
      tmp = worldToDataset->GetElement(2, i);
      tmp2 += tmp * tmp;

      // We use fabs() in case the spacing is negative.
      double worldSpacing = fabs(cellSpacing[i] * sqrt(tmp2));
      if (worldSpacing < minWorldSpacing)
      {
        minWorldSpacing = worldSpacing;
      }
      ++i;
    }

    // minWorldSpacing is the optimal sample distance in world space.
    // To go faster (reduceFactor<1.0), we multiply this distance
    // by 1/reduceFactor.
    this->ActualSampleDistance = static_cast<float>(minWorldSpacing);

    if (this->Parent->ReductionFactor < 1.0 && this->Parent->ReductionFactor != 0.0)
    {
      this->ActualSampleDistance /= static_cast<GLfloat>(this->Parent->ReductionFactor);
    }
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::LoadRequireDepthTextureExtensions(
  svtkRenderWindow* svtkNotUsed(renWin))
{
  // Reset the message stream for extensions
  this->LoadDepthTextureExtensionsSucceeded = true;
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::CreateBufferObjects()
{
  glGenVertexArrays(1, &this->CubeVAOId);
  glGenBuffers(1, &this->CubeVBOId);
  glGenBuffers(1, &this->CubeIndicesId);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::DeleteBufferObjects()
{
  if (this->CubeVBOId)
  {
    glBindBuffer(GL_ARRAY_BUFFER, this->CubeVBOId);
    glDeleteBuffers(1, &this->CubeVBOId);
    this->CubeVBOId = 0;
  }

  if (this->CubeIndicesId)
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->CubeIndicesId);
    glDeleteBuffers(1, &this->CubeIndicesId);
    this->CubeIndicesId = 0;
  }

  if (this->CubeVAOId)
  {
    glDeleteVertexArrays(1, &this->CubeVAOId);
    this->CubeVAOId = 0;
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ConvertTextureToImageData(
  svtkTextureObject* texture, svtkImageData* output)
{
  if (!texture)
  {
    return;
  }
  unsigned int tw = texture->GetWidth();
  unsigned int th = texture->GetHeight();
  unsigned int tnc = texture->GetComponents();
  int tt = texture->GetSVTKDataType();

  svtkPixelExtent texExt(0U, tw - 1U, 0U, th - 1U);

  int dataExt[6] = { 0, 0, 0, 0, 0, 0 };
  texExt.GetData(dataExt);

  double dataOrigin[6] = { 0, 0, 0, 0, 0, 0 };

  svtkImageData* id = svtkImageData::New();
  id->SetOrigin(dataOrigin);
  id->SetDimensions(tw, th, 1);
  id->SetExtent(dataExt);
  id->AllocateScalars(tt, tnc);

  svtkPixelBufferObject* pbo = texture->Download();

  svtkPixelTransfer::Blit(texExt, texExt, texExt, texExt, tnc, tt, pbo->MapPackedBuffer(), tnc, tt,
    id->GetScalarPointer(0, 0, 0));

  pbo->UnmapPackedBuffer();
  pbo->Delete();

  if (!output)
  {
    output = svtkImageData::New();
  }
  output->DeepCopy(id);
  id->Delete();
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::BeginImageSample(svtkRenderer* ren)
{
  auto vol = this->GetActiveVolume();
  const auto numBuffers = this->GetNumImageSampleDrawBuffers(vol);
  if (numBuffers != this->NumImageSampleDrawBuffers)
  {
    if (numBuffers > this->NumImageSampleDrawBuffers)
    {
      this->ReleaseImageSampleGraphicsResources(ren->GetRenderWindow());
    }

    this->NumImageSampleDrawBuffers = numBuffers;
    this->RebuildImageSampleProg = true;
  }

  float const xySampleDist = this->Parent->ImageSampleDistance;
  if (xySampleDist != 1.f && this->InitializeImageSampleFBO(ren))
  {
    this->ImageSampleFBO->GetContext()->GetState()->PushDrawFramebufferBinding();
    this->ImageSampleFBO->Bind(GL_DRAW_FRAMEBUFFER);
    this->ImageSampleFBO->ActivateDrawBuffers(
      static_cast<unsigned int>(this->NumImageSampleDrawBuffers));

    this->ImageSampleFBO->GetContext()->GetState()->svtkglClearColor(0.0, 0.0, 0.0, 0.0);
    this->ImageSampleFBO->GetContext()->GetState()->svtkglClear(GL_COLOR_BUFFER_BIT);
  }
}

//----------------------------------------------------------------------------
bool svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::InitializeImageSampleFBO(svtkRenderer* ren)
{
  // Set the FBO viewport size. These are used in the shader to normalize the
  // fragment coordinate, the normalized coordinate is used to fetch the depth
  // buffer.
  this->WindowSize[0] /= this->Parent->ImageSampleDistance;
  this->WindowSize[1] /= this->Parent->ImageSampleDistance;
  this->WindowLowerLeft[0] = 0;
  this->WindowLowerLeft[1] = 0;

  svtkOpenGLRenderWindow* win = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  // Set FBO viewport
  win->GetState()->svtkglViewport(
    this->WindowLowerLeft[0], this->WindowLowerLeft[1], this->WindowSize[0], this->WindowSize[1]);

  if (!this->ImageSampleFBO)
  {
    this->ImageSampleTexture.reserve(this->NumImageSampleDrawBuffers);
    this->ImageSampleTexNames.reserve(this->NumImageSampleDrawBuffers);
    for (size_t i = 0; i < this->NumImageSampleDrawBuffers; i++)
    {
      auto tex = svtkSmartPointer<svtkTextureObject>::New();
      tex->SetContext(win);
      tex->Create2D(this->WindowSize[0], this->WindowSize[1], 4, SVTK_UNSIGNED_CHAR, false);
      tex->Activate();
      tex->SetMinificationFilter(svtkTextureObject::Linear);
      tex->SetMagnificationFilter(svtkTextureObject::Linear);
      tex->SetWrapS(svtkTextureObject::ClampToEdge);
      tex->SetWrapT(svtkTextureObject::ClampToEdge);
      this->ImageSampleTexture.push_back(tex);

      std::stringstream ss;
      ss << i;
      const std::string name = "renderedTex_" + ss.str();
      this->ImageSampleTexNames.push_back(name);
    }

    this->ImageSampleFBO = svtkOpenGLFramebufferObject::New();
    this->ImageSampleFBO->SetContext(win);
    win->GetState()->PushFramebufferBindings();
    this->ImageSampleFBO->Bind();
    this->ImageSampleFBO->InitializeViewport(this->WindowSize[0], this->WindowSize[1]);

    auto num = static_cast<unsigned int>(this->NumImageSampleDrawBuffers);
    for (unsigned int i = 0; i < num; i++)
    {
      this->ImageSampleFBO->AddColorAttachment(i, this->ImageSampleTexture[i]);
    }

    // Verify completeness
    const int complete = this->ImageSampleFBO->CheckFrameBufferStatus(GL_FRAMEBUFFER);
    for (auto& tex : this->ImageSampleTexture)
    {
      tex->Deactivate();
    }
    win->GetState()->PopFramebufferBindings();

    if (!complete)
    {
      svtkGenericWarningMacro(<< "Failed to attach ImageSampleFBO!");
      this->ReleaseImageSampleGraphicsResources(win);
      return false;
    }

    this->RebuildImageSampleProg = true;
    return true;
  }

  // Resize if necessary
  int lastSize[2];
  this->ImageSampleFBO->GetLastSize(lastSize);
  if (lastSize[0] != this->WindowSize[0] || lastSize[1] != this->WindowSize[1])
  {
    this->ImageSampleFBO->Resize(this->WindowSize[0], this->WindowSize[1]);
  }

  return true;
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::EndImageSample(svtkRenderer* ren)
{
  if (this->Parent->ImageSampleDistance != 1.f)
  {
    this->ImageSampleFBO->DeactivateDrawBuffers();
    if (this->RenderPassAttached)
    {
      this->ImageSampleFBO->ActivateDrawBuffers(
        static_cast<unsigned int>(this->NumImageSampleDrawBuffers));
    }
    svtkOpenGLRenderWindow* win = static_cast<svtkOpenGLRenderWindow*>(ren->GetRenderWindow());
    win->GetState()->PopDrawFramebufferBinding();

    // Render the contents of ImageSampleFBO as a quad to intermix with the
    // rest of the scene.
    typedef svtkOpenGLRenderUtilities GLUtil;

    if (this->RebuildImageSampleProg)
    {
      std::string frag = GLUtil::GetFullScreenQuadFragmentShaderTemplate();

      svtkShaderProgram::Substitute(frag, "//SVTK::FSQ::Decl",
        svtkvolume::ImageSampleDeclarationFrag(
          this->ImageSampleTexNames, this->NumImageSampleDrawBuffers));
      svtkShaderProgram::Substitute(frag, "//SVTK::FSQ::Impl",
        svtkvolume::ImageSampleImplementationFrag(
          this->ImageSampleTexNames, this->NumImageSampleDrawBuffers));

      this->ImageSampleProg =
        win->GetShaderCache()->ReadyShaderProgram(GLUtil::GetFullScreenQuadVertexShader().c_str(),
          frag.c_str(), GLUtil::GetFullScreenQuadGeometryShader().c_str());
    }
    else
    {
      win->GetShaderCache()->ReadyShaderProgram(this->ImageSampleProg);
    }

    if (!this->ImageSampleProg)
    {
      svtkGenericWarningMacro(<< "Failed to initialize ImageSampleProgram!");
      return;
    }

    if (!this->ImageSampleVAO)
    {
      this->ImageSampleVAO = svtkOpenGLVertexArrayObject::New();
      GLUtil::PrepFullScreenVAO(win, this->ImageSampleVAO, this->ImageSampleProg);
    }

    svtkOpenGLState* ostate = win->GetState();

    // Adjust the GL viewport to SVTK's defined viewport
    ren->GetTiledSizeAndOrigin(
      this->WindowSize, this->WindowSize + 1, this->WindowLowerLeft, this->WindowLowerLeft + 1);
    ostate->svtkglViewport(
      this->WindowLowerLeft[0], this->WindowLowerLeft[1], this->WindowSize[0], this->WindowSize[1]);

    // Bind objects and draw
    ostate->svtkglEnable(GL_BLEND);
    ostate->svtkglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    ostate->svtkglDisable(GL_DEPTH_TEST);

    for (size_t i = 0; i < this->NumImageSampleDrawBuffers; i++)
    {
      this->ImageSampleTexture[i]->Activate();
      this->ImageSampleProg->SetUniformi(
        this->ImageSampleTexNames[i].c_str(), this->ImageSampleTexture[i]->GetTextureUnit());
    }

    this->ImageSampleVAO->Bind();
    GLUtil::DrawFullScreenQuad();
    this->ImageSampleVAO->Release();
    svtkOpenGLStaticCheckErrorMacro("Error after DrawFullScreenQuad()!");

    for (auto& tex : this->ImageSampleTexture)
    {
      tex->Deactivate();
    }
  }
}

//------------------------------------------------------------------------------
size_t svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::GetNumImageSampleDrawBuffers(svtkVolume* vol)
{
  if (this->RenderPassAttached)
  {
    svtkInformation* info = vol->GetPropertyKeys();
    const int num = info->Length(svtkOpenGLRenderPass::RenderPasses());
    svtkObjectBase* rpBase = info->Get(svtkOpenGLRenderPass::RenderPasses(), num - 1);
    svtkOpenGLRenderPass* rp = static_cast<svtkOpenGLRenderPass*>(rpBase);
    return static_cast<size_t>(rp->GetActiveDrawBuffers());
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetupRenderToTexture(svtkRenderer* ren)
{
  if (this->Parent->RenderToImage && this->Parent->CurrentPass == RenderPass)
  {
    if (this->Parent->ImageSampleDistance != 1.f)
    {
      this->WindowSize[0] /= this->Parent->ImageSampleDistance;
      this->WindowSize[1] /= this->Parent->ImageSampleDistance;
    }

    if ((this->LastRenderToImageWindowSize[0] != this->WindowSize[0]) ||
      (this->LastRenderToImageWindowSize[1] != this->WindowSize[1]))
    {
      this->LastRenderToImageWindowSize[0] = this->WindowSize[0];
      this->LastRenderToImageWindowSize[1] = this->WindowSize[1];
      this->ReleaseRenderToTextureGraphicsResources(ren->GetRenderWindow());
    }

    if (!this->FBO)
    {
      this->FBO = svtkOpenGLFramebufferObject::New();
    }

    svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
    this->FBO->SetContext(renWin);

    renWin->GetState()->PushFramebufferBindings();
    this->FBO->Bind();
    this->FBO->InitializeViewport(this->WindowSize[0], this->WindowSize[1]);

    int depthImageScalarType = this->Parent->GetDepthImageScalarType();
    bool initDepthTexture = true;
    // Re-instantiate the depth texture object if the scalar type requested has
    // changed from the last frame
    if (this->RTTDepthTextureObject && this->RTTDepthTextureType == depthImageScalarType)
    {
      initDepthTexture = false;
    }

    if (initDepthTexture)
    {
      if (this->RTTDepthTextureObject)
      {
        this->RTTDepthTextureObject->Delete();
        this->RTTDepthTextureObject = nullptr;
      }
      this->RTTDepthTextureObject = svtkTextureObject::New();
      this->RTTDepthTextureObject->SetContext(renWin);
      this->RTTDepthTextureObject->Create2D(
        this->WindowSize[0], this->WindowSize[1], 1, depthImageScalarType, false);
      this->RTTDepthTextureObject->Activate();
      this->RTTDepthTextureObject->SetMinificationFilter(svtkTextureObject::Nearest);
      this->RTTDepthTextureObject->SetMagnificationFilter(svtkTextureObject::Nearest);
      this->RTTDepthTextureObject->SetAutoParameters(0);

      // Cache the value of the scalar type
      this->RTTDepthTextureType = depthImageScalarType;
    }

    if (!this->RTTColorTextureObject)
    {
      this->RTTColorTextureObject = svtkTextureObject::New();

      this->RTTColorTextureObject->SetContext(
        svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
      this->RTTColorTextureObject->Create2D(
        this->WindowSize[0], this->WindowSize[1], 4, SVTK_UNSIGNED_CHAR, false);
      this->RTTColorTextureObject->Activate();
      this->RTTColorTextureObject->SetMinificationFilter(svtkTextureObject::Nearest);
      this->RTTColorTextureObject->SetMagnificationFilter(svtkTextureObject::Nearest);
      this->RTTColorTextureObject->SetAutoParameters(0);
    }

    if (!this->RTTDepthBufferTextureObject)
    {
      this->RTTDepthBufferTextureObject = svtkTextureObject::New();
      this->RTTDepthBufferTextureObject->SetContext(renWin);
      this->RTTDepthBufferTextureObject->AllocateDepth(
        this->WindowSize[0], this->WindowSize[1], svtkTextureObject::Float32);
      this->RTTDepthBufferTextureObject->Activate();
      this->RTTDepthBufferTextureObject->SetMinificationFilter(svtkTextureObject::Nearest);
      this->RTTDepthBufferTextureObject->SetMagnificationFilter(svtkTextureObject::Nearest);
      this->RTTDepthBufferTextureObject->SetAutoParameters(0);
    }

    this->FBO->Bind(GL_FRAMEBUFFER);
    this->FBO->AddDepthAttachment(this->RTTDepthBufferTextureObject);
    this->FBO->AddColorAttachment(0U, this->RTTColorTextureObject);
    this->FBO->AddColorAttachment(1U, this->RTTDepthTextureObject);
    this->FBO->ActivateDrawBuffers(2);

    this->FBO->CheckFrameBufferStatus(GL_FRAMEBUFFER);

    this->FBO->GetContext()->GetState()->svtkglClearColor(1.0, 1.0, 1.0, 0.0);
    this->FBO->GetContext()->GetState()->svtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ExitRenderToTexture(svtkRenderer* svtkNotUsed(ren))
{
  if (this->Parent->RenderToImage && this->Parent->CurrentPass == RenderPass)
  {
    this->FBO->RemoveDepthAttachment();
    this->FBO->RemoveColorAttachment(0U);
    this->FBO->RemoveColorAttachment(1U);
    this->FBO->DeactivateDrawBuffers();
    this->FBO->GetContext()->GetState()->PopFramebufferBindings();

    this->RTTDepthBufferTextureObject->Deactivate();
    this->RTTColorTextureObject->Deactivate();
    this->RTTDepthTextureObject->Deactivate();
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetupDepthPass(svtkRenderer* ren)
{
  if (this->Parent->ImageSampleDistance != 1.f)
  {
    this->WindowSize[0] /= this->Parent->ImageSampleDistance;
    this->WindowSize[1] /= this->Parent->ImageSampleDistance;
  }

  if ((this->LastDepthPassWindowSize[0] != this->WindowSize[0]) ||
    (this->LastDepthPassWindowSize[1] != this->WindowSize[1]))
  {
    this->LastDepthPassWindowSize[0] = this->WindowSize[0];
    this->LastDepthPassWindowSize[1] = this->WindowSize[1];
    this->ReleaseDepthPassGraphicsResources(ren->GetRenderWindow());
  }

  if (!this->DPFBO)
  {
    this->DPFBO = svtkOpenGLFramebufferObject::New();
  }

  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  this->DPFBO->SetContext(renWin);

  renWin->GetState()->PushFramebufferBindings();
  this->DPFBO->Bind();
  this->DPFBO->InitializeViewport(this->WindowSize[0], this->WindowSize[1]);

  if (!this->DPDepthBufferTextureObject || !this->DPColorTextureObject)
  {
    this->DPDepthBufferTextureObject = svtkTextureObject::New();
    this->DPDepthBufferTextureObject->SetContext(renWin);
    this->DPDepthBufferTextureObject->AllocateDepth(
      this->WindowSize[0], this->WindowSize[1], svtkTextureObject::Native);
    this->DPDepthBufferTextureObject->Activate();
    this->DPDepthBufferTextureObject->SetMinificationFilter(svtkTextureObject::Nearest);
    this->DPDepthBufferTextureObject->SetMagnificationFilter(svtkTextureObject::Nearest);
    this->DPDepthBufferTextureObject->SetAutoParameters(0);
    this->DPDepthBufferTextureObject->Bind();

    this->DPColorTextureObject = svtkTextureObject::New();

    this->DPColorTextureObject->SetContext(renWin);
    this->DPColorTextureObject->Create2D(
      this->WindowSize[0], this->WindowSize[1], 4, SVTK_UNSIGNED_CHAR, false);
    this->DPColorTextureObject->Activate();
    this->DPColorTextureObject->SetMinificationFilter(svtkTextureObject::Nearest);
    this->DPColorTextureObject->SetMagnificationFilter(svtkTextureObject::Nearest);
    this->DPColorTextureObject->SetAutoParameters(0);

    this->DPFBO->AddDepthAttachment(this->DPDepthBufferTextureObject);

    this->DPFBO->AddColorAttachment(0U, this->DPColorTextureObject);
  }

  this->DPFBO->ActivateDrawBuffers(1);
  this->DPFBO->CheckFrameBufferStatus(GL_FRAMEBUFFER);

  // Setup the contour polydata mapper to render to DPFBO
  this->ContourMapper->SetInputConnection(this->ContourFilter->GetOutputPort());

  svtkOpenGLState* ostate = this->DPFBO->GetContext()->GetState();
  ostate->svtkglClearColor(0.0, 0.0, 0.0, 0.0);
  ostate->svtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ostate->svtkglEnable(GL_DEPTH_TEST);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::RenderContourPass(svtkRenderer* ren)
{
  this->SetupDepthPass(ren);
  this->ContourActor->Render(ren, this->ContourMapper.GetPointer());
  this->ExitDepthPass(ren);
  this->DepthPassTime.Modified();
  this->Parent->CurrentPass = this->Parent->RenderPass;
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ExitDepthPass(svtkRenderer* svtkNotUsed(ren))
{
  this->DPFBO->DeactivateDrawBuffers();
  svtkOpenGLState* ostate = this->DPFBO->GetContext()->GetState();
  ostate->PopFramebufferBindings();

  this->DPDepthBufferTextureObject->Deactivate();
  this->DPColorTextureObject->Deactivate();
  ostate->svtkglDisable(GL_DEPTH_TEST);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal ::ReleaseRenderToTextureGraphicsResources(
  svtkWindow* win)
{
  svtkOpenGLRenderWindow* rwin = svtkOpenGLRenderWindow::SafeDownCast(win);

  if (rwin)
  {
    if (this->FBO)
    {
      this->FBO->Delete();
      this->FBO = nullptr;
    }

    if (this->RTTDepthBufferTextureObject)
    {
      this->RTTDepthBufferTextureObject->ReleaseGraphicsResources(win);
      this->RTTDepthBufferTextureObject->Delete();
      this->RTTDepthBufferTextureObject = nullptr;
    }

    if (this->RTTDepthTextureObject)
    {
      this->RTTDepthTextureObject->ReleaseGraphicsResources(win);
      this->RTTDepthTextureObject->Delete();
      this->RTTDepthTextureObject = nullptr;
    }

    if (this->RTTColorTextureObject)
    {
      this->RTTColorTextureObject->ReleaseGraphicsResources(win);
      this->RTTColorTextureObject->Delete();
      this->RTTColorTextureObject = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal ::ReleaseDepthPassGraphicsResources(
  svtkWindow* win)
{
  svtkOpenGLRenderWindow* rwin = svtkOpenGLRenderWindow::SafeDownCast(win);

  if (rwin)
  {
    if (this->DPFBO)
    {
      this->DPFBO->Delete();
      this->DPFBO = nullptr;
    }

    if (this->DPDepthBufferTextureObject)
    {
      this->DPDepthBufferTextureObject->ReleaseGraphicsResources(win);
      this->DPDepthBufferTextureObject->Delete();
      this->DPDepthBufferTextureObject = nullptr;
    }

    if (this->DPColorTextureObject)
    {
      this->DPColorTextureObject->ReleaseGraphicsResources(win);
      this->DPColorTextureObject->Delete();
      this->DPColorTextureObject = nullptr;
    }

    this->ContourMapper->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal ::ReleaseImageSampleGraphicsResources(
  svtkWindow* win)
{
  svtkOpenGLRenderWindow* rwin = svtkOpenGLRenderWindow::SafeDownCast(win);

  if (rwin)
  {
    if (this->ImageSampleFBO)
    {
      this->ImageSampleFBO->Delete();
      this->ImageSampleFBO = nullptr;
    }

    for (auto& tex : this->ImageSampleTexture)
    {
      tex->ReleaseGraphicsResources(win);
      tex = nullptr;
    }
    this->ImageSampleTexture.clear();
    this->ImageSampleTexNames.clear();

    if (this->ImageSampleVAO)
    {
      this->ImageSampleVAO->Delete();
      this->ImageSampleVAO = nullptr;
    }

    // Do not delete the shader program - Let the cache clean it up.
    this->ImageSampleProg = nullptr;
  }
}

//----------------------------------------------------------------------------
svtkOpenGLGPUVolumeRayCastMapper::svtkOpenGLGPUVolumeRayCastMapper()
  : svtkGPUVolumeRayCastMapper()
{
  this->Impl = new svtkInternal(this);
  this->ReductionFactor = 1.0;
  this->CurrentPass = RenderPass;

  this->ResourceCallback = new svtkOpenGLResourceFreeCallback<svtkOpenGLGPUVolumeRayCastMapper>(
    this, &svtkOpenGLGPUVolumeRayCastMapper::ReleaseGraphicsResources);

  //  this->VolumeTexture = svtkVolumeTexture::New();
  //  this->VolumeTexture->SetMapper(this);
}

//----------------------------------------------------------------------------
svtkOpenGLGPUVolumeRayCastMapper::~svtkOpenGLGPUVolumeRayCastMapper()
{
  if (this->ResourceCallback)
  {
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = nullptr;
  }

  delete this->Impl;
  this->Impl = nullptr;

  this->AssembledInputs.clear();
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ReductionFactor: " << this->ReductionFactor << "\n";
  os << indent << "CurrentPass: " << this->CurrentPass << "\n";
}

void svtkOpenGLGPUVolumeRayCastMapper::SetSharedDepthTexture(svtkTextureObject* nt)
{
  if (this->Impl->DepthTextureObject == nt)
  {
    return;
  }
  if (this->Impl->DepthTextureObject)
  {
    this->Impl->DepthTextureObject->Delete();
  }
  this->Impl->DepthTextureObject = nt;

  if (nt)
  {
    nt->Register(this); // as it will get deleted later on
    this->Impl->SharedDepthTextureObject = true;
  }
  else
  {
    this->Impl->SharedDepthTextureObject = false;
  }
}

//----------------------------------------------------------------------------
svtkTextureObject* svtkOpenGLGPUVolumeRayCastMapper::GetDepthTexture()
{
  return this->Impl->RTTDepthTextureObject;
}

//----------------------------------------------------------------------------
svtkTextureObject* svtkOpenGLGPUVolumeRayCastMapper::GetColorTexture()
{
  return this->Impl->RTTColorTextureObject;
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::GetDepthImage(svtkImageData* output)
{
  return this->Impl->ConvertTextureToImageData(this->Impl->RTTDepthTextureObject, output);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::GetColorImage(svtkImageData* output)
{
  return this->Impl->ConvertTextureToImageData(this->Impl->RTTColorTextureObject, output);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReleaseGraphicsResources(svtkWindow* window)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  this->Impl->DeleteBufferObjects();

  for (auto& input : this->AssembledInputs)
  {
    input.second.ReleaseGraphicsResources(window);
  }

  if (this->Impl->DepthTextureObject && !this->Impl->SharedDepthTextureObject)
  {
    this->Impl->DepthTextureObject->ReleaseGraphicsResources(window);
    this->Impl->DepthTextureObject->Delete();
    this->Impl->DepthTextureObject = nullptr;
  }

  this->Impl->ReleaseRenderToTextureGraphicsResources(window);
  this->Impl->ReleaseDepthPassGraphicsResources(window);
  this->Impl->ReleaseImageSampleGraphicsResources(window);

  if (this->Impl->CurrentMask)
  {
    this->Impl->CurrentMask->ReleaseGraphicsResources(window);
    this->Impl->CurrentMask = nullptr;
  }

  this->Impl->ReleaseGraphicsMaskTransfer(window);
  this->Impl->DeleteMaskTransfer();

  this->Impl->ReleaseResourcesTime.Modified();
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::GetShaderTemplate(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkOpenGLShaderProperty* p)
{
  if (shaders[svtkShader::Vertex])
  {
    if (p->HasVertexShaderCode())
    {
      shaders[svtkShader::Vertex]->SetSource(p->GetVertexShaderCode());
    }
    else
    {
      shaders[svtkShader::Vertex]->SetSource(raycastervs);
    }
  }

  if (shaders[svtkShader::Fragment])
  {
    if (p->HasFragmentShaderCode())
    {
      shaders[svtkShader::Fragment]->SetSource(p->GetFragmentShaderCode());
    }
    else
    {
      shaders[svtkShader::Fragment]->SetSource(raycasterfs);
    }
  }

  if (shaders[svtkShader::Geometry])
  {
    shaders[svtkShader::Geometry]->SetSource("");
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderCustomUniforms(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkOpenGLShaderProperty* p)
{
  svtkShader* vertexShader = shaders[svtkShader::Vertex];
  svtkOpenGLUniforms* vu = static_cast<svtkOpenGLUniforms*>(p->GetVertexCustomUniforms());
  svtkShaderProgram::Substitute(vertexShader, "//SVTK::CustomUniforms::Dec", vu->GetDeclarations());

  svtkShader* fragmentShader = shaders[svtkShader::Fragment];
  svtkOpenGLUniforms* fu = static_cast<svtkOpenGLUniforms*>(p->GetFragmentCustomUniforms());
  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::CustomUniforms::Dec", fu->GetDeclarations());

  svtkShader* geometryShader = shaders[svtkShader::Geometry];
  svtkOpenGLUniforms* gu = static_cast<svtkOpenGLUniforms*>(p->GetGeometryCustomUniforms());
  svtkShaderProgram::Substitute(geometryShader, "//SVTK::CustomUniforms::Dec", gu->GetDeclarations());
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderBase(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps)
{
  svtkShader* vertexShader = shaders[svtkShader::Vertex];
  svtkShader* fragmentShader = shaders[svtkShader::Fragment];

  // Every volume should have a property (cannot be nullptr);
  svtkVolumeProperty* volumeProperty = vol->GetProperty();
  int independentComponents = volumeProperty->GetIndependentComponents();

  svtkShaderProgram::Substitute(vertexShader, "//SVTK::ComputeClipPos::Impl",
    svtkvolume::ComputeClipPositionImplementation(ren, this, vol));

  svtkShaderProgram::Substitute(vertexShader, "//SVTK::ComputeTextureCoords::Impl",
    svtkvolume::ComputeTextureCoordinates(ren, this, vol));

  svtkShaderProgram::Substitute(vertexShader, "//SVTK::Base::Dec",
    svtkvolume::BaseDeclarationVertex(ren, this, vol, this->Impl->MultiVolume != nullptr));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::CallWorker::Impl", svtkvolume::WorkerImplementation(ren, this, vol));

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Base::Dec",
    svtkvolume::BaseDeclarationFragment(ren, this, this->AssembledInputs, this->Impl->NumberOfLights,
      this->Impl->LightComplexity, numComps, independentComponents));

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Base::Init",
    svtkvolume::BaseInit(ren, this, this->AssembledInputs, this->Impl->LightComplexity));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Base::Impl", svtkvolume::BaseImplementation(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Base::Exit", svtkvolume::BaseExit(ren, this, vol));
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderTermination(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol,
  int svtkNotUsed(numComps))
{
  svtkShader* vertexShader = shaders[svtkShader::Vertex];
  svtkShader* fragmentShader = shaders[svtkShader::Fragment];

  svtkShaderProgram::Substitute(vertexShader, "//SVTK::Termination::Dec",
    svtkvolume::TerminationDeclarationVertex(ren, this, vol));

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Termination::Dec",
    svtkvolume::TerminationDeclarationFragment(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Terminate::Init", svtkvolume::TerminationInit(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Terminate::Impl", svtkvolume::TerminationImplementation(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Terminate::Exit", svtkvolume::TerminationExit(ren, this, vol));
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderShading(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps)
{
  svtkShader* vertexShader = shaders[svtkShader::Vertex];
  svtkShader* fragmentShader = shaders[svtkShader::Fragment];

  // Every volume should have a property (cannot be nullptr);
  svtkVolumeProperty* volumeProperty = vol->GetProperty();
  int independentComponents = volumeProperty->GetIndependentComponents();

  svtkShaderProgram::Substitute(
    vertexShader, "//SVTK::Shading::Dec", svtkvolume::ShadingDeclarationVertex(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Shading::Dec", svtkvolume::ShadingDeclarationFragment(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Shading::Init", svtkvolume::ShadingInit(ren, this, vol));

  if (this->Impl->MultiVolume)
  {
    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Shading::Impl",
      svtkvolume::ShadingMultipleInputs(this, this->AssembledInputs));
  }
  else
  {
    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Shading::Impl",
      svtkvolume::ShadingSingleInput(ren, this, vol, this->MaskInput, this->Impl->CurrentMask,
        this->MaskType, numComps, independentComponents));
  }

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Shading::Exit",
    svtkvolume::ShadingExit(ren, this, vol, numComps, independentComponents));
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderCompute(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps)
{
  svtkShader* fragmentShader = shaders[svtkShader::Fragment];

  // Every volume should have a property (cannot be nullptr);
  svtkVolumeProperty* volumeProperty = vol->GetProperty();
  int independentComponents = volumeProperty->GetIndependentComponents();

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeGradient::Dec",
    svtkvolume::ComputeGradientDeclaration(this, this->AssembledInputs));

  if (this->Impl->MultiVolume)
  {
    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::GradientCache::Dec",
      svtkvolume::GradientCacheDec(ren, vol, this->AssembledInputs, independentComponents));

    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Transfer2D::Dec",
      svtkvolume::Transfer2DDeclaration(this->AssembledInputs));

    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeOpacity::Dec",
      svtkvolume::ComputeOpacityMultiDeclaration(this->AssembledInputs));

    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeGradientOpacity1D::Dec",
      svtkvolume::ComputeGradientOpacityMulti1DDecl(this->AssembledInputs));

    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeColor::Dec",
      svtkvolume::ComputeColorMultiDeclaration(this->AssembledInputs));
  }
  else
  {
    // Single input
    switch (volumeProperty->GetTransferFunctionMode())
    {
      case svtkVolumeProperty::TF_1D:
      {
        auto& input = this->AssembledInputs[0];

        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeOpacity::Dec",
          svtkvolume::ComputeOpacityDeclaration(
            ren, this, vol, numComps, independentComponents, input.OpacityTablesMap));

        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeGradientOpacity1D::Dec",
          svtkvolume::ComputeGradientOpacity1DDecl(
            vol, numComps, independentComponents, input.GradientOpacityTablesMap));

        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeColor::Dec",
          svtkvolume::ComputeColorDeclaration(
            ren, this, vol, numComps, independentComponents, input.RGBTablesMap));
      }
      break;
      case svtkVolumeProperty::TF_2D:
        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeOpacity::Dec",
          svtkvolume::ComputeOpacity2DDeclaration(ren, this, vol, numComps, independentComponents,
            this->AssembledInputs[0].TransferFunctions2DMap));

        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeColor::Dec",
          svtkvolume::ComputeColor2DDeclaration(ren, this, vol, numComps, independentComponents,
            this->AssembledInputs[0].TransferFunctions2DMap));

        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::GradientCache::Dec",
          svtkvolume::GradientCacheDec(ren, vol, this->AssembledInputs, independentComponents));

        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::PreComputeGradients::Impl",
          svtkvolume::PreComputeGradientsImpl(ren, vol, numComps, independentComponents));

        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Transfer2D::Dec",
          svtkvolume::Transfer2DDeclaration(this->AssembledInputs));
        break;
    }
  }

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeLighting::Dec",
    svtkvolume::ComputeLightingDeclaration(ren, this, vol, numComps, independentComponents,
      this->Impl->NumberOfLights, this->Impl->LightComplexity));

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::ComputeRayDirection::Dec",
    svtkvolume::ComputeRayDirectionDeclaration(ren, this, vol, numComps));
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderCropping(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol,
  int svtkNotUsed(numComps))
{
  svtkShader* vertexShader = shaders[svtkShader::Vertex];
  svtkShader* fragmentShader = shaders[svtkShader::Fragment];

  svtkShaderProgram::Substitute(
    vertexShader, "//SVTK::Cropping::Dec", svtkvolume::CroppingDeclarationVertex(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Cropping::Dec", svtkvolume::CroppingDeclarationFragment(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Cropping::Init", svtkvolume::CroppingInit(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Cropping::Impl", svtkvolume::CroppingImplementation(ren, this, vol));
  // true);

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Cropping::Exit", svtkvolume::CroppingExit(ren, this, vol));
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderClipping(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol,
  int svtkNotUsed(numComps))
{
  svtkShader* vertexShader = shaders[svtkShader::Vertex];
  svtkShader* fragmentShader = shaders[svtkShader::Fragment];

  svtkShaderProgram::Substitute(
    vertexShader, "//SVTK::Clipping::Dec", svtkvolume::ClippingDeclarationVertex(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Clipping::Dec", svtkvolume::ClippingDeclarationFragment(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Clipping::Init", svtkvolume::ClippingInit(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Clipping::Impl", svtkvolume::ClippingImplementation(ren, this, vol));

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Clipping::Exit", svtkvolume::ClippingExit(ren, this, vol));
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderMasking(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps)
{
  svtkShader* fragmentShader = shaders[svtkShader::Fragment];

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::BinaryMask::Dec",
    svtkvolume::BinaryMaskDeclaration(
      ren, this, vol, this->MaskInput, this->Impl->CurrentMask, this->MaskType));

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::BinaryMask::Impl",
    svtkvolume::BinaryMaskImplementation(
      ren, this, vol, this->MaskInput, this->Impl->CurrentMask, this->MaskType));

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::CompositeMask::Dec",
    svtkvolume::CompositeMaskDeclarationFragment(
      ren, this, vol, this->MaskInput, this->Impl->CurrentMask, this->MaskType));

  svtkShaderProgram::Substitute(fragmentShader, "//SVTK::CompositeMask::Impl",
    svtkvolume::CompositeMaskImplementation(
      ren, this, vol, this->MaskInput, this->Impl->CurrentMask, this->MaskType, numComps));
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderPicking(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol,
  int svtkNotUsed(numComps))
{
  svtkShader* fragmentShader = shaders[svtkShader::Fragment];

  if (this->Impl->CurrentSelectionPass != (svtkHardwareSelector::MIN_KNOWN_PASS - 1))
  {
    switch (this->Impl->CurrentSelectionPass)
    {
      case svtkHardwareSelector::CELL_ID_LOW24:
        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Picking::Exit",
          svtkvolume::PickingIdLow24PassExit(ren, this, vol));
        break;
      case svtkHardwareSelector::CELL_ID_HIGH24:
        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Picking::Exit",
          svtkvolume::PickingIdHigh24PassExit(ren, this, vol));
        break;
      default: // ACTOR_PASS, PROCESS_PASS
        svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Picking::Dec",
          svtkvolume::PickingActorPassDeclaration(ren, this, vol));

        svtkShaderProgram::Substitute(
          fragmentShader, "//SVTK::Picking::Exit", svtkvolume::PickingActorPassExit(ren, this, vol));
        break;
    }
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderRTT(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol,
  int svtkNotUsed(numComps))
{
  svtkShader* fragmentShader = shaders[svtkShader::Fragment];

  if (this->RenderToImage)
  {
    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::RenderToImage::Dec",
      svtkvolume::RenderToImageDeclarationFragment(ren, this, vol));

    svtkShaderProgram::Substitute(
      fragmentShader, "//SVTK::RenderToImage::Init", svtkvolume::RenderToImageInit(ren, this, vol));

    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::RenderToImage::Impl",
      svtkvolume::RenderToImageImplementation(ren, this, vol));

    svtkShaderProgram::Substitute(
      fragmentShader, "//SVTK::RenderToImage::Exit", svtkvolume::RenderToImageExit(ren, this, vol));
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderValues(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol,
  int noOfComponents)
{
  // Every volume should have a property (cannot be nullptr);
  svtkVolumeProperty* volumeProperty = vol->GetProperty();
  auto shaderProperty = svtkOpenGLShaderProperty::SafeDownCast(vol->GetShaderProperty());

  if (volumeProperty->GetShade())
  {
    svtkLightCollection* lc = ren->GetLights();
    svtkLight* light;
    this->Impl->NumberOfLights = 0;

    // Compute light complexity.
    svtkCollectionSimpleIterator sit;
    for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit));)
    {
      float status = light->GetSwitch();
      if (status > 0.0)
      {
        this->Impl->NumberOfLights++;
        if (this->Impl->LightComplexity == 0)
        {
          this->Impl->LightComplexity = 1;
        }
      }

      if (this->Impl->LightComplexity == 1 &&
        (this->Impl->NumberOfLights > 1 || light->GetIntensity() != 1.0 ||
          light->GetLightType() != SVTK_LIGHT_TYPE_HEADLIGHT))
      {
        this->Impl->LightComplexity = 2;
      }

      if (this->Impl->LightComplexity < 3 && (light->GetPositional()))
      {
        this->Impl->LightComplexity = 3;
        break;
      }
    }
  }

  // Render pass pre replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderRenderPass(shaders, vol, true);

  // Custom uniform variables replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderCustomUniforms(shaders, shaderProperty);

  // Base methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderBase(shaders, ren, vol, noOfComponents);

  // Termination methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderTermination(shaders, ren, vol, noOfComponents);

  // Shading methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderShading(shaders, ren, vol, noOfComponents);

  // Compute methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderCompute(shaders, ren, vol, noOfComponents);

  // Cropping methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderCropping(shaders, ren, vol, noOfComponents);

  // Clipping methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderClipping(shaders, ren, vol, noOfComponents);

  // Masking methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderMasking(shaders, ren, vol, noOfComponents);

  // Picking replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderPicking(shaders, ren, vol, noOfComponents);

  // Render to texture
  //---------------------------------------------------------------------------
  this->ReplaceShaderRTT(shaders, ren, vol, noOfComponents);

  // Set number of isosurfaces
  if (this->GetBlendMode() == svtkVolumeMapper::ISOSURFACE_BLEND)
  {
    std::ostringstream ss;
    ss << volumeProperty->GetIsoSurfaceValues()->GetNumberOfContours();
    svtkShaderProgram::Substitute(shaders[svtkShader::Fragment], "NUMBER_OF_CONTOURS", ss.str());
  }

  // Render pass post replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderRenderPass(shaders, vol, false);
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::BuildShader(svtkRenderer* ren)
{
  std::map<svtkShader::Type, svtkShader*> shaders;
  svtkShader* vertexShader = svtkShader::New();
  vertexShader->SetType(svtkShader::Vertex);
  shaders[svtkShader::Vertex] = vertexShader;
  svtkShader* fragmentShader = svtkShader::New();
  fragmentShader->SetType(svtkShader::Fragment);
  shaders[svtkShader::Fragment] = fragmentShader;
  svtkShader* geometryShader = svtkShader::New();
  geometryShader->SetType(svtkShader::Geometry);
  shaders[svtkShader::Geometry] = geometryShader;

  auto vol = this->Impl->GetActiveVolume();

  svtkOpenGLShaderProperty* sp = svtkOpenGLShaderProperty::SafeDownCast(vol->GetShaderProperty());
  this->GetShaderTemplate(shaders, sp);

  // user specified pre replacements
  svtkOpenGLShaderProperty::ReplacementMap repMap = sp->GetAllShaderReplacements();
  for (auto i : repMap)
  {
    if (i.first.ReplaceFirst)
    {
      std::string ssrc = shaders[i.first.ShaderType]->GetSource();
      svtkShaderProgram::Substitute(
        ssrc, i.first.OriginalValue, i.second.Replacement, i.second.ReplaceAll);
      shaders[i.first.ShaderType]->SetSource(ssrc);
    }
  }

  auto numComp = this->AssembledInputs[0].Texture->GetLoadedScalars()->GetNumberOfComponents();
  this->ReplaceShaderValues(shaders, ren, vol, numComp);

  // user specified post replacements
  for (auto i : repMap)
  {
    if (!i.first.ReplaceFirst)
    {
      std::string ssrc = shaders[i.first.ShaderType]->GetSource();
      svtkShaderProgram::Substitute(
        ssrc, i.first.OriginalValue, i.second.Replacement, i.second.ReplaceAll);
      shaders[i.first.ShaderType]->SetSource(ssrc);
    }
  }

  // Now compile the shader
  //--------------------------------------------------------------------------
  this->Impl->ShaderProgram = this->Impl->ShaderCache->ReadyShaderProgram(shaders);
  if (!this->Impl->ShaderProgram || !this->Impl->ShaderProgram->GetCompiled())
  {
    svtkErrorMacro("Shader failed to compile");
  }

  vertexShader->Delete();
  fragmentShader->Delete();
  geometryShader->Delete();

  this->Impl->ShaderBuildTime.Modified();
}

//-----------------------------------------------------------------------------
// Update the reduction factor of the render viewport (this->ReductionFactor)
// according to the time spent in seconds to render the previous frame
// (this->TimeToDraw) and a time in seconds allocated to render the next
// frame (allocatedTime).
// \pre valid_current_reduction_range: this->ReductionFactor>0.0 &&
// this->ReductionFactor<=1.0 \pre positive_TimeToDraw: this->TimeToDraw>=0.0
// \pre positive_time: allocatedTime>0.0
// \post valid_new_reduction_range: this->ReductionFactor>0.0 &&
// this->ReductionFactor<=1.0
//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ComputeReductionFactor(double allocatedTime)
{
  if (!this->AutoAdjustSampleDistances)
  {
    this->ReductionFactor = 1.0 / this->ImageSampleDistance;
    return;
  }

  if (this->TimeToDraw)
  {
    double oldFactor = this->ReductionFactor;

    double timeToDraw;
    if (allocatedTime < 1.0)
    {
      timeToDraw = this->SmallTimeToDraw;
      if (timeToDraw == 0.0)
      {
        timeToDraw = this->BigTimeToDraw / 3.0;
      }
    }
    else
    {
      timeToDraw = this->BigTimeToDraw;
    }

    // This should be the case when rendering the volume very first time
    // 10.0 is an arbitrary value chosen which happen to a large number
    // in this context
    if (timeToDraw == 0.0)
    {
      timeToDraw = 10.0;
    }

    double fullTime = timeToDraw / this->ReductionFactor;
    double newFactor = allocatedTime / fullTime;

    // Compute average factor
    this->ReductionFactor = (newFactor + oldFactor) / 2.0;

    // Discretize reduction factor so that it doesn't cause
    // visual artifacts when used to reduce the sample distance
    this->ReductionFactor = (this->ReductionFactor > 1.0) ? 1.0 : (this->ReductionFactor);

    if (this->ReductionFactor < 0.20)
    {
      this->ReductionFactor = 0.10;
    }
    else if (this->ReductionFactor < 0.50)
    {
      this->ReductionFactor = 0.20;
    }
    else if (this->ReductionFactor < 1.0)
    {
      this->ReductionFactor = 0.50;
    }

    // Clamp it
    if (1.0 / this->ReductionFactor > this->MaximumImageSampleDistance)
    {
      this->ReductionFactor = 1.0 / this->MaximumImageSampleDistance;
    }
    if (1.0 / this->ReductionFactor < this->MinimumImageSampleDistance)
    {
      this->ReductionFactor = 1.0 / this->MinimumImageSampleDistance;
    }
  }
}

//----------------------------------------------------------------------------
bool svtkOpenGLGPUVolumeRayCastMapper::PreLoadData(svtkRenderer* ren, svtkVolume* vol)
{
  if (!this->ValidateRender(ren, vol))
  {
    return false;
  }

  // have to register if we preload
  this->ResourceCallback->RegisterGraphicsResources(
    static_cast<svtkOpenGLRenderWindow*>(ren->GetSVTKWindow()));

  this->Impl->ClearRemovedInputs(ren->GetRenderWindow());
  return this->Impl->UpdateInputs(ren, vol);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ForceTransferInit()
{
  auto& inputs = this->Parent->AssembledInputs;
  auto fu = [](std::pair<const int, svtkVolumeInputHelper>& p) { p.second.ForceTransferInit(); };
  std::for_each(inputs.begin(), inputs.end(), fu);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ClearRemovedInputs(svtkWindow* win)
{
  bool orderChanged = false;
  for (const int& port : this->Parent->RemovedPorts)
  {
    auto it = this->Parent->AssembledInputs.find(port);
    if (it == this->Parent->AssembledInputs.cend())
    {
      continue;
    }

    auto input = (*it).second;
    input.Texture->ReleaseGraphicsResources(win);
    input.GradientOpacityTables->ReleaseGraphicsResources(win);
    input.OpacityTables->ReleaseGraphicsResources(win);
    input.RGBTables->ReleaseGraphicsResources(win);
    this->Parent->AssembledInputs.erase(it);
    orderChanged = true;
  }
  this->Parent->RemovedPorts.clear();

  if (orderChanged)
  {
    this->ForceTransferInit();
  }
}

//----------------------------------------------------------------------------
bool svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::UpdateInputs(svtkRenderer* ren, svtkVolume* vol)
{
  this->VolumePropertyChanged = false;
  bool orderChanged = false;
  bool success = true;
  for (const auto& port : this->Parent->Ports)
  {
    if (this->MultiVolume)
    {
      vol = this->MultiVolume->GetVolume(port);
    }
    auto property = vol->GetProperty();
    auto input = this->Parent->GetTransformedInput(port);

    // Check for property changes
    this->VolumePropertyChanged |= property->GetMTime() > this->ShaderBuildTime.GetMTime();

    auto it = this->Parent->AssembledInputs.find(port);
    if (it == this->Parent->AssembledInputs.cend())
    {
      // Create new input structure
      auto texture = svtkSmartPointer<svtkVolumeTexture>::New();

      VolumeInput currentInput(texture, vol);
      this->Parent->AssembledInputs[port] = std::move(currentInput);
      orderChanged = true;

      it = this->Parent->AssembledInputs.find(port);
    }
    assert(it != this->Parent->AssembledInputs.cend());

    /// TODO Currently, only input arrays with the same name/id/mode can be
    // (across input objects) can be rendered. This could be addressed by
    // overriding the mapper's settings with array settings defined in the
    // svtkMultiVolume instance.
    svtkDataArray* scalars =
      this->Parent->GetScalars(input, this->Parent->ScalarMode, this->Parent->ArrayAccessMode,
        this->Parent->ArrayId, this->Parent->ArrayName, this->Parent->CellFlag);

    if (this->NeedToInitializeResources || (input->GetMTime() > it->second.Texture->UploadTime) ||
      (scalars != it->second.Texture->GetLoadedScalars()) ||
      (scalars != nullptr && scalars->GetMTime() > it->second.Texture->UploadTime))
    {
      auto& volInput = this->Parent->AssembledInputs[port];
      auto volumeTex = volInput.Texture.GetPointer();
      volumeTex->SetPartitions(this->Partitions[0], this->Partitions[1], this->Partitions[2]);
      success &= volumeTex->LoadVolume(
        ren, input, scalars, this->Parent->CellFlag, property->GetInterpolationType());
      volInput.ComponentMode = this->GetComponentMode(property, scalars);
    }
    else
    {
      // Update svtkVolumeTexture
      it->second.Texture->UpdateVolume(property);
    }

    // Volume may have changed, so make sure the helper updates its reference to it.
    it->second.Volume = vol;
  }

  if (orderChanged)
  {
    this->ForceTransferInit();
  }

  return success;
}

int svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::GetComponentMode(
  svtkVolumeProperty* prop, svtkDataArray* array) const
{
  if (prop->GetIndependentComponents())
  {
    return VolumeInput::INDEPENDENT;
  }
  else
  {
    const auto numComp = array->GetNumberOfComponents();
    if (numComp == 1 || numComp == 2)
      return VolumeInput::LA;
    else if (numComp == 4)
      return VolumeInput::RGBA;
    else if (numComp == 3)
    {
      svtkGenericWarningMacro(<< "3 dependent components (e.g. RGB) are not supported."
                                "Only 2 (LA) and 4 (RGBA) supported.");
      return VolumeInput::INVALID;
    }
    else
      return VolumeInput::INVALID;
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::GPURender(svtkRenderer* ren, svtkVolume* vol)
{
  svtkOpenGLClearErrorMacro();

  svtkOpenGLCamera* cam = svtkOpenGLCamera::SafeDownCast(ren->GetActiveCamera());

  if (this->GetBlendMode() == svtkVolumeMapper::ISOSURFACE_BLEND &&
    vol->GetProperty()->GetIsoSurfaceValues()->GetNumberOfContours() == 0)
  {
    // Early exit: nothing to render.
    return;
  }

  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  this->ResourceCallback->RegisterGraphicsResources(static_cast<svtkOpenGLRenderWindow*>(renWin));
  // Make sure the context is current
  renWin->MakeCurrent();

  // Get window size and corners
  this->Impl->CheckPropertyKeys(vol);
  if (!this->Impl->PreserveViewport)
  {
    ren->GetTiledSizeAndOrigin(this->Impl->WindowSize, this->Impl->WindowSize + 1,
      this->Impl->WindowLowerLeft, this->Impl->WindowLowerLeft + 1);
  }
  else
  {
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    this->Impl->WindowLowerLeft[0] = vp[0];
    this->Impl->WindowLowerLeft[1] = vp[1];
    this->Impl->WindowSize[0] = vp[2];
    this->Impl->WindowSize[1] = vp[3];
  }

  this->Impl->NeedToInitializeResources =
    (this->Impl->ReleaseResourcesTime.GetMTime() > this->Impl->InitializationTime.GetMTime());

  this->ComputeReductionFactor(vol->GetAllocatedRenderTime());
  if (!this->Impl->SharedDepthTextureObject)
  {
    this->Impl->CaptureDepthTexture(ren);
  }

  svtkMTimeType renderPassTime = this->GetRenderPassStageMTime(vol);

  const auto multiVol = svtkMultiVolume::SafeDownCast(vol);
  this->Impl->MultiVolume = multiVol && this->GetInputCount() > 1 ? multiVol : nullptr;

  this->Impl->ClearRemovedInputs(renWin);
  this->Impl->UpdateInputs(ren, vol);
  this->Impl->UpdateSamplingDistance(ren);
  this->Impl->UpdateTransferFunctions(ren);

  // Masks are only supported on single-input rendring.
  if (!this->Impl->MultiVolume)
  {
    this->Impl->LoadMask(ren);
  }

  // Get the shader cache. This is important to make sure that shader cache
  // knows the state of various shader programs in use.
  this->Impl->ShaderCache =
    svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow())->GetShaderCache();

  this->Impl->CheckPickingState(ren);

  if (this->UseDepthPass && this->GetBlendMode() == svtkVolumeMapper::COMPOSITE_BLEND)
  {
    this->Impl->RenderWithDepthPass(ren, cam, renderPassTime);
  }
  else
  {
    if (this->Impl->IsPicking && !this->Impl->MultiVolume)
    {
      this->Impl->BeginPicking(ren);
    }
    svtkVolumeStateRAII glState(renWin->GetState(), this->Impl->PreserveGLState);

    if (this->Impl->ShaderRebuildNeeded(cam, vol, renderPassTime))
    {
      this->Impl->LastProjectionParallel = cam->GetParallelProjection();
      this->BuildShader(ren);
    }
    else
    {
      // Bind the shader
      this->Impl->ShaderCache->ReadyShaderProgram(this->Impl->ShaderProgram);
      this->InvokeEvent(svtkCommand::UpdateShaderEvent, this->Impl->ShaderProgram);
    }

    svtkOpenGLShaderProperty* shaderProperty =
      svtkOpenGLShaderProperty::SafeDownCast(vol->GetShaderProperty());
    if (this->RenderToImage)
    {
      this->Impl->SetupRenderToTexture(ren);
      this->Impl->SetRenderToImageParameters(this->Impl->ShaderProgram);
      this->DoGPURender(ren, cam, this->Impl->ShaderProgram, shaderProperty);
      this->Impl->ExitRenderToTexture(ren);
    }
    else
    {
      this->Impl->BeginImageSample(ren);
      this->DoGPURender(ren, cam, this->Impl->ShaderProgram, shaderProperty);
      this->Impl->EndImageSample(ren);
    }

    if (this->Impl->IsPicking && !this->Impl->MultiVolume)
    {
      this->Impl->EndPicking(ren);
    }
  }

  glFinish();
}

//----------------------------------------------------------------------------
bool svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::ShaderRebuildNeeded(
  svtkCamera* cam, svtkVolume* vol, svtkMTimeType renderPassTime)
{
  return (this->NeedToInitializeResources || this->VolumePropertyChanged ||
    vol->GetShaderProperty()->GetShaderMTime() > this->ShaderBuildTime.GetMTime() ||
    this->Parent->GetMTime() > this->ShaderBuildTime.GetMTime() ||
    cam->GetParallelProjection() != this->LastProjectionParallel ||
    this->SelectionStateTime.GetMTime() > this->ShaderBuildTime.GetMTime() ||
    renderPassTime > this->ShaderBuildTime);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::RenderWithDepthPass(
  svtkRenderer* ren, svtkOpenGLCamera* cam, svtkMTimeType renderPassTime)
{
  this->Parent->CurrentPass = DepthPass;
  auto& input = this->Parent->AssembledInputs[0];
  auto vol = input.Volume;
  auto volumeProperty = vol->GetProperty();
  auto shaderProperty = svtkOpenGLShaderProperty::SafeDownCast(vol->GetShaderProperty());

  if (this->NeedToInitializeResources ||
    volumeProperty->GetMTime() > this->DepthPassSetupTime.GetMTime() ||
    this->Parent->GetMTime() > this->DepthPassSetupTime.GetMTime() ||
    cam->GetParallelProjection() != this->LastProjectionParallel ||
    this->SelectionStateTime.GetMTime() > this->ShaderBuildTime.GetMTime() ||
    renderPassTime > this->ShaderBuildTime ||
    shaderProperty->GetShaderMTime() > this->ShaderBuildTime)
  {
    this->LastProjectionParallel = cam->GetParallelProjection();

    this->ContourFilter->SetInputData(this->Parent->GetTransformedInput(0));
    for (svtkIdType i = 0; i < this->Parent->GetDepthPassContourValues()->GetNumberOfContours(); ++i)
    {
      this->ContourFilter->SetValue(i, this->Parent->DepthPassContourValues->GetValue(i));
    }

    this->RenderContourPass(ren);
    this->DepthPassSetupTime.Modified();
    this->Parent->BuildShader(ren);
  }
  else if (cam->GetMTime() > this->DepthPassTime.GetMTime())
  {
    this->RenderContourPass(ren);
  }

  if (this->IsPicking)
  {
    this->BeginPicking(ren);
  }

  // Set OpenGL states
  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  svtkVolumeStateRAII glState(renWin->GetState(), this->PreserveGLState);

  if (this->Parent->RenderToImage)
  {
    this->SetupRenderToTexture(ren);
  }

  if (!this->PreserveViewport)
  {
    // NOTE: This is a must call or else, multiple viewport rendering would
    // not work. The glViewport could have been changed by any of the internal
    // FBOs (RenderToTexure, etc.).  The viewport should (ideally) not be set
    // within the mapper, because it could cause issues when svtkOpenGLRenderPass
    // instances modify it too (this is a workaround for that).
    renWin->GetState()->svtkglViewport(
      this->WindowLowerLeft[0], this->WindowLowerLeft[1], this->WindowSize[0], this->WindowSize[1]);
  }

  renWin->GetShaderCache()->ReadyShaderProgram(this->ShaderProgram);
  this->Parent->InvokeEvent(svtkCommand::UpdateShaderEvent, this->ShaderProgram);

  this->DPDepthBufferTextureObject->Activate();
  this->ShaderProgram->SetUniformi(
    "in_depthPassSampler", this->DPDepthBufferTextureObject->GetTextureUnit());
  this->Parent->DoGPURender(ren, cam, this->ShaderProgram, shaderProperty);
  this->DPDepthBufferTextureObject->Deactivate();

  if (this->IsPicking)
  {
    this->EndPicking(ren);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::BindTransformations(
  svtkShaderProgram* prog, svtkMatrix4x4* modelViewMat)
{
  // Bind transformations. Because the bounding box has its own transformations,
  // it is considered here as an actual volume (numInputs + 1).
  const int numInputs = static_cast<int>(this->Parent->AssembledInputs.size());
  const int numVolumes = this->MultiVolume ? numInputs + 1 : numInputs;

  this->VolMatVec.resize(numVolumes * 16, 0);
  this->InvMatVec.resize(numVolumes * 16, 0);
  this->TexMatVec.resize(numVolumes * 16, 0);
  this->InvTexMatVec.resize(numVolumes * 16, 0);
  this->TexEyeMatVec.resize(numVolumes * 16, 0);
  this->CellToPointVec.resize(numVolumes * 16, 0);
  this->TexMinVec.resize(numVolumes * 3, 0);
  this->TexMaxVec.resize(numVolumes * 3, 0);

  svtkNew<svtkMatrix4x4> dataToWorld, texToDataMat, texToViewMat, cellToPointMat;
  float defaultTexMin[3] = { 0.f, 0.f, 0.f };
  float defaultTexMax[3] = { 1.f, 1.f, 1.f };

  auto it = this->Parent->AssembledInputs.begin();
  for (int i = 0; i < numVolumes; i++)
  {
    const int vecOffset = i * 16;
    float *texMin, *texMax;

    if (this->MultiVolume && i == 0)
    {
      // Bounding box
      auto bBoxToWorld = this->MultiVolume->GetMatrix();
      dataToWorld->DeepCopy(bBoxToWorld);

      auto texToBBox = this->MultiVolume->GetTextureMatrix();
      texToDataMat->DeepCopy(texToBBox);

      cellToPointMat->Identity();
      texMin = defaultTexMin;
      texMax = defaultTexMax;
    }
    else
    {
      // Volume inputs
      auto& inputData = (*it).second;
      it++;
      auto volTex = inputData.Texture;
      auto volMatrix = inputData.Volume->GetMatrix();
      dataToWorld->DeepCopy(volMatrix);
      texToDataMat->DeepCopy(volTex->GetCurrentBlock()->TextureToDataset.GetPointer());

      // Texture matrices (texture to view)
      svtkMatrix4x4::Multiply4x4(volMatrix, texToDataMat.GetPointer(), texToViewMat.GetPointer());
      svtkMatrix4x4::Multiply4x4(modelViewMat, texToViewMat.GetPointer(), texToViewMat.GetPointer());

      // texToViewMat->Transpose();
      svtkInternal::CopyMatrixToVector<svtkMatrix4x4, 4, 4>(
        texToViewMat.GetPointer(), this->TexEyeMatVec.data(), vecOffset);

      // Cell to Point (texture-cells to texture-points)
      cellToPointMat->DeepCopy(volTex->CellToPointMatrix.GetPointer());
      texMin = volTex->AdjustedTexMin;
      texMax = volTex->AdjustedTexMax;
    }

    // Volume matrices (dataset to world)
    dataToWorld->Transpose();
    svtkInternal::CopyMatrixToVector<svtkMatrix4x4, 4, 4>(
      dataToWorld.GetPointer(), this->VolMatVec.data(), vecOffset);

    this->InverseVolumeMat->DeepCopy(dataToWorld.GetPointer());
    this->InverseVolumeMat->Invert();
    svtkInternal::CopyMatrixToVector<svtkMatrix4x4, 4, 4>(
      this->InverseVolumeMat.GetPointer(), this->InvMatVec.data(), vecOffset);

    // Texture matrices (texture to dataset)
    texToDataMat->Transpose();
    svtkInternal::CopyMatrixToVector<svtkMatrix4x4, 4, 4>(
      texToDataMat.GetPointer(), this->TexMatVec.data(), vecOffset);

    texToDataMat->Invert();
    svtkInternal::CopyMatrixToVector<svtkMatrix4x4, 4, 4>(
      texToDataMat.GetPointer(), this->InvTexMatVec.data(), vecOffset);

    // Cell to Point (texture adjustment)
    cellToPointMat->Transpose();
    svtkInternal::CopyMatrixToVector<svtkMatrix4x4, 4, 4>(
      cellToPointMat.GetPointer(), this->CellToPointVec.data(), vecOffset);
    svtkInternal::CopyVector<float, 3>(texMin, this->TexMinVec.data(), i * 3);
    svtkInternal::CopyVector<float, 3>(texMax, this->TexMaxVec.data(), i * 3);
  }

  // the matrix from data to world
  prog->SetUniformMatrix4x4v("in_volumeMatrix", numVolumes, this->VolMatVec.data());
  prog->SetUniformMatrix4x4v("in_inverseVolumeMatrix", numVolumes, this->InvMatVec.data());

  // the matrix from tcoords to data
  prog->SetUniformMatrix4x4v("in_textureDatasetMatrix", numVolumes, this->TexMatVec.data());
  prog->SetUniformMatrix4x4v(
    "in_inverseTextureDatasetMatrix", numVolumes, this->InvTexMatVec.data());

  // matrix from texture to view coordinates
  prog->SetUniformMatrix4x4v("in_textureToEye", numVolumes, this->TexEyeMatVec.data());

  // handle cell/point differences in tcoords
  prog->SetUniformMatrix4x4v("in_cellToPoint", numVolumes, this->CellToPointVec.data());

  prog->SetUniform3fv(
    "in_texMin", numVolumes, reinterpret_cast<const float(*)[3]>(this->TexMinVec.data()));
  prog->SetUniform3fv(
    "in_texMax", numVolumes, reinterpret_cast<const float(*)[3]>(this->TexMaxVec.data()));
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetVolumeShaderParameters(
  svtkShaderProgram* prog, int independentComponents, int noOfComponents, svtkMatrix4x4* modelViewMat)
{
  this->BindTransformations(prog, modelViewMat);

  // Bind other properties (per-input)
  const int numInputs = static_cast<int>(this->Parent->AssembledInputs.size());
  this->ScaleVec.resize(numInputs * 4, 0);
  this->BiasVec.resize(numInputs * 4, 0);
  this->StepVec.resize(numInputs * 3, 0);
  this->SpacingVec.resize(numInputs * 3, 0);
  this->RangeVec.resize(numInputs * 8, 0);

  int index = 0;
  for (auto& input : this->Parent->AssembledInputs)
  {
    // Bind volume textures
    auto block = input.second.Texture->GetCurrentBlock();
    std::stringstream ss;
    ss << "in_volume[" << index << "]";
    block->TextureObject->Activate();
    prog->SetUniformi(ss.str().c_str(), block->TextureObject->GetTextureUnit());

    // LargeDataTypes have been already biased and scaled so in those cases 0s
    // and 1s are passed respectively.
    float tscale[4] = { 1.0, 1.0, 1.0, 1.0 };
    float tbias[4] = { 0.0, 0.0, 0.0, 0.0 };
    float(*scalePtr)[4] = &tscale;
    float(*biasPtr)[4] = &tbias;
    auto volTex = input.second.Texture.GetPointer();
    if (!volTex->HandleLargeDataTypes &&
      (noOfComponents == 1 || noOfComponents == 2 || independentComponents))
    {
      scalePtr = &volTex->Scale;
      biasPtr = &volTex->Bias;
    }
    svtkInternal::CopyVector<float, 4>(*scalePtr, this->ScaleVec.data(), index * 4);
    svtkInternal::CopyVector<float, 4>(*biasPtr, this->BiasVec.data(), index * 4);
    svtkInternal::CopyVector<float, 3>(block->CellStep, this->StepVec.data(), index * 3);
    svtkInternal::CopyVector<float, 3>(volTex->CellSpacing, this->SpacingVec.data(), index * 3);

    // 8 elements stands for [min, max] per 4-components
    svtkInternal::CopyVector<float, 8>(
      reinterpret_cast<float*>(volTex->ScalarRange), this->RangeVec.data(), index * 8);

    input.second.ActivateTransferFunction(prog, this->Parent->BlendMode);
    index++;
  }
  prog->SetUniform4fv(
    "in_volume_scale", numInputs, reinterpret_cast<const float(*)[4]>(this->ScaleVec.data()));
  prog->SetUniform4fv(
    "in_volume_bias", numInputs, reinterpret_cast<const float(*)[4]>(this->BiasVec.data()));
  prog->SetUniform2fv(
    "in_scalarsRange", 4 * numInputs, reinterpret_cast<const float(*)[2]>(this->RangeVec.data()));
  prog->SetUniform3fv(
    "in_cellStep", numInputs, reinterpret_cast<const float(*)[3]>(this->StepVec.data()));
  prog->SetUniform3fv(
    "in_cellSpacing", numInputs, reinterpret_cast<const float(*)[3]>(this->SpacingVec.data()));
}

////----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetMapperShaderParameters(
  svtkShaderProgram* prog, svtkRenderer* ren, int independent, int numComp)
{
#ifndef GL_ES_VERSION_3_0
  // currently broken on ES
  if (!this->SharedDepthTextureObject)
  {
    this->DepthTextureObject->Activate();
  }
  prog->SetUniformi("in_depthSampler", this->DepthTextureObject->GetTextureUnit());
#endif

  if (this->Parent->GetUseJittering())
  {
    svtkOpenGLRenderWindow* win = static_cast<svtkOpenGLRenderWindow*>(ren->GetRenderWindow());
    prog->SetUniformi("in_noiseSampler", win->GetNoiseTextureUnit());
  }
  else
  {
    prog->SetUniformi("in_noiseSampler", 0);
  }

  prog->SetUniformi("in_useJittering", this->Parent->UseJittering);
  prog->SetUniformi("in_noOfComponents", numComp);
  prog->SetUniformi("in_independentComponents", independent);
  prog->SetUniformf("in_sampleDistance", this->ActualSampleDistance);

  // Set the scale and bias for color correction
  prog->SetUniformf("in_scale", 1.0 / this->Parent->FinalColorWindow);
  prog->SetUniformf(
    "in_bias", (0.5 - (this->Parent->FinalColorLevel / this->Parent->FinalColorWindow)));
}

////----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetCameraShaderParameters(
  svtkShaderProgram* prog, svtkRenderer* ren, svtkOpenGLCamera* cam)
{
  svtkMatrix4x4* glTransformMatrix;
  svtkMatrix4x4* modelViewMatrix;
  svtkMatrix3x3* normalMatrix;
  svtkMatrix4x4* projectionMatrix;
  cam->GetKeyMatrices(ren, modelViewMatrix, normalMatrix, projectionMatrix, glTransformMatrix);

  this->InverseProjectionMat->DeepCopy(projectionMatrix);
  this->InverseProjectionMat->Invert();
  prog->SetUniformMatrix("in_projectionMatrix", projectionMatrix);
  prog->SetUniformMatrix("in_inverseProjectionMatrix", this->InverseProjectionMat.GetPointer());

  this->InverseModelViewMat->DeepCopy(modelViewMatrix);
  this->InverseModelViewMat->Invert();
  prog->SetUniformMatrix("in_modelViewMatrix", modelViewMatrix);
  prog->SetUniformMatrix("in_inverseModelViewMatrix", this->InverseModelViewMat.GetPointer());

  float fvalue3[3];
  if (cam->GetParallelProjection())
  {
    double dir[4];
    cam->GetDirectionOfProjection(dir);
    svtkInternal::ToFloat(dir[0], dir[1], dir[2], fvalue3);
    prog->SetUniform3fv("in_projectionDirection", 1, &fvalue3);
  }

  svtkInternal::ToFloat(cam->GetPosition(), fvalue3, 3);
  prog->SetUniform3fv("in_cameraPos", 1, &fvalue3);

  // TODO Take consideration of reduction factor
  float fvalue2[2];
  svtkInternal::ToFloat(this->WindowLowerLeft, fvalue2);
  prog->SetUniform2fv("in_windowLowerLeftCorner", 1, &fvalue2);

  svtkInternal::ToFloat(1.0 / this->WindowSize[0], 1.0 / this->WindowSize[1], fvalue2);
  prog->SetUniform2fv("in_inverseOriginalWindowSize", 1, &fvalue2);

  svtkInternal::ToFloat(1.0 / this->WindowSize[0], 1.0 / this->WindowSize[1], fvalue2);
  prog->SetUniform2fv("in_inverseWindowSize", 1, &fvalue2);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetMaskShaderParameters(
  svtkShaderProgram* prog, svtkVolumeProperty* prop, int noOfComponents)
{
  if (this->CurrentMask)
  {
    auto maskTex = this->CurrentMask->GetCurrentBlock()->TextureObject;
    maskTex->Activate();
    prog->SetUniformi("in_mask", maskTex->GetTextureUnit());
  }

  if (noOfComponents == 1 && this->Parent->BlendMode != svtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
  {
    if (this->Parent->MaskInput != nullptr && this->Parent->MaskType == LabelMapMaskType)
    {
      this->LabelMapTransfer2D->Activate();
      prog->SetUniformi("in_labelMapTransfer", this->LabelMapTransfer2D->GetTextureUnit());
      if (prop->HasLabelGradientOpacity())
      {
        this->LabelMapGradientOpacity->Activate();
        prog->SetUniformi(
          "in_labelMapGradientOpacity", this->LabelMapGradientOpacity->GetTextureUnit());
      }
      prog->SetUniformf("in_maskBlendFactor", this->Parent->MaskBlendFactor);
      prog->SetUniformf("in_mask_scale", this->CurrentMask->Scale[0]);
      prog->SetUniformf("in_mask_bias", this->CurrentMask->Bias[0]);
      prog->SetUniformi("in_labelMapNumLabels", this->LabelMapTransfer2D->GetTextureHeight() - 1);
    }
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetRenderToImageParameters(
  svtkShaderProgram* prog)
{
  prog->SetUniformi("in_clampDepthToBackface", this->Parent->GetClampDepthToBackface());
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::SetAdvancedShaderParameters(svtkRenderer* ren,
  svtkShaderProgram* prog, svtkVolume* vol, svtkVolumeTexture::VolumeBlock* block, int numComp)
{
  // Cropping and clipping
  auto bounds = block->LoadedBoundsAA;
  this->SetCroppingRegions(prog, bounds);
  this->SetClippingPlanes(ren, prog, vol);

  // Picking
  if (this->CurrentSelectionPass < svtkHardwareSelector::POINT_ID_LOW24)
  {
    this->SetPickingId(ren);
  }

  auto blockExt = block->Extents;
  float fvalue3[3];
  svtkInternal::ToFloat(blockExt[0], blockExt[2], blockExt[4], fvalue3);
  prog->SetUniform3fv("in_textureExtentsMin", 1, &fvalue3);

  svtkInternal::ToFloat(blockExt[1], blockExt[3], blockExt[5], fvalue3);
  prog->SetUniform3fv("in_textureExtentsMax", 1, &fvalue3);

  // Component weights (independent components)
  auto volProperty = vol->GetProperty();
  float fvalue4[4];
  if (numComp > 1 && volProperty->GetIndependentComponents())
  {
    for (int i = 0; i < numComp; ++i)
    {
      fvalue4[i] = static_cast<float>(volProperty->GetComponentWeight(i));
    }
    prog->SetUniform4fv("in_componentWeight", 1, &fvalue4);
  }

  // Set the scalar range to be considered for average ip blend
  double avgRange[2];
  float fvalue2[2];
  this->Parent->GetAverageIPScalarRange(avgRange);
  if (avgRange[1] < avgRange[0])
  {
    double tmp = avgRange[1];
    avgRange[1] = avgRange[0];
    avgRange[0] = tmp;
  }
  svtkInternal::ToFloat(avgRange[0], avgRange[1], fvalue2);
  prog->SetUniform2fv("in_averageIPRange", 1, &fvalue2);

  // Set contour values for isosurface blend mode
  //--------------------------------------------------------------------------
  if (this->Parent->BlendMode == svtkVolumeMapper::ISOSURFACE_BLEND)
  {
    svtkIdType nbContours = volProperty->GetIsoSurfaceValues()->GetNumberOfContours();

    std::vector<float> values(nbContours);
    for (int i = 0; i < nbContours; i++)
    {
      values[i] = static_cast<float>(volProperty->GetIsoSurfaceValues()->GetValue(i));
    }

    // The shader expect (for efficiency purposes) the isovalues to be sorted.
    std::sort(values.begin(), values.end());

    prog->SetUniform1fv("in_isosurfacesValues", nbContours, values.data());
  }

  // Set function attributes for slice blend mode
  //--------------------------------------------------------------------------
  if (this->Parent->BlendMode == svtkVolumeMapper::SLICE_BLEND)
  {
    svtkPlane* plane = svtkPlane::SafeDownCast(volProperty->GetSliceFunction());

    if (plane)
    {
      double planeOrigin[3];
      double planeNormal[3];

      plane->GetOrigin(planeOrigin);
      plane->GetNormal(planeNormal);

      prog->SetUniform3f("in_slicePlaneOrigin", planeOrigin);
      prog->SetUniform3f("in_slicePlaneNormal", planeNormal);
    }
  }
}

void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::FinishRendering(const int numComp)
{
  for (auto& item : this->Parent->AssembledInputs)
  {
    auto& input = item.second;
    input.Texture->GetCurrentBlock()->TextureObject->Deactivate();
    input.DeactivateTransferFunction(this->Parent->BlendMode);
  }

#ifndef GL_ES_VERSION_3_0
  if (this->DepthTextureObject && !this->SharedDepthTextureObject)
  {
    this->DepthTextureObject->Deactivate();
  }
#endif

  if (this->CurrentMask)
  {
    this->CurrentMask->GetCurrentBlock()->TextureObject->Deactivate();
  }

  if (numComp == 1 && this->Parent->BlendMode != svtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
  {
    if (this->Parent->MaskInput != nullptr && this->Parent->MaskType == LabelMapMaskType)
    {
      this->LabelMapTransfer2D->Deactivate();
      this->LabelMapGradientOpacity->Deactivate();
    }
  }

  svtkOpenGLStaticCheckErrorMacro("Failed after FinishRendering!");
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::DoGPURender(svtkRenderer* ren, svtkOpenGLCamera* cam,
  svtkShaderProgram* prog, svtkOpenGLShaderProperty* shaderProperty)
{
  if (!prog)
  {
    return;
  }

  // Upload the value of user-defined uniforms in the program
  auto vu = static_cast<svtkOpenGLUniforms*>(shaderProperty->GetVertexCustomUniforms());
  vu->SetUniforms(prog);
  auto fu = static_cast<svtkOpenGLUniforms*>(shaderProperty->GetFragmentCustomUniforms());
  fu->SetUniforms(prog);
  auto gu = static_cast<svtkOpenGLUniforms*>(shaderProperty->GetGeometryCustomUniforms());
  gu->SetUniforms(prog);

  this->SetShaderParametersRenderPass();
  if (!this->Impl->MultiVolume)
  {
    this->Impl->RenderSingleInput(ren, cam, prog);
  }
  else
  {
    this->Impl->RenderMultipleInputs(ren, cam, prog);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::RenderMultipleInputs(
  svtkRenderer* ren, svtkOpenGLCamera* cam, svtkShaderProgram* prog)
{
  auto& input = this->Parent->AssembledInputs[0];
  auto vol = input.Volume;
  auto volumeTex = input.Texture.GetPointer();
  const int independent = vol->GetProperty()->GetIndependentComponents();
  const int numComp = volumeTex->GetLoadedScalars()->GetNumberOfComponents();
  int const numSamplers = (independent ? numComp : 1);
  auto geometry = this->MultiVolume->GetDataGeometry();

  svtkMatrix4x4 *wcvc, *vcdc, *wcdc;
  svtkMatrix3x3* norm;
  cam->GetKeyMatrices(ren, wcvc, norm, vcdc, wcdc);

  this->SetMapperShaderParameters(prog, ren, independent, numComp);
  this->SetVolumeShaderParameters(prog, independent, numComp, wcvc);
  this->SetLightingShaderParameters(ren, prog, this->MultiVolume, numSamplers);
  this->SetCameraShaderParameters(prog, ren, cam);
  this->RenderVolumeGeometry(ren, prog, this->MultiVolume, geometry);
  this->FinishRendering(numComp);
}

//----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::svtkInternal::RenderSingleInput(
  svtkRenderer* ren, svtkOpenGLCamera* cam, svtkShaderProgram* prog)
{
  auto& input = this->Parent->AssembledInputs[0];
  auto vol = input.Volume;
  auto volumeTex = input.Texture.GetPointer();

  // Sort blocks in case the viewpoint changed, it immediately returns if there
  // is a single block.
  volumeTex->SortBlocksBackToFront(ren, vol->GetMatrix());
  svtkVolumeTexture::VolumeBlock* block = volumeTex->GetCurrentBlock();

  if (this->CurrentMask)
  {
    this->CurrentMask->SortBlocksBackToFront(ren, vol->GetMatrix());
  }

  const int independent = vol->GetProperty()->GetIndependentComponents();
  const int numComp = volumeTex->GetLoadedScalars()->GetNumberOfComponents();
  while (block != nullptr)
  {
    const int numSamplers = (independent ? numComp : 1);
    this->SetMapperShaderParameters(prog, ren, independent, numComp);

    svtkMatrix4x4 *wcvc, *vcdc, *wcdc;
    svtkMatrix3x3* norm;
    cam->GetKeyMatrices(ren, wcvc, norm, vcdc, wcdc);
    this->SetVolumeShaderParameters(prog, independent, numComp, wcvc);

    this->SetMaskShaderParameters(prog, vol->GetProperty(), numComp);
    this->SetLightingShaderParameters(ren, prog, vol, numSamplers);
    this->SetCameraShaderParameters(prog, ren, cam);
    this->SetAdvancedShaderParameters(ren, prog, vol, block, numComp);

    this->RenderVolumeGeometry(ren, prog, vol, block->VolumeGeometry);

    this->FinishRendering(numComp);
    block = volumeTex->GetNextBlock();
    if (this->CurrentMask)
    {
      this->CurrentMask->GetNextBlock();
    }
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::SetPartitions(
  unsigned short x, unsigned short y, unsigned short z)
{
  this->Impl->Partitions[0] = x;
  this->Impl->Partitions[1] = y;
  this->Impl->Partitions[2] = z;
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkOpenGLGPUVolumeRayCastMapper::GetRenderPassStageMTime(svtkVolume* vol)
{
  svtkInformation* info = vol->GetPropertyKeys();
  svtkMTimeType renderPassMTime = 0;

  int curRenderPasses = 0;
  this->Impl->RenderPassAttached = false;
  if (info && info->Has(svtkOpenGLRenderPass::RenderPasses()))
  {
    curRenderPasses = info->Length(svtkOpenGLRenderPass::RenderPasses());
    this->Impl->RenderPassAttached = true;
  }

  int lastRenderPasses = 0;
  if (this->LastRenderPassInfo->Has(svtkOpenGLRenderPass::RenderPasses()))
  {
    lastRenderPasses = this->LastRenderPassInfo->Length(svtkOpenGLRenderPass::RenderPasses());
  }

  // Determine the last time a render pass changed stages:
  if (curRenderPasses != lastRenderPasses)
  {
    // Number of passes changed, definitely need to update.
    // Fake the time to force an update:
    renderPassMTime = SVTK_MTIME_MAX;
  }
  else
  {
    // Compare the current to the previous render passes:
    for (int i = 0; i < curRenderPasses; ++i)
    {
      svtkObjectBase* curRP = info->Get(svtkOpenGLRenderPass::RenderPasses(), i);
      svtkObjectBase* lastRP = this->LastRenderPassInfo->Get(svtkOpenGLRenderPass::RenderPasses(), i);

      if (curRP != lastRP)
      {
        // Render passes have changed. Force update:
        renderPassMTime = SVTK_MTIME_MAX;
        break;
      }
      else
      {
        // Render passes have not changed -- check MTime.
        svtkOpenGLRenderPass* rp = static_cast<svtkOpenGLRenderPass*>(curRP);
        renderPassMTime = std::max(renderPassMTime, rp->GetShaderStageMTime());
      }
    }
  }

  // Cache the current set of render passes for next time:
  if (info)
  {
    this->LastRenderPassInfo->CopyEntry(info, svtkOpenGLRenderPass::RenderPasses());
  }
  else
  {
    this->LastRenderPassInfo->Clear();
  }

  return renderPassMTime;
}

//-----------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderRenderPass(
  std::map<svtkShader::Type, svtkShader*>& shaders, svtkVolume* vol, bool prePass)
{
  std::string vertShader = shaders[svtkShader::Vertex]->GetSource();
  std::string geomShader = shaders[svtkShader::Geometry]->GetSource();
  std::string fragShader = shaders[svtkShader::Fragment]->GetSource();
  svtkInformation* info = vol->GetPropertyKeys();
  if (info && info->Has(svtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(svtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      svtkObjectBase* rpBase = info->Get(svtkOpenGLRenderPass::RenderPasses(), i);
      svtkOpenGLRenderPass* rp = static_cast<svtkOpenGLRenderPass*>(rpBase);
      if (prePass)
      {
        if (!rp->PreReplaceShaderValues(vertShader, geomShader, fragShader, this, vol))
        {
          svtkErrorMacro(
            "svtkOpenGLRenderPass::PreReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
      else
      {
        if (!rp->PostReplaceShaderValues(vertShader, geomShader, fragShader, this, vol))
        {
          svtkErrorMacro(
            "svtkOpenGLRenderPass::PostReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
    }
  }
  shaders[svtkShader::Vertex]->SetSource(vertShader);
  shaders[svtkShader::Geometry]->SetSource(geomShader);
  shaders[svtkShader::Fragment]->SetSource(fragShader);
}

//------------------------------------------------------------------------------
void svtkOpenGLGPUVolumeRayCastMapper::SetShaderParametersRenderPass()
{
  auto vol = this->Impl->GetActiveVolume();
  svtkInformation* info = vol->GetPropertyKeys();
  if (info && info->Has(svtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(svtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      svtkObjectBase* rpBase = info->Get(svtkOpenGLRenderPass::RenderPasses(), i);
      svtkOpenGLRenderPass* rp = static_cast<svtkOpenGLRenderPass*>(rpBase);
      if (!rp->SetShaderParameters(this->Impl->ShaderProgram, this, vol))
      {
        svtkErrorMacro(
          "RenderPass::SetShaderParameters failed for renderpass: " << rp->GetClassName());
      }
    }
  }
}
