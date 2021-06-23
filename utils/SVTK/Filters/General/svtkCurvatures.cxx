/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCurvatures.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCurvatures.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataNormals.h"
#include "svtkPolygon.h"
#include "svtkTriangle.h"

#include <memory> // For unique_ptr

svtkStandardNewMacro(svtkCurvatures);

//-------------------------------------------------------//
svtkCurvatures::svtkCurvatures()
{
  this->CurvatureType = SVTK_CURVATURE_GAUSS;
  this->InvertMeanCurvature = 0;
}
//-------------------------------------------------------//
void svtkCurvatures::GetMeanCurvature(svtkPolyData* mesh)
{
  svtkDebugMacro("Start svtkCurvatures::GetMeanCurvature");

  // Empty array check
  if (mesh->GetNumberOfPolys() == 0 || mesh->GetNumberOfPoints() == 0)
  {
    svtkErrorMacro("No points/cells to operate on");
    return;
  }

  int numPts = mesh->GetNumberOfPoints();

  //     create-allocate
  const svtkNew<svtkIdList> vertices;
  const svtkNew<svtkIdList> vertices_n;
  const svtkNew<svtkIdList> neighbours;
  const svtkNew<svtkDoubleArray> meanCurvature;
  meanCurvature->SetName("Mean_Curvature");
  meanCurvature->SetNumberOfComponents(1);
  meanCurvature->SetNumberOfTuples(numPts);
  // Get the array so we can write to it directly
  double* meanCurvatureData = meanCurvature->GetPointer(0);

  //     create-allocate
  double n_f[3]; // normal of facet (could be stored for later?)
  double n_n[3]; // normal of edge
  double t[3];   // to store the cross product of n_f n_n
  double ore[3]; // origin of e
  double end[3]; // end of e
  double oth[3]; //     third vertex necessary for comp of n
  double vn0[3];
  double vn1[3]; // vertices for computation of neighbour's n
  double vn2[3];
  double e[3]; // edge (oriented)

  mesh->BuildLinks();
  // data init
  const int F = mesh->GetNumberOfCells();
  // init, preallocate the mean curvature
  const std::unique_ptr<int[]> num_neighb(new int[numPts]);
  for (int v = 0; v < numPts; v++)
  {
    meanCurvatureData[v] = 0.0;
    num_neighb[v] = 0;
  }

  //     main loop
  svtkDebugMacro(<< "Main loop: loop over facets such that id > id of neighb");
  svtkDebugMacro(<< "so that every edge comes only once");
  //
  for (int f = 0; f < F; f++)
  {
    mesh->GetCellPoints(f, vertices);
    const int nv = vertices->GetNumberOfIds();

    for (int v = 0; v < nv; v++)
    {
      // get neighbour
      const int v_l = vertices->GetId(v);
      const int v_r = vertices->GetId((v + 1) % nv);
      const int v_o = vertices->GetId((v + 2) % nv);
      mesh->GetCellEdgeNeighbors(f, v_l, v_r, neighbours);

      int n; // n short for neighbor

      // compute only if there is really ONE neighbour
      // AND meanCurvature has not been computed yet!
      // (ensured by n > f)
      if (neighbours->GetNumberOfIds() == 1 && (n = neighbours->GetId(0)) > f)
      {
        double Hf; // temporary store

        // find 3 corners of f: in order!
        mesh->GetPoint(v_l, ore);
        mesh->GetPoint(v_r, end);
        mesh->GetPoint(v_o, oth);
        // compute normal of f
        svtkTriangle::ComputeNormal(ore, end, oth, n_f);
        // compute common edge
        e[0] = end[0];
        e[1] = end[1];
        e[2] = end[2];
        e[0] -= ore[0];
        e[1] -= ore[1];
        e[2] -= ore[2];
        const double length = svtkMath::Normalize(e);
        double Af = svtkTriangle::TriangleArea(ore, end, oth);
        // find 3 corners of n: in order!
        mesh->GetCellPoints(n, vertices_n);
        mesh->GetPoint(vertices_n->GetId(0), vn0);
        mesh->GetPoint(vertices_n->GetId(1), vn1);
        mesh->GetPoint(vertices_n->GetId(2), vn2);
        Af += double(svtkTriangle::TriangleArea(vn0, vn1, vn2));
        // compute normal of n
        svtkTriangle::ComputeNormal(vn0, vn1, vn2, n_n);
        // the cosine is n_f * n_n
        const double cs = svtkMath::Dot(n_f, n_n);
        // the sin is (n_f x n_n) * e
        svtkMath::Cross(n_f, n_n, t);
        const double sn = svtkMath::Dot(t, e);
        // signed angle in [-pi,pi]
        if (sn != 0.0 || cs != 0.0)
        {
          const double angle = atan2(sn, cs);
          Hf = length * angle;
        }
        else
        {
          Hf = 0.0;
        }
        // add weighted Hf to scalar at v_l and v_r
        if (Af != 0.0)
        {
          (Hf /= Af) *= 3.0;
        }
        meanCurvatureData[v_l] += Hf;
        meanCurvatureData[v_r] += Hf;
        num_neighb[v_l] += 1;
        num_neighb[v_r] += 1;
      }
    }
  }

  // put curvature in svtkArray
  for (int v = 0; v < numPts; v++)
  {
    if (num_neighb[v] > 0)
    {
      const double Hf = 0.5 * meanCurvatureData[v] / num_neighb[v];
      if (this->InvertMeanCurvature)
      {
        meanCurvatureData[v] = -Hf;
      }
      else
      {
        meanCurvatureData[v] = Hf;
      }
    }
    else
    {
      meanCurvatureData[v] = 0.0;
    }
  }

  mesh->GetPointData()->AddArray(meanCurvature);
  mesh->GetPointData()->SetActiveScalars("Mean_Curvature");

  svtkDebugMacro("Set Values of Mean Curvature: Done");
};
//--------------------------------------------
#define CLAMP_MACRO(v) ((v) < (-1) ? (-1) : (v) > (1) ? (1) : (v))
void svtkCurvatures::GetGaussCurvature(svtkPolyData* output)
{
  svtkDebugMacro("Start svtkCurvatures::GetGaussCurvature()");
  // svtk data
  svtkCellArray* facets = output->GetPolys();

  // Empty array check
  if (output->GetNumberOfPolys() == 0 || output->GetNumberOfPoints() == 0)
  {
    svtkErrorMacro("No points/cells to operate on");
    return;
  }

  // other data
  svtkIdType Nv = output->GetNumberOfPoints();

  const std::unique_ptr<double[]> K(new double[Nv]);
  const std::unique_ptr<double[]> dA(new double[Nv]);
  double pi2 = 2.0 * svtkMath::Pi();
  for (int k = 0; k < Nv; k++)
  {
    K[k] = pi2;
    dA[k] = 0.0;
  }

  double v0[3], v1[3], v2[3], e0[3], e1[3], e2[3];

  double A, alpha0, alpha1, alpha2;

  svtkIdType f;
  const svtkIdType* vert = nullptr;
  facets->InitTraversal();
  while (facets->GetNextCell(f, vert))
  {
    output->GetPoint(vert[0], v0);
    output->GetPoint(vert[1], v1);
    output->GetPoint(vert[2], v2);
    // edges
    e0[0] = v1[0];
    e0[1] = v1[1];
    e0[2] = v1[2];
    e0[0] -= v0[0];
    e0[1] -= v0[1];
    e0[2] -= v0[2];

    e1[0] = v2[0];
    e1[1] = v2[1];
    e1[2] = v2[2];
    e1[0] -= v1[0];
    e1[1] -= v1[1];
    e1[2] -= v1[2];

    e2[0] = v0[0];
    e2[1] = v0[1];
    e2[2] = v0[2];
    e2[0] -= v2[0];
    e2[1] -= v2[1];
    e2[2] -= v2[2];

    // normalise
    svtkMath::Normalize(e0);
    svtkMath::Normalize(e1);
    svtkMath::Normalize(e2);
    // angles
    // I get lots of acos domain errors so clamp the value to +/-1 as the
    // normalize function can return 1.000000001 etc (I think)
    double ac1 = svtkMath::Dot(e1, e2);
    double ac2 = svtkMath::Dot(e2, e0);
    double ac3 = svtkMath::Dot(e0, e1);
    alpha0 = acos(-CLAMP_MACRO(ac1));
    alpha1 = acos(-CLAMP_MACRO(ac2));
    alpha2 = acos(-CLAMP_MACRO(ac3));

    // surf. area
    A = double(svtkTriangle::TriangleArea(v0, v1, v2));
    // UPDATE
    dA[vert[0]] += A;
    dA[vert[1]] += A;
    dA[vert[2]] += A;
    K[vert[0]] -= alpha1;
    K[vert[1]] -= alpha2;
    K[vert[2]] -= alpha0;
  }

  int numPts = output->GetNumberOfPoints();
  // put curvature in svtkArray
  const svtkNew<svtkDoubleArray> gaussCurvature;
  gaussCurvature->SetName("Gauss_Curvature");
  gaussCurvature->SetNumberOfComponents(1);
  gaussCurvature->SetNumberOfTuples(numPts);
  double* gaussCurvatureData = gaussCurvature->GetPointer(0);

  for (int v = 0; v < Nv; v++)
  {
    if (dA[v] > 0.0)
    {
      gaussCurvatureData[v] = 3.0 * K[v] / dA[v];
    }
    else
    {
      gaussCurvatureData[v] = 0.0;
    }
  }

  output->GetPointData()->AddArray(gaussCurvature);
  output->GetPointData()->SetActiveScalars("Gauss_Curvature");

  svtkDebugMacro("Set Values of Gauss Curvature: Done");
};

void svtkCurvatures::GetMaximumCurvature(svtkPolyData* input, svtkPolyData* output)
{
  this->GetGaussCurvature(output);
  this->GetMeanCurvature(output);

  svtkIdType numPts = input->GetNumberOfPoints();

  const svtkNew<svtkDoubleArray> maximumCurvature;
  maximumCurvature->SetNumberOfComponents(1);
  maximumCurvature->SetNumberOfTuples(numPts);
  maximumCurvature->SetName("Maximum_Curvature");
  output->GetPointData()->AddArray(maximumCurvature);
  output->GetPointData()->SetActiveScalars("Maximum_Curvature");

  svtkDoubleArray* gauss =
    static_cast<svtkDoubleArray*>(output->GetPointData()->GetArray("Gauss_Curvature"));
  svtkDoubleArray* mean =
    static_cast<svtkDoubleArray*>(output->GetPointData()->GetArray("Mean_Curvature"));
  double k, h, k_max, tmp;

  for (svtkIdType i = 0; i < numPts; i++)
  {
    k = gauss->GetComponent(i, 0);
    h = mean->GetComponent(i, 0);
    tmp = h * h - k;
    if (tmp >= 0)
    {
      k_max = h + sqrt(tmp);
    }
    else
    {
      svtkDebugMacro(<< "Maximum Curvature undefined at point: " << i);
      // k_max can be any real number. Undefined points will be indistinguishable
      // from points that actually have a k_max == 0
      k_max = 0;
    }
    maximumCurvature->SetComponent(i, 0, k_max);
  }
}

void svtkCurvatures::GetMinimumCurvature(svtkPolyData* input, svtkPolyData* output)
{
  this->GetGaussCurvature(output);
  this->GetMeanCurvature(output);

  svtkIdType numPts = input->GetNumberOfPoints();

  const svtkNew<svtkDoubleArray> minimumCurvature;
  minimumCurvature->SetNumberOfComponents(1);
  minimumCurvature->SetNumberOfTuples(numPts);
  minimumCurvature->SetName("Minimum_Curvature");
  output->GetPointData()->AddArray(minimumCurvature);
  output->GetPointData()->SetActiveScalars("Minimum_Curvature");

  svtkDoubleArray* gauss =
    static_cast<svtkDoubleArray*>(output->GetPointData()->GetArray("Gauss_Curvature"));
  svtkDoubleArray* mean =
    static_cast<svtkDoubleArray*>(output->GetPointData()->GetArray("Mean_Curvature"));
  double k, h, k_min, tmp;

  for (svtkIdType i = 0; i < numPts; i++)
  {
    k = gauss->GetComponent(i, 0);
    h = mean->GetComponent(i, 0);
    tmp = h * h - k;
    if (tmp >= 0)
    {
      k_min = h - sqrt(tmp);
    }
    else
    {
      svtkDebugMacro(<< "Minimum Curvature undefined at point: " << i);
      // k_min can be any real number. Undefined points will be indistinguishable
      // from points that actually have a k_min == 0
      k_min = 0;
    }
    minimumCurvature->SetComponent(i, 0, k_min);
  }
}

//-------------------------------------------------------
int svtkCurvatures::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Null input check
  if (!input)
  {
    return 0;
  }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  //-------------------------------------------------------//
  //    Set Curvatures as PointData Scalars                //
  //-------------------------------------------------------//

  if (this->CurvatureType == SVTK_CURVATURE_GAUSS)
  {
    this->GetGaussCurvature(output);
  }
  else if (this->CurvatureType == SVTK_CURVATURE_MEAN)
  {
    this->GetMeanCurvature(output);
  }
  else if (this->CurvatureType == SVTK_CURVATURE_MAXIMUM)
  {
    this->GetMaximumCurvature(input, output);
  }
  else if (this->CurvatureType == SVTK_CURVATURE_MINIMUM)
  {
    this->GetMinimumCurvature(input, output);
  }
  else
  {
    svtkErrorMacro("Only Gauss, Mean, Max, and Min Curvature type available");
    return 1;
  }

  return 1;
}
/*-------------------------------------------------------*/
void svtkCurvatures::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CurvatureType: " << this->CurvatureType << "\n";
  os << indent << "InvertMeanCurvature: " << this->InvertMeanCurvature << "\n";
}
