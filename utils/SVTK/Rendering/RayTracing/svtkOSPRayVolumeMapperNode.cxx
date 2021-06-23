/*=========================================================================

   Program:   Visualization Toolkit
   Module:    svtkOSPRayVolumeMapperNode.cxx

   Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
   All rights reserved.
   See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

      This software is distributed WITHOUT ANY WARRANTY; without even
      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
      PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkOSPRayVolumeMapperNode.h"

#include "svtkCellData.h"
#include "svtkColorTransferFunction.h"
#include "svtkContourValues.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkOSPRayCache.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkObjectFactory.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkRenderer.h"
#include "svtkVolume.h"
#include "svtkVolumeMapper.h"
#include "svtkVolumeNode.h"
#include "svtkVolumeProperty.h"

#include <algorithm>
#include <vector>

//============================================================================
svtkStandardNewMacro(svtkOSPRayVolumeMapperNode);

//----------------------------------------------------------------------------
svtkOSPRayVolumeMapperNode::svtkOSPRayVolumeMapperNode()
{
  this->SamplingRate = 0.0;
  this->SamplingStep = 1.0;
  this->NumColors = 128;
  this->OSPRayVolume = nullptr;
  this->TransferFunction = nullptr;
  this->Cache = new svtkOSPRayCache<svtkOSPRayCacheItemObject>;
  this->UseSharedBuffers = false;
  this->SharedData = nullptr;
  this->Shade = false;
}

//----------------------------------------------------------------------------
svtkOSPRayVolumeMapperNode::~svtkOSPRayVolumeMapperNode()
{
  svtkOSPRayRendererNode* orn =
    static_cast<svtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("svtkOSPRayRendererNode"));
  if (orn)
  {
    RTW::Backend* backend = orn->GetBackend();
    if (backend != nullptr)
    {
      ospRelease(this->TransferFunction);
      ospRelease(this->SharedData);
      if (this->Cache->GetSize() == 0)
      {
        ospRelease(this->OSPRayVolume);
      }
    }
  }
  delete this->Cache;
}

//----------------------------------------------------------------------------
void svtkOSPRayVolumeMapperNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkOSPRayVolumeMapperNode::Render(bool prepass)
{
  if (prepass)
  {
    svtkVolumeNode* volNode = svtkVolumeNode::SafeDownCast(this->Parent);
    svtkVolume* vol = svtkVolume::SafeDownCast(volNode->GetRenderable());
    if (vol->GetVisibility() == false)
    {
      return;
    }
    svtkVolumeMapper* mapper = svtkVolumeMapper::SafeDownCast(this->GetRenderable());
    if (!vol->GetProperty())
    {
      // this is OK, happens in paraview client side for instance
      return;
    }

    svtkOSPRayRendererNode* orn =
      static_cast<svtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("svtkOSPRayRendererNode"));
    svtkRenderer* ren = svtkRenderer::SafeDownCast(orn->GetRenderable());
    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
    {
      return;
    }
    this->Cache->SetSize(svtkOSPRayRendererNode::GetTimeCacheSize(ren));

    OSPModel OSPRayModel = orn->GetOModel();

    // make sure that we have scalar input and update the scalar input
    if (mapper->GetDataSetInput() == nullptr)
    {
      // OK - PV cli/srv for instance svtkErrorMacro("VolumeMapper had no input!");
      return;
    }
    mapper->GetInputAlgorithm()->UpdateInformation();
    mapper->GetInputAlgorithm()->Update();

    svtkImageData* data = svtkImageData::SafeDownCast(mapper->GetDataSetInput());
    if (!data)
      return;

    int fieldAssociation;
    svtkDataArray* sa = svtkDataArray::SafeDownCast(this->GetArrayToProcess(data, fieldAssociation));
    if (!sa)
    {
      svtkErrorMacro("VolumeMapper's Input has no scalar array!");
      return;
    }

    if (!this->TransferFunction)
    {
      this->TransferFunction = ospNewTransferFunction("piecewise_linear");
    }

    svtkVolumeProperty* volProperty = vol->GetProperty();
    // when input data is modified
    svtkDataArray* sca = nullptr;
    if (mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      double tstep = svtkOSPRayRendererNode::GetViewTime(ren);
      auto cached_Volume = this->Cache->Get(tstep);
      if (cached_Volume)
      {
        this->OSPRayVolume = static_cast<OSPVolume>(cached_Volume->object);
      }
      else
      {
        int ncomp = sa->GetNumberOfComponents();
        if (ncomp > 1)
        {
          int comp = 0; // mapper->GetArrayComponent(); not yet supported
          sca = sa->NewInstance();
          sca->SetNumberOfComponents(1);
          sca->SetNumberOfTuples(sa->GetNumberOfTuples());
          sca->CopyComponent(0, sa, comp);
          sa = sca;
        }
        int ScalarDataType = sa->GetDataType();
        void* ScalarDataPointer = sa->GetVoidPointer(0);
        int dim[3];
        data->GetDimensions(dim);
        if (fieldAssociation == svtkDataObject::FIELD_ASSOCIATION_CELLS)
        {
          dim[0] = dim[0] - 1;
          dim[1] = dim[1] - 1;
          dim[2] = dim[2] - 1;
        }

        std::string voxelType;
        OSPDataType ospVoxelType = OSP_UNKNOWN;
        if (ScalarDataType == SVTK_FLOAT)
        {
          voxelType = "float";
          ospVoxelType = OSP_FLOAT;
        }
        else if (ScalarDataType == SVTK_UNSIGNED_CHAR)
        {
          voxelType = "uchar";
          ospVoxelType = OSP_UCHAR;
        }
        else if (ScalarDataType == SVTK_UNSIGNED_SHORT)
        {
          voxelType = "ushort";
          ospVoxelType = OSP_USHORT;
        }
        else if (ScalarDataType == SVTK_SHORT)
        {
          voxelType = "short";
          ospVoxelType = OSP_SHORT;
        }
        else if (ScalarDataType == SVTK_DOUBLE)
        {
          voxelType = "double";
          ospVoxelType = OSP_DOUBLE;
        }
        else
        {
          svtkErrorMacro("ERROR: Unsupported data type for ospray volumes, current supported data "
                        "types are: float, uchar, short, ushort, and double.");
          return;
        }

        if (this->Cache->GetSize() == 0)
        {
          ospRelease(this->OSPRayVolume);
        }
        if (this->UseSharedBuffers)
        {
          this->OSPRayVolume = ospNewVolume("shared_structured_volume");
        }
        else
        {
          this->OSPRayVolume = ospNewVolume("block_bricked_volume");
        }
        if (this->Cache->HasRoom())
        {
          auto cacheEntry = std::make_shared<svtkOSPRayCacheItemObject>(backend, this->OSPRayVolume);
          this->Cache->Set(tstep, std::move(cacheEntry));
        }
        //
        // Send Volumetric data to OSPRay
        //
        ospSet3i(this->OSPRayVolume, "dimensions", dim[0], dim[1], dim[2]);
        double origin[3];
        double scale[3];
        data->GetOrigin(origin);
        vol->GetScale(scale);
        const double* bds = vol->GetBounds();
        origin[0] = bds[0];
        origin[1] = bds[2];
        origin[2] = bds[4];

        double spacing[3];
        data->GetSpacing(spacing);
        scale[0] = (bds[1] - bds[0]) / double(dim[0] - 1);
        scale[1] = (bds[3] - bds[2]) / double(dim[1] - 1);
        scale[2] = (bds[5] - bds[4]) / double(dim[2] - 1);

        ospSet3f(this->OSPRayVolume, "gridOrigin", origin[0], origin[1], origin[2]);
        ospSet3f(this->OSPRayVolume, "gridSpacing", scale[0], scale[1], scale[2]);
        ospSetString(this->OSPRayVolume, "voxelType", voxelType.c_str());
        this->SamplingStep = std::min(scale[0], std::min(scale[1], scale[2]));

        if (this->UseSharedBuffers)
        {
          ospRelease(this->SharedData);
          this->SharedData = ospNewData(
            dim[0] * dim[1] * dim[2], ospVoxelType, ScalarDataPointer, OSP_DATA_SHARED_BUFFER);
          ospSetData(this->OSPRayVolume, "voxelData", this->SharedData);
        }
        else
        {
          osp::vec3i ll, uu;
          ll.x = 0, ll.y = 0, ll.z = 0;
          uu.x = dim[0], uu.y = dim[1], uu.z = dim[2];
          ospSetRegion(this->OSPRayVolume, ScalarDataPointer, ll, uu);
        }

        ospSetObject(this->OSPRayVolume, "transferFunction", this->TransferFunction);

        ospSet1f(OSPRayVolume, "adaptiveMaxSamplingRate", 1.2f);
        ospSet1f(OSPRayVolume, "adaptiveBacktrack", 0.01f);
        ospSet1i(OSPRayVolume, "adaptiveSampling", 1);
        if (this->SamplingRate == 0.0f) // 0 means automatic sampling rate
        {
          // automatically determine sampling rate
          int minBound = std::min(std::min(dim[0], dim[1]), dim[2]);
          float minSamplingRate = 0.075f; // lower for min adaptive sampling step
          if (minBound < 100)
          {
            float s = (100.0f - minBound) / 100.0f;
            ospSet1f(this->OSPRayVolume, "samplingRate", s * 6.f + 1.f);
            ospSet1i(this->OSPRayVolume, "adaptiveSampling", 0); // turn off preIntegration
          }
          else if (minBound < 1000)
          {
            float s = std::min((900.0f - minBound) / 1000.0f, 1.f);
            float s_new = (s * s * s * (0.5f - minSamplingRate) + minSamplingRate);
            ospSet1f(this->OSPRayVolume, "samplingRate", s_new);
            ospSet1f(this->OSPRayVolume, "adaptiveMaxSamplingRate", 2.f);
          }
          else
          {
            ospSet1f(this->OSPRayVolume, "samplingRate", minSamplingRate);
          }
        }
        else
        {
          ospSet1f(this->OSPRayVolume, "samplingRate", this->SamplingRate);
        }
        ospSet1f(this->OSPRayVolume, "adaptiveScalar", 15.f);
        ospSet1i(this->OSPRayVolume, "preIntegration", 0); // turn off preIntegration

        float rs =
          static_cast<float>(volProperty->GetSpecular(0) / 16.); // 16 chosen because near GL
        float gs = static_cast<float>(volProperty->GetSpecular(1) / 16.);
        float bs = static_cast<float>(volProperty->GetSpecular(2) / 16.);
        ospSet3f(this->OSPRayVolume, "specular", rs, gs, bs);
        this->Shade = volProperty->GetShade();
        ospSet1i(this->OSPRayVolume, "gradientShadingEnabled", this->Shade);

        ospCommit(this->TransferFunction);
        ospCommit(this->OSPRayVolume);
      }
    }

    if (mapper->GetMTime() > this->BuildTime)
    {
      if (mapper->GetCropping())
      {
        double planes[6];
        mapper->GetCroppingRegionPlanes(planes);
        ospSet3f(this->OSPRayVolume, "volumeClippingBoxLower", planes[0], planes[2], planes[4]);
        ospSet3f(this->OSPRayVolume, "volumeClippingBoxUpper", planes[1], planes[3], planes[5]);
      }
      else
      {
        ospRemoveParam(this->OSPRayVolume, "volumeClippingBoxLower");
        ospRemoveParam(this->OSPRayVolume, "volumeClippingBoxUpper");
      }
      ospCommit(this->OSPRayVolume);
    }

    // test for modifications to volume properties
    if (volProperty->GetMTime() > this->PropertyTime ||
      mapper->GetDataSetInput()->GetMTime() > this->BuildTime)
    {
      this->UpdateTransferFunction(backend, vol, sa->GetRange());
      bool shade = volProperty->GetShade();
      if (this->Shade != shade)
      {
        ospSet1i(this->OSPRayVolume, "gradientShadingEnabled", shade);
        ospCommit(this->OSPRayVolume);
        this->Shade = shade;
      }
    }

    this->RenderTime = volNode->GetMTime();
    this->BuildTime.Modified();

    svtkContourValues* contours = volProperty->GetIsoSurfaceValues();
    if (mapper->GetBlendMode() == svtkVolumeMapper::ISOSURFACE_BLEND)
    {
      int nbContours = contours->GetNumberOfContours();
      if (nbContours > 0)
      {
        double* p = contours->GetValues();
        std::vector<float> values(p, p + nbContours);

        this->OSPRayIsosurface = ospNewGeometry("isosurfaces");
        OSPData isosurfaces = ospNewData(values.size(), OSP_FLOAT, values.data());

        ospSetData(this->OSPRayIsosurface, "isovalues", isosurfaces);
        ospSetObject(this->OSPRayIsosurface, "volume", this->OSPRayVolume);
        ospCommit(this->OSPRayIsosurface);
        ospAddGeometry(OSPRayModel, this->OSPRayIsosurface);
      }
      else
      {
        svtkWarningMacro("Isosurface mode is selected but no contour is defined");
      }
    }
    else
    {
      ospAddVolume(OSPRayModel, this->OSPRayVolume);
    }

    if (sca)
    {
      sca->Delete();
    }
  }
}

//------------------------------------------------------------------------------
void svtkOSPRayVolumeMapperNode::UpdateTransferFunction(
  RTW::Backend* backend, svtkVolume* vol, double* dataRange)
{
  if (backend == nullptr)
    return;
  svtkVolumeProperty* volProperty = vol->GetProperty();
  svtkColorTransferFunction* colorTF = volProperty->GetRGBTransferFunction(0);
  svtkPiecewiseFunction* scalarTF = volProperty->GetScalarOpacity(0);

  this->TFVals.resize(this->NumColors * 3);
  this->TFOVals.resize(this->NumColors);
  double tfRangeD[2];
  // prefer transfer function's range
  colorTF->GetRange(tfRangeD);
  // but use data's range if we can and we have to
  if (dataRange && (dataRange[1] > dataRange[0]) && (tfRangeD[1] <= tfRangeD[0]))
  {
    tfRangeD[0] = dataRange[0];
    tfRangeD[1] = dataRange[1];
  }
  osp::vec2f tfRange = { float(tfRangeD[0]), float(tfRangeD[1]) };
  scalarTF->GetTable(tfRangeD[0], tfRangeD[1], this->NumColors, &this->TFOVals[0]);
  colorTF->GetTable(tfRangeD[0], tfRangeD[1], this->NumColors, &this->TFVals[0]);

  // todo: samplingStep should be adjusted for AMR/unstructured
  float scalarOpacityUnitDistance = volProperty->GetScalarOpacityUnitDistance();
  if (scalarOpacityUnitDistance < 1e-29) // avoid div by 0
  {
    scalarOpacityUnitDistance = 1e-29;
  }
  for (int i = 0; i < this->NumColors; i++)
  {
    this->TFOVals[i] = this->TFOVals[i] / scalarOpacityUnitDistance * this->SamplingStep;
  }

  OSPData colorData = ospNewData(this->NumColors, OSP_FLOAT3, &this->TFVals[0]);
  ospSetData(this->TransferFunction, "colors", colorData);

  OSPData tfAlphaData = ospNewData(this->NumColors, OSP_FLOAT, &this->TFOVals[0]);
  ospSetData(this->TransferFunction, "opacities", tfAlphaData);

  ospSet2f(this->TransferFunction, "valueRange", tfRange.x, tfRange.y);
  ospCommit(this->TransferFunction);
  ospSetObject(this->OSPRayVolume, "transferFunction", this->TransferFunction);
  ospRelease(colorData);
  ospRelease(tfAlphaData);

  this->PropertyTime.Modified();
}
