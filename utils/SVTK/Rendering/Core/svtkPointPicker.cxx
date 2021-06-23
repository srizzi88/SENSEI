/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointPicker.h"

#include "svtkAbstractVolumeMapper.h"
#include "svtkBox.h"
#include "svtkCellArray.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkMapper.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkProp3D.h"

inline svtkCellArray* GET_CELLS(int cell_type, svtkPolyData* poly_input)
{
  switch (cell_type)
  {
    case 0:
      return poly_input->GetVerts();
    case 1:
      return poly_input->GetLines();
    case 2:
      return poly_input->GetPolys();
    case 3:
      return poly_input->GetStrips();
  }
  return nullptr;
}

svtkStandardNewMacro(svtkPointPicker);

svtkPointPicker::svtkPointPicker()
{
  this->PointId = -1;
  this->UseCells = 0;
}

double svtkPointPicker::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  svtkAssemblyPath* path, svtkProp3D* p, svtkAbstractMapper3D* m)
{
  svtkIdType minPtId = -1;
  double tMin = SVTK_DOUBLE_MAX;
  double minXYZ[3];
  svtkDataSet* input;
  svtkMapper* mapper;
  svtkAbstractVolumeMapper* volumeMapper = nullptr;
  svtkImageMapper3D* imageMapper = nullptr;

  double ray[3], rayFactor;
  if (!svtkPicker::CalculateRay(p1, p2, ray, rayFactor))
  {
    svtkDebugMacro("Zero length ray");
    return 2.0;
  }

  // Get the underlying dataset.
  //
  if ((mapper = svtkMapper::SafeDownCast(m)) != nullptr)
  {
    input = mapper->GetInput();
  }
  else if ((volumeMapper = svtkAbstractVolumeMapper::SafeDownCast(m)) != nullptr)
  {
    input = volumeMapper->GetDataSetInput();
  }
  else if ((imageMapper = svtkImageMapper3D::SafeDownCast(m)) != nullptr)
  {
    input = imageMapper->GetInput();
  }
  else
  {
    return 2.0;
  }

  //   For image, find the single intersection point
  //
  if (imageMapper != nullptr)
  {
    if (input->GetNumberOfPoints() == 0)
    {
      svtkDebugMacro("No points in input");
      return 2.0;
    }

    // Get the slice plane for the image and intersect with ray
    double normal[4];
    imageMapper->GetSlicePlaneInDataCoords(p->GetMatrix(), normal);
    double w1 = svtkMath::Dot(p1, normal) + normal[3];
    double w2 = svtkMath::Dot(p2, normal) + normal[3];
    if (w1 * w2 >= 0)
    {
      w1 = 0.0;
      w2 = 1.0;
    }
    double w = (w2 - w1);
    double x[3];
    x[0] = (p1[0] * w2 - p2[0] * w1) / w;
    x[1] = (p1[1] * w2 - p2[1] * w1) / w;
    x[2] = (p1[2] * w2 - p2[2] * w1) / w;

    // Get the one point that will be checked
    minPtId = input->FindPoint(x);
    if (minPtId > -1)
    {
      input->GetPoint(minPtId, minXYZ);
      double distMin = SVTK_DOUBLE_MAX;
      this->UpdateClosestPoint(minXYZ, p1, ray, rayFactor, tol, tMin, distMin);

      //  Now compare this against other actors.
      //
      if (tMin < this->GlobalTMin)
      {
        this->MarkPicked(path, p, m, tMin, minXYZ);
        this->PointId = minPtId;
        svtkDebugMacro("Picked point id= " << minPtId);
      }
    }
  }
  else if (input)
  {
    //  Project each point onto ray.  Keep track of the one within the
    //  tolerance and closest to the eye (and within the clipping range).
    //
    minPtId = this->IntersectDataSetWithLine(p1, ray, rayFactor, tol, input, tMin, minXYZ);

    //  Now compare this against other actors.
    //
    if (minPtId > -1 && tMin < this->GlobalTMin)
    {
      this->MarkPicked(path, p, m, tMin, minXYZ);
      this->PointId = minPtId;
      svtkDebugMacro("Picked point id= " << minPtId);
    }
  }
  else if (mapper != nullptr)
  {
    // a mapper mapping composite dataset input returns a nullptr svtkDataSet.
    // Iterate over all leaf datasets and find the closest point in any of
    // the leaf data sets
    svtkCompositeDataSet* composite =
      svtkCompositeDataSet::SafeDownCast(mapper->GetInputDataObject(0, 0));
    if (composite)
    {
      svtkIdType flatIndex = -1;
      svtkSmartPointer<svtkCompositeDataIterator> iter;
      iter.TakeReference(composite->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (!ds)
        {
          svtkDebugMacro(<< "Skipping " << iter->GetCurrentDataObject()->GetClassName()
                        << " block at index " << iter->GetCurrentFlatIndex());
          continue;
        }

        // First check if the bounding box of the data set is hit.
        double bounds[6];
        ds->GetBounds(bounds);
        bounds[0] -= tol;
        bounds[1] += tol;
        bounds[2] -= tol;
        bounds[3] += tol;
        bounds[4] -= tol;
        bounds[5] += tol;
        double tDummy;
        double xyzDummy[3];

        // only intersect dataset if bounding box is hit
        if (svtkBox::IntersectBox(bounds, p1, ray, xyzDummy, tDummy))
        {
          svtkIdType ptId =
            this->IntersectDataSetWithLine(p1, ray, rayFactor, tol, ds, tMin, minXYZ);
          if (ptId > -1)
          {
            input = ds;
            minPtId = ptId;
            flatIndex = iter->GetCurrentFlatIndex();
          }
        }
      }
      if (minPtId > -1 && tMin < this->GlobalTMin)
      {
        this->MarkPickedData(path, tMin, minXYZ, mapper, input, flatIndex);
        this->PointId = minPtId;
        svtkDebugMacro("Picked point id= " << minPtId << " in block " << flatIndex);
      }
    }
  }
  return tMin;
}

svtkIdType svtkPointPicker::IntersectDataSetWithLine(const double p1[3], double ray[3],
  double rayFactor, double tol, svtkDataSet* dataSet, double& tMin, double minXYZ[3])
{
  if (dataSet->GetNumberOfPoints() == 0)
  {
    svtkDebugMacro("No points in input");
    return 2.0;
  }
  svtkIdType minPtId = -1;
  svtkPolyData* poly_input = svtkPolyData::SafeDownCast(dataSet);
  if (this->UseCells && (poly_input != nullptr))
  {
    double minPtDist = SVTK_DOUBLE_MAX;

    for (int iCellType = 0; iCellType < 4; iCellType++)
    {
      svtkCellArray* cells = GET_CELLS(iCellType, poly_input);
      if (cells != nullptr)
      {
        cells->InitTraversal();
        svtkIdType n_cell_pts = 0;
        const svtkIdType* pt_ids = nullptr;
        while (cells->GetNextCell(n_cell_pts, pt_ids))
        {
          for (svtkIdType ptIndex = 0; ptIndex < n_cell_pts; ptIndex++)
          {
            svtkIdType ptId = pt_ids[ptIndex];
            double x[3];
            dataSet->GetPoint(ptId, x);

            if (this->UpdateClosestPoint(x, p1, ray, rayFactor, tol, tMin, minPtDist))
            {
              minPtId = ptId;
              minXYZ[0] = x[0];
              minXYZ[1] = x[1];
              minXYZ[2] = x[2];
            }
          }
        }
      }
    }
  }
  else
  {
    svtkIdType numPts = dataSet->GetNumberOfPoints();
    double minPtDist = SVTK_DOUBLE_MAX;

    for (svtkIdType ptId = 0; ptId < numPts; ptId++)
    {
      double x[3];
      dataSet->GetPoint(ptId, x);
      if (this->UpdateClosestPoint(x, p1, ray, rayFactor, tol, tMin, minPtDist))
      {
        minPtId = ptId;
        minXYZ[0] = x[0];
        minXYZ[1] = x[1];
        minXYZ[2] = x[2];
      }
    }
  }
  return minPtId;
}

bool svtkPointPicker::UpdateClosestPoint(double x[3], const double p1[3], double ray[3],
  double rayFactor, double tol, double& tMin, double& distMin)
{
  double t =
    (ray[0] * (x[0] - p1[0]) + ray[1] * (x[1] - p1[1]) + ray[2] * (x[2] - p1[2])) / rayFactor;

  // If we find a point closer than we currently have, see whether it
  // lies within the pick tolerance and clipping planes. We keep track
  // of the point closest to the line (use a fudge factor for points
  // nearly the same distance away.)

  if (t < 0. || t > 1. || t > tMin + this->Tolerance)
  {
    return false;
  }

  double maxDist = 0.0;
  double projXYZ[3];
  for (int i = 0; i < 3; i++)
  {
    projXYZ[i] = p1[i] + t * ray[i];
    double dist = fabs(x[i] - projXYZ[i]);
    if (dist > maxDist)
    {
      maxDist = dist;
    }
  }

  if (maxDist <= tol && maxDist < distMin)
  {
    distMin = maxDist;
    tMin = t;
    return true;
  }
  return false;
}

void svtkPointPicker::Initialize()
{
  this->PointId = (-1);
  this->svtkPicker::Initialize();
}

void svtkPointPicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point Id: " << this->PointId << "\n";
}
