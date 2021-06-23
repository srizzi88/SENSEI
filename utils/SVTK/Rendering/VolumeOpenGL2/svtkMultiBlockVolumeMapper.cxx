/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockVolumeMapper.cxx

  copyright (c) ken martin, will schroeder, bill lorensen
  all rights reserved.
  see copyright.txt or http://www.kitware.com/copyright.htm for details.

  this software is distributed without any warranty; without even
  the implied warranty of merchantability or fitness for a particular
  purpose.  see the above copyright notice for more information.

=========================================================================*/
#include <algorithm>

#include "svtkBlockSortHelper.h"
#include "svtkBoundingBox.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiBlockVolumeMapper.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLGPUVolumeRayCastMapper.h"
#include "svtkPerlinNoise.h"
#include "svtkRenderWindow.h"
#include "svtkSmartVolumeMapper.h"

//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkMultiBlockVolumeMapper);

//------------------------------------------------------------------------------
svtkMultiBlockVolumeMapper::svtkMultiBlockVolumeMapper()
  : FallBackMapper(nullptr)
  , BlockLoadingTime(0)
  , BoundsComputeTime(0)
  , VectorMode(svtkSmartVolumeMapper::DISABLED)
  , VectorComponent(0)
  , RequestedRenderMode(svtkSmartVolumeMapper::DefaultRenderMode)
{
}

//------------------------------------------------------------------------------
svtkMultiBlockVolumeMapper::~svtkMultiBlockVolumeMapper()
{
  this->ClearMappers();
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::Render(svtkRenderer* ren, svtkVolume* vol)
{
  svtkDataObject* dataObj = this->GetDataObjectInput();
  if (dataObj->GetMTime() != this->BlockLoadingTime)
  {
    svtkDebugMacro("Reloading data blocks!");
    this->LoadDataSet(ren, vol);
    this->BlockLoadingTime = dataObj->GetMTime();
  }

  this->SortMappers(ren, vol->GetMatrix());

  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    if (this->FallBackMapper)
    {
      svtkImageData* image = (*it)->GetInput();
      image->Modified();
      this->FallBackMapper->SetInputData(image);
      this->FallBackMapper->Render(ren, vol);
      continue;
    }

    (*it)->Render(ren, vol);
  }
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SortMappers(svtkRenderer* ren, svtkMatrix4x4* volumeMat)
{
  svtkBlockSortHelper::BackToFront<svtkVolumeMapper> sortMappers(ren, volumeMat);
  std::sort(this->Mappers.begin(), this->Mappers.end(), sortMappers);
}

//------------------------------------------------------------------------------
double* svtkMultiBlockVolumeMapper::GetBounds()
{
  if (!this->GetDataObjectTreeInput())
  {
    return this->Superclass::GetBounds();
  }
  else
  {
    this->Update();
    this->ComputeBounds();
    return this->Bounds;
  }
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::ComputeBounds()
{
  auto input = this->GetDataObjectTreeInput();
  assert(input != nullptr);
  if (input->GetMTime() == this->BoundsComputeTime)
  {
    // don't need to recompute bounds.
    return;
  }

  // Loop over the hierarchy of data objects to compute bounds
  svtkBoundingBox bbox;
  svtkCompositeDataIterator* iter = input->NewIterator();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (svtkImageData* img = svtkImageData::SafeDownCast(iter->GetCurrentDataObject()))
      if (img)
      {
        double bds[6];
        img->GetBounds(bds);
        bbox.AddBounds(bds);
      }
  }
  iter->Delete();

  svtkMath::UninitializeBounds(this->Bounds);
  if (bbox.IsValid())
  {
    bbox.GetBounds(this->Bounds);
  }

  this->BoundsComputeTime = input->GetMTime();
}

//------------------------------------------------------------------------------
svtkDataObjectTree* svtkMultiBlockVolumeMapper::GetDataObjectTreeInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkDataObjectTree::SafeDownCast(this->GetInputDataObject(0, 0));
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::LoadDataSet(svtkRenderer* ren, svtkVolume* vol)
{
  this->ClearMappers();

  auto input = this->GetDataObjectInput();
  if (auto inputTree = svtkDataObjectTree::SafeDownCast(input))
  {
    this->CreateMappers(inputTree, ren, vol);
  }
  else if (auto inputImage = svtkImageData::SafeDownCast(input))
  {
    svtkSmartVolumeMapper* mapper = this->CreateMapper();
    mapper->SetInputData(inputImage);
    this->Mappers.push_back(mapper);
  }
  else
  {
    svtkErrorMacro(
      "Cannot handle input of type '" << (input ? input->GetClassName() : "(nullptr)") << "'.");
  }
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::CreateMappers(
  svtkDataObjectTree* input, svtkRenderer* ren, svtkVolume* vol)
{
  // Hierarchical case
  svtkCompositeDataIterator* it = input->NewIterator();
  it->GoToFirstItem();

  bool warnedOnce = false;
  bool allBlocksLoaded = true;
  while (!it->IsDoneWithTraversal())
  {
    svtkImageData* currentIm = svtkImageData::SafeDownCast(it->GetCurrentDataObject());
    if (!warnedOnce && !currentIm)
    {
      svtkErrorMacro("At least one block in the data object is not of type"
                    " svtkImageData.  These blocks will be ignored.");
      warnedOnce = true;
      it->GoToNextItem();
      continue;
    }

    svtkSmartVolumeMapper* mapper = this->CreateMapper();
    this->Mappers.push_back(mapper);

    svtkImageData* im = svtkImageData::New();
    im->ShallowCopy(currentIm);
    mapper->SetInputData(im);

    // Try allocating GPU memory only while succeeding
    if (allBlocksLoaded)
    {
      svtkOpenGLGPUVolumeRayCastMapper* glMapper =
        svtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper->GetGPUMapper());

      if (glMapper)
      {
        svtkImageData* imageInternal = svtkImageData::New();
        imageInternal->ShallowCopy(currentIm);

        glMapper->SetInputData(imageInternal);
        glMapper->SelectScalarArray(this->ArrayName);
        glMapper->SelectScalarArray(this->ArrayId);
        glMapper->SetScalarMode(this->ScalarMode);
        glMapper->SetArrayAccessMode(this->ArrayAccessMode);

        allBlocksLoaded &= glMapper->PreLoadData(ren, vol);
        imageInternal->Delete();
      }
    }
    im->Delete();
    it->GoToNextItem();
  }
  it->Delete();

  // If loading all of the blocks failed, fall back to using a single mapper.
  // Use a separate instance in order to keep using the Mappers vector for
  // sorting.
  if (!allBlocksLoaded)
  {
    svtkRenderWindow* win = ren->GetRenderWindow();
    this->ReleaseGraphicsResources(win);

    this->FallBackMapper = this->CreateMapper();
  }
}

//------------------------------------------------------------------------------
svtkSmartVolumeMapper* svtkMultiBlockVolumeMapper::CreateMapper()
{
  svtkSmartVolumeMapper* mapper = svtkSmartVolumeMapper::New();

  mapper->SetRequestedRenderMode(this->RequestedRenderMode);
  mapper->SelectScalarArray(this->ArrayName);
  mapper->SelectScalarArray(this->ArrayId);
  mapper->SetScalarMode(this->ScalarMode);
  mapper->SetArrayAccessMode(this->ArrayAccessMode);
  mapper->SetVectorMode(this->VectorMode);
  mapper->SetVectorComponent(this->VectorComponent);
  mapper->SetBlendMode(this->GetBlendMode());
  mapper->SetCropping(this->GetCropping());
  mapper->SetCroppingRegionFlags(this->GetCroppingRegionFlags());
  mapper->SetCroppingRegionPlanes(this->GetCroppingRegionPlanes());

  svtkOpenGLGPUVolumeRayCastMapper* glMapper =
    svtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper->GetGPUMapper());

  if (glMapper != nullptr)
  {
    glMapper->UseJitteringOn();
  }
  return mapper;
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::ReleaseGraphicsResources(svtkWindow* window)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->ReleaseGraphicsResources(window);
  }

  if (this->FallBackMapper)
  {
    this->FallBackMapper->ReleaseGraphicsResources(window);
  }
}

//------------------------------------------------------------------------------
int svtkMultiBlockVolumeMapper::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObjectTree");
  return 1;
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << "Number Of Mappers: " << this->Mappers.size() << "\n";
  os << "BlockLoadingTime: " << this->BlockLoadingTime << "\n";
  os << "BoundsComputeTime: " << this->BoundsComputeTime << "\n";
  os << "VectorMode: " << this->VectorMode << "\n";
  os << "VectorComponent: " << this->VectorComponent << "\n";
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::ClearMappers()
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->Delete();
  }
  this->Mappers.clear();

  if (this->FallBackMapper)
  {
    this->FallBackMapper->Delete();
    this->FallBackMapper = nullptr;
  }
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SelectScalarArray(int arrayNum)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SelectScalarArray(arrayNum);
  }
  Superclass::SelectScalarArray(arrayNum);
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SelectScalarArray(char const* arrayName)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SelectScalarArray(arrayName);
  }
  Superclass::SelectScalarArray(arrayName);
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetScalarMode(int scalarMode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetScalarMode(scalarMode);
  }
  Superclass::SetScalarMode(scalarMode);
}

//------------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetArrayAccessMode(int accessMode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetArrayAccessMode(accessMode);
  }
  Superclass::SetArrayAccessMode(accessMode);
}

//----------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetBlendMode(int mode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetBlendMode(mode);
  }
  Superclass::SetBlendMode(mode);
}

//----------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetCropping(svtkTypeBool mode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetCropping(mode);
  }
  Superclass::SetCropping(mode);
}

//----------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetCroppingRegionFlags(int mode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetCroppingRegionFlags(mode);
  }
  Superclass::SetCroppingRegionFlags(mode);
}

//----------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetCroppingRegionPlanes(const double* planes)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetCroppingRegionPlanes(
      planes[0], planes[1], planes[2], planes[3], planes[4], planes[5]);
  }
  Superclass::SetCroppingRegionPlanes(planes);
}

//----------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetCroppingRegionPlanes(
  double arg1, double arg2, double arg3, double arg4, double arg5, double arg6)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetCroppingRegionPlanes(arg1, arg2, arg3, arg4, arg5, arg6);
  }
  Superclass::SetCroppingRegionPlanes(arg1, arg2, arg3, arg4, arg5, arg6);
}

//----------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetVectorMode(int mode)
{
  if (this->VectorMode != mode)
  {
    MapperVec::const_iterator end = this->Mappers.end();
    for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
    {
      (*it)->SetVectorMode(mode);
    }
    this->VectorMode = mode;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetVectorComponent(int component)
{
  if (this->VectorComponent != component)
  {
    MapperVec::const_iterator end = this->Mappers.end();
    for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
    {
      (*it)->SetVectorComponent(component);
    }
    this->VectorComponent = component;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkMultiBlockVolumeMapper::SetRequestedRenderMode(int mode)
{
  if (this->RequestedRenderMode != mode)
  {
    for (auto& mapper : this->Mappers)
    {
      mapper->SetRequestedRenderMode(mode);
    }
    this->RequestedRenderMode = mode;
    this->Modified();
  }
}
