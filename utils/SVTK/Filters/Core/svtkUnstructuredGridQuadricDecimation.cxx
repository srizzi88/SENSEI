/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridQuadricDecimation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright 2007, 2008 by University of Utah.

=========================================================================*/
#include "svtkUnstructuredGridQuadricDecimation.h"

#include "svtkCellArray.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkUnstructuredGrid.h"
#include <map>

class svtkUnstructuredGridQuadricDecimationEdge;
class svtkUnstructuredGridQuadricDecimationFace;
class svtkUnstructuredGridQuadricDecimationFaceHash;
class svtkUnstructuredGridQuadricDecimationFaceMap;
class svtkUnstructuredGridQuadricDecimationVec4;
class svtkUnstructuredGridQuadricDecimationSymMat4;
class svtkUnstructuredGridQuadricDecimationQEF;
class svtkUnstructuredGridQuadricDecimationTetra;
class svtkUnstructuredGridQuadricDecimationVertex;
class svtkUnstructuredGridQuadricDecimationTetMesh;

// floating point epsilons
#define SVTK_FEPS 1e-6
#define SVTK_TEPS 1e-6
#define SVTK_SWAP(a, b, type)                                                                       \
  {                                                                                                \
    type t = a;                                                                                    \
    a = b;                                                                                         \
    b = t;                                                                                         \
  }
#define SVTK_PRECHECK(pointer)                                                                      \
  if (pointer)                                                                                     \
    delete[](pointer);                                                                             \
  (pointer)

// =============================================================================
// Vector 4 class
class svtkUnstructuredGridQuadricDecimationVec4
{
public:
  svtkUnstructuredGridQuadricDecimationVec4() { memset(values, 0, sizeof(values)); }

  svtkUnstructuredGridQuadricDecimationVec4(float v1, float v2, float v3, float v4)
  {
    values[0] = v1;
    values[1] = v2;
    values[2] = v3;
    values[3] = v4;
  }

  float& operator[](const int& index) { return values[index]; }
  float operator[](const int& index) const { return values[index]; }

  svtkUnstructuredGridQuadricDecimationVec4 operator+(
    const svtkUnstructuredGridQuadricDecimationVec4& v) const
  {
    return svtkUnstructuredGridQuadricDecimationVec4(values[0] + v.values[0],
      values[1] + v.values[1], values[2] + v.values[2], values[3] + v.values[3]);
  }

  svtkUnstructuredGridQuadricDecimationVec4 operator-(
    const svtkUnstructuredGridQuadricDecimationVec4& v) const
  {
    return svtkUnstructuredGridQuadricDecimationVec4(values[0] - v.values[0],
      values[1] - v.values[1], values[2] - v.values[2], values[3] - v.values[3]);
  }

  svtkUnstructuredGridQuadricDecimationVec4 operator*(const float& f) const
  {
    return svtkUnstructuredGridQuadricDecimationVec4(
      values[0] * f, values[1] * f, values[2] * f, values[3] * f);
  }
  float operator*(const svtkUnstructuredGridQuadricDecimationVec4& v) const
  {
    return values[0] * v.values[0] + values[1] * v.values[1] + values[2] * v.values[2] +
      values[3] * v.values[3];
  }

  svtkUnstructuredGridQuadricDecimationVec4 operator/(const float& f) const
  {
    return svtkUnstructuredGridQuadricDecimationVec4(
      values[0] / f, values[1] / f, values[2] / f, values[3] / f);
  }

  const svtkUnstructuredGridQuadricDecimationVec4& operator*=(const float& f)
  {
    values[0] *= f;
    values[1] *= f;
    values[2] *= f;
    values[3] *= f;
    return *this;
  }

  const svtkUnstructuredGridQuadricDecimationVec4& operator/=(const float& f)
  {
    values[0] /= f;
    values[1] /= f;
    values[2] /= f;
    values[3] /= f;
    return *this;
  }

  const svtkUnstructuredGridQuadricDecimationVec4& operator+=(
    const svtkUnstructuredGridQuadricDecimationVec4& v)
  {
    values[0] += v.values[0];
    values[1] += v.values[1];
    values[2] += v.values[2];
    values[3] += v.values[3];
    return *this;
  }

  const svtkUnstructuredGridQuadricDecimationVec4& operator-=(
    const svtkUnstructuredGridQuadricDecimationVec4& v)
  {
    values[0] -= v.values[0];
    values[1] -= v.values[1];
    values[2] -= v.values[2];
    values[3] -= v.values[3];
    return *this;
  }

  // dot product
  float Dot(const svtkUnstructuredGridQuadricDecimationVec4& v) const
  {
    return values[0] * v.values[0] + values[1] * v.values[1] + values[2] * v.values[2] +
      values[3] * v.values[3];
  }

  // A = e * eT
  svtkUnstructuredGridQuadricDecimationSymMat4 MultTransposeSym() const;

  float Length() const
  {
    return sqrt(values[0] * values[0] + values[1] * values[1] + values[2] * values[2] +
      values[3] * values[3]);
  }

  void Normalize()
  {
    float len = Length();
    if (len != 0)
    {
      values[0] /= len;
      values[1] /= len;
      values[2] /= len;
      values[3] /= len;
    }
  }

  float values[4];
};

// =============================================================================
// Symmetric 4x4 Matrix class
// Storing lower half
#define SM4op(i, OP) result.values[i] = values[i] OP m.values[i];

class svtkUnstructuredGridQuadricDecimationSymMat4
{
public:
  svtkUnstructuredGridQuadricDecimationSymMat4() { memset(values, 0, sizeof(values)); }
  void Identity()
  {
    memset(values, 0, sizeof(values));
    values[0] = values[2] = values[5] = values[9] = 1.;
  }

  svtkUnstructuredGridQuadricDecimationSymMat4 operator+(
    const svtkUnstructuredGridQuadricDecimationSymMat4& m) const
  {
    static svtkUnstructuredGridQuadricDecimationSymMat4 result;
    SM4op(0, +);
    SM4op(1, +);
    SM4op(2, +);
    SM4op(3, +);
    SM4op(4, +);
    SM4op(5, +);
    SM4op(6, +);
    SM4op(7, +);
    SM4op(8, +);
    SM4op(9, +);
    return result;
  }

  svtkUnstructuredGridQuadricDecimationSymMat4 operator-(
    const svtkUnstructuredGridQuadricDecimationSymMat4& m) const
  {
    static svtkUnstructuredGridQuadricDecimationSymMat4 result;
    SM4op(0, -);
    SM4op(1, -);
    SM4op(2, -);
    SM4op(3, -);
    SM4op(4, -);
    SM4op(5, -);
    SM4op(6, -);
    SM4op(7, -);
    SM4op(8, -);
    SM4op(9, -);
    return result;
  }

  svtkUnstructuredGridQuadricDecimationVec4 operator*(
    const svtkUnstructuredGridQuadricDecimationVec4& v) const
  {
    return svtkUnstructuredGridQuadricDecimationVec4(values[0] * v.values[0] +
        values[1] * v.values[1] + values[3] * v.values[2] + values[6] * v.values[3],
      values[1] * v.values[0] + values[2] * v.values[1] + values[4] * v.values[2] +
        values[7] * v.values[3],
      values[3] * v.values[0] + values[4] * v.values[1] + values[5] * v.values[2] +
        values[8] * v.values[3],
      values[6] * v.values[0] + values[7] * v.values[1] + values[8] * v.values[2] +
        values[9] * v.values[3]);
  }

  svtkUnstructuredGridQuadricDecimationSymMat4 operator*(const float& f) const
  {
    svtkUnstructuredGridQuadricDecimationSymMat4 result;
    for (int i = 0; i < 10; i++)
    {
      result.values[i] = values[i] * f;
    }
    return result;
  }

  svtkUnstructuredGridQuadricDecimationSymMat4 operator/(const float& f) const
  {
    svtkUnstructuredGridQuadricDecimationSymMat4 result;
    for (int i = 0; i < 10; i++)
    {
      result.values[i] = values[i] / f;
    }
    return result;
  }

  const svtkUnstructuredGridQuadricDecimationSymMat4& operator*=(const float& f)
  {
    for (int i = 0; i < 10; i++)
    {
      values[i] *= f;
    }
    return *this;
  }

  const svtkUnstructuredGridQuadricDecimationSymMat4& operator/=(const float& f)
  {
    for (int i = 0; i < 10; i++)
    {
      values[i] /= f;
    }
    return *this;
  }

  const svtkUnstructuredGridQuadricDecimationSymMat4& operator+=(
    const svtkUnstructuredGridQuadricDecimationSymMat4& m)
  {
    for (int i = 0; i < 10; i++)
    {
      values[i] += m.values[i];
    }
    return *this;
  }

  const svtkUnstructuredGridQuadricDecimationSymMat4& operator-=(
    const svtkUnstructuredGridQuadricDecimationSymMat4& m)
  {
    for (int i = 0; i < 10; i++)
    {
      values[i] -= m.values[i];
    }
    return *this;
  }

  float square(const svtkUnstructuredGridQuadricDecimationVec4& v) const
  {
    return v.values[0] *
      (values[0] * v.values[0] + values[1] * v.values[1] + values[3] * v.values[2] +
        values[6] * v.values[3]) +
      v.values[1] *
      (values[1] * v.values[0] + values[2] * v.values[1] + values[4] * v.values[2] +
        values[7] * v.values[3]) +
      v.values[2] *
      (values[3] * v.values[0] + values[4] * v.values[1] + values[5] * v.values[2] +
        values[8] * v.values[3]) +
      v.values[3] *
      (values[6] * v.values[0] + values[7] * v.values[1] + values[8] * v.values[2] +
        values[9] * v.values[3]);
  }

  bool ConjugateR(const svtkUnstructuredGridQuadricDecimationSymMat4& A1,
    const svtkUnstructuredGridQuadricDecimationSymMat4& A2,
    const svtkUnstructuredGridQuadricDecimationVec4& p1,
    svtkUnstructuredGridQuadricDecimationVec4& x) const;
  float values[10];
};
#undef SM4op

svtkUnstructuredGridQuadricDecimationSymMat4
svtkUnstructuredGridQuadricDecimationVec4::MultTransposeSym() const
{
  static svtkUnstructuredGridQuadricDecimationSymMat4 result;
  result.values[0] = values[0] * values[0];
  result.values[1] = values[0] * values[1];
  result.values[2] = values[1] * values[1];
  result.values[3] = values[0] * values[2];
  result.values[4] = values[1] * values[2];
  result.values[5] = values[2] * values[2];
  result.values[6] = values[0] * values[3];
  result.values[7] = values[1] * values[3];
  result.values[8] = values[2] * values[3];
  result.values[9] = values[3] * values[3];
  return result;
}

bool svtkUnstructuredGridQuadricDecimationSymMat4::ConjugateR(
  const svtkUnstructuredGridQuadricDecimationSymMat4& A1,
  const svtkUnstructuredGridQuadricDecimationSymMat4& A2,
  const svtkUnstructuredGridQuadricDecimationVec4& p1,
  svtkUnstructuredGridQuadricDecimationVec4& x) const
{
  float e(1e-3 / 4. * (values[0] + values[2] + values[5] + values[9]));
  svtkUnstructuredGridQuadricDecimationVec4 r((A1 - A2) * (p1 - x));
  svtkUnstructuredGridQuadricDecimationVec4 p;
  for (int k = 0; k < 4; k++)
  {
    float s(r.Dot(r));
    if (s <= 0)
    {
      break;
    }
    p += (r / s);
    svtkUnstructuredGridQuadricDecimationVec4 q((*this) * p);
    float t(p.Dot(q));
    if (s * t <= e)
    {
      break;
    }
    r -= (q / t);
    x += (p / t);
  }
  return true;
}

// =============================================================================
// QEF (QEF Error Function) Representation
class svtkUnstructuredGridQuadricDecimationQEF
{
public:
  svtkUnstructuredGridQuadricDecimationQEF()
    : p(0, 0, 0, 0)
    , e(0.0)
  {
  }
  svtkUnstructuredGridQuadricDecimationQEF(const svtkUnstructuredGridQuadricDecimationSymMat4& AA,
    const svtkUnstructuredGridQuadricDecimationVec4& pp, const double& ee)
    : A(AA)
    , p(pp)
    , e(ee)
  {
  }
  svtkUnstructuredGridQuadricDecimationQEF(const svtkUnstructuredGridQuadricDecimationQEF& Q1,
    const svtkUnstructuredGridQuadricDecimationQEF& Q2,
    const svtkUnstructuredGridQuadricDecimationVec4& x)
    : A(Q1.A + Q2.A)
    , p(x)
  {
    A.ConjugateR(Q1.A, Q2.A, Q1.p, p);
    e = Q1.e + Q2.e + Q1.A.square(p - Q1.p) + Q2.A.square(p - Q2.p);
  }

  void Zeros()
  {
    memset(A.values, 0, sizeof(A.values));
    memset(p.values, 0, sizeof(p.values));
    e = 0.0;
  }

  void Sum(const svtkUnstructuredGridQuadricDecimationQEF& Q1,
    const svtkUnstructuredGridQuadricDecimationQEF& Q2,
    const svtkUnstructuredGridQuadricDecimationVec4& x)
  {
    A = Q1.A + Q2.A;
    p = x;
    A.ConjugateR(Q1.A, Q2.A, Q1.p, p);
    e = Q1.e + Q2.e + Q1.A.square(p - Q1.p) + Q2.A.square(p - Q2.p);
  }

  void Sum(const svtkUnstructuredGridQuadricDecimationQEF& Q1,
    const svtkUnstructuredGridQuadricDecimationQEF& Q2)
  {
    A = Q1.A + Q2.A;
    p = (Q1.p + Q2.p) * 0.5;
    A.ConjugateR(Q1.A, Q2.A, Q1.p, p);
    e = Q1.e + Q2.e + Q1.A.square(p - Q1.p) + Q2.A.square(p - Q2.p);
  }

  void Scale(const double& f)
  {
    A *= f;
    p *= f;
    e *= f;
  }

  svtkUnstructuredGridQuadricDecimationSymMat4 A;
  svtkUnstructuredGridQuadricDecimationVec4 p;
  float e;
};

class svtkUnstructuredGridQuadricDecimationVertex
{
public:
  svtkUnstructuredGridQuadricDecimationVertex()
    : Corner(-1)
  {
  }
  svtkUnstructuredGridQuadricDecimationVertex(float ix, float iy, float iz, float is = 0.0)
    : Corner(-1)
  {
    Q.p = svtkUnstructuredGridQuadricDecimationVec4(ix, iy, iz, is);
  }

  svtkUnstructuredGridQuadricDecimationQEF Q;

  int Corner;
};

class svtkUnstructuredGridQuadricDecimationEdge
{
public:
  // NOTE: vertex list are always sorted
  svtkUnstructuredGridQuadricDecimationEdge() { Verts[0] = Verts[1] = nullptr; }
  svtkUnstructuredGridQuadricDecimationEdge(
    svtkUnstructuredGridQuadricDecimationVertex* va, svtkUnstructuredGridQuadricDecimationVertex* vb)
  {
    Verts[0] = va;
    Verts[1] = vb;
    SortVerts();
  }

  bool operator==(const svtkUnstructuredGridQuadricDecimationEdge& e) const
  {
    return (Verts[0] == e.Verts[0] && Verts[1] == e.Verts[1]);
  }

  // '<' operand is useful with hash_map
  bool operator<(const svtkUnstructuredGridQuadricDecimationEdge& e) const
  {
    if (Verts[0] < e.Verts[0])
    {
      return true;
    }
    else
    {
      if (Verts[0] == e.Verts[0] && Verts[1] < e.Verts[1])
      {
        return true;
      }
    }
    return false;
  }

  // sort the vertices to be increasing
  void SortVerts()
  {
    if (Verts[0] > Verts[1])
    {
      svtkUnstructuredGridQuadricDecimationVertex* v = Verts[1];
      Verts[1] = Verts[0];
      Verts[0] = v;
    }
  }

  void ChangeVerts(
    svtkUnstructuredGridQuadricDecimationVertex* v1, svtkUnstructuredGridQuadricDecimationVertex* v2)
  {
    Verts[0] = v1;
    Verts[1] = v2;
    SortVerts();
  }

  // 2 ends of the edge
  svtkUnstructuredGridQuadricDecimationVertex* Verts[2];
};

class svtkUnstructuredGridQuadricDecimationFace
{
public:
  svtkUnstructuredGridQuadricDecimationFace(svtkUnstructuredGridQuadricDecimationVertex* va,
    svtkUnstructuredGridQuadricDecimationVertex* vb, svtkUnstructuredGridQuadricDecimationVertex* vc)
  {
    Verts[0] = va;
    Verts[1] = vb;
    Verts[2] = vc;
    SortVerts();
  }

  bool operator==(const svtkUnstructuredGridQuadricDecimationFace& f) const
  {
    return (Verts[0] == f.Verts[0] && Verts[1] == f.Verts[1] && Verts[2] == f.Verts[2]);
  }

  // compare by sorting the verts and one-one comparison
  bool operator<(const svtkUnstructuredGridQuadricDecimationFace& f) const
  {
    for (int i = 0; i < 3; i++)
    {
      if (Verts[i] < f.Verts[i])
      {
        return true;
      }
      else
      {
        if (Verts[i] > f.Verts[i])
        {
          return false;
        }
      }
    }
    return false;
  }

  // has the magnitude of 2 * area of this face
  // we don't care about if it pos or neg now
  double Orientation() const;

  // compute the normal of the face
  svtkUnstructuredGridQuadricDecimationVec4 Normal() const;

  // check to see if a vertex belongs to this face
  bool ContainVertex(svtkUnstructuredGridQuadricDecimationVertex* v) const;

  // change vertex v1 on the list to vertex v2 (for edge collapsing)
  void ChangeVertex(svtkUnstructuredGridQuadricDecimationVertex* fromV,
    svtkUnstructuredGridQuadricDecimationVertex* toV);

  // sort the vertices
  void SortVerts()
  {
    if (Verts[1] < Verts[0] && Verts[1] < Verts[2])
    {
      SVTK_SWAP(Verts[0], Verts[1], svtkUnstructuredGridQuadricDecimationVertex*);
    }

    if (Verts[2] < Verts[0] && Verts[2] < Verts[1])
    {
      SVTK_SWAP(Verts[0], Verts[2], svtkUnstructuredGridQuadricDecimationVertex*);
    }

    if (Verts[2] < Verts[1])
    {
      SVTK_SWAP(Verts[1], Verts[2], svtkUnstructuredGridQuadricDecimationVertex*);
    }
  }

  svtkUnstructuredGridQuadricDecimationVertex* Verts[3];

  // find the orthonormal e1, e2, the tangent plane
  void FindOrthonormal(svtkUnstructuredGridQuadricDecimationVec4& e1,
    svtkUnstructuredGridQuadricDecimationVec4& e2) const;

  // Compute the Quadric Error for this face
  void UpdateQuadric(float boundaryWeight = 1.);
};

//=============================================================
// Face_hash class declaration
// bundle of support functions for Face hash_map
class svtkUnstructuredGridQuadricDecimationFaceHash
{
public:
  // This is the average load that the hash table tries to maintain.
  static const size_t bucket_size = 4;

  // This is the minimum number of buckets in the hash table.  It must be
  // a positive power of two.
  static const size_t min_buckets = 8;

  // This method must define an ordering on two faces.  It is used to compare
  // faces that has to the same bucket.
  bool operator()(const svtkUnstructuredGridQuadricDecimationFace& f1,
    const svtkUnstructuredGridQuadricDecimationFace& f2) const
  {
    return f1 < f2;
  }

  // This produces the actual hash code for the Face
  size_t operator()(const svtkUnstructuredGridQuadricDecimationFace& f) const
  {
    return (size_t)f.Verts[0] * (size_t)f.Verts[1] * (size_t)f.Verts[2];
  }
};

//=============================================================
// FaceMap declaration
// try to make a hash_map from Face -> Face *
typedef std::map<svtkUnstructuredGridQuadricDecimationFace,
  svtkUnstructuredGridQuadricDecimationFace*, svtkUnstructuredGridQuadricDecimationFaceHash>
  svtkUnstructuredGridQuadricDecimationFaceHashMap;

class svtkUnstructuredGridQuadricDecimationFaceMap
{
public:
  svtkUnstructuredGridQuadricDecimationFaceMap() = default;
  ~svtkUnstructuredGridQuadricDecimationFaceMap() { clear(); }

  // for iteration
  svtkUnstructuredGridQuadricDecimationFaceHashMap::iterator begin() { return faces.begin(); }
  svtkUnstructuredGridQuadricDecimationFaceHashMap::iterator end() { return faces.end(); }

  // clear
  void clear();
  size_t size() const { return faces.size(); }

  // insert a new face
  svtkUnstructuredGridQuadricDecimationFace* AddFace(
    const svtkUnstructuredGridQuadricDecimationFace& f);

  // return the face that is the same as f
  // if there's no such face, then return nullptr
  svtkUnstructuredGridQuadricDecimationFace* GetFace(
    const svtkUnstructuredGridQuadricDecimationFace& f);

  // remove the face has the content f
  void RemoveFace(const svtkUnstructuredGridQuadricDecimationFace& f);

  // add a face, and check if it can't be a border face
  // then kill it. Return nullptr -> failed adding
  svtkUnstructuredGridQuadricDecimationFace* AddFaceBorder(
    const svtkUnstructuredGridQuadricDecimationFace& f);

  svtkUnstructuredGridQuadricDecimationFaceHashMap faces;

private:
  // add face without checking existence
  svtkUnstructuredGridQuadricDecimationFace* DirectAddFace(
    const svtkUnstructuredGridQuadricDecimationFace& f);

  // remove face without checking existence
  void DirectRemoveFace(svtkUnstructuredGridQuadricDecimationFace* f);
  void DirectRemoveFace(svtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i);
};

//=============================================================
// Face class implementation
// NOTE: the vertices of the Face are always sorted!
// construct the face, sort the vertice as well
// calculate the orientation, or 2*area of the face
double svtkUnstructuredGridQuadricDecimationFace::Orientation() const
{
  svtkUnstructuredGridQuadricDecimationVec4 v;
  // this is cross product of v1-v0 and v2-v0
  v[0] = (Verts[1]->Q.p[1] - Verts[0]->Q.p[1]) * (Verts[2]->Q.p[2] - Verts[0]->Q.p[2]) -
    (Verts[2]->Q.p[1] - Verts[0]->Q.p[1]) * (Verts[1]->Q.p[2] - Verts[0]->Q.p[2]);

  v[1] = -(Verts[1]->Q.p[0] - Verts[0]->Q.p[0]) * (Verts[2]->Q.p[2] - Verts[0]->Q.p[2]) +
    (Verts[2]->Q.p[0] - Verts[0]->Q.p[0]) * (Verts[1]->Q.p[2] - Verts[0]->Q.p[2]);

  v[2] = (Verts[1]->Q.p[0] - Verts[0]->Q.p[0]) * (Verts[2]->Q.p[1] - Verts[0]->Q.p[1]) -
    (Verts[2]->Q.p[0] - Verts[0]->Q.p[0]) * (Verts[1]->Q.p[1] - Verts[0]->Q.p[1]);

  v[3] = 0;

  return v.Length();
}

svtkUnstructuredGridQuadricDecimationVec4 svtkUnstructuredGridQuadricDecimationFace::Normal() const
{
  svtkUnstructuredGridQuadricDecimationVec4 v;
  // this is cross product of v1-v0 and v2-v0
  v[0] = (Verts[1]->Q.p[1] - Verts[0]->Q.p[1]) * (Verts[2]->Q.p[2] - Verts[0]->Q.p[2]) -
    (Verts[2]->Q.p[1] - Verts[0]->Q.p[1]) * (Verts[1]->Q.p[2] - Verts[0]->Q.p[2]);

  v[1] = -(Verts[1]->Q.p[0] - Verts[0]->Q.p[0]) * (Verts[2]->Q.p[2] - Verts[0]->Q.p[2]) +
    (Verts[2]->Q.p[0] - Verts[0]->Q.p[0]) * (Verts[1]->Q.p[2] - Verts[0]->Q.p[2]);

  v[2] = (Verts[1]->Q.p[0] - Verts[0]->Q.p[0]) * (Verts[2]->Q.p[1] - Verts[0]->Q.p[1]) -
    (Verts[2]->Q.p[0] - Verts[0]->Q.p[0]) * (Verts[1]->Q.p[1] - Verts[0]->Q.p[1]);

  v[3] = 0;
  return v / v.Length();
}

// Compute the Quadric Error for this face
void svtkUnstructuredGridQuadricDecimationFace::UpdateQuadric(float boundaryWeight)
{
  svtkUnstructuredGridQuadricDecimationVec4 e1, e2;

  e1 = Verts[1]->Q.p - Verts[0]->Q.p;
  e2 = Verts[2]->Q.p - Verts[0]->Q.p;

  e1.Normalize();

  e2 = e2 - e1 * e2.Dot(e1);
  e2.Normalize();

  // A = I - e1.e1T - e2.e2T
  static svtkUnstructuredGridQuadricDecimationSymMat4 A;
  A.Identity();
  A -= (e1.MultTransposeSym() + e2.MultTransposeSym());
  A *= fabs(Orientation()) / 6.0 * boundaryWeight;
  //  A *= boundaryWeight;
  for (int i = 0; i < 3; i++)
  {
    Verts[i]->Q.A += A;
  }
}

bool svtkUnstructuredGridQuadricDecimationFace::ContainVertex(
  svtkUnstructuredGridQuadricDecimationVertex* v) const
{
  for (int i = 0; i < 3; i++)
  {
    if (Verts[i] == v)
    {
      return true;
    }
  }
  return false;
}

// change vertex v1 on the list to vertex v2 (for edge collapsing)
void svtkUnstructuredGridQuadricDecimationFace::ChangeVertex(
  svtkUnstructuredGridQuadricDecimationVertex* v1, svtkUnstructuredGridQuadricDecimationVertex* v2)
{
  for (int i = 0; i < 3; i++)
  {
    if (Verts[i] == v1)
    {
      Verts[i] = v2;
    }
  }
  SortVerts();
}

//=============================================================
// FaceMap class implementation
void svtkUnstructuredGridQuadricDecimationFaceMap::clear()
{
  svtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i = faces.begin();
  // free all the memory
  while (i != faces.end())
  {
    delete (*i).second;
    ++i;
  }
  // clear the hash table
  faces.clear();
}

// insert in new faces
svtkUnstructuredGridQuadricDecimationFace* svtkUnstructuredGridQuadricDecimationFaceMap::AddFace(
  const svtkUnstructuredGridQuadricDecimationFace& f)
{
  if (GetFace(f) == nullptr)
  {
    return DirectAddFace(f);
  }
  else
  {
    return nullptr;
  }
}

// return the face that is the same as f
// if there's no such face, then return nullptr
svtkUnstructuredGridQuadricDecimationFace* svtkUnstructuredGridQuadricDecimationFaceMap::GetFace(
  const svtkUnstructuredGridQuadricDecimationFace& f)
{
  svtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i = faces.find(f);
  if (i != faces.end())
  {
    return (*i).second;
  }
  else
  {
    return nullptr;
  }
}

// remove the face has the content f
void svtkUnstructuredGridQuadricDecimationFaceMap::RemoveFace(
  const svtkUnstructuredGridQuadricDecimationFace& f)
{
  svtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i = faces.find(f);
  if (i != faces.end())
  {
    DirectRemoveFace(i);
  }
}

// add a face, and check if it can't be a border face
// then kill it. Return nullptr -> failed adding
svtkUnstructuredGridQuadricDecimationFace*
svtkUnstructuredGridQuadricDecimationFaceMap::AddFaceBorder(
  const svtkUnstructuredGridQuadricDecimationFace& f)
{
  svtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i = faces.find(f);
  if (i != faces.end())
  {
    // exist -> has 2 tets -> not a border -> kill it
    DirectRemoveFace(i);
    return nullptr;
  }
  else
  {
    // not exist -> add it in
    return DirectAddFace(f);
  }
}

// add face without checking existence
svtkUnstructuredGridQuadricDecimationFace*
svtkUnstructuredGridQuadricDecimationFaceMap::DirectAddFace(
  const svtkUnstructuredGridQuadricDecimationFace& f)
{
  svtkUnstructuredGridQuadricDecimationFace* newF = new svtkUnstructuredGridQuadricDecimationFace(f);
  faces[f] = newF;
  return newF;
}

// remove face without checking existence
void svtkUnstructuredGridQuadricDecimationFaceMap::DirectRemoveFace(
  svtkUnstructuredGridQuadricDecimationFace* f)
{
  faces.erase(*f);
  delete f;
}

void svtkUnstructuredGridQuadricDecimationFaceMap::DirectRemoveFace(
  svtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i)
{
  svtkUnstructuredGridQuadricDecimationFace* f = (*i).second;
  faces.erase(i);
  delete f;
}

class svtkUnstructuredGridQuadricDecimationTetra
{
public:
  svtkUnstructuredGridQuadricDecimationTetra()
    : index(-1)
  {
  }
  svtkUnstructuredGridQuadricDecimationTetra(svtkUnstructuredGridQuadricDecimationVertex* va,
    svtkUnstructuredGridQuadricDecimationVertex* vb, svtkUnstructuredGridQuadricDecimationVertex* vc,
    svtkUnstructuredGridQuadricDecimationVertex* vd)
  {
    Verts[0] = va;
    Verts[1] = vb;
    Verts[2] = vc;
    Verts[3] = vd;
  }

  // pointers to 4 vertices
  svtkUnstructuredGridQuadricDecimationVertex* Verts[4];

  //
  // the orientation of this order of vertices
  // positive - good orientation
  // zero - all in one plane
  // negative - bad orientation
  //
  // it is also 6 times the volume of this tetrahedra
  float Orientation() const;
  float Orientation(const svtkUnstructuredGridQuadricDecimationVec4& v0,
    const svtkUnstructuredGridQuadricDecimationVec4& v1,
    const svtkUnstructuredGridQuadricDecimationVec4& v2,
    const svtkUnstructuredGridQuadricDecimationVec4& v3) const;

  // swap vertices so that the orientation is positive
  void FixOrientation();

  // check to see if a vertex belongs to this tetrahedron
  bool ContainVertex(svtkUnstructuredGridQuadricDecimationVertex* v) const
  {
    if (Verts[0] == v || Verts[1] == v || Verts[2] == v || Verts[3] == v)
    {
      return true;
    }
    return false;
  }

  // check to see if we can change fromV to toV without changing the orietation
  bool Changeable(svtkUnstructuredGridQuadricDecimationVertex* fromV,
    const svtkUnstructuredGridQuadricDecimationVec4& v4)
  {
    if (fromV == Verts[0])
    {
      return Orientation(v4, Verts[1]->Q.p, Verts[2]->Q.p, Verts[3]->Q.p) > SVTK_TEPS;
    }
    if (fromV == Verts[1])
    {
      return Orientation(Verts[0]->Q.p, v4, Verts[2]->Q.p, Verts[3]->Q.p) > SVTK_TEPS;
    }
    if (fromV == Verts[2])
    {
      return Orientation(Verts[0]->Q.p, Verts[1]->Q.p, v4, Verts[3]->Q.p) > SVTK_TEPS;
    }
    if (fromV == Verts[3])
    {
      return Orientation(Verts[0]->Q.p, Verts[1]->Q.p, Verts[2]->Q.p, v4) > SVTK_TEPS;
    }
    return true;
  }

  // change vertex v1 on the list to vertex v2 (for edge collapsing)
  void ChangeVertex(svtkUnstructuredGridQuadricDecimationVertex* fromV,
    svtkUnstructuredGridQuadricDecimationVertex* toV);

  // find the orthonormal e1, e2, e3, the tangent space
  void FindOrthonormal(svtkUnstructuredGridQuadricDecimationVec4& e1,
    svtkUnstructuredGridQuadricDecimationVec4& e2,
    svtkUnstructuredGridQuadricDecimationVec4& e3) const;

  // Compute the Quadric Error for this tetrahedron
  void UpdateQuadric();

  int index;
};

#define U(c) (Verts[1]->Q.p[c] - Verts[0]->Q.p[c])
#define V(c) (Verts[2]->Q.p[c] - Verts[0]->Q.p[c])
#define W(c) (Verts[3]->Q.p[c] - Verts[0]->Q.p[c])
float svtkUnstructuredGridQuadricDecimationTetra::Orientation() const
{
  return U(0) * (V(1) * W(2) - V(2) * W(1)) - V(0) * (U(1) * W(2) - U(2) * W(1)) +
    W(0) * (U(1) * V(2) - U(2) * V(1));
}
#undef U
#undef V
#undef W

#define U(c) (v1[c] - v0[c])
#define V(c) (v2[c] - v0[c])
#define W(c) (v3[c] - v0[c])
float svtkUnstructuredGridQuadricDecimationTetra::Orientation(
  const svtkUnstructuredGridQuadricDecimationVec4& v0,
  const svtkUnstructuredGridQuadricDecimationVec4& v1,
  const svtkUnstructuredGridQuadricDecimationVec4& v2,
  const svtkUnstructuredGridQuadricDecimationVec4& v3) const
{
  return U(0) * (V(1) * W(2) - V(2) * W(1)) - V(0) * (U(1) * W(2) - U(2) * W(1)) +
    W(0) * (U(1) * V(2) - U(2) * V(1));
}
#undef U
#undef V
#undef W

void svtkUnstructuredGridQuadricDecimationTetra::FixOrientation()
{
  if (Orientation() < 0)
  {
    SVTK_SWAP(Verts[2], Verts[3], svtkUnstructuredGridQuadricDecimationVertex*);
  }
  if (Orientation() < 0)
  {
    SVTK_SWAP(Verts[1], Verts[2], svtkUnstructuredGridQuadricDecimationVertex*);
  }
}

// change vertex v1 on the list to vertex v2 (for edge collapsing)
void svtkUnstructuredGridQuadricDecimationTetra::ChangeVertex(
  svtkUnstructuredGridQuadricDecimationVertex* v1, svtkUnstructuredGridQuadricDecimationVertex* v2)
{
  for (int i = 0; i < 4; i++)
  {
    if (Verts[i] == v1)
    {
      Verts[i] = v2;
    }
  }
}

// find the orthonormal tangent space e1, e2, e3
void svtkUnstructuredGridQuadricDecimationTetra::FindOrthonormal(
  svtkUnstructuredGridQuadricDecimationVec4& e1, svtkUnstructuredGridQuadricDecimationVec4& e2,
  svtkUnstructuredGridQuadricDecimationVec4& e3) const
{
  svtkUnstructuredGridQuadricDecimationVec4 e0(Verts[0]->Q.p);

  // Ei = Ui - U0
  e1 = Verts[1]->Q.p - e0;
  e2 = Verts[2]->Q.p - e0;
  e3 = Verts[3]->Q.p - e0;

  e1.Normalize();

  e2 = e2 - e1 * e2.Dot(e1);
  e2.Normalize();

  e3 = e3 - e1 * e3.Dot(e1) - e2 * e3.Dot(e2);
  e3.Normalize();
}

// compute the quadric error based on this tet
#define ax a.values[0]
#define ay a.values[1]
#define az a.values[2]
#define af a.values[3]
#define bx b.values[0]
#define by b.values[1]
#define bz b.values[2]
#define bf b.values[3]
#define cx c.values[0]
#define cy c.values[1]
#define cz c.values[2]
#define cf c.values[3]
void svtkUnstructuredGridQuadricDecimationTetra::UpdateQuadric()
{
  svtkUnstructuredGridQuadricDecimationVec4 a(Verts[1]->Q.p - Verts[0]->Q.p);
  svtkUnstructuredGridQuadricDecimationVec4 b(Verts[2]->Q.p - Verts[0]->Q.p);
  svtkUnstructuredGridQuadricDecimationVec4 c(Verts[3]->Q.p - Verts[0]->Q.p);
  svtkUnstructuredGridQuadricDecimationVec4 n(
    ay * (bz * cf - bf * cz) + az * (bf * cy - by * cf) + af * (by * cz - bz * cy),
    az * (bx * cf - bf * cx) + af * (bz * cx - bx * cz) + ax * (bf * cz - bz * cf),
    af * (bx * cy - by * cx) + ax * (by * cf - bf * cy) + ay * (bf * cx - bx * cf),
    ax * (bz * cy - by * cz) + ay * (bx * cz - bz * cx) + az * (by * cx - bx * cy));
  svtkUnstructuredGridQuadricDecimationSymMat4 A(n.MultTransposeSym());
  // weight by the volume of the tet
  //  Q.Scale(Orientation()/6.0);
  // we want to divide by 4 also, for each vertex
  A *= 1.5 / fabs(Orientation());

  for (int i = 0; i < 4; i++)
  {
    Verts[i]->Q.A += A;
  }
}
#undef ax
#undef ay
#undef az
#undef af
#undef bx
#undef by
#undef bz
#undef bf
#undef cx
#undef cy
#undef cz
#undef cf

class svtkUnstructuredGridQuadricDecimationTetMesh
{
public:
  svtkUnstructuredGridQuadricDecimationTetMesh()
    : setSize(8)
    , doublingRatio(0.4)
    , noDoubling(false)
    , boundaryWeight(100.0)
    , Verts(nullptr)
    , tets(nullptr)
    , PT(nullptr)
    , unusedTets(0)
    , unusedVerts(0)
    , L(nullptr)
  {
  }

  ~svtkUnstructuredGridQuadricDecimationTetMesh() { clear(); }

  void AddTet(svtkUnstructuredGridQuadricDecimationTetra* t);

  void clear(); // clear the mesh -> empty

  int LoadUnstructuredGrid(svtkUnstructuredGrid* vgrid, const char* scalarsName);
  int SaveUnstructuredGrid(svtkUnstructuredGrid* vgrid);

  // Simplification
  int setSize;
  float doublingRatio;
  bool noDoubling;
  float boundaryWeight;
  void BuildFullMesh();
  int Simplify(int n, int desiredTets);

  int vCount;
  int tCount;
  svtkUnstructuredGridQuadricDecimationVertex* Verts;
  svtkUnstructuredGridQuadricDecimationTetra* tets;
  svtkUnstructuredGridQuadricDecimationTetra** PT;
  svtkUnstructuredGridQuadricDecimationFaceMap faces;

  // number of tets deleted but not free
  int unusedTets;
  int unusedVerts;
  int maxTet;

  int* L;

private:
  void AddCorner(svtkUnstructuredGridQuadricDecimationVertex* v, int corner);

  // check if this edge can be collapsed (i.e. without violating boundary,
  // vol...)
  bool Contractable(svtkUnstructuredGridQuadricDecimationEdge& e,
    const svtkUnstructuredGridQuadricDecimationVec4& target);

  // Simplification
  void MergeTets(svtkUnstructuredGridQuadricDecimationVertex* dst,
    svtkUnstructuredGridQuadricDecimationVertex* src);
  void DeleteMin(
    svtkUnstructuredGridQuadricDecimationEdge& e, svtkUnstructuredGridQuadricDecimationQEF& Q);
};

#define SVTK_ADDFACEBORDER(i, j, k)                                                                 \
  faces.AddFaceBorder(                                                                             \
    svtkUnstructuredGridQuadricDecimationFace(t->Verts[i], t->Verts[j], t->Verts[k]))
void svtkUnstructuredGridQuadricDecimationTetMesh::AddTet(
  svtkUnstructuredGridQuadricDecimationTetra* t)
{
  if (t->Orientation() < -SVTK_FEPS)
  {
    t->FixOrientation();
  }

  // add all of its faces to the FaceMap => 4 faces
  // NOTE: adding faces to vertices' list will be done
  // after we have all the faces (because some faces
  // might be deleted if it is not on the surface!!!)
  SVTK_ADDFACEBORDER(0, 1, 2);
  SVTK_ADDFACEBORDER(0, 1, 3);
  SVTK_ADDFACEBORDER(0, 2, 3);
  SVTK_ADDFACEBORDER(1, 2, 3);
}
#undef SVTK_ADDFACEBORDER

void svtkUnstructuredGridQuadricDecimationTetMesh::AddCorner(
  svtkUnstructuredGridQuadricDecimationVertex* v, int corner)
{
  if (v->Corner < 0)
  {
    v->Corner = corner;
    L[corner] = corner;
  }
  else
  {
    L[corner] = L[v->Corner];
    L[v->Corner] = corner;
  }
}

// Clean the mesh
void svtkUnstructuredGridQuadricDecimationTetMesh::clear()
{
  SVTK_PRECHECK(Verts) = nullptr;
  SVTK_PRECHECK(tets) = nullptr;
  SVTK_PRECHECK(PT) = nullptr;
  SVTK_PRECHECK(L) = nullptr;
  faces.clear();
  unusedTets = 0;
  unusedVerts = 0;
}

// SIMPLIFICATION IMPLEMENTATION

// BuildFullMesh
//  - Adding faces to vertices list and initialize their quadrics
//  - Compute quadric error at each vertex or remove unused vertices
void svtkUnstructuredGridQuadricDecimationTetMesh::BuildFullMesh()
{
  svtkUnstructuredGridQuadricDecimationFaceHashMap::iterator fi = faces.begin();
  while (fi != faces.end())
  {
    svtkUnstructuredGridQuadricDecimationFace* f = (*fi).second;
    f->UpdateQuadric(boundaryWeight);
    ++fi;
  }
}

void svtkUnstructuredGridQuadricDecimationTetMesh::DeleteMin(
  svtkUnstructuredGridQuadricDecimationEdge& finalE, svtkUnstructuredGridQuadricDecimationQEF& minQ)
{
  // Multiple Choice Randomize set
  static float lasterror = 0;
  bool stored(false);
  svtkUnstructuredGridQuadricDecimationQEF Q;
  svtkUnstructuredGridQuadricDecimationEdge e(nullptr, nullptr);
  for (int j = 0; j < 2; j++)
  {
    for (int i = 0; i < setSize; i++)
    {
      int k = rand() % maxTet;
      if (tets[k].index < 0)
      {
        do
        {
          maxTet--;
        } while (maxTet > 0 && tets[maxTet].index < 0);
        if (k < maxTet)
        {
          int idx = tets[k].index;
          tets[k] = tets[maxTet];
          tets[maxTet].index = idx;
          PT[tets[k].index] = &tets[k];
        }
        else
        {
          k = maxTet++;
        }
      }

      e.Verts[0] = tets[k].Verts[rand() % 4];
      do
      {
        e.Verts[1] = tets[k].Verts[rand() % 4];
      } while (e.Verts[1] == e.Verts[0]);

      if (!stored)
      {
        finalE = e;
        minQ.Sum(e.Verts[0]->Q, e.Verts[1]->Q);
        stored = true;
      }
      else
      {
        if (e.Verts[0]->Q.e + e.Verts[1]->Q.e < minQ.e)
        {
          Q.Sum(e.Verts[0]->Q, e.Verts[1]->Q);
          if (Q.e < minQ.e)
          {
            finalE = e;
            minQ = Q;
          }
        }
      }
    }
    if ((lasterror != 0.0 && (noDoubling || (minQ.e - lasterror) / lasterror <= doublingRatio)))
    {
      break;
    }
  }
  lasterror = minQ.e;
}

// Simplify the mesh by a series of N edge contractions
// or to the number of desiredTets
// it returns the actual number of edge contractions
int svtkUnstructuredGridQuadricDecimationTetMesh::Simplify(int n, int desiredTets)
{
  int count = 0;
  int uncontractable = 0;
  int run = 0;
  while ((count < n || desiredTets < (tCount - unusedTets)) && (run < 1000))
  {
    // as long as we want to collapse
    svtkUnstructuredGridQuadricDecimationQEF Q;
    svtkUnstructuredGridQuadricDecimationEdge e;

    DeleteMin(e, Q);

    if (Contractable(e, Q.p))
    {
      run = 0;
      // begin to collapse the edge Va + Vb -> Va = e.target
      svtkUnstructuredGridQuadricDecimationVertex* va = e.Verts[0];
      svtkUnstructuredGridQuadricDecimationVertex* vb = e.Verts[1];

      // Constructing new vertex
      va->Q = Q;

      // Merge all faces and tets of Va and Vb, remove the degenerated ones
      MergeTets(va, vb);
      vb->Corner = -1;
      unusedVerts++;

      // Complete the edge contraction
      count++;
    }
    else
    {
      uncontractable++;
      run++;
    }
  }
  return count;
}

// Merge all tets of Vb to Va by changing Vb to Va
// and add all tets of Vb to Va's Tet List
// Also, it will remove all tets contain both Vb and Va
// In fact, this is merging corners
void svtkUnstructuredGridQuadricDecimationTetMesh::MergeTets(
  svtkUnstructuredGridQuadricDecimationVertex* dst, svtkUnstructuredGridQuadricDecimationVertex* src)
{
  int next = src->Corner;
  svtkUnstructuredGridQuadricDecimationTetra* t = nullptr;
  do
  {
    t = PT[next / 4];
    if (t)
    {
      if (t->ContainVertex(dst))
      {
        t->index = -t->index - 1;
        unusedTets++;
        PT[next / 4] = nullptr;
      }
      else
      {
        t->ChangeVertex(src, dst);
      }
    }
    next = L[next];
  } while (next != src->Corner);

  // Then we merge them all together
  next = L[dst->Corner];
  L[dst->Corner] = L[src->Corner];
  L[src->Corner] = next;
  bool notstop = true;
  int prev = dst->Corner;
  next = L[prev];
  do
  {
    notstop = next != dst->Corner;
    t = PT[next / 4];
    if (!t)
    {
      next = L[next];
      L[prev] = next;
    }
    else
    {
      prev = next;
      next = L[next];
    }
  } while (notstop);
  dst->Corner = prev;
}

// Check if an edge can be contracted
bool svtkUnstructuredGridQuadricDecimationTetMesh::Contractable(
  svtkUnstructuredGridQuadricDecimationEdge& e,
  const svtkUnstructuredGridQuadricDecimationVec4& target)
{
  // need to check all the tets around both vertices to see if they can
  // adapt the new target vertex or not
  svtkUnstructuredGridQuadricDecimationTetra* t = nullptr;
  for (int i = 0; i < 2; i++)
  {
    int c = e.Verts[i]->Corner;
    do
    {
      t = PT[c / 4];
      if (t)
      {
        if (!(t->ContainVertex(e.Verts[0]) && t->ContainVertex(e.Verts[1])) &&
          !(t->Changeable(e.Verts[i], target)))
        {
          return false;
        }
      }
      c = L[c];
    } while (c != e.Verts[i]->Corner);
  }
  return true;
}

int svtkUnstructuredGridQuadricDecimationTetMesh::LoadUnstructuredGrid(
  svtkUnstructuredGrid* vgrid, const char* scalarsName)
{
  clear();
  // Read all the vertices first
  vCount = vgrid->GetNumberOfPoints();
  SVTK_PRECHECK(Verts) = new svtkUnstructuredGridQuadricDecimationVertex[vCount];
  svtkPoints* vp = vgrid->GetPoints();
  svtkDataArray* vs = nullptr;
  if (scalarsName)
  {
    vs = vgrid->GetPointData()->GetArray(scalarsName);
  }
  else
  {
    vs = vgrid->GetPointData()->GetScalars();
    if (!vs)
    {
      vs = vgrid->GetPointData()->GetArray("scalars");
    }
  }
  if (!vs)
  {
    return svtkUnstructuredGridQuadricDecimation::NO_SCALARS;
  }
  for (int i = 0; i < vCount; i++)
  {
    double* pos = vp->GetPoint(i);
    double* scalar = vs->GetTuple(i);
    Verts[i].Q.p[0] = pos[0];
    Verts[i].Q.p[1] = pos[1];
    Verts[i].Q.p[2] = pos[2];
    Verts[i].Q.p[3] = scalar[0];
  }

  // Read all the tets
  tCount = vgrid->GetNumberOfCells();
  if (!tCount)
  {
    return svtkUnstructuredGridQuadricDecimation::NO_CELLS;
  }
  maxTet = tCount;
  SVTK_PRECHECK(tets) = new svtkUnstructuredGridQuadricDecimationTetra[tCount];
  SVTK_PRECHECK(PT) = new svtkUnstructuredGridQuadricDecimationTetra*[tCount];
  SVTK_PRECHECK(L) = new int[4 * tCount];
  svtkCellArray* vt = vgrid->GetCells();
  svtkIdType npts;
  const svtkIdType* idx;
  for (int i = 0; i < tCount; i++)
  {
    vt->GetCellAtId(i, npts, idx);
    if (npts == 4)
    {
      for (int k = 0; k < 4; k++)
      {
        tets[i].Verts[k] = &Verts[idx[k]];
      }
      AddTet(&tets[i]);
      AddCorner(tets[i].Verts[0], i * 4 + 0);
      AddCorner(tets[i].Verts[1], i * 4 + 1);
      AddCorner(tets[i].Verts[2], i * 4 + 2);
      AddCorner(tets[i].Verts[3], i * 4 + 3);
      tets[i].UpdateQuadric();
      PT[i] = &tets[i];
      tets[i].index = i;
    }
    else
    {
      return svtkUnstructuredGridQuadricDecimation::NON_TETRAHEDRA;
    }
  }

  return svtkUnstructuredGridQuadricDecimation::NO_ERROR;
}

int svtkUnstructuredGridQuadricDecimationTetMesh::SaveUnstructuredGrid(svtkUnstructuredGrid* vgrid)
{
  svtkIdType growSize = (tCount - unusedTets) * 4;
  vgrid->Allocate(growSize, growSize);
  svtkPoints* vp = svtkPoints::New();
  svtkDoubleArray* vs = svtkDoubleArray::New();

  // Output vertices
  // We need a map for indexing
  std::map<svtkUnstructuredGridQuadricDecimationVertex*, int> indexes;

  int nPoints = 0;
  for (int i = 0; i < vCount; i++)
  {
    if (Verts[i].Corner >= 0)
    {
      ++nPoints;
    }
  }

  vp->SetNumberOfPoints(nPoints);
  vs->SetNumberOfValues(nPoints);
  int vIdx = 0;
  for (int i = 0; i < vCount; i++)
  {
    if (Verts[i].Corner >= 0)
    {
      vp->SetPoint(vIdx, Verts[i].Q.p[0], Verts[i].Q.p[1], Verts[i].Q.p[2]);
      vs->SetValue(vIdx, Verts[i].Q.p[3]);
      indexes[&Verts[i]] = vIdx++;
    }
  }
  vgrid->SetPoints(vp);
  vp->Delete();
  vs->SetName("scalars");
  vgrid->GetPointData()->AddArray(vs);
  vgrid->GetPointData()->SetScalars(vs);
  vs->Delete();

  svtkIdType idx[4];
  for (int i = 0; i < maxTet; i++)
  {
    if (tets[i].index >= 0)
    {
      for (int j = 0; j < 4; j++)
      {
        idx[j] = indexes[tets[i].Verts[j]];
      }
      vgrid->InsertNextCell(SVTK_TETRA, 4, idx);
    }
  }
  return svtkUnstructuredGridQuadricDecimation::NO_ERROR;
}

#undef SVTK_PRECHECK
#undef SVTK_FEPS
#undef SVTK_TEPS
#undef SVTK_SWAP

////////////////////////////////////////////////////////////////////////////////
/* ========================================================================== */
////////////////////////////////////////////////////////////////////////////////
/* === svtkUnstructuredGridQuadricDecimation                               === */
////////////////////////////////////////////////////////////////////////////////
/* ========================================================================== */
////////////////////////////////////////////////////////////////////////////////

svtkStandardNewMacro(svtkUnstructuredGridQuadricDecimation);

void svtkUnstructuredGridQuadricDecimation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Target Reduction: " << this->TargetReduction << "\n";
  os << indent << "Number of Tets to Output: " << this->NumberOfTetsOutput << "\n";
  os << indent << "Number of Edges to Decimate: " << this->NumberOfEdgesToDecimate << "\n";
  os << indent << "Number of Candidates Per Set: " << this->NumberOfCandidates << "\n";
  os << indent << "AutoAddCandidates: " << this->AutoAddCandidates << "\n";
  os << indent << "AutoAddCandidatesThreshold: " << this->AutoAddCandidatesThreshold << "\n";
  os << indent << "Boundary Weight: " << this->BoundaryWeight << "\n";
}

svtkUnstructuredGridQuadricDecimation::svtkUnstructuredGridQuadricDecimation()
{
  this->TargetReduction = 1.0;
  this->NumberOfTetsOutput = 0;
  this->NumberOfEdgesToDecimate = 0;
  this->NumberOfCandidates = 8;
  this->AutoAddCandidates = 1;
  this->AutoAddCandidatesThreshold = 0.4;
  this->BoundaryWeight = 100.0;
  this->ScalarsName = nullptr;
}

svtkUnstructuredGridQuadricDecimation::~svtkUnstructuredGridQuadricDecimation()
{
  delete[] this->ScalarsName;
}

void svtkUnstructuredGridQuadricDecimation::ReportError(int err)
{
  switch (err)
  {
    case svtkUnstructuredGridQuadricDecimation::NON_TETRAHEDRA:
      svtkErrorMacro(<< "Non-tetrahedral cells not supported!");
      break;
    case svtkUnstructuredGridQuadricDecimation::NO_SCALARS:
      svtkErrorMacro(<< "Can't simplify without scalars!");
      break;
    case svtkUnstructuredGridQuadricDecimation::NO_CELLS:
      svtkErrorMacro(<< "No Cells!");
      break;
    default:
      break;
  }
}

int svtkUnstructuredGridQuadricDecimation::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkUnstructuredGrid* input =
    svtkUnstructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkUnstructuredGridQuadricDecimationTetMesh myMesh;
  myMesh.doublingRatio = this->AutoAddCandidatesThreshold;
  myMesh.noDoubling = !this->AutoAddCandidates;
  myMesh.boundaryWeight = this->BoundaryWeight;
  int err = myMesh.LoadUnstructuredGrid((svtkUnstructuredGrid*)(input), this->ScalarsName);
  if (err != svtkUnstructuredGridQuadricDecimation::NO_ERROR)
  {
    this->ReportError(err);
    return 0;
  }

  myMesh.BuildFullMesh();

  int desiredTets = this->NumberOfTetsOutput;
  if (desiredTets == 0)
  {
    desiredTets = (int)((1 - this->TargetReduction) * myMesh.tCount);
  }
  myMesh.Simplify(this->NumberOfEdgesToDecimate, desiredTets);
  myMesh.SaveUnstructuredGrid(output);
  return 1;
}
