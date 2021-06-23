/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSMPTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSMPTransform.h"
#include "svtkDataArray.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"

#include "svtkSMPTools.h"

#include <cstdlib>

svtkStandardNewMacro(svtkSMPTransform);

svtkSMPTransform::svtkSMPTransform()
{
  SVTK_LEGACY_BODY(svtkSMPTransform::svtkSMPTransform, "SVTK 8.1");
}

//----------------------------------------------------------------------------
void svtkSMPTransform::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void svtkSMPTransformPoint(T1 matrix[4][4], T2 in[3], T3 out[3])
{
  T3 x = static_cast<T3>(
    matrix[0][0] * in[0] + matrix[0][1] * in[1] + matrix[0][2] * in[2] + matrix[0][3]);
  T3 y = static_cast<T3>(
    matrix[1][0] * in[0] + matrix[1][1] * in[1] + matrix[1][2] * in[2] + matrix[1][3]);
  T3 z = static_cast<T3>(
    matrix[2][0] * in[0] + matrix[2][1] * in[1] + matrix[2][2] * in[2] + matrix[2][3]);

  out[0] = x;
  out[1] = y;
  out[2] = z;
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3, class T4>
inline void svtkSMPTransformDerivative(T1 matrix[4][4], T2 in[3], T3 out[3], T4 derivative[3][3])
{
  svtkSMPTransformPoint(matrix, in, out);

  for (int i = 0; i < 3; i++)
  {
    derivative[0][i] = static_cast<T4>(matrix[0][i]);
    derivative[1][i] = static_cast<T4>(matrix[1][i]);
    derivative[2][i] = static_cast<T4>(matrix[2][i]);
  }
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void svtkSMPTransformVector(T1 matrix[4][4], T2 in[3], T3 out[3])
{
  T3 x = static_cast<T3>(matrix[0][0] * in[0] + matrix[0][1] * in[1] + matrix[0][2] * in[2]);
  T3 y = static_cast<T3>(matrix[1][0] * in[0] + matrix[1][1] * in[1] + matrix[1][2] * in[2]);
  T3 z = static_cast<T3>(matrix[2][0] * in[0] + matrix[2][1] * in[1] + matrix[2][2] * in[2]);

  out[0] = x;
  out[1] = y;
  out[2] = z;
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void svtkSMPTransformNormal(T1 mat[4][4], T2 in[3], T3 out[3])
{
  // to transform the normal, multiply by the transposed inverse matrix
  T1 matrix[4][4];
  memcpy(*matrix, *mat, 16 * sizeof(T1));
  svtkMatrix4x4::Invert(*matrix, *matrix);
  svtkMatrix4x4::Transpose(*matrix, *matrix);

  svtkSMPTransformVector(matrix, in, out);

  svtkMath::Normalize(out);
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the
// transformation.  Either inNms or inVrs can be set to nullptr.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform.
class TransformAllFunctor
{
public:
  svtkPoints* inPts;
  svtkPoints* outPts;
  svtkDataArray* inNms;
  svtkDataArray* outNms;
  svtkDataArray* inVcs;
  svtkDataArray* outVcs;
  int nOptionalVectors;
  svtkDataArray** inVrsArr;
  svtkDataArray** outVrsArr;
  double (*matrix)[4];
  double (*matrixInvTr)[4];
  void operator()(svtkIdType begin, svtkIdType end) const
  {
    for (svtkIdType id = begin; id < end; id++)
    {
      double point[3];
      inPts->GetPoint(id, point);
      svtkSMPTransformPoint(matrix, point, point);
      outPts->SetPoint(id, point);
      if (inVcs)
      {
        inVcs->GetTuple(id, point);
        svtkSMPTransformVector(matrix, point, point);
        outVcs->SetTuple(id, point);
      }
      if (inVrsArr)
      {
        for (int iArr = 0; iArr < nOptionalVectors; iArr++)
        {
          inVrsArr[iArr]->GetTuple(id, point);
          svtkSMPTransformVector(matrix, point, point);
          outVrsArr[iArr]->SetTuple(id, point);
        }
      }
      if (inNms)
      {
        inNms->GetTuple(id, point);
        svtkSMPTransformVector(matrixInvTr, point, point);
        svtkMath::Normalize(point);
        outNms->SetTuple(id, point);
      }
    }
  }
};

void svtkSMPTransform::TransformPointsNormalsVectors(svtkPoints* inPts, svtkPoints* outPts,
  svtkDataArray* inNms, svtkDataArray* outNms, svtkDataArray* inVrs, svtkDataArray* outVrs,
  int nOptionalVectors, svtkDataArray** inVrsArr, svtkDataArray** outVrsArr)
{
  svtkIdType n = inPts->GetNumberOfPoints();
  double matrix[4][4];
  this->Update();

  TransformAllFunctor functor;
  functor.inPts = inPts;
  functor.outPts = outPts;
  functor.inNms = inNms;
  functor.outNms = outNms;
  functor.inVcs = inVrs;
  functor.outVcs = outVrs;
  functor.nOptionalVectors = nOptionalVectors;
  functor.inVrsArr = inVrsArr;
  functor.outVrsArr = outVrsArr;
  functor.matrix = this->Matrix->Element;
  if (inNms)
  {
    svtkMatrix4x4::DeepCopy(*matrix, this->Matrix);
    svtkMatrix4x4::Invert(*matrix, *matrix);
    svtkMatrix4x4::Transpose(*matrix, *matrix);
    functor.matrixInvTr = matrix;
  }

  svtkSMPTools::For(0, n, functor);
}

//----------------------------------------------------------------------------
class TransformPointsFunctor
{
public:
  svtkPoints* inPts;
  svtkPoints* outPts;
  double (*matrix)[4];
  void operator()(svtkIdType begin, svtkIdType end) const
  {
    for (svtkIdType id = begin; id < end; id++)
    {
      double point[3];
      inPts->GetPoint(id, point);
      svtkSMPTransformPoint(matrix, point, point);
      outPts->SetPoint(id, point);
    }
  }
};

void svtkSMPTransform::TransformPoints(svtkPoints* inPts, svtkPoints* outPts)
{
  svtkIdType n = inPts->GetNumberOfPoints();
  this->Update();

  TransformPointsFunctor functor;
  functor.inPts = inPts;
  functor.outPts = outPts;
  functor.matrix = this->Matrix->Element;

  svtkSMPTools::For(0, n, functor);
}

//----------------------------------------------------------------------------
class TransformNormalsFunctor
{
public:
  svtkDataArray* inNms;
  svtkDataArray* outNms;
  double (*matrix)[4];
  void operator()(svtkIdType begin, svtkIdType end) const
  {
    for (svtkIdType id = begin; id < end; id++)
    {
      double norm[3];
      inNms->GetTuple(id, norm);
      svtkSMPTransformVector(matrix, norm, norm);
      svtkMath::Normalize(norm);
      outNms->SetTuple(id, norm);
    }
  }
};

void svtkSMPTransform::TransformNormals(svtkDataArray* inNms, svtkDataArray* outNms)
{
  svtkIdType n = inNms->GetNumberOfTuples();
  double matrix[4][4];

  this->Update();

  // to transform the normal, multiply by the transposed inverse matrix
  svtkMatrix4x4::DeepCopy(*matrix, this->Matrix);
  svtkMatrix4x4::Invert(*matrix, *matrix);
  svtkMatrix4x4::Transpose(*matrix, *matrix);

  TransformNormalsFunctor functor;
  functor.inNms = inNms;
  functor.outNms = outNms;
  functor.matrix = matrix;

  svtkSMPTools::For(0, n, functor);
}

//----------------------------------------------------------------------------
class TransformVectorsFunctor
{
public:
  svtkDataArray* inVcs;
  svtkDataArray* outVcs;
  double (*matrix)[4];
  void operator()(svtkIdType begin, svtkIdType end) const
  {
    for (svtkIdType id = begin; id < end; id++)
    {
      double vec[3];
      inVcs->GetTuple(id, vec);
      svtkSMPTransformVector(matrix, vec, vec);
      outVcs->SetTuple(id, vec);
    }
  }
};

void svtkSMPTransform::TransformVectors(svtkDataArray* inNms, svtkDataArray* outNms)
{
  svtkIdType n = inNms->GetNumberOfTuples();
  this->Update();

  TransformVectorsFunctor functor;
  functor.inVcs = inNms;
  functor.outVcs = outNms;
  functor.matrix = this->Matrix->Element;

  svtkSMPTools::For(0, n, functor);
}
