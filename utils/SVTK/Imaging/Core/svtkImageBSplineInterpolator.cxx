/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageBSplineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageBSplineInterpolator.h"
#include "svtkDataArray.h"
#include "svtkImageBSplineInternals.h"
#include "svtkImageData.h"
#include "svtkImageInterpolatorInternals.h"
#include "svtkObjectFactory.h"

#include "svtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types, because
// they cannot be faithfully represented by doubles
#undef SVTK_USE_INT64
#define SVTK_USE_INT64 0
#undef SVTK_USE_UINT64
#define SVTK_USE_UINT64 0

#define SVTK_BSPLINE_KERNEL_SIZE_MAX (SVTK_IMAGE_BSPLINE_DEGREE_MAX + 1)
#define SVTK_BSPLINE_INT_INITIALIZER                                                                \
  {                                                                                                \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                                                  \
  }

// kernel lookup table size must be 256*n where n is kernel half-width
// in order to provide sufficient precision for 16-bit images
#ifdef SVTK_BSPLINE_USE_KERNEL_TABLE
#define SVTK_BSPLINE_KERNEL_TABLE_DIVISIONS 256
#endif

svtkStandardNewMacro(svtkImageBSplineInterpolator);

//----------------------------------------------------------------------------
svtkImageBSplineInterpolator::svtkImageBSplineInterpolator()
{
  this->SplineDegree = 3;
  this->KernelLookupTable = nullptr;
}

//----------------------------------------------------------------------------
svtkImageBSplineInterpolator::~svtkImageBSplineInterpolator()
{
  if (this->KernelLookupTable)
  {
    this->FreeKernelLookupTable();
  }
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SplineDegree: " << this->SplineDegree << "\n";
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::ComputeSupportSize(
  const double svtkNotUsed(matrix)[16], int size[3])
{
  int n = this->SplineDegree + 1;
  size[0] = n;
  size[1] = n;
  size[2] = n;
}

//----------------------------------------------------------------------------
bool svtkImageBSplineInterpolator::IsSeparable()
{
  return true;
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::SetSplineDegree(int degree)
{
  degree = svtkMath::ClampValue(degree, 0, SVTK_IMAGE_BSPLINE_DEGREE_MAX);
  if (this->SplineDegree != degree)
  {
    this->SplineDegree = degree;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::InternalDeepCopy(svtkAbstractImageInterpolator* a)
{
  svtkImageBSplineInterpolator* obj = svtkImageBSplineInterpolator::SafeDownCast(a);

  if (obj)
  {
    this->SetSplineDegree(obj->SplineDegree);
  }

  if (this->KernelLookupTable)
  {
    this->FreeKernelLookupTable();
  }
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::InternalUpdate()
{
  int mode = this->SplineDegree;

  if (this->InterpolationInfo->InterpolationMode != mode || this->KernelLookupTable == nullptr)
  {
    this->BuildKernelLookupTable();
  }

  this->InterpolationInfo->InterpolationMode = mode;
  this->InterpolationInfo->ExtraInfo = this->KernelLookupTable;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//  Interpolation subroutines and associated code
//----------------------------------------------------------------------------

namespace
{

#ifdef SVTK_BSPLINE_USE_KERNEL_TABLE
//----------------------------------------------------------------------------
// B-spline kernel computation: compute half of the symmetric kernel.
// In the table, x=0.0 corresponds to index position zero, and each
// "m" bins corresponds to a unit spacing.  The full size of the table
// will be m*(n + 1)/2.

struct svtkBSplineKernel
{
  template <class F>
  static void BSpline(F* kernel, int size, int n);
};

template <class F>
void svtkBSplineKernel::BSpline(F* kernel, int m, int n)
{
  long order = n;
  int kn = (n + 2) / 2;
  int km = n - kn + 1;
  double offset = 0;
  double delta = 1.0 / m;
  double weights[SVTK_BSPLINE_KERNEL_SIZE_MAX];

  // special case for order zero
  if (n == 0)
  {
    int k = m * (n + 1) / 2;
    do
    {
      *kernel++ = 1.0;
    } while (--k);
    return;
  }

  // for offset of zero, weights are symmetrical
  svtkImageBSplineInternals::GetInterpolationWeights(weights, offset, order);
  double* weights2 = weights + kn;
  F* kernel2 = kernel;
  int k2 = kn;
  do
  {
    --weights2;
    *kernel2 = *weights2;
    kernel2 += m;
  } while (--k2);

  // need the opposite end of the kernel array
  kernel2 = kernel + km * m;

  int j = m / 2;
  if (j)
    do
    {
      kernel++;
      kernel2--;
      offset += delta;

      svtkImageBSplineInternals::GetInterpolationWeights(weights, offset, order);

      double* weights1 = weights + n + 1;
      F* kernel1 = kernel2;
      int k = km;
      do
      {
        --weights1;
        *kernel1 = *weights1;
        kernel1 -= m;
      } while (--k);
      kernel1 = kernel;
      k = kn;
      do
      {
        --weights1;
        *kernel1 = *weights1;
        kernel1 += m;
      } while (--k);
    } while (--j);
}

//----------------------------------------------------------------------------
// Use kernel lookup table, might improve performance for large kernels
template <class T, class F>
void svtkBSplineInterpWeights(T* kernel, F* fX, F fx, int m)
{
  if (m == 0)
  {
    fX[0] = 1;
    return;
  }

  // table bins per unit
  int p = SVTK_BSPLINE_KERNEL_TABLE_DIVISIONS;

  // compute table interpolation info
  F f = fx * p;
  int offset = static_cast<int>(f);
  f -= offset;
  F r = 1 - f;

  // interpolate the table
  int n = m + 1;
  F s = 0;
  int i = (1 - ((n + 1) >> 1)) * p - offset;
  do
  {
    int i0 = i;
    int i1 = i + 1;
    int ni = -i0;
    i0 = ((i0 >= 0) ? i0 : ni);
    ni = -i1;
    i1 = ((i1 >= 0) ? i1 : ni);
    F y = r * kernel[i0] + f * kernel[i1];
    *fX = y;
    s += y;
    i += p;
    fX++;
  } while (--n);
}
#endif

//----------------------------------------------------------------------------
template <class F, class T>
struct svtkImageBSplineInterpolate
{
  static void BSpline(svtkInterpolationInfo* info, const F point[3], F* outPtr);
};

//----------------------------------------------------------------------------
template <class F, class T>
void svtkImageBSplineInterpolate<F, T>::BSpline(
  svtkInterpolationInfo* info, const F point[3], F* outPtr)
{
  const T* inPtr = static_cast<const T*>(info->Pointer);
  int* inExt = info->Extent;
  svtkIdType* inInc = info->Increments;
  int numscalars = info->NumberOfComponents;

#ifdef SVTK_BSPLINE_USE_KERNEL_TABLE
  // kernel lookup table
  float* kernel = static_cast<float*>(info->ExtraInfo);
#endif

  // size of kernel is degree of spline plus one
  int n = info->InterpolationMode;
  int m = n + 1;

  // index to kernel midpoint position
  int m2 = (n >> 1);

  // offset for odd-size kernels
  F offset = 0.5 * (m & 1);

  F fx, fy, fz;
  int inIdX0 = svtkInterpolationMath::Floor(point[0] + offset, fx);
  int inIdY0 = svtkInterpolationMath::Floor(point[1] + offset, fy);
  int inIdZ0 = svtkInterpolationMath::Floor(point[2] + offset, fz);

  fx -= offset;
  fy -= offset;
  fz -= offset;

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
  svtkIdType factX[SVTK_BSPLINE_KERNEL_SIZE_MAX + 4];
  svtkIdType factY[SVTK_BSPLINE_KERNEL_SIZE_MAX];
  svtkIdType factZ[SVTK_BSPLINE_KERNEL_SIZE_MAX];

  // handle the borders
  int xi = inIdX0 - m2;
  int yi = inIdY0 - m2;
  int zi = inIdZ0 - m2;

  switch (info->BorderMode)
  {
    case SVTK_IMAGE_BORDER_REPEAT:
    {
      int l = 0;
      int mm = m;
      do
      {
        factX[l] = svtkInterpolationMath::Wrap(xi, minX, maxX) * inIncX;
        factY[l] = svtkInterpolationMath::Wrap(yi, minY, maxY) * inIncY;
        factZ[l] = svtkInterpolationMath::Wrap(zi, minZ, maxZ) * inIncZ;
        l++;
        xi++;
        yi++;
        zi++;
      } while (--mm);
    }
    break;

    case SVTK_IMAGE_BORDER_MIRROR:
    {
      int l = 0;
      int mm = m;
      do
      {
        factX[l] = svtkInterpolationMath::Mirror(xi, minX, maxX) * inIncX;
        factY[l] = svtkInterpolationMath::Mirror(yi, minY, maxY) * inIncY;
        factZ[l] = svtkInterpolationMath::Mirror(zi, minZ, maxZ) * inIncZ;
        l++;
        xi++;
        yi++;
        zi++;
      } while (--mm);
    }
    break;

    default:
    {
      int l = 0;
      int mm = m;
      do
      {
        factX[l] = svtkInterpolationMath::Clamp(xi, minX, maxX) * inIncX;
        factY[l] = svtkInterpolationMath::Clamp(yi, minY, maxY) * inIncY;
        factZ[l] = svtkInterpolationMath::Clamp(zi, minZ, maxZ) * inIncZ;
        l++;
        xi++;
        yi++;
        zi++;
      } while (--mm);
    }
    break;
  }

  // compute the kernel weights (pad X for loop unrolling)
  F fX[SVTK_BSPLINE_KERNEL_SIZE_MAX + 4];
  F fY[SVTK_BSPLINE_KERNEL_SIZE_MAX];
  F fZ[SVTK_BSPLINE_KERNEL_SIZE_MAX];

  // check if only one slice in a particular direction
  int nx = n * (minX != maxX);
  int ny = n * (minY != maxY);
  int nz = n * (minZ != maxZ);

#ifdef SVTK_BSPLINE_USE_KERNEL_TABLE
  svtkBSplineInterpWeights(kernel, fX, fx, nx);
  svtkBSplineInterpWeights(kernel, fY, fy, ny);
  svtkBSplineInterpWeights(kernel, fZ, fz, nz);
#else
  svtkImageBSplineInternals::GetInterpolationWeights(fX, fx, nx);
  svtkImageBSplineInternals::GetInterpolationWeights(fY, fy, ny);
  svtkImageBSplineInternals::GetInterpolationWeights(fZ, fz, nz);
#endif

  // pad coeffs to allow unrolling of inner loop
  int lm = ((nx + 4) >> 2);
  svtkIdType factXl = factX[nx];
  svtkIdType* tmpfactXl = &factX[nx + 1];
  F* tmpfXl = &fX[nx + 1];
  tmpfactXl[0] = factXl;
  tmpfactXl[1] = factXl;
  tmpfactXl[2] = factXl;
  tmpfXl[0] = 0;
  tmpfXl[1] = 0;
  tmpfXl[2] = 0;

  do // loop over components
  {
    F val = 0;
    int k = 0;
    do // loop over z
    {
      F ifz = fZ[k];
      svtkIdType factz = factZ[k];
      int j = 0;
      do // loop over y
      {
        F ify = fY[j];
        F fzy = ifz * ify;
        svtkIdType factzy = factz + factY[j];
        // loop over x
        const T* tmpPtr = inPtr + factzy;
        const F* tmpfX = fX;
        const svtkIdType* tmpfactX = factX;
        F tmpval = 0;
        // unroll inner loop for efficiency
        int l = lm;
        do
        {
          tmpval += tmpfX[0] * tmpPtr[tmpfactX[0]];
          tmpval += tmpfX[1] * tmpPtr[tmpfactX[1]];
          tmpval += tmpfX[2] * tmpPtr[tmpfactX[2]];
          tmpval += tmpfX[3] * tmpPtr[tmpfactX[3]];
          tmpfX += 4;
          tmpfactX += 4;
        } while (--l);
        val += fzy * tmpval;
      } while (++j <= ny);
    } while (++k <= nz);

    *outPtr++ = val;
    inPtr++;
  } while (--numscalars);
}

//----------------------------------------------------------------------------
// Get the interpolation function for the specified data types
template <class F>
void svtkImageBSplineInterpolatorGetInterpolationFunc(
  void (**interpolate)(svtkInterpolationInfo*, const F[3], F*), int dataType,
  int svtkNotUsed(interpolationMode))
{
  switch (dataType)
  {
    svtkTemplateAliasMacro(*interpolate = &(svtkImageBSplineInterpolate<F, SVTK_TT>::BSpline));
    default:
      *interpolate = nullptr;
  }
}

//----------------------------------------------------------------------------
// Interpolation for precomputed weights

template <class F, class T>
struct svtkImageBSplineRowInterpolate
{
  static void BSpline(
    svtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);
};

//--------------------------------------------------------------------------
// helper function for high-order interpolation
template <class F, class T>
void svtkImageBSplineRowInterpolate<F, T>::BSpline(
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
  const svtkIdType* factX = weights->Positions[0] + idX;
  const svtkIdType* factY = weights->Positions[1] + idY;
  const svtkIdType* factZ = weights->Positions[2] + idZ;
  const T* inPtr = static_cast<const T*>(weights->Pointer);

  // part of the loop unrolling scheme
  int lm = ((stepX + 3) >> 2);

  int numscalars = weights->NumberOfComponents;
  for (int i = n; i > 0; --i)
  {
    // allow unrolling of inner loop
    F fX1[SVTK_BSPLINE_KERNEL_SIZE_MAX + 4];
    svtkIdType factX1[SVTK_BSPLINE_KERNEL_SIZE_MAX + 4];
    F* tmpfX = fX1;
    svtkIdType* tmpfactX = factX1;
    int ii = stepX;
    do
    {
      *tmpfX++ = *fX++;
      *tmpfactX++ = *factX++;
    } while (--ii);
    svtkIdType lfactX = *(tmpfactX - 1);
    tmpfX[0] = 0.0;
    tmpfactX[0] = lfactX;
    tmpfX[1] = 0.0;
    tmpfactX[1] = lfactX;
    tmpfX[2] = 0.0;
    tmpfactX[2] = lfactX;

    const T* inPtr0 = inPtr;
    int c = numscalars;
    do // loop over components
    {
      F val = 0;
      int k = 0;
      do // loop over z
      {
        F ifz = fZ[k];
        svtkIdType factz = factZ[k];
        int j = 0;
        do // loop over y
        {
          F ify = fY[j];
          F fzy = ifz * ify;
          svtkIdType factzy = factz + factY[j];
          // loop over x
          const T* tmpPtr = inPtr0 + factzy;
          tmpfX = fX1;
          tmpfactX = factX1;
          F tmpval = 0;
          // unroll inner loop for efficiency
          int l = lm;
          do
          {
            tmpval += tmpfX[0] * tmpPtr[tmpfactX[0]];
            tmpval += tmpfX[1] * tmpPtr[tmpfactX[1]];
            tmpval += tmpfX[2] * tmpPtr[tmpfactX[2]];
            tmpval += tmpfX[3] * tmpPtr[tmpfactX[3]];
            tmpfX += 4;
            tmpfactX += 4;
          } while (--l);
          val += fzy * tmpval;
        } while (++j < stepY);
      } while (++k < stepZ);

      *outPtr++ = val;
      inPtr0++;
    } while (--c);
  }
}

//----------------------------------------------------------------------------
// get row interpolation function for different interpolation modes
// and different scalar types
template <class F>
void svtkImageBSplineInterpolatorGetRowInterpolationFunc(
  void (**summation)(svtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n),
  int scalarType, int svtkNotUsed(interpolationMode))
{
  switch (scalarType)
  {
    svtkTemplateAliasMacro(*summation = &(svtkImageBSplineRowInterpolate<F, SVTK_TT>::BSpline));
    default:
      *summation = nullptr;
  }
}

//----------------------------------------------------------------------------
template <class F>
void svtkImageBSplineInterpolatorPrecomputeWeights(const F newmat[16], const int outExt[6],
  int clipExt[6], const F bounds[6], svtkInterpolationWeights* weights)
{
#ifdef SVTK_BSPLINE_USE_KERNEL_TABLE
  float* kernel = static_cast<float*>(weights->ExtraInfo);
#endif
  weights->WeightType = svtkTypeTraits<F>::SVTKTypeID();
  int degree = weights->InterpolationMode;
  int m = degree + 1;

  // set up input positions table for interpolation
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
    int m2 = ((m - 1) >> 1);
    int step = m;
    int inCount = maxExt - minExt + 1;
    step = ((step < inCount) ? step : inCount);

    // allocate space for the weights
    svtkIdType size = step * (outExt[2 * j + 1] - outExt[2 * j] + 1);
    svtkIdType* positions = new svtkIdType[size];
    positions -= step * outExt[2 * j];
    F* constants = new F[size];
    constants -= step * outExt[2 * j];

    weights->KernelSize[j] = step;
    weights->Positions[j] = positions;
    weights->Weights[j] = constants;
    weights->WeightExtent[2 * j] = outExt[2 * j];
    weights->WeightExtent[2 * j + 1] = outExt[2 * j + 1];

    int region = 0;
    for (int i = outExt[2 * j]; i <= outExt[2 * j + 1]; i++)
    {
      F point = matrow[3] + i * matrow[j];

      // offset for odd-size kernels
      F offset = 0.5 * (m & 1);
      F f = 0;
      int idx = svtkInterpolationMath::Floor(point + offset, f);
      f -= offset;
      if (step > 1)
      {
        idx -= m2;
      }

      // compute the weights and offsets
      svtkIdType inInc = weights->Increments[k];
      if (inCount == 1)
      {
        positions[step * i] = 0;
        constants[step * i] = static_cast<F>(1);
      }
      else
      {
        // initialization is needed to avoid a warning for gcc 4.9.2, but
        // not for other compilers or for valgrind
        int inId[SVTK_BSPLINE_KERNEL_SIZE_MAX] = SVTK_BSPLINE_INT_INITIALIZER;

        int l = 0;
        switch (weights->BorderMode)
        {
          case SVTK_IMAGE_BORDER_REPEAT:
            do
            {
              inId[l] = svtkInterpolationMath::Wrap(idx++, minExt, maxExt);
            } while (++l < m);
            break;

          case SVTK_IMAGE_BORDER_MIRROR:
            do
            {
              inId[l] = svtkInterpolationMath::Mirror(idx++, minExt, maxExt);
            } while (++l < m);
            break;

          default:
            do
            {
              inId[l] = svtkInterpolationMath::Clamp(idx++, minExt, maxExt);
            } while (++l < m);
            break;
        }

        F g[SVTK_BSPLINE_KERNEL_SIZE_MAX];
#ifdef SVTK_BSPLINE_USE_KERNEL_TABLE
        svtkBSplineInterpWeights(kernel, g, f, m - 1);
#else
        svtkImageBSplineInternals::GetInterpolationWeights(g, f, m - 1);
#endif
        if (step == m)
        {
          int ll = 0;
          do
          {
            positions[step * i + ll] = inId[ll] * inInc;
            constants[step * i + ll] = g[ll];
          } while (++ll < step);
        }
        else
        {
          // it gets tricky if the data is thinner than the kernel
          F gg[SVTK_BSPLINE_KERNEL_SIZE_MAX];
          int ll = 0;
          do
          {
            gg[ll] = 0;
          } while (++ll < m);
          ll = 0;
          do
          {
            int rIdx = inId[ll];
            gg[rIdx] += g[ll];
          } while (++ll < m);
          ll = 0;
          do
          {
            positions[step * i + ll] = ll * inInc;
            constants[step * i + ll] = gg[ll];
          } while (++ll < step);
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
void svtkImageBSplineInterpolator::GetInterpolationFunc(
  void (**func)(svtkInterpolationInfo*, const double[3], double*))
{
  svtkImageBSplineInterpolatorGetInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->SplineDegree);
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::GetInterpolationFunc(
  void (**func)(svtkInterpolationInfo*, const float[3], float*))
{
  svtkImageBSplineInterpolatorGetInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->SplineDegree);
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::GetRowInterpolationFunc(
  void (**func)(svtkInterpolationWeights*, int, int, int, double*, int))
{
  svtkImageBSplineInterpolatorGetRowInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->SplineDegree);
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::GetRowInterpolationFunc(
  void (**func)(svtkInterpolationWeights*, int, int, int, float*, int))
{
  svtkImageBSplineInterpolatorGetRowInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->SplineDegree);
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::PrecomputeWeightsForExtent(
  const double matrix[16], const int extent[6], int newExtent[6], svtkInterpolationWeights*& weights)
{
  weights = new svtkInterpolationWeights(*this->InterpolationInfo);

  svtkImageBSplineInterpolatorPrecomputeWeights(
    matrix, extent, newExtent, this->StructuredBoundsDouble, weights);
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::PrecomputeWeightsForExtent(
  const float matrix[16], const int extent[6], int newExtent[6], svtkInterpolationWeights*& weights)
{
  weights = new svtkInterpolationWeights(*this->InterpolationInfo);

  svtkImageBSplineInterpolatorPrecomputeWeights(
    matrix, extent, newExtent, this->StructuredBoundsFloat, weights);
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::FreePrecomputedWeights(svtkInterpolationWeights*& weights)
{
  this->Superclass::FreePrecomputedWeights(weights);
}

//----------------------------------------------------------------------------
// build any tables required for the interpolation
void svtkImageBSplineInterpolator::BuildKernelLookupTable()
{
#ifdef SVTK_BSPLINE_USE_KERNEL_TABLE
  if (this->KernelLookupTable)
  {
    this->FreeKernelLookupTable();
  }

  float* kernel = 0;

  // kernel parameters
  int m = this->SplineDegree + 1;

  // compute lookup table size and step size
  int n = SVTK_BSPLINE_KERNEL_TABLE_DIVISIONS;
  int size = n * m / 2;

  // allocate a little extra space
  kernel = new float[size + 4];

  // compute the table
  svtkBSplineKernel::BSpline(kernel, n, m - 1);

  // pad with a few zeros
  for (int j = 0; j < 4; j++)
  {
    kernel[size + j] = 0.0;
  }

  this->KernelLookupTable = kernel;
#endif
}

//----------------------------------------------------------------------------
void svtkImageBSplineInterpolator::FreeKernelLookupTable()
{
#ifdef SVTK_BSPLINE_USE_KERNEL_TABLE
  float* kernel = this->KernelLookupTable;
  delete[] kernel;
#endif
}
