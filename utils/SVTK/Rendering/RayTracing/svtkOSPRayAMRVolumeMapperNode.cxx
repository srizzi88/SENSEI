/*=========================================================================

   Program:   Visualization Toolkit
   Module:    svtkOSPRayAMRVolumeMapperNode.cxx

   Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
   All rights reserved.
   See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

      This software is distributed WITHOUT ANY WARRANTY; without even
      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
      PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkOSPRayAMRVolumeMapperNode.h"

#include "svtkAMRBox.h"
#include "svtkAMRInformation.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkOSPRayCache.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkUniformGridAMRDataIterator.h"
#include "svtkVolume.h"
#include "svtkVolumeMapper.h"
#include "svtkVolumeNode.h"
#include "svtkVolumeProperty.h"

#include "RTWrapper/RTWrapper.h"

#include <cassert>

namespace ospray
{
namespace amr
{
struct BrickInfo
{
  /*! bounding box of integer coordinates of cells. note that
      this EXCLUDES the width of the rightmost cell: ie, a 4^3
      box at root level pos (0,0,0) would have a _box_ of
      [(0,0,0)-(3,3,3)] (because 3,3,3 is the highest valid
      coordinate in this box!), while its bounds would be
      [(0,0,0)-(4,4,4)]. Make sure to NOT use box.size() for the
      grid dimensions, since this will always be one lower than
      the dims of the grid.... */
  ospcommon::box3i box;
  //! level this brick is at
  int level;
  // width of each cell in this level
  float cellWidth;

  // inline ospcommon::box3f worldBounds() const {
  //  return ospcommon::box3f(ospcommon::vec3f(box.lower)*cellWidth,
  //        ospcommon::vec3f(box.upper+ospcommon::vec3i(1))*cellWidth);
  //}
};
}
}

using namespace ospray::amr;

svtkStandardNewMacro(svtkOSPRayAMRVolumeMapperNode);

//----------------------------------------------------------------------------
svtkOSPRayAMRVolumeMapperNode::svtkOSPRayAMRVolumeMapperNode()
{
  this->NumColors = 128;
  this->TransferFunction = nullptr;
  this->SamplingRate = 0.5f;
  this->OldSamplingRate = -1.f;
}

//----------------------------------------------------------------------------
void svtkOSPRayAMRVolumeMapperNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkOSPRayAMRVolumeMapperNode::Render(bool prepass)
{
  if (prepass)
  {
    svtkVolumeNode* volNode = svtkVolumeNode::SafeDownCast(this->Parent);
    if (!volNode)
    {
      svtkErrorMacro("invalid volumeNode");
      return;
    }
    svtkVolume* vol = svtkVolume::SafeDownCast(volNode->GetRenderable());
    if (vol->GetVisibility() == false)
    {
      return;
    }
    svtkVolumeMapper* mapper = svtkVolumeMapper::SafeDownCast(this->GetRenderable());
    if (!mapper)
    {
      svtkErrorMacro("invalid mapper");
      return;
    }
    if (!vol->GetProperty())
    {
      svtkErrorMacro("VolumeMapper had no svtkProperty");
      return;
    }

    svtkOSPRayRendererNode* orn =
      static_cast<svtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("svtkOSPRayRendererNode"));
    svtkRenderer* ren = svtkRenderer::SafeDownCast(orn->GetRenderable());

    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
      return;

    if (!this->TransferFunction)
    {
      this->TransferFunction = ospNewTransferFunction("piecewise_linear");
    }

    this->Cache->SetSize(svtkOSPRayRendererNode::GetTimeCacheSize(ren));

    OSPModel OSPRayModel = orn->GetOModel();
    if (!OSPRayModel)
    {
      return;
    }

    svtkOverlappingAMR* amr = svtkOverlappingAMR::SafeDownCast(mapper->GetInputDataObject(0, 0));
    if (!amr)
    {
      svtkErrorMacro("couldn't get amr data\n");
      return;
    }

    svtkVolumeProperty* volProperty = vol->GetProperty();

    bool volDirty = false;
    if (!this->OSPRayVolume || amr->GetMTime() > this->BuildTime)
    {
      double tstep = svtkOSPRayRendererNode::GetViewTime(ren);
      auto cached_Volume = this->Cache->Get(tstep);
      if (cached_Volume)
      {
        this->OSPRayVolume = static_cast<OSPVolume>(cached_Volume->object);
      }
      else
      {
        if (this->Cache->GetSize() == 0)
        {
          ospRelease(this->OSPRayVolume);
        }
        this->OSPRayVolume = ospNewVolume("amr_volume");
        if (this->Cache->HasRoom())
        {
          auto cacheEntry = std::make_shared<svtkOSPRayCacheItemObject>(backend, this->OSPRayVolume);
          this->Cache->Set(tstep, cacheEntry);
        }
        volDirty = true;

        unsigned int lastLevel = 0;
        std::vector<OSPData> brickDataArray;
        std::vector<BrickInfo> brickInfoArray;
        size_t totalDataSize = 0;

        svtkAMRInformation* amrInfo = amr->GetAMRInfo();
        svtkSmartPointer<svtkUniformGridAMRDataIterator> iter;
        iter.TakeReference(svtkUniformGridAMRDataIterator::SafeDownCast(amr->NewIterator()));
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          unsigned int level = iter->GetCurrentLevel();
          if (!(level >= lastLevel))
          {
            svtkErrorMacro("ospray requires level info be ordered lowest to highest");
          };
          lastLevel = level;
          unsigned int index = iter->GetCurrentIndex();

          // note: this iteration "naturally" goes from datasets at lower levels to
          // those at higher levels.
          svtkImageData* data = svtkImageData::SafeDownCast(iter->GetCurrentDataObject());
          if (!data)
          {
            return;
          }
          float* dataPtr;
          int dim[3];

          const svtkAMRBox& box = amrInfo->GetAMRBox(level, index);
          const int* lo = box.GetLoCorner();
          const int* hi = box.GetHiCorner();
          ospcommon::vec3i lo_v = { lo[0], lo[1], lo[2] };
          ospcommon::vec3i hi_v = { hi[0], hi[1], hi[2] };
          dim[0] = hi[0] - lo[0] + 1;
          dim[1] = hi[1] - lo[1] + 1;
          dim[2] = hi[2] - lo[2] + 1;

          int fieldAssociation;
          mapper->SetScalarMode(SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
          svtkDataArray* cellArray =
            svtkDataArray::SafeDownCast(this->GetArrayToProcess(data, fieldAssociation));
          if (!cellArray)
          {
            std::cerr << "could not get data!\n";
            return;
          }

          if (cellArray->GetDataType() != SVTK_FLOAT)
          {
            if (cellArray->GetDataType() == SVTK_DOUBLE)
            {
              float* fdata = new float[dim[0] * dim[1] * size_t(dim[2])];
              double* dptr;
              dptr = (double*)cellArray->WriteVoidPointer(0, cellArray->GetSize());
              for (size_t i = 0; i < dim[0] * dim[1] * size_t(dim[2]); i++)
              {
                fdata[i] = dptr[i];
              }
              dataPtr = fdata;
            }
            else
            {
              std::cerr
                << "Only doubles and floats are supported in OSPRay AMR volume mapper currently";
              return;
            }
          }
          else
          {
            dataPtr = (float*)cellArray->WriteVoidPointer(0, cellArray->GetSize());
          }

          totalDataSize += sizeof(float) * size_t(dim[0] * dim[1]) * dim[2];
          OSPData odata =
            ospNewData(dim[0] * dim[1] * dim[2], OSP_FLOAT, dataPtr, OSP_DATA_SHARED_BUFFER);
          brickDataArray.push_back(odata);

          BrickInfo bi;
          ospcommon::box3i obox = { lo_v, hi_v };
          bi.box = obox;
          double spacing[3];
          amrInfo->GetSpacing(level, spacing);
          bi.cellWidth = spacing[0];
          // cell bounds:  origin + box.LoCorner*spacing,
          bi.level = level;
          totalDataSize += sizeof(BrickInfo);

          brickInfoArray.push_back(bi);
        }

        assert(brickDataArray.size() == brickInfoArray.size());
        ospSet1f(this->OSPRayVolume, "samplingRate", this->SamplingRate); // TODO: gui option

        double origin[3];
        vol->GetOrigin(origin);
        double* bds = mapper->GetBounds();
        origin[0] = bds[0];
        origin[1] = bds[2];
        origin[2] = bds[4];

        double spacing[3];
        amr->GetAMRInfo()->GetSpacing(0, spacing);
        ospSet3f(this->OSPRayVolume, "gridOrigin", origin[0], origin[1], origin[2]);
        ospSetString(this->OSPRayVolume, "voxelType", "float");

        OSPData brickDataData =
          ospNewData(brickDataArray.size(), OSP_OBJECT, &brickDataArray[0], 0);
        ospSetData(this->OSPRayVolume, "brickData", brickDataData);
        OSPData brickInfoData = ospNewData(
          brickInfoArray.size() * sizeof(brickInfoArray[0]), OSP_RAW, &brickInfoArray[0], 0);
        ospSetData(this->OSPRayVolume, "brickInfo", brickInfoData);
        ospSetObject(this->OSPRayVolume, "transferFunction", this->TransferFunction);
        this->BuildTime.Modified();
      }
    }
    if ((vol->GetProperty()->GetMTime() > this->PropertyTime) || volDirty)
    {
      //
      // transfer function
      //
      this->UpdateTransferFunction(backend, vol);
      ospSet1i(this->OSPRayVolume, "gradientShadingEnabled", volProperty->GetShade());
      this->PropertyTime.Modified();
    }

    if (this->OldSamplingRate != this->SamplingRate)
    {
      this->OldSamplingRate = this->SamplingRate;
      volDirty = true;
    }

    if (volDirty)
    {
      ospSet1f(this->OSPRayVolume, "samplingRate", this->SamplingRate);
      ospCommit(this->OSPRayVolume);
    }
    ospAddVolume(OSPRayModel, this->OSPRayVolume);
    ospCommit(OSPRayModel);
  } // prepass
}
