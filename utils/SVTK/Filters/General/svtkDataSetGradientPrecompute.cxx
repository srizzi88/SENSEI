/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetGradientPrecompute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm
// contributed by CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br> BP12,
// F-91297 Arpajon, France. <br> Implementation by Thierry Carrard (CEA)

#include "svtkDataSetGradientPrecompute.h"

#include "svtkCell.h"
#include "svtkCell3D.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkTetra.h"
#include "svtkTriangle.h"

#define SVTK_DATASET_GRADIENT_TETRA_OPTIMIZATION
#define SVTK_DATASET_GRADIENT_TRIANGLE_OPTIMIZATION
//#define DEBUG

svtkStandardNewMacro(svtkDataSetGradientPrecompute);

svtkDataSetGradientPrecompute::svtkDataSetGradientPrecompute() = default;

svtkDataSetGradientPrecompute::~svtkDataSetGradientPrecompute() = default;

void svtkDataSetGradientPrecompute::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

#define ADD_VEC(a, b)                                                                              \
  a[0] += b[0];                                                                                    \
  a[1] += b[1];                                                                                    \
  a[2] += b[2]
#define SCALE_VEC(a, b)                                                                            \
  a[0] *= b;                                                                                       \
  a[1] *= b;                                                                                       \
  a[2] *= b
#define ZERO_VEC(a)                                                                                \
  a[0] = 0;                                                                                        \
  a[1] = 0;                                                                                        \
  a[2] = 0
#define MAX_CELL_POINTS 128
#define SVTK_CQS_EPSILON 1e-12

static inline void TETRA_CQS_VECTOR(
  double v0[3], double v1[3], double v2[3], double p[3], double cqs[3])
{
  double surface = fabs(svtkTriangle::TriangleArea(v0, v1, v2));

  svtkTriangle::ComputeNormal(v0, v1, v2, cqs);

  // inverse face normal if not toward opposite vertex
  double edge[3];
  edge[0] = p[0] - v0[0];
  edge[1] = p[1] - v0[1];
  edge[2] = p[2] - v0[2];
  if (svtkMath::Dot(edge, cqs) < 0)
  {
    cqs[0] = -cqs[0];
    cqs[1] = -cqs[1];
    cqs[2] = -cqs[2];
  }

  SCALE_VEC(cqs, surface / 2.0);
}

static inline void TRIANGLE_CQS_VECTOR(double v0[3], double v1[3], double p[3], double cqs[3])
{
  double length = sqrt(svtkMath::Distance2BetweenPoints(v0, v1));
  double a[3], b[3], c[3];
  for (int i = 0; i < 3; i++)
  {
    a[i] = v1[i] - v0[i];
    b[i] = p[i] - v0[i];
  }
  svtkMath::Cross(a, b, c);
  svtkMath::Cross(c, a, cqs);
  svtkMath::Normalize(cqs);
  SCALE_VEC(cqs, length / 2.0);
}

static inline void LINE_CQS_VECTOR(double v0[3], double p[3], double cqs[3])
{
  cqs[0] = p[0] - v0[0];
  cqs[1] = p[1] - v0[1];
  cqs[2] = p[2] - v0[2];
  svtkMath::Normalize(cqs);
}

int svtkDataSetGradientPrecompute::GradientPrecompute(svtkDataSet* ds)
{
  svtkIdType nCells = ds->GetNumberOfCells();
  svtkIdType nCellNodes = 0;
  for (svtkIdType i = 0; i < nCells; i++)
  {
    nCellNodes += ds->GetCell(i)->GetNumberOfPoints();
  }

  svtkDoubleArray* cqs = svtkDoubleArray::New();
  cqs->SetName("GradientPrecomputation");
  cqs->SetNumberOfComponents(3);
  cqs->SetNumberOfTuples(nCellNodes);
  cqs->FillComponent(0, 0.0);
  cqs->FillComponent(1, 0.0);
  cqs->FillComponent(2, 0.0);

  // The cell size determines the amount of space the cell takes up.  For 3D
  // cells this is the volume.  For 2D cells this is the area.  For 1D cells
  // this is the length.  For 0D cells this is undefined, but we set it to 1 so
  // as not to get invalid results when normalizing something by the cell size.
  svtkDoubleArray* cellSize = svtkDoubleArray::New();
  cellSize->SetName("CellSize");
  cellSize->SetNumberOfTuples(nCells);

  svtkIdType curPoint = 0;
  for (svtkIdType c = 0; c < nCells; c++)
  {
    svtkCell* cell = ds->GetCell(c);
    int np = cell->GetNumberOfPoints();

    double cellCenter[3] = { 0, 0, 0 };
    double cellPoints[MAX_CELL_POINTS][3];
    double cellVectors[MAX_CELL_POINTS][3];
    double tmp[3];
    double size = 0.0;

    for (int p = 0; p < np; p++)
    {
      ds->GetPoint(cell->GetPointId(p), cellPoints[p]);
      ADD_VEC(cellCenter, cellPoints[p]);
      ZERO_VEC(cellVectors[p]);
    }
    SCALE_VEC(cellCenter, 1.0 / np);

    // -= 3 D =-
    if (cell->GetCellDimension() == 3)
    {
#ifdef SVTK_DATASET_GRADIENT_TETRA_OPTIMIZATION
      if (np == 4) // cell is a tetrahedra
      {
        // svtkWarningMacro(<<"Tetra detected\n");
        size = fabs(svtkTetra::ComputeVolume(
                 cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3])) *
          1.5;

        TETRA_CQS_VECTOR(cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3], tmp);
        ADD_VEC(cellVectors[3], tmp);

        TETRA_CQS_VECTOR(cellPoints[1], cellPoints[2], cellPoints[3], cellPoints[0], tmp);
        ADD_VEC(cellVectors[0], tmp);

        TETRA_CQS_VECTOR(cellPoints[2], cellPoints[3], cellPoints[0], cellPoints[1], tmp);
        ADD_VEC(cellVectors[1], tmp);

        TETRA_CQS_VECTOR(cellPoints[3], cellPoints[0], cellPoints[1], cellPoints[2], tmp);
        ADD_VEC(cellVectors[2], tmp);
      }
      else if (np > 4)
#endif
      {
        svtkCell3D* cell3d = static_cast<svtkCell3D*>(cell);
        int nf = cell->GetNumberOfFaces();
        for (int f = 0; f < nf; f++)
        {
          const svtkIdType* faceIds = nullptr;
          int nfp = cell->GetFace(f)->GetNumberOfPoints();
          cell3d->GetFacePoints(f, faceIds);
#ifdef SVTK_DATASET_GRADIENT_TRIANGLE_OPTIMIZATION
          if (nfp == 3) // face is a triangle
          {
            // svtkWarningMacro(<<"triangular face detected\n");
            size += fabs(svtkTetra::ComputeVolume(cellCenter, cellPoints[faceIds[0]],
                      cellPoints[faceIds[1]], cellPoints[faceIds[2]])) *
              1.5;

            TETRA_CQS_VECTOR(cellCenter, cellPoints[faceIds[0]], cellPoints[faceIds[1]],
              cellPoints[faceIds[2]], tmp);
            ADD_VEC(cellVectors[faceIds[2]], tmp);

            TETRA_CQS_VECTOR(cellCenter, cellPoints[faceIds[1]], cellPoints[faceIds[2]],
              cellPoints[faceIds[0]], tmp);
            ADD_VEC(cellVectors[faceIds[0]], tmp);

            TETRA_CQS_VECTOR(cellCenter, cellPoints[faceIds[2]], cellPoints[faceIds[0]],
              cellPoints[faceIds[1]], tmp);
            ADD_VEC(cellVectors[faceIds[1]], tmp);
          }
          else if (nfp > 3) // generic case
#endif
          {
            double faceCenter[3] = { 0, 0, 0 };
            for (int p = 0; p < nfp; p++)
            {
              ADD_VEC(faceCenter, cellPoints[faceIds[p]]);
            }
            SCALE_VEC(faceCenter, 1.0 / nfp);
            for (int p = 0; p < nfp; p++)
            {
              int p2 = (p + 1) % nfp;
              size += fabs(svtkTetra::ComputeVolume(
                cellCenter, faceCenter, cellPoints[faceIds[p]], cellPoints[faceIds[p2]]));

              TETRA_CQS_VECTOR(
                cellCenter, faceCenter, cellPoints[faceIds[p]], cellPoints[faceIds[p2]], tmp);
              ADD_VEC(cellVectors[faceIds[p2]], tmp);

              TETRA_CQS_VECTOR(
                cellCenter, faceCenter, cellPoints[faceIds[p2]], cellPoints[faceIds[p]], tmp);
              ADD_VEC(cellVectors[faceIds[p]], tmp);
            }
          }
        }
      }
    }

    // -= 2 D =-
    else if (cell->GetCellDimension() == 2)
    {
      if (np == 3) // cell is a triangle
      {
        size = fabs(svtkTriangle::TriangleArea(cellPoints[0], cellPoints[1], cellPoints[2]));

        TRIANGLE_CQS_VECTOR(cellPoints[0], cellPoints[1], cellPoints[2], tmp);
        ADD_VEC(cellVectors[2], tmp);

        TRIANGLE_CQS_VECTOR(cellPoints[1], cellPoints[2], cellPoints[0], tmp);
        ADD_VEC(cellVectors[0], tmp);

        TRIANGLE_CQS_VECTOR(cellPoints[2], cellPoints[0], cellPoints[1], tmp);
        ADD_VEC(cellVectors[1], tmp);
      }
      else if (np > 3) // generic case
      {
        for (int f = 0; f < np; f++)
        {
          const int e0 = f;
          const int e1 = (f + 1) % np;
          size += fabs(svtkTriangle::TriangleArea(cellCenter, cellPoints[e0], cellPoints[e1]));
          TRIANGLE_CQS_VECTOR(cellCenter, cellPoints[e0], cellPoints[e1], tmp);
          ADD_VEC(cellVectors[e1], tmp);

          TRIANGLE_CQS_VECTOR(cellCenter, cellPoints[e1], cellPoints[e0], tmp);
          ADD_VEC(cellVectors[e0], tmp);
        }
      }
      else
      {
        // svtkWarningMacro(<<"Can't process 2D cells with less than 3 points.");
        // return 0;
      }
    }

    // -= 1 D =-
    else if (cell->GetCellDimension() == 1)
    {
      if (np == 2) // cell is a single line segment
      {
        size = sqrt(svtkMath::Distance2BetweenPoints(cellPoints[0], cellPoints[1]));

        LINE_CQS_VECTOR(cellPoints[0], cellPoints[1], tmp);
        ADD_VEC(cellVectors[1], tmp);

        LINE_CQS_VECTOR(cellPoints[1], cellPoints[0], tmp);
        ADD_VEC(cellVectors[0], tmp);
      }
      else if (np > 2) // generic case, a poly line
      {
        for (int p = 0; p < np; p++)
        {
          size += sqrt(svtkMath::Distance2BetweenPoints(cellCenter, cellPoints[p]));
          LINE_CQS_VECTOR(cellCenter, cellPoints[p], tmp);
          ADD_VEC(cellVectors[p], tmp);
        }
      }
    }

    // -= 0 D =-
    else
    {
      // For vertex cells, estimate gradient as weighted sum of vectors from
      // centroid.
      size = 1.0;
      for (int p = 0; p < np; p++)
      {
        cellVectors[p][0] = cellPoints[p][0] - cellCenter[0];
        cellVectors[p][1] = cellPoints[p][1] - cellCenter[1];
        cellVectors[p][2] = cellPoints[p][2] - cellCenter[2];
      }
    }

    cellSize->SetTuple1(c, size);
    for (int p = 0; p < np; ++p)
    {
      cqs->SetTuple(curPoint + p, cellVectors[p]);
    }

    // check cqs consistency
#ifdef DEBUG
    double checkZero[3] = { 0, 0, 0 };
    double checkVolume = 0;
    for (int p = 0; p < np; p++)
    {
      checkVolume += svtkMath::Dot(cellPoints[p], cellVectors[p]);
      ADD_VEC(checkZero, cellVectors[p]);
    }
    checkVolume /= (double)cell->GetCellDimension();

    if (svtkMath::Norm(checkZero) > SVTK_CQS_EPSILON || fabs(size - checkVolume) > SVTK_CQS_EPSILON)
    {
      cout << "Bad CQS sum at cell #" << c << ", Sum=" << svtkMath::Norm(checkZero)
           << ", volume=" << size << ", ratio Vol=" << size / checkVolume << "\n";
    }
#endif

    curPoint += np;
  }

  ds->GetFieldData()->AddArray(cqs);
  ds->GetCellData()->AddArray(cellSize);
  cqs->Delete();
  cellSize->Delete();

  return 1;
}

int svtkDataSetGradientPrecompute::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get connected input & output
  svtkDataSet* _output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* _input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (_input == nullptr || _output == nullptr)
  {
    svtkErrorMacro(<< "missing input/output connection\n");
    return 0;
  }

  _output->ShallowCopy(_input);
  return svtkDataSetGradientPrecompute::GradientPrecompute(_output);
}
