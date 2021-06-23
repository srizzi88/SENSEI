/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSurfaceLICComposite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSurfaceLICComposite.h"

#include "svtkObjectFactory.h"
#include "svtkPixelExtent.h"
#include "svtkPixelExtentIO.h"

#include <algorithm>

using std::deque;
using std::vector;

// Enable debug output
// 0 -- off
// 1 -- dump extents
// 2 -- all
#define svtkSurfaceLICCompositeDEBUG 0

//-----------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkSurfaceLICComposite);

// ----------------------------------------------------------------------------
svtkSurfaceLICComposite::svtkSurfaceLICComposite()
  : Pass(0)
  , WindowExt()
  , BlockExts()
  , CompositeExt()
  , GuardExt()
  , DisjointGuardExt()
  , Strategy(COMPOSITE_AUTO)
  , StepSize(0)
  , NumberOfSteps(0)
  , NormalizeVectors(1)
  , NumberOfGuardLevels(1)
{
}

// ----------------------------------------------------------------------------
svtkSurfaceLICComposite::~svtkSurfaceLICComposite() {}

// ----------------------------------------------------------------------------
void svtkSurfaceLICComposite::Initialize(const svtkPixelExtent& winExt,
  const deque<svtkPixelExtent>& blockExts, int strategy, double stepSize, int nSteps,
  int normalizeVectors, int enhancedLIC, int antialias)
{
  this->Pass = 0;
  this->WindowExt = winExt;
  this->BlockExts = blockExts;
  this->CompositeExt.clear();
  this->GuardExt.clear();
  this->DisjointGuardExt.clear();
  this->Strategy = strategy;
  this->StepSize = stepSize;
  this->NumberOfSteps = nSteps;
  this->NormalizeVectors = normalizeVectors;
  // TODO -- FIXME
  // type of NumberOfGuardLevels should be float. The change is
  // fairly involved and needs to be thoroughly tested. Note too
  // few guard pixels and you get an incorrect result, too many
  // and you destroy performance and scaling. while getting this
  // right the following will quiets dashboard warnings and keeps
  // the existing well tested behavior.
  this->NumberOfGuardLevels = 1;
  // this->NumberOfGuardLevels = enhancedLIC ? 1.5 : 1;
  this->NumberOfEEGuardPixels = enhancedLIC ? 1 : 0;
  this->NumberOfAAGuardPixels = 2 * antialias;
}

// ----------------------------------------------------------------------------
int svtkSurfaceLICComposite::VectorMax(
  const deque<svtkPixelExtent>& exts, float* vectors, vector<float>& vMax)
{
#if svtkSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkSurfaceLICComposite::VectorMax" << endl;
#endif

  // find the max on each extent
  size_t nBlocks = exts.size();
  vector<float> tmpMax(nBlocks, 0.0f);
  for (size_t b = 0; b < nBlocks; ++b)
  {
    tmpMax[b] = this->VectorMax(exts[b], vectors);
  }

  // use larger of this extent and its neighbors
  vMax.resize(nBlocks, 0.0f);
  for (size_t a = 0; a < nBlocks; ++a)
  {
    svtkPixelExtent extA = exts[a];
    extA.Grow(1);
    for (size_t b = 0; b < nBlocks; ++b)
    {
      svtkPixelExtent extB = exts[b];
      extB &= extA;

      // it's a neighbor(or self) use the larger of ours and theirs
      if (!extB.Empty())
      {
        vMax[a] = vMax[a] < tmpMax[b] ? tmpMax[b] : vMax[a];
      }
    }
  }

  return 0;
}

// ----------------------------------------------------------------------------
float svtkSurfaceLICComposite::VectorMax(const svtkPixelExtent& ext, float* vectors)
{
#if svtkSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkSurfaceLICComposite::VectorMax" << endl;
#endif

  int nx[2];
  this->WindowExt.Size(nx);

  // find the max over this region
  // scaling by 1/nx since that's what LIC'er does.
  float eMax = 0.0;
  for (int j = ext[2]; j <= ext[3]; ++j)
  {
    int idx = 4 * (nx[0] * j + ext[0]);
    for (int i = ext[0]; i <= ext[1]; ++i, idx += 4)
    {
      float eMag = 0.0;
      for (int c = 0; c < 2; ++c)
      {
        float eVec = vectors[idx + c] / static_cast<float>(nx[c]);
        eMag += eVec * eVec;
      }
      eMag = sqrt(eMag);
      eMax = eMax < eMag ? eMag : eMax;
    }
  }

  return eMax;
}

// ----------------------------------------------------------------------------
int svtkSurfaceLICComposite::MakeDecompDisjoint(
  const deque<svtkPixelExtent>& in, deque<svtkPixelExtent>& out, float* vectors)
{
#if svtkSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkSurfaceLICComposite::MakeDecompDisjoint" << endl;
#endif

  // serial implementation

  // sort by size
  deque<svtkPixelExtent> tmpIn(in);
  sort(tmpIn.begin(), tmpIn.end());

  // from largest to smallest, make it disjoint
  // to others. This order has the best chance of
  // leaving each rank with some data.
  deque<svtkPixelExtent> tmpOut0;

  this->MakeDecompDisjoint(tmpIn, tmpOut0);

  // minimize and remove empty extents.
  int nx[2];
  this->WindowExt.Size(nx);
  while (!tmpOut0.empty())
  {
    svtkPixelExtent outExt = tmpOut0.back();
    tmpOut0.pop_back();

    GetPixelBounds(vectors, nx[0], outExt);
    if (!outExt.Empty())
    {
      out.push_back(outExt);
    }
  }

  /*
  // merge compatible extents
  svtkPixelExtent::Merge(tmpOut0);
  */

  return 0;
}

// ----------------------------------------------------------------------------
int svtkSurfaceLICComposite::MakeDecompDisjoint(
  deque<svtkPixelExtent>& in, deque<svtkPixelExtent>& out)
{
  while (!in.empty())
  {
    // for each element
    deque<svtkPixelExtent> tmpOut(1, in.back());
    in.pop_back();

    // subtract other elements
    // to make it disjoint
    size_t ns = in.size();
    for (size_t se = 0; se < ns; ++se)
    {
      svtkPixelExtent& selem = in[se];
      deque<svtkPixelExtent> tmpOut2;
      size_t nl = tmpOut.size();
      for (size_t le = 0; le < nl; ++le)
      {
        svtkPixelExtent& lelem = tmpOut[le];
        svtkPixelExtent::Subtract(lelem, selem, tmpOut2);
      }
      tmpOut = tmpOut2;
    }

    // append new disjoint elements
    out.insert(out.end(), tmpOut.begin(), tmpOut.end());
  }

  return 0;
}

// TODO -- this is needed in part because our step size is incorrect
// due to anisotropic (in aspect ratio) trasnsform to texture
// space. see how we transform step size in surface lic painter.
// also there is bleeding at the edges so you do need a bit extra
// paddding.
// ----------------------------------------------------------------------------
float svtkSurfaceLICComposite::GetFudgeFactor(int nx[2])
{
  float aspect = float(nx[0]) / float(nx[1]);
  float fudge = (aspect > 4.0f) ? 3.0f
                                : (aspect > 1.0f)
      ? (2.0f / 3.0f) * aspect + (5.0f / 6.0f)
      : (aspect < 0.25) ? 3.0f : (aspect < 1.0f) ? (-8.0f / 3.0f) * aspect + (25.0f / 6.0f) : 1.5f;
  return fudge;
}

// ----------------------------------------------------------------------------
int svtkSurfaceLICComposite::AddGuardPixels(const deque<svtkPixelExtent>& exts,
  deque<svtkPixelExtent>& guardExts, deque<svtkPixelExtent>& disjointGuardExts, float* vectors)
{
#if svtkSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkSurfaceLICComposite::AddGuardPixles" << endl;
#endif

  int nx[2];
  this->WindowExt.Size(nx);
  float fudge = this->GetFudgeFactor(nx);
  float arc = this->StepSize * this->NumberOfSteps * this->NumberOfGuardLevels * fudge;

  if (this->NormalizeVectors)
  {
    // when normalizing velocity is always 1, all extents have the
    // same number of guard cells.
    int ng = static_cast<int>(arc) + this->NumberOfEEGuardPixels + this->NumberOfAAGuardPixels;
    ng = ng < 2 ? 2 : ng;
    // cerr << "ng=" << ng << endl;
    deque<svtkPixelExtent> tmpExts(exts);
    size_t nExts = tmpExts.size();
    // add guard pixels
    for (size_t b = 0; b < nExts; ++b)
    {
      tmpExts[b].Grow(ng);
      tmpExts[b] &= this->DataSetExt;
    }
    guardExts = tmpExts;
    // make sure it's disjoint
    disjointGuardExts.clear();
    this->MakeDecompDisjoint(tmpExts, disjointGuardExts);
  }
  else
  {
    // when not normailzing during integration we need max(V) on the LIC
    // decomp. Each domain has the potential to require a unique number
    // of guard cells.
    vector<float> vectorMax;
    this->VectorMax(exts, vectors, vectorMax);
    // cerr << "ng=";
    deque<svtkPixelExtent> tmpExts(exts);
    size_t nExts = tmpExts.size();
    // add guard pixels
    for (size_t b = 0; b < nExts; ++b)
    {
      int ng = static_cast<int>(
        vectorMax[b] * arc + this->NumberOfEEGuardPixels + this->NumberOfAAGuardPixels);
      ng = ng < 2 ? 2 : ng;
      // cerr << " " << ng;
      tmpExts[b].Grow(ng);
      tmpExts[b] &= this->DataSetExt;
    }
    guardExts = tmpExts;
    // cerr << endl;
    // make sure it's disjoint
    disjointGuardExts.clear();
    this->MakeDecompDisjoint(tmpExts, disjointGuardExts);
  }

  return 0;
}

// ----------------------------------------------------------------------------
void svtkSurfaceLICComposite::GetPixelBounds(float* rgba, int ni, svtkPixelExtent& ext)
{
  svtkPixelExtent text;
  for (int j = ext[2]; j <= ext[3]; ++j)
  {
    for (int i = ext[0]; i <= ext[1]; ++i)
    {
      if (rgba[4 * (j * ni + i) + 3] > 0.0f)
      {
        text[0] = text[0] > i ? i : text[0];
        text[1] = text[1] < i ? i : text[1];
        text[2] = text[2] > j ? j : text[2];
        text[3] = text[3] < j ? j : text[3];
      }
    }
  }
  ext = text;
}

// ----------------------------------------------------------------------------
int svtkSurfaceLICComposite::InitializeCompositeExtents(float* vectors)
{
  // determine screen bounds of all blocks
  size_t nBlocks = this->BlockExts.size();
  for (size_t b = 0; b < nBlocks; ++b)
  {
    this->DataSetExt |= this->BlockExts[b];
  }

  // Make all of the input block extents disjoint so that
  // LIC is computed once per pixel.
  this->MakeDecompDisjoint(this->BlockExts, this->CompositeExt, vectors);

  // add guard cells to the new decomp that prevent artifacts
  this->AddGuardPixels(this->CompositeExt, this->GuardExt, this->DisjointGuardExt, vectors);

#if svtkSurfaceLICCompositeDEBUG >= 1
  svtkPixelExtentIO::Write(0, "SerViewExtent.svtk", this->WindowExt);
  svtkPixelExtentIO::Write(0, "SerGeometryDecomp.svtk", this->BlockExts);
  svtkPixelExtentIO::Write(0, "SerLICDecomp.svtk", this->CompositeExt);
  svtkPixelExtentIO::Write(0, "SerLICDecompGuard.svtk", this->GuardExt);
  svtkPixelExtentIO::Write(0, "SerLICDecompDisjointGuard.svtk", this->DisjointGuardExt);
#endif

  return 0;
}

// ----------------------------------------------------------------------------
void svtkSurfaceLICComposite::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkObject::PrintSelf(os, indent);
  os << *this << endl;
}

// ****************************************************************************
ostream& operator<<(ostream& os, svtkSurfaceLICComposite& ss)
{
  os << "winExt=" << ss.WindowExt << endl;
  os << "blockExts=" << endl;
  size_t nExts = ss.BlockExts.size();
  for (size_t i = 0; i < nExts; ++i)
  {
    os << "  " << ss.BlockExts[i] << endl;
  }
  os << "compositeExts=" << endl;
  nExts = ss.CompositeExt.size();
  for (size_t i = 0; i < nExts; ++i)
  {
    os << ss.CompositeExt[i] << endl;
  }
  os << "guardExts=" << endl;
  for (size_t i = 0; i < nExts; ++i)
  {
    os << ss.GuardExt[i] << endl;
  }
  os << "disjointGuardExts=" << endl;
  for (size_t i = 0; i < nExts; ++i)
  {
    os << ss.DisjointGuardExt[i] << endl;
  }
  return os;
}
