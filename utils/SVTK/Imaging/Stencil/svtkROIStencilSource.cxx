/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkROIStencilSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkROIStencilSource.h"

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkImageStencilData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkROIStencilSource);

//----------------------------------------------------------------------------
svtkROIStencilSource::svtkROIStencilSource()
{
  this->SetNumberOfInputPorts(0);

  this->Shape = svtkROIStencilSource::BOX;

  this->Bounds[0] = 0.0;
  this->Bounds[1] = 0.0;
  this->Bounds[2] = 0.0;
  this->Bounds[3] = 0.0;
  this->Bounds[4] = 0.0;
  this->Bounds[5] = 0.0;
}

//----------------------------------------------------------------------------
svtkROIStencilSource::~svtkROIStencilSource() = default;

//----------------------------------------------------------------------------
void svtkROIStencilSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Shape: " << this->GetShapeAsString() << "\n";
  os << indent << "Bounds: " << this->Bounds[0] << " " << this->Bounds[1] << " " << this->Bounds[2]
     << " " << this->Bounds[3] << " " << this->Bounds[4] << " " << this->Bounds[5] << "\n";
}

//----------------------------------------------------------------------------
const char* svtkROIStencilSource::GetShapeAsString()
{
  switch (this->Shape)
  {
    case svtkROIStencilSource::BOX:
      return "Box";
    case svtkROIStencilSource::ELLIPSOID:
      return "Ellipsoid";
    case svtkROIStencilSource::CYLINDERX:
      return "CylinderX";
    case svtkROIStencilSource::CYLINDERY:
      return "CylinderY";
    case svtkROIStencilSource::CYLINDERZ:
      return "CylinderZ";
  }
  return "";
}

//----------------------------------------------------------------------------
// tolerance for stencil operations

#define SVTK_STENCIL_TOL 7.62939453125e-06

//----------------------------------------------------------------------------
// Compute a reduced extent based on the Center and Size of the shape.
//
// Also returns the center and radius in voxel-index units.
static void svtkROIStencilSourceSubExtent(svtkROIStencilSource* self, const double origin[3],
  const double spacing[3], const int extent[6], int subextent[6], double icenter[3],
  double iradius[3])
{
  double bounds[6];
  self->GetBounds(bounds);

  for (int i = 0; i < 3; i++)
  {
    icenter[i] = (0.5 * (bounds[2 * i] + bounds[2 * i + 1]) - origin[i]) / spacing[i];
    iradius[i] = 0.5 * (bounds[2 * i + 1] - bounds[2 * i]) / spacing[i];

    if (iradius[i] < 0)
    {
      iradius[i] = -iradius[i];
    }
    iradius[i] += SVTK_STENCIL_TOL;

    double emin = icenter[i] - iradius[i];
    double emax = icenter[i] + iradius[i];

    subextent[2 * i] = extent[2 * i];
    subextent[2 * i + 1] = extent[2 * i + 1];

    if (extent[2 * i] < emin)
    {
      subextent[2 * i] = SVTK_INT_MAX;
      if (extent[2 * i + 1] >= emin)
      {
        subextent[2 * i] = svtkMath::Floor(emin) + 1;
      }
    }

    if (extent[2 * i + 1] > emax)
    {
      subextent[2 * i + 1] = SVTK_INT_MIN;
      if (extent[2 * i] <= emax)
      {
        subextent[2 * i + 1] = svtkMath::Floor(emax);
      }
    }
  }
}

//----------------------------------------------------------------------------
static int svtkROIStencilSourceBox(svtkROIStencilSource* self, svtkImageStencilData* data,
  const int extent[6], const double origin[3], const double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  svtkROIStencilSourceSubExtent(self, origin, spacing, extent, subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1) * (subextent[3] - subextent[2] + 1) / 50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
  {
    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
    {
      if (count % target == 0)
      {
        self->UpdateProgress(count / (50.0 * target));
      }
      count++;

      int r1 = subextent[0];
      int r2 = subextent[1];

      if (r2 >= r1)
      {
        data->InsertNextExtent(r1, r2, idY, idZ);
      }
    } // for idY
  }   // for idZ

  return 1;
}

//----------------------------------------------------------------------------
static int svtkROIStencilSourceEllipsoid(svtkROIStencilSource* self, svtkImageStencilData* data,
  const int extent[6], const double origin[3], const double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  svtkROIStencilSourceSubExtent(self, origin, spacing, extent, subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1) * (subextent[3] - subextent[2] + 1) / 50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
  {
    double z = (idZ - icenter[2]) / iradius[2];

    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
    {
      if (count % target == 0)
      {
        self->UpdateProgress(count / (50.0 * target));
      }
      count++;

      double y = (idY - icenter[1]) / iradius[1];
      double x2 = 1.0 - y * y - z * z;
      if (x2 < 0)
      {
        continue;
      }
      double x = sqrt(x2);

      int r1 = subextent[0];
      int r2 = subextent[1];
      double xmin = icenter[0] - x * iradius[0];
      double xmax = icenter[0] + x * iradius[0];

      if (r1 < xmin)
      {
        r1 = svtkMath::Floor(xmin) + 1;
      }
      if (r2 > xmax)
      {
        r2 = svtkMath::Floor(xmax);
      }

      if (r2 >= r1)
      {
        data->InsertNextExtent(r1, r2, idY, idZ);
      }
    } // for idY
  }   // for idZ

  return 1;
}

//----------------------------------------------------------------------------
static int svtkROIStencilSourceCylinderX(svtkROIStencilSource* self, svtkImageStencilData* data,
  const int extent[6], const double origin[3], const double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  svtkROIStencilSourceSubExtent(self, origin, spacing, extent, subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1) * (subextent[3] - subextent[2] + 1) / 50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
  {
    double z = (idZ - icenter[2]) / iradius[2];

    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
    {
      if (count % target == 0)
      {
        self->UpdateProgress(count / (50.0 * target));
      }
      count++;

      double y = (idY - icenter[1]) / iradius[1];
      if (y * y + z * z > 1.0)
      {
        continue;
      }

      int r1 = subextent[0];
      int r2 = subextent[1];

      if (r2 >= r1)
      {
        data->InsertNextExtent(r1, r2, idY, idZ);
      }
    } // for idY
  }   // for idZ

  return 1;
}

//----------------------------------------------------------------------------
static int svtkROIStencilSourceCylinderY(svtkROIStencilSource* self, svtkImageStencilData* data,
  int extent[6], double origin[3], double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  svtkROIStencilSourceSubExtent(self, origin, spacing, extent, subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1) * (subextent[3] - subextent[2] + 1) / 50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
  {
    double z = (idZ - icenter[2]) / iradius[2];

    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
    {
      if (count % target == 0)
      {
        self->UpdateProgress(count / (50.0 * target));
      }
      count++;

      double x2 = 1.0 - z * z;
      if (x2 < 0)
      {
        continue;
      }
      double x = sqrt(x2);

      int r1 = subextent[0];
      int r2 = subextent[1];
      double xmin = icenter[0] - x * iradius[0];
      double xmax = icenter[0] + x * iradius[0];

      if (r1 < xmin)
      {
        r1 = svtkMath::Floor(xmin) + 1;
      }
      if (r2 > xmax)
      {
        r2 = svtkMath::Floor(xmax);
      }

      if (r2 >= r1)
      {
        data->InsertNextExtent(r1, r2, idY, idZ);
      }
    } // for idY
  }   // for idZ

  return 1;
}

//----------------------------------------------------------------------------
static int svtkROIStencilSourceCylinderZ(svtkROIStencilSource* self, svtkImageStencilData* data,
  int extent[6], double origin[3], double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  svtkROIStencilSourceSubExtent(self, origin, spacing, extent, subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1) * (subextent[3] - subextent[2] + 1) / 50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
  {
    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
    {
      if (count % target == 0)
      {
        self->UpdateProgress(count / (50.0 * target));
      }
      count++;

      double y = (idY - icenter[1]) / iradius[1];
      double x2 = 1.0 - y * y;
      if (x2 < 0)
      {
        continue;
      }
      double x = sqrt(x2);

      int r1 = subextent[0];
      int r2 = subextent[1];
      double xmin = icenter[0] - x * iradius[0];
      double xmax = icenter[0] + x * iradius[0];

      if (r1 < xmin)
      {
        r1 = svtkMath::Floor(xmin) + 1;
      }
      if (r2 > xmax)
      {
        r2 = svtkMath::Floor(xmax);
      }

      if (r2 >= r1)
      {
        data->InsertNextExtent(r1, r2, idY, idZ);
      }
    } // for idY
  }   // for idZ

  return 1;
}

//----------------------------------------------------------------------------
int svtkROIStencilSource::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int extent[6];
  double origin[3];
  double spacing[3];
  int result = 1;

  this->Superclass::RequestData(request, inputVector, outputVector);

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageStencilData* data =
    svtkImageStencilData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  outInfo->Get(svtkDataObject::ORIGIN(), origin);
  outInfo->Get(svtkDataObject::SPACING(), spacing);

  switch (this->Shape)
  {
    case svtkROIStencilSource::BOX:
      result = svtkROIStencilSourceBox(this, data, extent, origin, spacing);
      break;
    case svtkROIStencilSource::ELLIPSOID:
      result = svtkROIStencilSourceEllipsoid(this, data, extent, origin, spacing);
      break;
    case svtkROIStencilSource::CYLINDERX:
      result = svtkROIStencilSourceCylinderX(this, data, extent, origin, spacing);
      break;
    case svtkROIStencilSource::CYLINDERY:
      result = svtkROIStencilSourceCylinderY(this, data, extent, origin, spacing);
      break;
    case svtkROIStencilSource::CYLINDERZ:
      result = svtkROIStencilSourceCylinderZ(this, data, extent, origin, spacing);
      break;
  }

  return result;
}
