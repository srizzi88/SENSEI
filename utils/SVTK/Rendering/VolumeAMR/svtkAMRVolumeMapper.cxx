/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAMRVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAMRVolumeMapper.h"

#include "svtkAMRResampleFilter.h"
#include "svtkBoundingBox.h"
#include "svtkCamera.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiThreader.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSmartVolumeMapper.h"
#include "svtkUniformGrid.h"

#include "svtkNew.h"
#include "svtkTimerLog.h"

svtkStandardNewMacro(svtkAMRVolumeMapper);

// Construct a svtkAMRVolumeMapper
//----------------------------------------------------------------------------
svtkAMRVolumeMapper::svtkAMRVolumeMapper()
{
  this->InternalMapper = svtkSmartVolumeMapper::New();
  this->Resampler = svtkAMRResampleFilter::New();
  this->HasMetaData = false;
  this->Resampler->SetDemandDrivenMode(0);
  this->Grid = nullptr;
  this->NumberOfSamples[0] = 128;
  this->NumberOfSamples[1] = 128;
  this->NumberOfSamples[2] = 128;
  this->RequestedResamplingMode = 0; // Frustrum Mode
  this->FreezeFocalPoint = false;
  this->LastFocalPointPosition[0] = this->LastFocalPointPosition[1] =
    this->LastFocalPointPosition[2] = 0.0;
  // Set the camera position to focal point distance to
  // something that would indicate an initial update is needed
  this->LastPostionFPDistance = -1.0;
  this->ResamplerUpdateTolerance = 10e-8;
  this->GridNeedsToBeUpdated = true;
  this->UseDefaultThreading = false;
}

//----------------------------------------------------------------------------
svtkAMRVolumeMapper::~svtkAMRVolumeMapper()
{
  this->InternalMapper->Delete();
  this->InternalMapper = nullptr;
  this->Resampler->Delete();
  this->Resampler = nullptr;
  if (this->Grid)
  {
    this->Grid->Delete();
    this->Grid = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetInputData(svtkImageData* svtkNotUsed(genericInput))
{
  svtkErrorMacro("Mapper expects a hierarchical dataset as input");
  this->Resampler->SetInputConnection(0, 0);
}

//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetInputData(svtkDataSet* svtkNotUsed(genericInput))
{
  svtkErrorMacro("Mapper expects a hierarchical dataset as input");
  this->Resampler->SetInputConnection(0, 0);
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetInputData(svtkOverlappingAMR* hdata)
{
  this->SetInputDataInternal(0, hdata);
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetInputConnection(int port, svtkAlgorithmOutput* input)
{
  if ((this->Resampler->GetNumberOfInputConnections(0) > 0) &&
    (this->Resampler->GetInputConnection(port, 0) == input))
  {
    return;
  }
  this->Resampler->SetInputConnection(port, input);
  this->svtkVolumeMapper::SetInputConnection(port, input);
  if (this->Grid)
  {
    this->Grid->Delete();
    this->Grid = nullptr;
  }
}
//----------------------------------------------------------------------------
double* svtkAMRVolumeMapper::GetBounds()
{
  svtkOverlappingAMR* hdata;
  hdata = svtkOverlappingAMR::SafeDownCast(this->Resampler->GetInputDataObject(0, 0));
  if (!hdata)
  {
    svtkMath::UninitializeBounds(this->Bounds);
  }
  else
  {
    hdata->GetBounds(this->Bounds);
  }
  return this->Bounds;
}
//----------------------------------------------------------------------------
int svtkAMRVolumeMapper::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkOverlappingAMR");
  return 1;
}

//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SelectScalarArray(int arrayNum)
{
  this->InternalMapper->SelectScalarArray(arrayNum);
}

//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SelectScalarArray(const char* arrayName)
{
  this->InternalMapper->SelectScalarArray(arrayName);
}

//----------------------------------------------------------------------------
const char* svtkAMRVolumeMapper::GetScalarModeAsString()
{
  return this->InternalMapper->GetScalarModeAsString();
}

//----------------------------------------------------------------------------
char* svtkAMRVolumeMapper::GetArrayName()
{
  return this->InternalMapper->GetArrayName();
}

//----------------------------------------------------------------------------
int svtkAMRVolumeMapper::GetArrayId()
{
  return this->InternalMapper->GetArrayId();
}

//----------------------------------------------------------------------------
int svtkAMRVolumeMapper::GetArrayAccessMode()
{
  return this->InternalMapper->GetArrayAccessMode();
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetScalarMode(int mode)
{
  this->svtkVolumeMapper::SetScalarMode(mode);
  // for the internal mapper we need to convert all cell based
  // modes to point based since this is what the resample filter is doing
  int newMode = mode;
  if (mode == SVTK_SCALAR_MODE_USE_CELL_DATA)
  {
    newMode = SVTK_SCALAR_MODE_USE_POINT_DATA;
  }
  else if (mode == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    newMode = SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA;
  }

  this->InternalMapper->SetScalarMode(newMode);
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetBlendMode(int mode)
{
  this->InternalMapper->SetBlendMode(mode);
}
//----------------------------------------------------------------------------
int svtkAMRVolumeMapper::GetBlendMode()
{
  return this->InternalMapper->GetBlendMode();
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetCropping(svtkTypeBool mode)
{
  this->InternalMapper->SetCropping(mode);
}
//----------------------------------------------------------------------------
svtkTypeBool svtkAMRVolumeMapper::GetCropping()
{
  return this->InternalMapper->GetCropping();
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetCroppingRegionFlags(int mode)
{
  this->InternalMapper->SetCroppingRegionFlags(mode);
}
//----------------------------------------------------------------------------
int svtkAMRVolumeMapper::GetCroppingRegionFlags()
{
  return this->InternalMapper->GetCroppingRegionFlags();
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetCroppingRegionPlanes(
  double arg1, double arg2, double arg3, double arg4, double arg5, double arg6)
{
  this->InternalMapper->SetCroppingRegionPlanes(arg1, arg2, arg3, arg4, arg5, arg6);
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::GetCroppingRegionPlanes(double* planes)
{
  this->InternalMapper->GetCroppingRegionPlanes(planes);
}
//----------------------------------------------------------------------------
double* svtkAMRVolumeMapper::GetCroppingRegionPlanes()
{
  return this->InternalMapper->GetCroppingRegionPlanes();
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetRequestedRenderMode(int mode)
{
  this->InternalMapper->SetRequestedRenderMode(mode);
}
//----------------------------------------------------------------------------
int svtkAMRVolumeMapper::GetRequestedRenderMode()
{
  return this->InternalMapper->GetRequestedRenderMode();
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::SetInterpolationMode(int mode)
{
  this->InternalMapper->SetInterpolationMode(mode);
}
//----------------------------------------------------------------------------
int svtkAMRVolumeMapper::GetInterpolationMode()
{
  return this->InternalMapper->GetInterpolationMode();
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::ReleaseGraphicsResources(svtkWindow* window)
{
  this->InternalMapper->ReleaseGraphicsResources(window);
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::Render(svtkRenderer* ren, svtkVolume* vol)
{
  // Hack - Make sure the camera is in the right mode for moving the focal point
  ren->GetActiveCamera()->SetFreezeFocalPoint(this->FreezeFocalPoint);
  // If there is no grid initially we need to see if we can create one
  // The grid is not created if it is an interactive render; meaning the desired
  // time is less than the previous time to draw
  if (!(this->Grid &&
        (1.0 / ren->GetRenderWindow()->GetDesiredUpdateRate() <
          this->InternalMapper->GetTimeToDraw())))
  {
    if (!this->HasMetaData)
    {
      // If there is no meta data then the resample filter has not been updated
      // with the proper frustrun bounds else it would have been done when
      // processing request information
      this->UpdateResampler(ren, nullptr);
    }
    if (this->GridNeedsToBeUpdated)
    {
      this->UpdateGrid();
    }

    if (this->Grid == nullptr)
    {
      // Could not create a grid
      return;
    }

    this->InternalMapper->SetInputData(this->Grid);
  }
  // Enable threading for the internal volume renderer and the reset the
  // original value when done - need when running inside of ParaView
  if (this->UseDefaultThreading)
  {
    int maxNumThreads = svtkMultiThreader::GetGlobalMaximumNumberOfThreads();
    svtkMultiThreader::SetGlobalMaximumNumberOfThreads(0);
    this->InternalMapper->Render(ren, vol);
    svtkMultiThreader::SetGlobalMaximumNumberOfThreads(maxNumThreads);
  }
  else
  {
    this->InternalMapper->Render(ren, vol);
  }
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::UpdateResampler(svtkRenderer* ren, svtkOverlappingAMR* amr)
{
  // Set the bias of the resample filter to be the projection direction
  double bvec[3];
  svtkCamera* cam = ren->GetActiveCamera();
  double d, d2, fp[3], pd, gb[6];
  d = cam->GetDistance();
  cam->GetFocalPoint(fp);

  if (this->Grid)
  {
    // Compare distances with the grid's bounds
    this->Grid->GetBounds(gb);
    svtkBoundingBox bbox(gb);
    double maxL = bbox.GetMaxLength();
    // If the grid's max length is 0 then we need to update
    if (maxL > 0.0)
    {
      pd = fabs(d - this->LastPostionFPDistance) / this->LastPostionFPDistance;
      if ((this->LastPostionFPDistance > 0.0) && (pd <= this->ResamplerUpdateTolerance))
      {
        // Lets see if the focal point has not moved enough to cause an update
        d2 = svtkMath::Distance2BetweenPoints(fp, this->LastFocalPointPosition) / (maxL * maxL);
        if (d2 <= (this->ResamplerUpdateTolerance * this->ResamplerUpdateTolerance))
        {
          // nothing needs to be updated
          return;
        }
        else
        {
          //          int oops = 1;
        }
      }
    }
  }
  cam->GetDirectionOfProjection(bvec);
  this->Resampler->SetBiasVector(bvec);
  this->Resampler->SetUseBiasVector(true);
  this->LastPostionFPDistance = d;
  this->LastFocalPointPosition[0] = fp[0];
  this->LastFocalPointPosition[1] = fp[1];
  this->LastFocalPointPosition[2] = fp[2];

  if (this->RequestedResamplingMode == 0)
  {
    this->UpdateResamplerFrustrumMethod(ren, amr);
  }
  else
  {
    // This is the focal point approach where we
    // center the grid on the focal point and set its length
    // to be the distance between the camera and its focal point
    double p[3];
    p[0] = fp[0] - d;
    p[1] = fp[1] - d;
    p[2] = fp[2] - d;
    // Now set the min/max of the resample filter
    this->Resampler->SetMin(p);
    p[0] = fp[0] + d;
    p[1] = fp[1] + d;
    p[2] = fp[2] + d;
    this->Resampler->SetMax(p);
    this->Resampler->SetNumberOfSamples(this->NumberOfSamples);
  }
  // The grid may have changed
  this->GridNeedsToBeUpdated = true;
}

//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::UpdateResamplerFrustrumMethod(svtkRenderer* ren, svtkOverlappingAMR* amr)
{
  double bounds[6];
  // If we have been passed a valid amr then assume this is the proper
  // meta data to use
  if (amr)
  {
    amr->GetBounds(bounds);
  }
  else
  {
    // Make sure the bounds are up to date
    this->GetBounds(bounds);
  }

  double computed_bounds[6];
  if (svtkAMRVolumeMapper::ComputeResamplerBoundsFrustumMethod(
        ren->GetActiveCamera(), ren, bounds, computed_bounds))
  {
    svtkBoundingBox bbox(computed_bounds);
    // Now set the min/max of the resample filter
    this->Resampler->SetMin(const_cast<double*>(bbox.GetMinPoint()));
    this->Resampler->SetMax(const_cast<double*>(bbox.GetMaxPoint()));
    this->Resampler->SetNumberOfSamples(this->NumberOfSamples);
  }
}

//----------------------------------------------------------------------------
bool svtkAMRVolumeMapper::ComputeResamplerBoundsFrustumMethod(
  svtkCamera* camera, svtkRenderer* renderer, const double bounds[6], double out_bounds[6])
{
  svtkMath::UninitializeBounds(out_bounds);

  // First we need to create a bounding box that represents the visible region
  // of the camera in World Coordinates

  // In order to produce as tight of bounding box as possible we need to determine
  // the z range in view coordinates of the data and then project that part
  // of the view volume back into world coordinates

  // We would just use the renderer's WorldToView and ViewToWorld methods but those
  // implementations are not efficient for example ViewToWorld would do 8
  // matrix inverse ops when all we really need to do is one

  // Get the camera transformation
  svtkMatrix4x4* matrix =
    camera->GetCompositeProjectionTransformMatrix(renderer->GetTiledAspectRatio(), 0, 1);

  int i, j, k;
  double pnt[4], tpnt[4];
  svtkBoundingBox bbox;
  pnt[3] = 1.0;
  for (i = 0; i < 2; i++)
  {
    pnt[0] = bounds[i];
    for (j = 2; j < 4; j++)
    {
      pnt[1] = bounds[j];
      for (k = 4; k < 6; k++)
      {
        pnt[2] = bounds[k];
        matrix->MultiplyPoint(pnt, tpnt);
        if (tpnt[3])
        {
          bbox.AddPoint(tpnt[0] / tpnt[3], tpnt[1] / tpnt[3], tpnt[2] / tpnt[3]);
        }
        else
        {
          svtkGenericWarningMacro("UpdateResampler: Found an Ideal Point going to VC!");
        }
      }
    }
  }

  double zRange[2];
  if (bbox.IsValid())
  {
    zRange[0] = bbox.GetMinPoint()[2];
    zRange[1] = bbox.GetMaxPoint()[2];
    // Normalize the z values to make sure they are between -1 and 1
    for (i = 0; i < 2; i++)
    {
      if (zRange[i] < -1.0)
      {
        zRange[i] = -1.0;
      }
      else if (zRange[i] > 1.0)
      {
        zRange[i] = 1.0;
      }
    }
  }
  else
  {
    // Since we could not find a valid bounding box assume that the
    // zrange is -1 to 1
    zRange[0] = -1.0;
    zRange[1] = 1.0;
  }

  // Now that we have the z range of the data in View Coordinates lets
  // convert that part of the View Volume back into World Coordinates
  double mat[16];
  // Need the inverse
  svtkMatrix4x4::Invert(*matrix->Element, mat);

  bbox.Reset();
  // Compute the bounding box
  for (i = -1; i < 2; i += 2)
  {
    pnt[0] = i;
    for (j = -1; j < 2; j += 2)
    {
      pnt[1] = j;
      for (k = 0; k < 2; k++)
      {
        pnt[2] = zRange[k];
        svtkMatrix4x4::MultiplyPoint(mat, pnt, tpnt);
        if (tpnt[3])
        {
          bbox.AddPoint(tpnt[0] / tpnt[3], tpnt[1] / tpnt[3], tpnt[2] / tpnt[3]);
        }
        else
        {
          svtkGenericWarningMacro("UpdateResampler: Found an Ideal Point going to WC!");
        }
      }
    }
  }

  // Check to see if the box is valid
  if (!bbox.IsValid())
  {
    return false; // There is nothing we can do
  }
  bbox.GetBounds(out_bounds);
  return true;
}

//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::UpdateGrid()
{
  // This is for debugging
#define PRINTSTATS 0
#if PRINTSTATS
  svtkNew<svtkTimerLog> timer;
  int gridDim[3];
  double gridOrigin[3];
  timer->StartTimer();
#endif
  this->Resampler->Update();
#if PRINTSTATS
  timer->StopTimer();
  std::cerr << "Resample Time:" << timer->GetElapsedTime() << " ";
  std::cerr << "New Bounds: [" << bbox.GetMinPoint()[0] << ", " << bbox.GetMaxPoint()[0] << "], ["
            << bbox.GetMinPoint()[1] << ", " << bbox.GetMaxPoint()[1] << "], ["
            << bbox.GetMinPoint()[2] << ", " << bbox.GetMaxPoint()[2] << "\n";
#endif
  svtkMultiBlockDataSet* mb = this->Resampler->GetOutput();
  if (!mb)
  {
    return;
  }
  unsigned int nb = mb->GetNumberOfBlocks();
  if (!nb)
  {
    // No new grid was created
    return;
  }
  if (nb != 1)
  {
    svtkErrorMacro("UpdateGrid: Resampler created more than 1 Grid!");
  }
  if (this->Grid)
  {
    this->Grid->Delete();
  }
  this->Grid = svtkUniformGrid::SafeDownCast(mb->GetBlock(0));
  this->Grid->Register(0);
  this->GridNeedsToBeUpdated = false;
#if PRINTSTATS
  this->Grid->GetDimensions(gridDim);
  this->Grid->GetOrigin(gridOrigin);
  std::cerr << "Grid Dimensions: (" << gridDim[0] << ", " << gridDim[1] << ", " << gridDim[2]
            << ") Origin:(" << gridOrigin[0] << ", " << gridOrigin[1] << ", " << gridOrigin[2]
            << ")\n";
#endif
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::ProcessUpdateExtentRequest(svtkRenderer* svtkNotUsed(ren),
  svtkInformation* info, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->Resampler->RequestUpdateExtent(info, inputVector, outputVector);
}
//----------------------------------------------------------------------------
void svtkAMRVolumeMapper::ProcessInformationRequest(svtkRenderer* ren, svtkInformation* info,
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  if (!(input && input->Has(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA())))
  {
    this->HasMetaData = false;
    this->Resampler->SetDemandDrivenMode(0);
    return;
  }

  if (!this->HasMetaData)
  {
    this->HasMetaData = true;
    this->Resampler->SetDemandDrivenMode(1);
  }
  svtkOverlappingAMR* amrMetaData = svtkOverlappingAMR::SafeDownCast(
    input->Get(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));

  this->UpdateResampler(ren, amrMetaData);
  this->Resampler->RequestInformation(info, inputVector, outputVector);
}
//----------------------------------------------------------------------------

// Print the svtkAMRVolumeMapper
void svtkAMRVolumeMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScalarMode: " << this->GetScalarModeAsString() << endl;

  if (this->ScalarMode == SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
    this->ScalarMode == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    if (this->ArrayAccessMode == SVTK_GET_ARRAY_BY_ID)
    {
      os << indent << "ArrayId: " << this->ArrayId << endl;
    }
    else
    {
      os << indent << "ArrayName: " << this->ArrayName << endl;
    }
  }
  os << indent << "UseDefaultThreading:" << this->UseDefaultThreading << "\n";
  os << indent << "ResampledUpdateTolerance: " << this->ResamplerUpdateTolerance << "\n";
  os << indent << "NumberOfSamples: ";
  for (int i = 0; i < 3; ++i)
  {
    os << this->NumberOfSamples[i] << " ";
  }
  os << std::endl;
  os << indent << "RequestedResamplingMode: " << this->RequestedResamplingMode << "\n";
  os << indent << "FreezeFocalPoint: " << this->FreezeFocalPoint << "\n";
}
//----------------------------------------------------------------------------
