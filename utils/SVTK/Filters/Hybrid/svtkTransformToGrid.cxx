/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformToGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTransformToGrid.h"

#include "svtkAbstractTransform.h"
#include "svtkIdentityTransform.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkTransformToGrid);

svtkCxxSetObjectMacro(svtkTransformToGrid, Input, svtkAbstractTransform);

//----------------------------------------------------------------------------
svtkTransformToGrid::svtkTransformToGrid()
{
  this->Input = nullptr;

  this->GridScalarType = SVTK_FLOAT;

  for (int i = 0; i < 3; i++)
  {
    this->GridExtent[2 * i] = this->GridExtent[2 * i + 1] = 0;
    this->GridOrigin[i] = 0.0;
    this->GridSpacing[i] = 1.0;
  }

  this->DisplacementScale = 1.0;
  this->DisplacementShift = 0.0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkTransformToGrid::~svtkTransformToGrid()
{
  this->SetInput(static_cast<svtkAbstractTransform*>(nullptr));
}

//----------------------------------------------------------------------------
void svtkTransformToGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  int i;

  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: (" << this->Input << ")\n";

  os << indent << "GridSpacing: (" << this->GridSpacing[0];
  for (i = 1; i < 3; ++i)
  {
    os << ", " << this->GridSpacing[i];
  }
  os << ")\n";

  os << indent << "GridOrigin: (" << this->GridOrigin[0];
  for (i = 1; i < 3; ++i)
  {
    os << ", " << this->GridOrigin[i];
  }
  os << ")\n";

  os << indent << "GridExtent: (" << this->GridExtent[0];
  for (i = 1; i < 6; ++i)
  {
    os << ", " << this->GridExtent[i];
  }
  os << ")\n";

  os << indent << "GridScalarType: " << svtkImageScalarTypeNameMacro(this->GridScalarType) << "\n";

  this->UpdateShiftScale();

  os << indent << "DisplacementScale: " << this->DisplacementScale << "\n";
  os << indent << "DisplacementShift: " << this->DisplacementShift << "\n";
}

//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
void svtkTransformToGrid::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->GetInput() == nullptr)
  {
    svtkErrorMacro("Missing input");
    return;
  }

  // update the transform, maybe in the future make transforms part of the
  // pipeline
  this->Input->Update();

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->GridExtent, 6);
  outInfo->Set(svtkDataObject::SPACING(), this->GridSpacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), this->GridOrigin, 3);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->GridScalarType, 3);
}

//----------------------------------------------------------------------------
// Return the maximum absolute displacement of the transform over
// the entire grid extent -- this is extremely robust and extremely
// inefficient, it should be possible to do much better than this.
static void svtkTransformToGridMinMax(
  svtkTransformToGrid* self, int extent[6], double& minDisplacement, double& maxDisplacement)
{
  svtkAbstractTransform* transform = self->GetInput();
  transform->Update();

  if (!transform)
  {
    minDisplacement = -1.0;
    maxDisplacement = +1.0;
    return;
  }

  double* spacing = self->GetGridSpacing();
  double* origin = self->GetGridOrigin();

  maxDisplacement = -1e37;
  minDisplacement = +1e37;

  double point[3], newPoint[3], displacement;

  for (int k = extent[4]; k <= extent[5]; k++)
  {
    point[2] = k * spacing[2] + origin[2];
    for (int j = extent[2]; j <= extent[3]; j++)
    {
      point[1] = j * spacing[1] + origin[1];
      for (int i = extent[0]; i <= extent[1]; i++)
      {
        point[0] = i * spacing[0] + origin[0];

        transform->InternalTransformPoint(point, newPoint);

        for (int l = 0; l < 3; l++)
        {
          displacement = newPoint[l] - point[l];

          if (displacement > maxDisplacement)
          {
            maxDisplacement = displacement;
          }

          if (displacement < minDisplacement)
          {
            minDisplacement = displacement;
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkTransformToGrid::UpdateShiftScale()
{
  int gridType = this->GridScalarType;

  // nothing to do for double or float
  if (gridType == SVTK_DOUBLE || gridType == SVTK_FLOAT)
  {
    this->DisplacementShift = 0.0;
    this->DisplacementScale = 1.0;
    svtkDebugMacro(<< "displacement (scale, shift) = (" << this->DisplacementScale << ", "
                  << this->DisplacementShift << ")");
    return;
  }

  // check mtime
  if (this->ShiftScaleTime.GetMTime() > this->GetMTime())
  {
    return;
  }

  // get the maximum displacement
  double minDisplacement, maxDisplacement;
  svtkTransformToGridMinMax(this, this->GridExtent, minDisplacement, maxDisplacement);

  svtkDebugMacro(<< "displacement (min, max) = (" << minDisplacement << ", " << maxDisplacement
                << ")");

  double typeMin, typeMax;

  switch (gridType)
  {
    case SVTK_SHORT:
      typeMin = SVTK_SHORT_MIN;
      typeMax = SVTK_SHORT_MAX;
      break;
    case SVTK_UNSIGNED_SHORT:
      typeMin = SVTK_UNSIGNED_SHORT_MIN;
      typeMax = SVTK_UNSIGNED_SHORT_MAX;
      break;
    case SVTK_CHAR:
      typeMin = SVTK_CHAR_MIN;
      typeMax = SVTK_CHAR_MAX;
      break;
    case SVTK_UNSIGNED_CHAR:
      typeMin = SVTK_UNSIGNED_CHAR_MIN;
      typeMax = SVTK_UNSIGNED_CHAR_MAX;
      break;
    default:
      svtkErrorMacro(<< "UpdateShiftScale: Unknown input ScalarType");
      return;
  }

  this->DisplacementScale = ((maxDisplacement - minDisplacement) / (typeMax - typeMin));
  this->DisplacementShift =
    ((typeMax * minDisplacement - typeMin * maxDisplacement) / (typeMax - typeMin));

  if (this->DisplacementScale == 0.0)
  {
    this->DisplacementScale = 1.0;
  }

  svtkDebugMacro(<< "displacement (scale, shift) = (" << this->DisplacementScale << ", "
                << this->DisplacementShift << ")");

  this->ShiftScaleTime.Modified();
}

//----------------------------------------------------------------------------
// macros to ensure proper round-to-nearest behaviour

inline void svtkGridRound(double val, unsigned char& rnd)
{
  rnd = (unsigned char)(val + 0.5f);
}

inline void svtkGridRound(double val, char& rnd)
{
  rnd = (char)((val + 128.5f) - 128);
}

inline void svtkGridRound(double val, short& rnd)
{
  rnd = (short)((int)(val + 32768.5f) - 32768);
}

inline void svtkGridRound(double val, unsigned short& rnd)
{
  rnd = (unsigned short)(val + 0.5f);
}

inline void svtkGridRound(double val, float& rnd)
{
  rnd = (float)(val);
}

inline void svtkGridRound(double val, double& rnd)
{
  rnd = (double)(val);
}

//----------------------------------------------------------------------------
template <class T>
void svtkTransformToGridExecute(svtkTransformToGrid* self, svtkImageData* grid, T* gridPtr,
  int extent[6], double shift, double scale, int id)
{
  svtkAbstractTransform* transform = self->GetInput();
  int isIdentity = 0;
  if (transform == nullptr)
  {
    transform = svtkIdentityTransform::New();
    isIdentity = 1;
  }

  double* spacing = grid->GetSpacing();
  double* origin = grid->GetOrigin();
  svtkIdType increments[3];
  grid->GetIncrements(increments);

  double invScale = 1.0 / scale;

  double point[3];
  double newPoint[3];

  T* gridPtr0 = gridPtr;

  unsigned long count = 0;
  unsigned long target =
    (unsigned long)((extent[5] - extent[4] + 1) * (extent[3] - extent[2] + 1) / 50.0);
  target++;

  for (int k = extent[4]; k <= extent[5]; k++)
  {
    point[2] = k * spacing[2] + origin[2];
    T* gridPtr1 = gridPtr0;

    for (int j = extent[2]; j <= extent[3]; j++)
    {

      if (id == 0)
      {
        if (count % target == 0)
        {
          self->UpdateProgress(count / (50.0 * target));
        }
        count++;
      }

      point[1] = j * spacing[1] + origin[1];
      gridPtr = gridPtr1;

      for (int i = extent[0]; i <= extent[1]; i++)
      {
        point[0] = i * spacing[0] + origin[0];

        transform->InternalTransformPoint(point, newPoint);

        svtkGridRound((newPoint[0] - point[0] - shift) * invScale, *gridPtr++);
        svtkGridRound((newPoint[1] - point[1] - shift) * invScale, *gridPtr++);
        svtkGridRound((newPoint[2] - point[2] - shift) * invScale, *gridPtr++);
      }

      gridPtr1 += increments[1];
    }

    gridPtr0 += increments[2];
  }

  if (isIdentity)
  {
    transform->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkTransformToGrid::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the data object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* grid = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  grid->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  grid->AllocateScalars(outInfo);
  int* extent = grid->GetExtent();

  void* gridPtr = grid->GetScalarPointerForExtent(extent);
  int gridType = grid->GetScalarType();

  this->UpdateShiftScale();

  double scale = this->DisplacementScale;
  double shift = this->DisplacementShift;

  int id = 0;

  switch (gridType)
  {
    case SVTK_DOUBLE:
      svtkTransformToGridExecute(this, grid, (double*)(gridPtr), extent, shift, scale, id);
      break;
    case SVTK_FLOAT:
      svtkTransformToGridExecute(this, grid, (float*)(gridPtr), extent, shift, scale, id);
      break;
    case SVTK_SHORT:
      svtkTransformToGridExecute(this, grid, (short*)(gridPtr), extent, shift, scale, id);
      break;
    case SVTK_UNSIGNED_SHORT:
      svtkTransformToGridExecute(this, grid, (unsigned short*)(gridPtr), extent, shift, scale, id);
      break;
    case SVTK_CHAR:
      svtkTransformToGridExecute(this, grid, (char*)(gridPtr), extent, shift, scale, id);
      break;
    case SVTK_UNSIGNED_CHAR:
      svtkTransformToGridExecute(this, grid, (unsigned char*)(gridPtr), extent, shift, scale, id);
      break;
    default:
      svtkErrorMacro(<< "Execute: Unknown input ScalarType");
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkTransformToGrid::GetMTime()
{
  svtkMTimeType mtime = this->Superclass::GetMTime();

  if (this->Input)
  {
    svtkMTimeType mtime2 = this->Input->GetMTime();
    if (mtime2 > mtime)
    {
      mtime = mtime2;
    }
  }

  return mtime;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkTransformToGrid::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->RequestData(request, inputVector, outputVector);
    return 1;
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    this->RequestInformation(request, inputVector, outputVector);
    // after executing set the origin and spacing from the
    // info
    int i;
    for (i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outputVector->GetInformationObject(i);
      svtkImageData* output = svtkImageData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
      // if execute info didn't set origin and spacing then we set them
      if (!info->Has(svtkDataObject::ORIGIN()))
      {
        info->Set(svtkDataObject::ORIGIN(), 0, 0, 0);
        info->Set(svtkDataObject::SPACING(), 1, 1, 1);
      }
      if (output)
      {
        output->SetOrigin(info->Get(svtkDataObject::ORIGIN()));
        output->SetSpacing(info->Get(svtkDataObject::SPACING()));
      }
    }
    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
svtkImageData* svtkTransformToGrid::GetOutput()
{
  return svtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

int svtkTransformToGrid::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  return 1;
}
