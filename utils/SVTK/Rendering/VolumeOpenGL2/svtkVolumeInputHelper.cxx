#include <sstream>

#include "svtkColorTransferFunction.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkOpenGLGPUVolumeRayCastMapper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLVolumeGradientOpacityTable.h"
#include "svtkOpenGLVolumeOpacityTable.h"
#include "svtkOpenGLVolumeRGBTable.h"
#include "svtkOpenGLVolumeTransferFunction2D.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"
#include "svtkVolume.h"
#include "svtkVolumeInputHelper.h"
#include "svtkVolumeProperty.h"
#include "svtkVolumeTexture.h"

svtkVolumeInputHelper::svtkVolumeInputHelper(svtkSmartPointer<svtkVolumeTexture> tex, svtkVolume* vol)
  : Texture(tex)
  , Volume(vol)
{
}

void svtkVolumeInputHelper::RefreshTransferFunction(
  svtkRenderer* ren, const int uniformIndex, const int blendMode, const float samplingDist)
{
  if (this->InitializeTransfer ||
    this->Volume->GetProperty()->GetMTime() > this->LutInit.GetMTime())
  {
    this->InitializeTransferFunction(ren, uniformIndex);
  }
  this->UpdateTransferFunctions(ren, blendMode, samplingDist);
}

void svtkVolumeInputHelper::InitializeTransferFunction(svtkRenderer* ren, const int index)
{
  const int transferMode = this->Volume->GetProperty()->GetTransferFunctionMode();
  switch (transferMode)
  {
    case svtkVolumeProperty::TF_2D:
      this->CreateTransferFunction2D(ren, index);
      break;

    case svtkVolumeProperty::TF_1D:
    default:
      this->CreateTransferFunction1D(ren, index);
  }
  this->InitializeTransfer = false;
}

void svtkVolumeInputHelper::UpdateTransferFunctions(
  svtkRenderer* ren, const int blendMode, const float samplingDist)
{
  auto vol = this->Volume;
  const int transferMode = vol->GetProperty()->GetTransferFunctionMode();
  const int numComp = this->Texture->GetLoadedScalars()->GetNumberOfComponents();
  switch (transferMode)
  {
    case svtkVolumeProperty::TF_1D:
      switch (this->ComponentMode)
      {
        case svtkVolumeInputHelper::INDEPENDENT:
          for (int i = 0; i < numComp; ++i)
          {
            this->UpdateOpacityTransferFunction(ren, vol, i, blendMode, samplingDist);
            this->UpdateGradientOpacityTransferFunction(ren, vol, i, samplingDist);
            this->UpdateColorTransferFunction(ren, vol, i);
          }
          break;
        default: // RGBA or LA
          this->UpdateOpacityTransferFunction(ren, vol, numComp - 1, blendMode, samplingDist);
          this->UpdateGradientOpacityTransferFunction(ren, vol, numComp - 1, samplingDist);
          this->UpdateColorTransferFunction(ren, vol, 0);
      }
      break;

    case svtkVolumeProperty::TF_2D:
      switch (this->ComponentMode)
      {
        case svtkVolumeInputHelper::INDEPENDENT:
          for (int i = 0; i < numComp; ++i)
          {
            this->UpdateTransferFunction2D(ren, i);
          }
          break;
        default: // RGBA or LA
          this->UpdateTransferFunction2D(ren, 0);
      }
      break;
  }
}

int svtkVolumeInputHelper::UpdateOpacityTransferFunction(svtkRenderer* ren, svtkVolume* vol,
  unsigned int component, const int blendMode, const float samplingDist)
{
  svtkVolumeProperty* volumeProperty = vol->GetProperty();

  // Use the first LUT when using dependent components
  unsigned int lookupTableIndex = volumeProperty->GetIndependentComponents() ? component : 0;
  svtkPiecewiseFunction* scalarOpacity = volumeProperty->GetScalarOpacity(lookupTableIndex);

  auto volumeTex = this->Texture.GetPointer();
  double componentRange[2];
  if (scalarOpacity->GetSize() < 1 ||
    this->ScalarOpacityRangeType == svtkGPUVolumeRayCastMapper::SCALAR)
  {
    for (int i = 0; i < 2; ++i)
    {
      componentRange[i] = volumeTex->ScalarRange[component][i];
    }
  }
  else
  {
    scalarOpacity->GetRange(componentRange);
  }

  if (scalarOpacity->GetSize() < 1)
  {
    scalarOpacity->AddPoint(componentRange[0], 0.0);
    scalarOpacity->AddPoint(componentRange[1], 0.5);
  }

  int filterVal = volumeProperty->GetInterpolationType() == SVTK_LINEAR_INTERPOLATION
    ? svtkTextureObject::Linear
    : svtkTextureObject::Nearest;

  this->OpacityTables->GetTable(lookupTableIndex)
    ->Update(scalarOpacity, componentRange, blendMode, samplingDist,
      volumeProperty->GetScalarOpacityUnitDistance(component),
#ifndef GL_ES_VERSION_3_0
      filterVal,
#else
      svtkTextureObject::Nearest,
#endif
      svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  return 0;
}

int svtkVolumeInputHelper::UpdateColorTransferFunction(
  svtkRenderer* ren, svtkVolume* vol, unsigned int component)
{
  svtkVolumeProperty* volumeProperty = vol->GetProperty();

  // Build the colormap in a 1D texture. 1D RGB-texture-mapping from scalar
  // values to color values build the table.
  svtkColorTransferFunction* colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(component);

  auto volumeTex = this->Texture.GetPointer();
  double componentRange[2];
  if (colorTransferFunction->GetSize() < 1 ||
    this->ColorRangeType == svtkGPUVolumeRayCastMapper::SCALAR)
  {
    for (int i = 0; i < 2; ++i)
    {
      componentRange[i] = volumeTex->ScalarRange[component][i];
    }
  }
  else
  {
    colorTransferFunction->GetRange(componentRange);
  }

  // Add points only if its not being added before
  if (colorTransferFunction->GetSize() < 1)
  {
    colorTransferFunction->AddRGBPoint(componentRange[0], 0.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(componentRange[1], 1.0, 1.0, 1.0);
  }

  int filterVal = volumeProperty->GetInterpolationType() == SVTK_LINEAR_INTERPOLATION
    ? svtkTextureObject::Linear
    : svtkTextureObject::Nearest;

  this->RGBTables->GetTable(component)->Update(volumeProperty->GetRGBTransferFunction(component),
    componentRange, 0, 0, 0,
#ifndef GL_ES_VERSION_3_0
    filterVal,
#else
    svtkTextureObject::Nearest,
#endif
    svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  return 0;
}

int svtkVolumeInputHelper::UpdateGradientOpacityTransferFunction(
  svtkRenderer* ren, svtkVolume* vol, unsigned int component, const float samplingDist)
{
  svtkVolumeProperty* volumeProperty = vol->GetProperty();

  // Use the first LUT when using dependent components
  unsigned int lookupTableIndex = volumeProperty->GetIndependentComponents() ? component : 0;

  if (!volumeProperty->HasGradientOpacity(lookupTableIndex) || !this->GradientOpacityTables)
  {
    return 1;
  }

  svtkPiecewiseFunction* gradientOpacity = volumeProperty->GetGradientOpacity(lookupTableIndex);

  auto volumeTex = this->Texture.GetPointer();
  double componentRange[2];
  if (gradientOpacity->GetSize() < 1 ||
    this->GradientOpacityRangeType == svtkGPUVolumeRayCastMapper::SCALAR)
  {
    for (int i = 0; i < 2; ++i)
    {
      componentRange[i] = volumeTex->ScalarRange[component][i];
    }
  }
  else
  {
    gradientOpacity->GetRange(componentRange);
  }

  if (gradientOpacity->GetSize() < 1)
  {
    gradientOpacity->AddPoint(componentRange[0], 0.0);
    gradientOpacity->AddPoint(componentRange[1], 0.5);
  }

  int filterVal = volumeProperty->GetInterpolationType() == SVTK_LINEAR_INTERPOLATION
    ? svtkTextureObject::Linear
    : svtkTextureObject::Nearest;

  this->GradientOpacityTables->GetTable(lookupTableIndex)
    ->Update(gradientOpacity, componentRange, 0, samplingDist,
      volumeProperty->GetScalarOpacityUnitDistance(component),
#ifndef GL_ES_VERSION_3_0
      filterVal,
#else
      svtkTextureObject::Nearest,
#endif
      svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  return 0;
}

void svtkVolumeInputHelper::UpdateTransferFunction2D(svtkRenderer* ren, unsigned int component)
{
  // Use the first LUT when using dependent components
  svtkVolumeProperty* prop = this->Volume->GetProperty();
  unsigned int const lutIndex = prop->GetIndependentComponents() ? component : 0;

  svtkImageData* transfer2D = prop->GetTransferFunction2D(lutIndex);
#ifndef GL_ES_VERSION_3_0
  int const interp = prop->GetInterpolationType() == SVTK_LINEAR_INTERPOLATION
    ? svtkTextureObject::Linear
    : svtkTextureObject::Nearest;
#else
  int const interp = svtkTextureObject::Nearest;
#endif

  double scalarRange[2] = { 0, 1 };
  this->TransferFunctions2D->GetTable(lutIndex)->Update(transfer2D, scalarRange, 0, 0, 0, interp,
    svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
}

void svtkVolumeInputHelper::ActivateTransferFunction(svtkShaderProgram* prog, const int blendMode)
{
  int const transferMode = this->Volume->GetProperty()->GetTransferFunctionMode();
  int const numActiveLuts =
    this->ComponentMode == INDEPENDENT ? Texture->GetLoadedScalars()->GetNumberOfComponents() : 1;
  switch (transferMode)
  {
    case svtkVolumeProperty::TF_1D:
      for (int i = 0; i < numActiveLuts; ++i)
      {
        this->OpacityTables->GetTable(i)->Activate();
        prog->SetUniformi(
          this->OpacityTablesMap[i].c_str(), this->OpacityTables->GetTable(i)->GetTextureUnit());

        if (blendMode != svtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
        {
          this->RGBTables->GetTable(i)->Activate();
          prog->SetUniformi(
            this->RGBTablesMap[i].c_str(), this->RGBTables->GetTable(i)->GetTextureUnit());
        }

        if (this->GradientOpacityTables)
        {
          this->GradientOpacityTables->GetTable(i)->Activate();
          prog->SetUniformi(this->GradientOpacityTablesMap[i].c_str(),
            this->GradientOpacityTables->GetTable(i)->GetTextureUnit());
        }
      }
      break;
    case svtkVolumeProperty::TF_2D:
      for (int i = 0; i < numActiveLuts; ++i)
      {
        svtkOpenGLVolumeTransferFunction2D* table = this->TransferFunctions2D->GetTable(i);
        table->Activate();
        prog->SetUniformi(this->TransferFunctions2DMap[i].c_str(), table->GetTextureUnit());
      }
      break;
  }
}

void svtkVolumeInputHelper::DeactivateTransferFunction(const int blendMode)
{
  int const transferMode = this->Volume->GetProperty()->GetTransferFunctionMode();
  int const numActiveLuts =
    this->ComponentMode == INDEPENDENT ? Texture->GetLoadedScalars()->GetNumberOfComponents() : 1;
  switch (transferMode)
  {
    case svtkVolumeProperty::TF_1D:
      for (int i = 0; i < numActiveLuts; ++i)
      {
        this->OpacityTables->GetTable(i)->Deactivate();
        if (blendMode != svtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
        {
          this->RGBTables->GetTable(i)->Deactivate();
        }
        if (this->GradientOpacityTables)
        {
          this->GradientOpacityTables->GetTable(i)->Deactivate();
        }
      }
      break;
    case svtkVolumeProperty::TF_2D:
      for (int i = 0; i < numActiveLuts; ++i)
      {
        this->TransferFunctions2D->GetTable(i)->Deactivate();
      }
      break;
  }
}

void svtkVolumeInputHelper::CreateTransferFunction1D(svtkRenderer* ren, const int index)
{
  this->ReleaseGraphicsTransfer1D(ren->GetRenderWindow());

  int const numActiveLuts =
    this->ComponentMode == INDEPENDENT ? Texture->GetLoadedScalars()->GetNumberOfComponents() : 1;

  // Create RGB and opacity (scalar and gradient) lookup tables. Up to four
  // components are supported in single-input independentComponents mode.
  this->RGBTables = svtkSmartPointer<svtkOpenGLVolumeLookupTables<svtkOpenGLVolumeRGBTable> >::New();
  this->RGBTables->Create(numActiveLuts);
  this->OpacityTables =
    svtkSmartPointer<svtkOpenGLVolumeLookupTables<svtkOpenGLVolumeOpacityTable> >::New();
  this->OpacityTables->Create(numActiveLuts);
  this->GradientOpacityTables =
    svtkSmartPointer<svtkOpenGLVolumeLookupTables<svtkOpenGLVolumeGradientOpacityTable> >::New();
  this->GradientOpacityTables->Create(numActiveLuts);

  this->OpacityTablesMap.clear();
  this->RGBTablesMap.clear();
  this->GradientOpacityTablesMap.clear();

  std::ostringstream idx;
  idx << index;

  this->GradientCacheName = "g_gradients_" + idx.str();

  for (int i = 0; i < numActiveLuts; ++i)
  {
    std::ostringstream comp;
    comp << "[" << i << "]";

    this->OpacityTablesMap[i] = "in_opacityTransferFunc_" + idx.str() + comp.str();
    this->RGBTablesMap[i] = "in_colorTransferFunc_" + idx.str() + comp.str();

    // Unlike color and scalar-op, graident-op is optional (some inputs may
    // or may not have gradient-op active).
    if (this->Volume->GetProperty()->HasGradientOpacity())
    {
      this->GradientOpacityTablesMap[i] = "in_gradientTransferFunc_" + idx.str() + comp.str();
    }
  }

  this->LutInit.Modified();
}

void svtkVolumeInputHelper::CreateTransferFunction2D(svtkRenderer* ren, const int index)
{
  this->ReleaseGraphicsTransfer2D(ren->GetRenderWindow());

  unsigned int const num =
    this->ComponentMode == INDEPENDENT ? Texture->GetLoadedScalars()->GetNumberOfComponents() : 1;

  this->TransferFunctions2D =
    svtkSmartPointer<svtkOpenGLVolumeLookupTables<svtkOpenGLVolumeTransferFunction2D> >::New();
  this->TransferFunctions2D->Create(num);

  std::ostringstream idx;
  idx << index;

  this->GradientCacheName = "g_gradients_" + idx.str();

  for (unsigned int i = 0; i < num; i++)
  {
    std::ostringstream comp;
    comp << "[" << i << "]";

    this->TransferFunctions2DMap[i] = "in_transfer2D_" + idx.str() + comp.str();
  }

  this->LutInit.Modified();
}

void svtkVolumeInputHelper::ReleaseGraphicsResources(svtkWindow* window)
{
  this->ReleaseGraphicsTransfer1D(window);
  this->ReleaseGraphicsTransfer2D(window);
  this->Texture->ReleaseGraphicsResources(window);
  this->InitializeTransfer = true;
}

void svtkVolumeInputHelper::ReleaseGraphicsTransfer1D(svtkWindow* window)
{
  if (this->RGBTables)
  {
    this->RGBTables->ReleaseGraphicsResources(window);
  }
  this->RGBTables = nullptr;

  if (this->OpacityTables)
  {
    this->OpacityTables->ReleaseGraphicsResources(window);
  }
  this->OpacityTables = nullptr;

  if (this->GradientOpacityTables)
  {
    this->GradientOpacityTables->ReleaseGraphicsResources(window);
  }
  this->GradientOpacityTables = nullptr;
}

void svtkVolumeInputHelper::ReleaseGraphicsTransfer2D(svtkWindow* window)
{
  if (this->TransferFunctions2D)
  {
    this->TransferFunctions2D->ReleaseGraphicsResources(window);
  }
  this->TransferFunctions2D = nullptr;
}

void svtkVolumeInputHelper::ForceTransferInit()
{
  this->InitializeTransfer = true;
}
