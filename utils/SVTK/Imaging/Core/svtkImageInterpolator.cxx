/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageInterpolator.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkImageInterpolatorInternals.h"
#include "svtkObjectFactory.h"
#include "svtkTypeTraits.h"

#include "svtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types, because
// they cannot be faithfully represented by doubles
#undef SVTK_USE_INT64
#define SVTK_USE_INT64 0
#undef SVTK_USE_UINT64
#define SVTK_USE_UINT64 0

svtkStandardNewMacro(svtkImageInterpolator);

//----------------------------------------------------------------------------
svtkImageInterpolator::svtkImageInterpolator()
{
  this->InterpolationMode = SVTK_LINEAR_INTERPOLATION;
}

//----------------------------------------------------------------------------
svtkImageInterpolator::~svtkImageInterpolator() = default;

//----------------------------------------------------------------------------
void svtkImageInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InterpolationMode: " << this->GetInterpolationModeAsString() << "\n";
}

//----------------------------------------------------------------------------
bool svtkImageInterpolator::IsSeparable()
{
  return true;
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::SetInterpolationMode(int mode)
{
  mode = svtkMath::ClampValue(mode, SVTK_NEAREST_INTERPOLATION, SVTK_CUBIC_INTERPOLATION);
  if (this->InterpolationMode != mode)
  {
    this->InterpolationMode = mode;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkImageInterpolator::GetInterpolationModeAsString()
{
  switch (this->InterpolationMode)
  {
    case SVTK_NEAREST_INTERPOLATION:
      return "Nearest";
    case SVTK_LINEAR_INTERPOLATION:
      return "Linear";
    case SVTK_CUBIC_INTERPOLATION:
      return "Cubic";
  }
  return "";
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::ComputeSupportSize(const double matrix[16], int size[3])
{
  int s = 1;
  if (this->InterpolationMode == SVTK_LINEAR_INTERPOLATION)
  {
    s = 2;
  }
  else if (this->InterpolationMode == SVTK_CUBIC_INTERPOLATION)
  {
    s = 4;
  }

  size[0] = s;
  size[1] = s;
  size[2] = s;

  if (matrix == nullptr)
  {
    return;
  }

  // check whether matrix does perspective
  if (matrix[12] != 0 || matrix[13] != 0 || matrix[14] != 0 || matrix[15] != 1.0)
  {
    return;
  }

  // find conditions where matrix maps integers to integer
  for (int i = 0; i < 3; i++)
  {
    int integerRow = 1;
    for (int j = 0; j < 3; j++)
    {
      // verify that the element is an integer
      double x = matrix[4 * i + j];
      // check fraction that remains after floor operation
      double f;
      svtkInterpolationMath::Floor(x, f);
      integerRow &= (f == 0);
    }
    // no extra support required in this direction
    if (integerRow)
    {
      size[i] = 1;
    }
  }
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::InternalDeepCopy(svtkAbstractImageInterpolator* a)
{
  svtkImageInterpolator* obj = svtkImageInterpolator::SafeDownCast(a);
  if (obj)
  {
    this->SetInterpolationMode(obj->InterpolationMode);
  }
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::InternalUpdate()
{
  svtkInterpolationInfo* info = this->InterpolationInfo;
  info->InterpolationMode = this->InterpolationMode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//  Interpolation subroutines and associated code
//----------------------------------------------------------------------------

namespace
{

//----------------------------------------------------------------------------
template <class F, class T>
struct svtkImageNLCInterpolate
{
  static void Nearest(svtkInterpolationInfo* info, const F point[3], F* outPtr);

  static void Trilinear(svtkInterpolationInfo* info, const F point[3], F* outPtr);

  static void Tricubic(svtkInterpolationInfo* info, const F point[3], F* outPtr);
};

//----------------------------------------------------------------------------
template <class F, class T>
void svtkImageNLCInterpolate<F, T>::Nearest(svtkInterpolationInfo* info, const F point[3], F* outPtr)
{
  const T* inPtr = static_cast<const T*>(info->Pointer);
  int* inExt = info->Extent;
  svtkIdType* inInc = info->Increments;
  int numscalars = info->NumberOfComponents;

  int inIdX0 = svtkInterpolationMath::Round(point[0]);
  int inIdY0 = svtkInterpolationMath::Round(point[1]);
  int inIdZ0 = svtkInterpolationMath::Round(point[2]);

  switch (info->BorderMode)
  {
    case SVTK_IMAGE_BORDER_REPEAT:
      inIdX0 = svtkInterpolationMath::Wrap(inIdX0, inExt[0], inExt[1]);
      inIdY0 = svtkInterpolationMath::Wrap(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = svtkInterpolationMath::Wrap(inIdZ0, inExt[4], inExt[5]);
      break;

    case SVTK_IMAGE_BORDER_MIRROR:
      inIdX0 = svtkInterpolationMath::Mirror(inIdX0, inExt[0], inExt[1]);
      inIdY0 = svtkInterpolationMath::Mirror(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = svtkInterpolationMath::Mirror(inIdZ0, inExt[4], inExt[5]);
      break;

    default:
      inIdX0 = svtkInterpolationMath::Clamp(inIdX0, inExt[0], inExt[1]);
      inIdY0 = svtkInterpolationMath::Clamp(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = svtkInterpolationMath::Clamp(inIdZ0, inExt[4], inExt[5]);
      break;
  }

  inPtr += inIdX0 * inInc[0] + inIdY0 * inInc[1] + inIdZ0 * inInc[2];
  do
  {
    *outPtr++ = *inPtr++;
  } while (--numscalars);
}

//----------------------------------------------------------------------------
template <class F, class T>
void svtkImageNLCInterpolate<F, T>::Trilinear(
  svtkInterpolationInfo* info, const F point[3], F* outPtr)
{
  const T* inPtr = static_cast<const T*>(info->Pointer);
  int* inExt = info->Extent;
  svtkIdType* inInc = info->Increments;
  int numscalars = info->NumberOfComponents;

  F fx, fy, fz;
  int inIdX0 = svtkInterpolationMath::Floor(point[0], fx);
  int inIdY0 = svtkInterpolationMath::Floor(point[1], fy);
  int inIdZ0 = svtkInterpolationMath::Floor(point[2], fz);

  int inIdX1 = inIdX0 + (fx != 0);
  int inIdY1 = inIdY0 + (fy != 0);
  int inIdZ1 = inIdZ0 + (fz != 0);

  switch (info->BorderMode)
  {
    case SVTK_IMAGE_BORDER_REPEAT:
      inIdX0 = svtkInterpolationMath::Wrap(inIdX0, inExt[0], inExt[1]);
      inIdY0 = svtkInterpolationMath::Wrap(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = svtkInterpolationMath::Wrap(inIdZ0, inExt[4], inExt[5]);

      inIdX1 = svtkInterpolationMath::Wrap(inIdX1, inExt[0], inExt[1]);
      inIdY1 = svtkInterpolationMath::Wrap(inIdY1, inExt[2], inExt[3]);
      inIdZ1 = svtkInterpolationMath::Wrap(inIdZ1, inExt[4], inExt[5]);
      break;

    case SVTK_IMAGE_BORDER_MIRROR:
      inIdX0 = svtkInterpolationMath::Mirror(inIdX0, inExt[0], inExt[1]);
      inIdY0 = svtkInterpolationMath::Mirror(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = svtkInterpolationMath::Mirror(inIdZ0, inExt[4], inExt[5]);

      inIdX1 = svtkInterpolationMath::Mirror(inIdX1, inExt[0], inExt[1]);
      inIdY1 = svtkInterpolationMath::Mirror(inIdY1, inExt[2], inExt[3]);
      inIdZ1 = svtkInterpolationMath::Mirror(inIdZ1, inExt[4], inExt[5]);
      break;

    default:
      inIdX0 = svtkInterpolationMath::Clamp(inIdX0, inExt[0], inExt[1]);
      inIdY0 = svtkInterpolationMath::Clamp(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = svtkInterpolationMath::Clamp(inIdZ0, inExt[4], inExt[5]);

      inIdX1 = svtkInterpolationMath::Clamp(inIdX1, inExt[0], inExt[1]);
      inIdY1 = svtkInterpolationMath::Clamp(inIdY1, inExt[2], inExt[3]);
      inIdZ1 = svtkInterpolationMath::Clamp(inIdZ1, inExt[4], inExt[5]);
      break;
  }

  svtkIdType factX0 = inIdX0 * inInc[0];
  svtkIdType factX1 = inIdX1 * inInc[0];
  svtkIdType factY0 = inIdY0 * inInc[1];
  svtkIdType factY1 = inIdY1 * inInc[1];
  svtkIdType factZ0 = inIdZ0 * inInc[2];
  svtkIdType factZ1 = inIdZ1 * inInc[2];

  svtkIdType i00 = factY0 + factZ0;
  svtkIdType i01 = factY0 + factZ1;
  svtkIdType i10 = factY1 + factZ0;
  svtkIdType i11 = factY1 + factZ1;

  F rx = 1 - fx;
  F ry = 1 - fy;
  F rz = 1 - fz;

  F ryrz = ry * rz;
  F fyrz = fy * rz;
  F ryfz = ry * fz;
  F fyfz = fy * fz;

  const T* inPtr0 = inPtr + factX0;
  const T* inPtr1 = inPtr + factX1;

  do
  {
    *outPtr++ =
      (rx * (ryrz * inPtr0[i00] + ryfz * inPtr0[i01] + fyrz * inPtr0[i10] + fyfz * inPtr0[i11]) +
        fx * (ryrz * inPtr1[i00] + ryfz * inPtr1[i01] + fyrz * inPtr1[i10] + fyfz * inPtr1[i11]));
    inPtr0++;
    inPtr1++;
  } while (--numscalars);
}

//----------------------------------------------------------------------------
// cubic helper function: set up the lookup indices and the interpolation
// coefficients

template <class T>
inline void svtkTricubicInterpWeights(T F[4], T f)
{
  static const T half = T(0.5);

  // cubic interpolation
  T fm1 = f - 1;
  T fd2 = f * half;
  T ft3 = f * 3;
  F[0] = -fd2 * fm1 * fm1;
  F[1] = ((ft3 - 2) * fd2 - 1) * fm1;
  F[2] = -((ft3 - 4) * f - 1) * fd2;
  F[3] = f * fd2 * fm1;
}

//----------------------------------------------------------------------------
// tricubic interpolation
template <class F, class T>
void svtkImageNLCInterpolate<F, T>::Tricubic(svtkInterpolationInfo* info, const F point[3], F* outPtr)
{
  const T* inPtr = static_cast<const T*>(info->Pointer);
  int* inExt = info->Extent;
  svtkIdType* inInc = info->Increments;
  int numscalars = info->NumberOfComponents;

  F fx, fy, fz;
  int inIdX0 = svtkInterpolationMath::Floor(point[0], fx);
  int inIdY0 = svtkInterpolationMath::Floor(point[1], fy);
  int inIdZ0 = svtkInterpolationMath::Floor(point[2], fz);

  // change arrays into locals
  svtkIdType inIncX = inInc[0];
  svtkIdType inIncY = inInc[1];
  svtkIdType inIncZ = inInc[2];

  int minX = inExt[0];
  int maxX = inExt[1];
  int minY = inExt[2];
  int maxY = inExt[3];
  int minZ = inExt[4];
  int maxZ = inExt[5];

  // the memory offsets
  svtkIdType factX[4], factY[4], factZ[4];

  switch (info->BorderMode)
  {
    case SVTK_IMAGE_BORDER_REPEAT:
      factX[0] = svtkInterpolationMath::Wrap(inIdX0 - 1, minX, maxX) * inIncX;
      factX[1] = svtkInterpolationMath::Wrap(inIdX0, minX, maxX) * inIncX;
      factX[2] = svtkInterpolationMath::Wrap(inIdX0 + 1, minX, maxX) * inIncX;
      factX[3] = svtkInterpolationMath::Wrap(inIdX0 + 2, minX, maxX) * inIncX;

      factY[0] = svtkInterpolationMath::Wrap(inIdY0 - 1, minY, maxY) * inIncY;
      factY[1] = svtkInterpolationMath::Wrap(inIdY0, minY, maxY) * inIncY;
      factY[2] = svtkInterpolationMath::Wrap(inIdY0 + 1, minY, maxY) * inIncY;
      factY[3] = svtkInterpolationMath::Wrap(inIdY0 + 2, minY, maxY) * inIncY;

      factZ[0] = svtkInterpolationMath::Wrap(inIdZ0 - 1, minZ, maxZ) * inIncZ;
      factZ[1] = svtkInterpolationMath::Wrap(inIdZ0, minZ, maxZ) * inIncZ;
      factZ[2] = svtkInterpolationMath::Wrap(inIdZ0 + 1, minZ, maxZ) * inIncZ;
      factZ[3] = svtkInterpolationMath::Wrap(inIdZ0 + 2, minZ, maxZ) * inIncZ;
      break;

    case SVTK_IMAGE_BORDER_MIRROR:
      factX[0] = svtkInterpolationMath::Mirror(inIdX0 - 1, minX, maxX) * inIncX;
      factX[1] = svtkInterpolationMath::Mirror(inIdX0, minX, maxX) * inIncX;
      factX[2] = svtkInterpolationMath::Mirror(inIdX0 + 1, minX, maxX) * inIncX;
      factX[3] = svtkInterpolationMath::Mirror(inIdX0 + 2, minX, maxX) * inIncX;

      factY[0] = svtkInterpolationMath::Mirror(inIdY0 - 1, minY, maxY) * inIncY;
      factY[1] = svtkInterpolationMath::Mirror(inIdY0, minY, maxY) * inIncY;
      factY[2] = svtkInterpolationMath::Mirror(inIdY0 + 1, minY, maxY) * inIncY;
      factY[3] = svtkInterpolationMath::Mirror(inIdY0 + 2, minY, maxY) * inIncY;

      factZ[0] = svtkInterpolationMath::Mirror(inIdZ0 - 1, minZ, maxZ) * inIncZ;
      factZ[1] = svtkInterpolationMath::Mirror(inIdZ0, minZ, maxZ) * inIncZ;
      factZ[2] = svtkInterpolationMath::Mirror(inIdZ0 + 1, minZ, maxZ) * inIncZ;
      factZ[3] = svtkInterpolationMath::Mirror(inIdZ0 + 2, minZ, maxZ) * inIncZ;
      break;

    default:
      factX[0] = svtkInterpolationMath::Clamp(inIdX0 - 1, minX, maxX) * inIncX;
      factX[1] = svtkInterpolationMath::Clamp(inIdX0, minX, maxX) * inIncX;
      factX[2] = svtkInterpolationMath::Clamp(inIdX0 + 1, minX, maxX) * inIncX;
      factX[3] = svtkInterpolationMath::Clamp(inIdX0 + 2, minX, maxX) * inIncX;

      factY[0] = svtkInterpolationMath::Clamp(inIdY0 - 1, minY, maxY) * inIncY;
      factY[1] = svtkInterpolationMath::Clamp(inIdY0, minY, maxY) * inIncY;
      factY[2] = svtkInterpolationMath::Clamp(inIdY0 + 1, minY, maxY) * inIncY;
      factY[3] = svtkInterpolationMath::Clamp(inIdY0 + 2, minY, maxY) * inIncY;

      factZ[0] = svtkInterpolationMath::Clamp(inIdZ0 - 1, minZ, maxZ) * inIncZ;
      factZ[1] = svtkInterpolationMath::Clamp(inIdZ0, minZ, maxZ) * inIncZ;
      factZ[2] = svtkInterpolationMath::Clamp(inIdZ0 + 1, minZ, maxZ) * inIncZ;
      factZ[3] = svtkInterpolationMath::Clamp(inIdZ0 + 2, minZ, maxZ) * inIncZ;
      break;
  }

  // get the interpolation coefficients
  F fX[4], fY[4], fZ[4];
  svtkTricubicInterpWeights(fX, fx);
  svtkTricubicInterpWeights(fY, fy);
  svtkTricubicInterpWeights(fZ, fz);

  // check if only one slice in a particular direction
  int multipleY = (minY != maxY);
  int multipleZ = (minZ != maxZ);

  // or if fractional offset is zero
  multipleY &= (fy != 0);
  multipleZ &= (fz != 0);

  // the limits to use when doing the interpolation
  int j1 = 1 - multipleY;
  int j2 = 1 + 2 * multipleY;

  int k1 = 1 - multipleZ;
  int k2 = 1 + 2 * multipleZ;

  // if only one coefficient will be used
  if (multipleY == 0)
  {
    fY[1] = 1;
  }
  if (multipleZ == 0)
  {
    fZ[1] = 1;
  }

  do // loop over components
  {
    F val = 0;
    int k = k1;
    do // loop over z
    {
      F ifz = fZ[k];
      svtkIdType factz = factZ[k];
      int j = j1;
      do // loop over y
      {
        F ify = fY[j];
        F fzy = ifz * ify;
        svtkIdType factzy = factz + factY[j];
        const T* tmpPtr = inPtr + factzy;
        // loop over x is unrolled (significant performance boost)
        val += fzy *
          (fX[0] * tmpPtr[factX[0]] + fX[1] * tmpPtr[factX[1]] + fX[2] * tmpPtr[factX[2]] +
            fX[3] * tmpPtr[factX[3]]);
      } while (++j <= j2);
    } while (++k <= k2);

    *outPtr++ = val;
    inPtr++;
  } while (--numscalars);
}

//----------------------------------------------------------------------------
// Get the interpolation function for the specified data types
template <class F>
void svtkImageInterpolatorGetInterpolationFunc(
  void (**interpolate)(svtkInterpolationInfo*, const F[3], F*), int dataType, int interpolationMode)
{
  switch (interpolationMode)
  {
    case SVTK_NEAREST_INTERPOLATION:
      switch (dataType)
      {
        svtkTemplateAliasMacro(*interpolate = &(svtkImageNLCInterpolate<F, SVTK_TT>::Nearest));
        default:
          *interpolate = nullptr;
      }
      break;
    case SVTK_LINEAR_INTERPOLATION:
      switch (dataType)
      {
        svtkTemplateAliasMacro(*interpolate = &(svtkImageNLCInterpolate<F, SVTK_TT>::Trilinear));
        default:
          *interpolate = nullptr;
      }
      break;
    case SVTK_CUBIC_INTERPOLATION:
      switch (dataType)
      {
        svtkTemplateAliasMacro(*interpolate = &(svtkImageNLCInterpolate<F, SVTK_TT>::Tricubic));
        default:
          *interpolate = nullptr;
      }
      break;
  }
}

//----------------------------------------------------------------------------
// Interpolation for precomputed weights

template <class F, class T>
struct svtkImageNLCRowInterpolate
{
  static void Nearest(
    svtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);

  static void Trilinear(
    svtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);

  static void Tricubic(
    svtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);
};

//----------------------------------------------------------------------------
// helper function for nearest neighbor interpolation
template <class F, class T>
void svtkImageNLCRowInterpolate<F, T>::Nearest(
  svtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n)
{
  const svtkIdType* iX = weights->Positions[0] + idX;
  const svtkIdType* iY = weights->Positions[1] + idY;
  const svtkIdType* iZ = weights->Positions[2] + idZ;
  const T* inPtr0 = static_cast<const T*>(weights->Pointer) + iY[0] + iZ[0];

  // get the number of components per pixel
  int numscalars = weights->NumberOfComponents;

  // This is a hot loop.
  for (int i = n; i > 0; --i)
  {
    const T* tmpPtr = &inPtr0[iX[0]];
    iX++;
    int m = numscalars;
    do
    {
      *outPtr++ = *tmpPtr++;
    } while (--m);
  }
}

//----------------------------------------------------------------------------
// helper function for linear interpolation
template <class F, class T>
void svtkImageNLCRowInterpolate<F, T>::Trilinear(
  svtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n)
{
  int stepX = weights->KernelSize[0];
  int stepY = weights->KernelSize[1];
  int stepZ = weights->KernelSize[2];
  idX *= stepX;
  idY *= stepY;
  idZ *= stepZ;
  const F* fX = static_cast<F*>(weights->Weights[0]) + idX;
  const F* fY = static_cast<F*>(weights->Weights[1]) + idY;
  const F* fZ = static_cast<F*>(weights->Weights[2]) + idZ;
  const svtkIdType* iX = weights->Positions[0] + idX;
  const svtkIdType* iY = weights->Positions[1] + idY;
  const svtkIdType* iZ = weights->Positions[2] + idZ;
  const T* inPtr = static_cast<const T*>(weights->Pointer);

  // get the number of components per pixel
  int numscalars = weights->NumberOfComponents;

  // create a 2x2 bilinear kernel in local variables
  svtkIdType i00 = iY[0] + iZ[0];
  svtkIdType i01 = i00;
  svtkIdType i10 = i00;
  svtkIdType i11 = i00;

  F ry = static_cast<F>(1);
  F fy = static_cast<F>(0);
  F rz = static_cast<F>(1);
  F fz = static_cast<F>(0);

  if (stepY == 2)
  {
    i10 = iY[1] + iZ[0];
    i11 = i10;
    ry = fY[0];
    fy = fY[1];
  }

  if (stepZ == 2)
  {
    i01 = iY[0] + iZ[1];
    i11 = i01;
    rz = fZ[0];
    fz = fZ[1];
  }

  if (stepY + stepZ == 4)
  {
    i11 = iY[1] + iZ[1];
  }

  F ryrz = ry * rz;
  F ryfz = ry * fz;
  F fyrz = fy * rz;
  F fyfz = fy * fz;

  if (stepX == 1)
  {
    if (fy == 0 && fz == 0)
    { // no interpolation needed at all
      const T* inPtr1 = inPtr + i00;
      for (int i = n; i > 0; --i)
      {
        const T* inPtr0 = inPtr1 + *iX++;
        int m = numscalars;
        do
        {
          *outPtr++ = *inPtr0++;
        } while (--m);
      }
    }
    else if (fy == 0)
    { // only need linear z interpolation
      for (int i = n; i > 0; --i)
      {
        const T* inPtr0 = inPtr + *iX++;
        int m = numscalars;
        do
        {
          *outPtr++ = (rz * inPtr0[i00] + fz * inPtr0[i01]);
          inPtr0++;
        } while (--m);
      }
    }
    else
    { // interpolate in y and z but not in x
      for (int i = n; i > 0; --i)
      {
        const T* inPtr0 = inPtr + *iX++;
        int m = numscalars;
        do
        {
          *outPtr++ =
            (ryrz * inPtr0[i00] + ryfz * inPtr0[i01] + fyrz * inPtr0[i10] + fyfz * inPtr0[i11]);
          inPtr0++;
        } while (--m);
      }
    }
  }
  else if (fz == 0)
  { // bilinear interpolation in x,y
    for (int i = n; i > 0; --i)
    {
      F rx = fX[0];
      F fx = fX[1];
      fX += 2;

      svtkIdType t0 = iX[0];
      svtkIdType t1 = iX[1];
      iX += 2;

      const T* inPtr0 = inPtr + t0;
      const T* inPtr1 = inPtr + t1;
      int m = numscalars;
      do
      {
        *outPtr++ =
          (rx * (ry * inPtr0[i00] + fy * inPtr0[i10]) + fx * (ry * inPtr1[i00] + fy * inPtr1[i10]));
        inPtr0++;
        inPtr1++;
      } while (--m);
    }
  }
  else
  { // do full trilinear interpolation
    for (int i = n; i > 0; --i)
    {
      F rx = fX[0];
      F fx = fX[1];
      fX += 2;

      svtkIdType t0 = iX[0];
      svtkIdType t1 = iX[1];
      iX += 2;

      const T* inPtr0 = inPtr + t0;
      const T* inPtr1 = inPtr + t1;
      int m = numscalars;
      do
      {
        *outPtr++ = (rx *
            (ryrz * inPtr0[i00] + ryfz * inPtr0[i01] + fyrz * inPtr0[i10] + fyfz * inPtr0[i11]) +
          fx * (ryrz * inPtr1[i00] + ryfz * inPtr1[i01] + fyrz * inPtr1[i10] + fyfz * inPtr1[i11]));
        inPtr0++;
        inPtr1++;
      } while (--m);
    }
  }
}

//--------------------------------------------------------------------------
// helper function for tricubic interpolation
template <class F, class T>
void svtkImageNLCRowInterpolate<F, T>::Tricubic(
  svtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n)
{
  int stepX = weights->KernelSize[0];
  int stepY = weights->KernelSize[1];
  int stepZ = weights->KernelSize[2];
  idX *= stepX;
  idY *= stepY;
  idZ *= stepZ;
  const F* fX = static_cast<F*>(weights->Weights[0]) + idX;
  const F* fY = static_cast<F*>(weights->Weights[1]) + idY;
  const F* fZ = static_cast<F*>(weights->Weights[2]) + idZ;
  const svtkIdType* iX = weights->Positions[0] + idX;
  const svtkIdType* iY = weights->Positions[1] + idY;
  const svtkIdType* iZ = weights->Positions[2] + idZ;
  const T* inPtr = static_cast<const T*>(weights->Pointer);

  // get the number of components per pixel
  int numscalars = weights->NumberOfComponents;

  for (int i = n; i > 0; --i)
  {
    svtkIdType iX0 = iX[0];
    svtkIdType iX1 = iX0;
    svtkIdType iX2 = iX0;
    svtkIdType iX3 = iX0;
    F fX0 = static_cast<F>(1);
    F fX1 = static_cast<F>(0);
    F fX2 = fX1;
    F fX3 = fX1;

    switch (stepX)
    {
      case 4:
        iX3 = iX[3];
        fX3 = fX[3];
        SVTK_FALLTHROUGH;
      case 3:
        iX2 = iX[2];
        fX2 = fX[2];
        SVTK_FALLTHROUGH;
      case 2:
        iX1 = iX[1];
        fX1 = fX[1];
        fX0 = fX[0];
    }

    iX += stepX;
    fX += stepX;

    const T* inPtr0 = inPtr;
    int c = numscalars;
    do
    { // loop over components
      F result = 0;

      int k = 0;
      do
      { // loop over z
        F fz = fZ[k];
        if (fz != 0)
        {
          svtkIdType iz = iZ[k];
          int j = 0;
          do
          { // loop over y
            F fy = fY[j];
            F fzy = fz * fy;
            svtkIdType izy = iz + iY[j];
            const T* tmpPtr = inPtr0 + izy;
            // loop over x is unrolled (significant performance boost)
            result +=
              fzy * (fX0 * tmpPtr[iX0] + fX1 * tmpPtr[iX1] + fX2 * tmpPtr[iX2] + fX3 * tmpPtr[iX3]);
          } while (++j < stepY);
        }
      } while (++k < stepZ);

      *outPtr++ = result;
      inPtr0++;
    } while (--c);
  }
}

//----------------------------------------------------------------------------
// get row interpolation function for different interpolation modes
// and different scalar types
template <class F>
void svtkImageInterpolatorGetRowInterpolationFunc(
  void (**summation)(svtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n),
  int scalarType, int interpolationMode)
{
  switch (interpolationMode)
  {
    case SVTK_NEAREST_INTERPOLATION:
      switch (scalarType)
      {
        svtkTemplateAliasMacro(*summation = &(svtkImageNLCRowInterpolate<F, SVTK_TT>::Nearest));
        default:
          *summation = nullptr;
      }
      break;
    case SVTK_LINEAR_INTERPOLATION:
      switch (scalarType)
      {
        svtkTemplateAliasMacro(*summation = &(svtkImageNLCRowInterpolate<F, SVTK_TT>::Trilinear));
        default:
          *summation = nullptr;
      }
      break;
    case SVTK_CUBIC_INTERPOLATION:
      switch (scalarType)
      {
        svtkTemplateAliasMacro(*summation = &(svtkImageNLCRowInterpolate<F, SVTK_TT>::Tricubic));
        default:
          *summation = nullptr;
      }
      break;
  }
}

//----------------------------------------------------------------------------
template <class F>
void svtkImageInterpolatorPrecomputeWeights(const F newmat[16], const int outExt[6], int clipExt[6],
  const F bounds[6], svtkInterpolationWeights* weights)
{
  weights->WeightType = svtkTypeTraits<F>::SVTKTypeID();
  int interpMode = weights->InterpolationMode;

  // set up input traversal table
  bool validClip = true;
  for (int j = 0; j < 3; j++)
  {
    // set k to the row for which the element in column j is nonzero,
    // and set matrow to the elements of that row
    int k = 0;
    const F* matrow = newmat;
    while (k < 3 && matrow[j] == 0)
    {
      k++;
      matrow += 4;
    }

    // get the extents
    clipExt[2 * j] = outExt[2 * j];
    clipExt[2 * j + 1] = outExt[2 * j + 1];
    int minExt = weights->Extent[2 * k];
    int maxExt = weights->Extent[2 * k + 1];
    F minBounds = bounds[2 * k];
    F maxBounds = bounds[2 * k + 1];

    // the kernel size should not exceed the input dimension
    int step = 1;
    step = ((interpMode < SVTK_LINEAR_INTERPOLATION) ? step : 2);
    step = ((interpMode < SVTK_CUBIC_INTERPOLATION) ? step : 4);
    int inCount = maxExt - minExt + 1;
    step = ((step < inCount) ? step : inCount);

    // if output pixels lie exactly on top of the input pixels
    F f1, f2;
    svtkInterpolationMath::Floor(matrow[j], f1);
    svtkInterpolationMath::Floor(matrow[3], f2);
    if (f1 == 0 && f2 == 0)
    {
      step = 1;
    }

    // allocate space for the weights
    svtkIdType size = step * (outExt[2 * j + 1] - outExt[2 * j] + 1);
    svtkIdType* positions = new svtkIdType[size];
    positions -= step * outExt[2 * j];
    F* constants = nullptr;
    if (interpMode != SVTK_NEAREST_INTERPOLATION)
    {
      constants = new F[size];
      constants -= step * outExt[2 * j];
    }

    // store the info in the "weights" object
    weights->KernelSize[j] = step;
    weights->WeightExtent[2 * j] = outExt[2 * j];
    weights->WeightExtent[2 * j + 1] = outExt[2 * j + 1];
    weights->Positions[j] = positions;
    weights->Weights[j] = constants;

    // march through the indices
    int region = 0;
    for (int i = outExt[2 * j]; i <= outExt[2 * j + 1]; i++)
    {
      F point = matrow[3] + i * matrow[j];

      int lcount = step;
      int inId0 = 0;
      F f = 0;
      if (interpMode == SVTK_NEAREST_INTERPOLATION)
      {
        inId0 = svtkInterpolationMath::Round(point);
      }
      else
      {
        inId0 = svtkInterpolationMath::Floor(point, f);
        if (interpMode == SVTK_CUBIC_INTERPOLATION && step != 1)
        {
          inId0--;
          lcount = 4;
        }
      }

      int inId[4] = { 0, 0, 0, 0 };
      int l = 0;
      switch (weights->BorderMode)
      {
        case SVTK_IMAGE_BORDER_REPEAT:
          do
          {
            inId[l] = svtkInterpolationMath::Wrap(inId0, minExt, maxExt);
            inId0++;
          } while (++l < lcount);
          break;

        case SVTK_IMAGE_BORDER_MIRROR:
          do
          {
            inId[l] = svtkInterpolationMath::Mirror(inId0, minExt, maxExt);
            inId0++;
          } while (++l < lcount);
          break;

        default:
          do
          {
            inId[l] = svtkInterpolationMath::Clamp(inId0, minExt, maxExt);
            inId0++;
          } while (++l < lcount);
          break;
      }

      // compute the weights and offsets
      svtkIdType inInc = weights->Increments[k];
      positions[step * i] = inId[0] * inInc;
      if (interpMode != SVTK_NEAREST_INTERPOLATION)
      {
        constants[step * i] = static_cast<F>(1);
      }
      if (step > 1)
      {
        if (interpMode == SVTK_LINEAR_INTERPOLATION)
        {
          positions[step * i + 1] = inId[1] * inInc;
          constants[step * i] = static_cast<F>(1.0 - f);
          constants[step * i + 1] = static_cast<F>(f);
        }
        else if (interpMode == SVTK_CUBIC_INTERPOLATION)
        {
          F g[4];
          svtkTricubicInterpWeights(g, f);
          if (step == 4)
          {
            for (int ll = 0; ll < 4; ll++)
            {
              positions[step * i + ll] = inId[ll] * inInc;
              constants[step * i + ll] = g[ll];
            }
          }
          else
          {
            // it gets tricky if there are fewer than 4 slices
            F gg[4] = { 0, 0, 0, 0 };
            for (int ll = 0; ll < 4; ll++)
            {
              int rIdx = inId[ll] - minExt;
              gg[rIdx] += g[ll];
            }
            for (int jj = 0; jj < step; jj++)
            {
              positions[step * i + jj] = minExt + jj;
              constants[step * i + jj] = gg[jj];
            }
          }
        }
      }

      if (point >= minBounds && point <= maxBounds)
      {
        if (region == 0)
        { // entering the input extent
          region = 1;
          clipExt[2 * j] = i;
        }
      }
      else
      {
        if (region == 1)
        { // leaving the input extent
          region = 2;
          clipExt[2 * j + 1] = i - 1;
        }
      }
    }

    if (region == 0 || clipExt[2 * j] > clipExt[2 * j + 1])
    { // never entered input extent!
      validClip = false;
    }
  }

  if (!validClip)
  {
    // output extent doesn't itersect input extent
    for (int j = 0; j < 3; j++)
    {
      clipExt[2 * j] = outExt[2 * j];
      clipExt[2 * j + 1] = outExt[2 * j] - 1;
    }
  }
}

//----------------------------------------------------------------------------
} // ends anonymous namespace

//----------------------------------------------------------------------------
void svtkImageInterpolator::GetInterpolationFunc(
  void (**func)(svtkInterpolationInfo*, const double[3], double*))
{
  svtkImageInterpolatorGetInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->InterpolationMode);
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::GetInterpolationFunc(
  void (**func)(svtkInterpolationInfo*, const float[3], float*))
{
  svtkImageInterpolatorGetInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->InterpolationMode);
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::GetRowInterpolationFunc(
  void (**func)(svtkInterpolationWeights*, int, int, int, double*, int))
{
  svtkImageInterpolatorGetRowInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->InterpolationMode);
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::GetRowInterpolationFunc(
  void (**func)(svtkInterpolationWeights*, int, int, int, float*, int))
{
  svtkImageInterpolatorGetRowInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->InterpolationMode);
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::PrecomputeWeightsForExtent(
  const double matrix[16], const int extent[6], int newExtent[6], svtkInterpolationWeights*& weights)
{
  weights = new svtkInterpolationWeights(*this->InterpolationInfo);

  svtkImageInterpolatorPrecomputeWeights(
    matrix, extent, newExtent, this->StructuredBoundsDouble, weights);
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::PrecomputeWeightsForExtent(
  const float matrix[16], const int extent[6], int newExtent[6], svtkInterpolationWeights*& weights)
{
  weights = new svtkInterpolationWeights(*this->InterpolationInfo);

  svtkImageInterpolatorPrecomputeWeights(
    matrix, extent, newExtent, this->StructuredBoundsFloat, weights);
}

//----------------------------------------------------------------------------
void svtkImageInterpolator::FreePrecomputedWeights(svtkInterpolationWeights*& weights)
{
  this->Superclass::FreePrecomputedWeights(weights);
}
