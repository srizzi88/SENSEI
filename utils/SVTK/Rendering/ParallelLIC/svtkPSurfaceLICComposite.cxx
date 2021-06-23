/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPSurfaceLICComposite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPSurfaceLICComposite.h"

#include "svtkMPI.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPPainterCommunicator.h"
#include "svtkPPixelTransfer.h"
#include "svtkPainterCommunicator.h"
#include "svtkPixelBufferObject.h"
#include "svtkPixelExtent.h"
#include "svtkRenderWindow.h"
#include "svtkRenderbuffer.h"
#include "svtkRenderingOpenGLConfigure.h"
#include "svtkTextureObject.h"

#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLHelper.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkPSurfaceLICComposite_CompFS.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObjectVS.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <list>
#include <utility>
#include <vector>

using std::deque;
using std::list;
using std::pair;
using std::vector;

// use parallel timer for benchmarks and scaling
// if not defined svtkTimerLOG is used.
// #define svtkSurfaceLICPainterTIME
#if defined(svtkSurfaceLICPainterTIME)
#include "svtkParallelTimer.h"
#endif

// Enable debug output.
// 1 decomp extents, 2 +intermediate compositing steps
#define svtkPSurfaceLICCompositeDEBUG 0
#if svtkPSurfaceLICCompositeDEBUG >= 1
#include "svtkPixelExtentIO.h"
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
#include "svtkTextureIO.h"
#include <sstream>
using std::ostringstream;
using std::string;
//----------------------------------------------------------------------------
static string mpifn(int rank, const char* fn)
{
  ostringstream oss;
  oss << rank << "_" << fn;
  return oss.str();
}
#endif

// use PBO's for MPI communication.

#define PBO_RECV_BUFFERS

// isolate this class's communications.
// this is a non-scalable operation so
// only use it for debugging.

#define DUPLICATE_COMMUNICATOR 0

// ***************************************************************************
static int maxNumPasses()
{
  return 100;
}

// ***************************************************************************
static int encodeTag(int id, int tagBase)
{
  return maxNumPasses() * (id + 1) + tagBase;
}

// ***************************************************************************
static int decodeTag(int tag, int tagBase)
{
  return (tag - tagBase) / maxNumPasses() - 1;
}

// to sort rank/extent pairs by extent size
// ***************************************************************************
static bool operator<(const pair<int, svtkPixelExtent>& l, const pair<int, svtkPixelExtent>& r)
{
  return l.second < r.second;
}

// In Windows our callback must use the same calling convention
// as the MPI library. Currently this is only an issue with
// MS MPI which uses __stdcall/__fastcall other's use __cdecl
// which match SVTK's defaults.
#ifndef MPIAPI
#define MPIAPI
#endif
// for parallel union of extents
// ***************************************************************************
static void MPIAPI svtkPixelExtentUnion(void* in, void* out, int* len, MPI_Datatype* type)
{
  (void)type; // known to be MPI_INT
  int n = *len / 4;
  for (int i = 0; i < n; ++i)
  {
    int ii = 4 * i;
    svtkPixelExtent lhs(((int*)in) + ii);
    svtkPixelExtent rhs(((int*)out) + ii);
    rhs |= lhs;
    rhs.GetData(((int*)out) + ii);
  }
}

// Description:
// Container for our custom MPI_Op's
class svtkPPixelExtentOps
{
public:
  svtkPPixelExtentOps()
    : Union(MPI_OP_NULL)
  {
  }
  ~svtkPPixelExtentOps();

  // Description:
  // Create/Delete the custom operations. If these
  // methods are used before MPI initialize or after
  // MPI finalize they have no affect.
  void CreateOps();
  void DeleteOps();

  // Description:
  // Get the operator for performing parallel
  // unions.
  MPI_Op GetUnion() { return this->Union; }

private:
  MPI_Op Union;
};

// ---------------------------------------------------------------------------
svtkPPixelExtentOps::~svtkPPixelExtentOps()
{
  this->DeleteOps();
}

// ---------------------------------------------------------------------------
void svtkPPixelExtentOps::CreateOps()
{
  if ((this->Union == MPI_OP_NULL) && svtkPPainterCommunicator::MPIInitialized())
  {
    MPI_Op_create(svtkPixelExtentUnion, 1, &this->Union);
  }
}

// ---------------------------------------------------------------------------
void svtkPPixelExtentOps::DeleteOps()
{
  if ((this->Union != MPI_OP_NULL) && svtkPPainterCommunicator::MPIInitialized() &&
    !svtkPPainterCommunicator::MPIFinalized())
  {
    MPI_Op_free(&this->Union);
  }
}

// ****************************************************************************
void MPITypeFree(deque<MPI_Datatype>& types)
{
  size_t n = types.size();
  for (size_t i = 0; i < n; ++i)
  {
    MPI_Type_free(&types[i]);
  }
}

// ****************************************************************************
static size_t Size(deque<deque<svtkPixelExtent> > exts)
{
  size_t np = 0;
  size_t nr = exts.size();
  for (size_t r = 0; r < nr; ++r)
  {
    const deque<svtkPixelExtent>& rexts = exts[r];
    size_t ne = rexts.size();
    for (size_t e = 0; e < ne; ++e)
    {
      np += rexts[e].Size();
    }
  }
  return np;
}

#if svtkPSurfaceLICCompositeDEBUG >= 1 || defined(svtkSurfaceLICPainterTIME)
// ****************************************************************************
static int NumberOfExtents(deque<deque<svtkPixelExtent> > exts)
{
  size_t ne = 0;
  size_t nr = exts.size();
  for (size_t r = 0; r < nr; ++r)
  {
    ne += exts[r].size();
  }
  return static_cast<int>(ne);
}
#endif

#if svtkPSurfaceLICCompositeDEBUG > 0
// ****************************************************************************
static ostream& operator<<(ostream& os, const vector<float>& vf)
{
  size_t n = vf.size();
  if (n)
  {
    os << vf[0];
  }
  for (size_t i = 1; i < n; ++i)
  {
    os << ", " << vf[i];
  }
  return os;
}

// ****************************************************************************
static ostream& operator<<(ostream& os, const vector<vector<float> >& vvf)
{
  size_t n = vvf.size();
  for (size_t i = 0; i < n; ++i)
  {
    os << i << " = {" << vvf[i] << "}" << endl;
  }
  return os;
}
#endif

#if svtkPSurfaceLICCompositeDEBUG >= 2
// ****************************************************************************
static int ScanMPIStatusForError(vector<MPI_Status>& stat)
{
  int nStats = stat.size();
  for (int q = 0; q < nStats; ++q)
  {
    int ierr = stat[q].MPI_ERROR;
    if ((ierr != MPI_SUCCESS) && (ierr != MPI_ERR_PENDING))
    {
      char eStr[MPI_MAX_ERROR_STRING] = { '\0' };
      int eStrLen = 0;
      MPI_Error_string(ierr, eStr, &eStrLen);
      cerr << "transaction for request " << q << " failed." << endl << eStr << endl << endl;
      return -1;
    }
  }
  return 0;
}
#endif

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPSurfaceLICComposite);

// ----------------------------------------------------------------------------
svtkPSurfaceLICComposite::svtkPSurfaceLICComposite()
  : svtkSurfaceLICComposite()
  , PainterComm(nullptr)
  , PixelOps(nullptr)
  , CommRank(0)
  , CommSize(1)
  , Context(nullptr)
  , FBO(nullptr)
  , CompositeShader(nullptr)
{
  this->PainterComm = new svtkPPainterCommunicator;
  this->PixelOps = new svtkPPixelExtentOps;
}

// ----------------------------------------------------------------------------
svtkPSurfaceLICComposite::~svtkPSurfaceLICComposite()
{
  delete this->PainterComm;
  delete this->PixelOps;
  if (this->CompositeShader)
  {
    delete this->CompositeShader;
    this->CompositeShader = 0;
  }
  if (this->FBO)
  {
    this->FBO->Delete();
  }
}

// ----------------------------------------------------------------------------
void svtkPSurfaceLICComposite::SetCommunicator(svtkPainterCommunicator* comm)
{
#if DUPLICATE_COMMUNICATOR
  this->PainterComm->Duplicate(comm);
#else
  this->PainterComm->Copy(comm, false);
#endif
  this->CommRank = this->PainterComm->GetRank();
  this->CommSize = this->PainterComm->GetSize();
  // do this here since we know that
  // mpi is initialized by now.
  this->PixelOps->CreateOps();
}

// ----------------------------------------------------------------------------
void svtkPSurfaceLICComposite::SetContext(svtkOpenGLRenderWindow* rwin)
{
  if (this->Context == rwin)
  {
    return;
  }
  this->Context = rwin;

  // free the existing shader and fbo
  if (this->CompositeShader)
  {
    this->CompositeShader->ReleaseGraphicsResources(rwin);
    delete this->CompositeShader;
    this->CompositeShader = nullptr;
  }

  if (this->FBO)
  {
    this->FBO->Delete();
    this->FBO = nullptr;
  }

  if (this->Context)
  {
    // load, compile, and link the shader
    this->CompositeShader = new svtkOpenGLHelper;
    std::string GSSource;
    this->CompositeShader->Program = rwin->GetShaderCache()->ReadyShaderProgram(
      svtkTextureObjectVS, svtkPSurfaceLICComposite_CompFS, GSSource.c_str());

    // setup a FBO for rendering
    this->FBO = svtkOpenGLFramebufferObject::New();

    this->FBO->SetContext(this->Context);
  }
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::AllGatherExtents(const deque<svtkPixelExtent>& localExts,
  deque<deque<svtkPixelExtent> >& remoteExts, svtkPixelExtent& dataSetExt)
{
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkPSurfaceLICComposite::AllGatherExtents" << endl;
#endif

  // serialize the local extents
  int nLocal = static_cast<int>(localExts.size());
  int localSize = 4 * nLocal;
  int* sendBuf = static_cast<int*>(malloc(localSize * sizeof(int)));
  for (int i = 0; i < nLocal; ++i)
  {
    localExts[i].GetData(sendBuf + 4 * i);
  }

  // share local extent counts
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  int* nRemote = static_cast<int*>(malloc(this->CommSize * sizeof(int)));

  MPI_Allgather(&nLocal, 1, MPI_INT, nRemote, 1, MPI_INT, comm);

  // allocate a buffer to receive the remote exts
  int* recvCounts = static_cast<int*>(malloc(this->CommSize * sizeof(int)));
  int* recvDispls = static_cast<int*>(malloc(this->CommSize * sizeof(int)));
  int bufSize = 0;
  for (int i = 0; i < this->CommSize; ++i)
  {
    int n = 4 * nRemote[i];
    recvCounts[i] = n;
    recvDispls[i] = bufSize;
    bufSize += n;
  }
  int* recvBuf = static_cast<int*>(malloc(bufSize * sizeof(int)));

  // collect remote extents
  MPI_Allgatherv(sendBuf, localSize, MPI_INT, recvBuf, recvCounts, recvDispls, MPI_INT, comm);

  // de-serialize the set of extents
  dataSetExt.Clear();
  remoteExts.resize(this->CommSize);
  for (int i = 0; i < this->CommSize; ++i)
  {
    int nRemt = recvCounts[i] / 4;
    remoteExts[i].resize(nRemt);

    int* pBuf = recvBuf + recvDispls[i];

    for (int j = 0; j < nRemt; ++j)
    {
      svtkPixelExtent& remoteExt = remoteExts[i][j];
      remoteExt.SetData(pBuf + 4 * j);
      dataSetExt |= remoteExt;
    }
  }

  free(sendBuf);
  free(nRemote);
  free(recvCounts);
  free(recvDispls);
  free(recvBuf);

  return 0;
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::AllReduceVectorMax(
  const deque<svtkPixelExtent>& originalExts,    // local data
  const deque<deque<svtkPixelExtent> >& newExts, // all composited regions
  float* vectors, vector<vector<float> >& vectorMax)
{
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkPSurfaceLICComposite::AllReduceVectorMax" << endl;
#endif

  // vector data is currently on the original decomp (m blocks for n ranks)
  // the new decomp (p blocks for n ranks), for each of the p new blocks
  // each rank computes the max on this region, a reduction is made to get
  // the true value.
  size_t nOriginal = originalExts.size();
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  vector<vector<float> > tmpMax(this->CommSize);
  for (int r = 0; r < this->CommSize; ++r)
  {
    // check the intersection of each new extent with that of each
    // original extent. data for origial extent is local.
    size_t nNew = newExts[r].size();
    tmpMax[r].resize(nNew, -SVTK_FLOAT_MAX);
    for (size_t n = 0; n < nNew; ++n)
    {
      const svtkPixelExtent& newExt = newExts[r][n];
      float eMax = -SVTK_FLOAT_MAX;
      for (size_t o = 0; o < nOriginal; ++o)
      {
        svtkPixelExtent intExt(originalExts[o]);
        intExt &= newExt;
        if (!intExt.Empty())
        {
          float oMax = this->VectorMax(intExt, vectors);
          eMax = eMax < oMax ? oMax : eMax;
        }
      }

      MPI_Allreduce(MPI_IN_PLACE, &eMax, 1, MPI_FLOAT, MPI_MAX, comm);

      tmpMax[r][n] = eMax;
    }
  }

  // since integration run's into other blocks data use the max of the
  // block and it's neighbors for guard cell size computation
  vectorMax.resize(this->CommSize);
  for (int r = 0; r < this->CommSize; ++r)
  {
    size_t nNew = newExts[r].size();
    vectorMax[r].resize(nNew);
    for (size_t n = 0; n < nNew; ++n)
    {
      svtkPixelExtent newExt = newExts[r][n];
      newExt.Grow(1);

      float eMax = tmpMax[r][n];

      // find neighbors
      for (int R = 0; R < this->CommSize; ++R)
      {
        size_t NNew = newExts[R].size();
        for (size_t N = 0; N < NNew; ++N)
        {
          svtkPixelExtent intExt(newExts[R][N]);
          intExt &= newExt;

          if (!intExt.Empty())
          {
            // this is a neighbor(or self), take the larger of ours
            // and theirs
            float nMax = tmpMax[R][N];
            eMax = eMax < nMax ? nMax : eMax;
          }
        }
      }

      vectorMax[r][n] = eMax;
    }
  }

  return 0;
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::DecomposeExtent(
  svtkPixelExtent& in, int nPieces, list<svtkPixelExtent>& out)
{
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkPSurfaceLICComposite::DecomposeWindowExtent" << endl;
#endif

  int res[3];
  in.Size(res);

  int nPasses[2] = { 0, 0 };
  int maxPasses[2] = { res[0] / 2, res[1] / 2 };

  out.push_back(in);

  list<svtkPixelExtent> splitExts;

  int dir = 0;
  while (1)
  {
    // stop when we have enough out or all out have unit size
    int nExts = static_cast<int>(out.size());
    if ((nExts >= nPieces) || ((nPasses[0] > maxPasses[0]) && (nPasses[1] > maxPasses[1])))
    {
      break;
    }

    for (int i = 0; i < nExts; ++i)
    {
      int nExtsTotal = static_cast<int>(out.size() + splitExts.size());
      if (nExtsTotal >= nPieces)
      {
        break;
      }

      // split this ext into two
      svtkPixelExtent ext = out.back();
      out.pop_back();

      svtkPixelExtent newExt = ext.Split(dir);

      splitExts.push_back(ext);

      if (!newExt.Empty())
      {
        splitExts.push_back(newExt);
      }
    }

    // transfer the split out to the head so that
    // they are split again only after others.
    out.insert(out.begin(), splitExts.begin(), splitExts.end());
    splitExts.clear();

    nPasses[dir] += 1;

    // alternate splitting direction
    dir = (dir + 1) % 2;
    if (nPasses[dir] > maxPasses[dir])
    {
      dir = (dir + 1) % 2;
    }
  }

  return 0;
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::DecomposeScreenExtent(
  deque<deque<svtkPixelExtent> >& newExts, float* vectors)
{
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkPSurfaceLICComposite::DecomposeWindowExtent" << endl;
#endif

  // TODO -- the balanced compositor is not finished. details
  // below.
  (void)vectors;

  // use 128x128 extents
  int dataSetSize[2];
  this->DataSetExt.Size(dataSetSize);

  int ni = dataSetSize[0] / 128;
  ni = ni < 1 ? 1 : ni;

  int nj = dataSetSize[1] / 128;
  nj = nj < 1 ? 1 : nj;

  int nPieces = ni * nj;
  nPieces = nPieces < this->CommSize ? this->CommSize : nPieces;

  // decompose
  list<svtkPixelExtent> tmpOut0;
  this->DecomposeExtent(this->DataSetExt, nPieces, tmpOut0);

  // make the assignment to ranks
  int nPer = nPieces / this->CommSize;
  int nLarge = nPieces % this->CommSize;

  deque<deque<svtkPixelExtent> > tmpOut1;
  tmpOut1.resize(this->CommSize);

  int N = static_cast<int>(tmpOut0.size());
  list<svtkPixelExtent>::iterator it = tmpOut0.begin();

  for (int r = 0; r < this->CommSize; ++r)
  {
    int n = nPer;
    if (r < nLarge)
    {
      ++n;
    }
    for (int i = 0; (i < n) && (N > 0); ++i, --N, ++it)
    {
      tmpOut1[r].push_back(*it);
    }
  }

  // TODO -- we need to implement some sore of load
  // balancing here.
  // compute tight extents and assign to ranks based on weight
  // and location
  newExts = tmpOut1;

  return 0;
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::MakeDecompLocallyDisjoint(
  const deque<deque<svtkPixelExtent> >& in, deque<deque<svtkPixelExtent> >& out)
{
  size_t nr = in.size();
  out.clear();
  out.resize(nr);
  for (size_t r = 0; r < nr; ++r)
  {
    deque<svtkPixelExtent> tmp(in[r]);
    this->MakeDecompDisjoint(tmp, out[r]);
  }
  return 0;
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::MakeDecompDisjoint(
  const deque<deque<svtkPixelExtent> >& in, deque<deque<svtkPixelExtent> >& out, float* vectors)
{
  // flatten
  deque<pair<int, svtkPixelExtent> > tmpIn;
  for (int r = 0; r < this->CommSize; ++r)
  {
    const deque<svtkPixelExtent>& blocks = in[r];
    size_t nBlocks = blocks.size();
    for (size_t b = 0; b < nBlocks; ++b)
    {
      pair<int, svtkPixelExtent> elem(r, blocks[b]);
      tmpIn.push_back(elem);
    }
  }
  // sort by size
  sort(tmpIn.begin(), tmpIn.end());

  // from largest to smallest, make it disjoint
  // to others
  deque<pair<int, svtkPixelExtent> > tmpOut0;

  while (!tmpIn.empty())
  {
    // largest element
    int rank = tmpIn.back().first;
    deque<svtkPixelExtent> tmpOut1(1, tmpIn.back().second);

    tmpIn.pop_back();

    // subtract smaller elements
    size_t ns = tmpIn.size();
    for (size_t se = 0; se < ns; ++se)
    {
      svtkPixelExtent& selem = tmpIn[se].second;
      deque<svtkPixelExtent> tmpOut2;
      size_t nl = tmpOut1.size();
      for (size_t le = 0; le < nl; ++le)
      {
        svtkPixelExtent& lelem = tmpOut1[le];
        svtkPixelExtent::Subtract(lelem, selem, tmpOut2);
      }
      tmpOut1 = tmpOut2;
    }

    // move to output
    size_t nn = tmpOut1.size();
    for (size_t ne = 0; ne < nn; ++ne)
    {
      pair<int, svtkPixelExtent> elem(rank, tmpOut1[ne]);
      tmpOut0.push_back(elem);
    }
  }

  // reduce communication and compositing overhead by
  // shrinking the new set of extents to tightly bound the
  // data on it's new/future layout.
  int nx[2];
  this->WindowExt.Size(nx);

  const deque<svtkPixelExtent>& inR = in[this->CommRank];
  size_t ni = inR.size();

  deque<pair<int, svtkPixelExtent> > tmpOut1(tmpOut0);
  size_t ne = tmpOut1.size();
  for (size_t e = 0; e < ne; ++e)
  {
    svtkPixelExtent& newExt = tmpOut1[e].second;
    svtkPixelExtent tightExt;
    for (size_t i = 0; i < ni; ++i)
    {
      svtkPixelExtent inExt(inR[i]);
      inExt &= newExt;
      if (!inExt.Empty())
      {
        GetPixelBounds(vectors, nx[0], inExt);
        tightExt |= inExt; // accumulate the contrib from local data
      }
    }
    newExt = tightExt;
  }

  // accumulate contrib from remote data
  size_t remSize = 4 * ne;
  vector<int> rem(remSize);
  int* pRem = ne ? &rem[0] : nullptr;
  for (size_t e = 0; e < ne; ++e, pRem += 4)
  {
    tmpOut1[e].second.GetData(pRem);
  }
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  MPI_Op parUnion = this->PixelOps->GetUnion();
  MPI_Allreduce(MPI_IN_PLACE, ne ? &rem[0] : nullptr, (int)remSize, MPI_INT, parUnion, comm);

  // move from flat order back to rank indexed order and remove
  // empty extents
  pRem = ne ? &rem[0] : nullptr;
  out.resize(this->CommSize);
  for (size_t e = 0; e < ne; ++e, pRem += 4)
  {
    int r = tmpOut1[e].first;
    svtkPixelExtent ext(pRem);
    if (!ext.Empty())
    {
      out[r].push_back(ext);
    }
  }

  // merge compatible extents
  for (int r = 0; r < this->CommSize; ++r)
  {
    svtkPixelExtent::Merge(out[r]);
  }

  return 0;
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::AddGuardPixels(const deque<deque<svtkPixelExtent> >& exts,
  deque<deque<svtkPixelExtent> >& guardExts, deque<deque<svtkPixelExtent> >& disjointGuardExts,
  float* vectors)
{
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkPSurfaceLICComposite::AddGuardPixels" << endl;
#endif
#ifdef svtkSurfaceLICPainterTIME
  svtkParallelTimer* log = svtkParallelTimer::GetGlobalInstance();
#endif

  guardExts.resize(this->CommSize);
  disjointGuardExts.resize(this->CommSize);

  int nx[2];
  this->WindowExt.Size(nx);
  float fudge = this->GetFudgeFactor(nx);
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << " fudge=" << fudge << endl;
#endif

  float arc = this->StepSize * this->NumberOfSteps * this->NumberOfGuardLevels * fudge;

  if (this->NormalizeVectors)
  {
    // when normalizing velocity is always 1, all extents have the
    // same number of guard cells.
    int ng = static_cast<int>(arc) + this->NumberOfEEGuardPixels + this->NumberOfAAGuardPixels;
    ng = ng < 2 ? 2 : ng;
#ifdef svtkSurfaceLICPainterTIME
    log->GetHeader() << "ng=" << ng << "\n";
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
    cerr << "ng=" << ng << endl;
#endif
    for (int r = 0; r < this->CommSize; ++r)
    {
      deque<svtkPixelExtent> tmpExts(exts[r]);
      int nExts = static_cast<int>(tmpExts.size());
      // add guard pixles
      for (int b = 0; b < nExts; ++b)
      {
        tmpExts[b].Grow(ng);
        tmpExts[b] &= this->DataSetExt;
      }
      guardExts[r] = tmpExts;
      // make sure it's disjoint
      disjointGuardExts[r].clear();
      this->MakeDecompDisjoint(tmpExts, disjointGuardExts[r]);
    }
  }
  else
  {
    // when not normailzing during integration we need max(V) on the LIC
    // decomp. Each domain has the potential to require a unique number
    // of guard cells.
    vector<vector<float> > vectorMax;
    this->AllReduceVectorMax(this->BlockExts, exts, vectors, vectorMax);

#ifdef svtkSurfaceLICPainterTIME
    log->GetHeader() << "ng=";
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
    cerr << "ng=";
#endif
    for (int r = 0; r < this->CommSize; ++r)
    {
      deque<svtkPixelExtent> tmpExts(exts[r]);
      size_t nExts = tmpExts.size();
      for (size_t b = 0; b < nExts; ++b)
      {
        int ng = static_cast<int>(vectorMax[r][b] * arc) + this->NumberOfEEGuardPixels +
          this->NumberOfAAGuardPixels;
        ng = ng < 2 ? 2 : ng;
#ifdef svtkSurfaceLICPainterTIME
        log->GetHeader() << " " << ng;
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
        cerr << "  " << ng;
#endif
        tmpExts[b].Grow(ng);
        tmpExts[b] &= this->DataSetExt;
      }
      guardExts[r] = tmpExts;
      // make sure it's disjoint
      disjointGuardExts[r].clear();
      this->MakeDecompDisjoint(tmpExts, disjointGuardExts[r]);
    }
#ifdef svtkSurfaceLICPainterTIME
    log->GetHeader() << "\n";
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
    cerr << endl;
#endif
  }

  return 0;
}

// ----------------------------------------------------------------------------
double svtkPSurfaceLICComposite::EstimateCommunicationCost(
  const deque<deque<svtkPixelExtent> >& srcExts, const deque<deque<svtkPixelExtent> >& destExts)
{
  // compute the number off rank overlapping pixels, this is the
  // the number of pixels that need to be communicated. This is
  // not the number of pixels to be composited since some of those
  // may be on-rank.

  size_t total = 0;
  size_t overlap = 0;

  for (int sr = 0; sr < this->CommSize; ++sr)
  {
    size_t nse = srcExts[sr].size();
    for (size_t se = 0; se < nse; ++se)
    {
      const svtkPixelExtent& srcExt = srcExts[sr][se];
      total += srcExt.Size(); // count all pixels in the total

      for (int dr = 0; dr < this->CommSize; ++dr)
      {
        // only off rank overlap incurrs comm cost
        if (sr == dr)
        {
          continue;
        }

        size_t nde = destExts[dr].size();
        for (size_t de = 0; de < nde; ++de)
        {
          svtkPixelExtent destExt = destExts[dr][de];
          destExt &= srcExt;
          if (!destExt.Empty())
          {
            overlap += destExt.Size(); // cost is number of overlap pixels
          }
        }
      }
    }
  }

  return (static_cast<double>(overlap)) / (static_cast<double>(total));
}

// ----------------------------------------------------------------------------
double svtkPSurfaceLICComposite::EstimateDecompEfficiency(
  const deque<deque<svtkPixelExtent> >& exts, const deque<deque<svtkPixelExtent> >& guardExts)
{
  // number of pixels in the domain decomp
  double ne = static_cast<double>(Size(exts));
  double nge = static_cast<double>(Size(guardExts));

  // efficiency is the ratio of valid pixels
  // to guard pixels
  return ne / fabs(ne - nge);
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::BuildProgram(float* vectors)
{
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkPSurfaceLICComposite::BuildProgram" << endl;
#endif

#ifdef svtkSurfaceLICPainterTIME
  svtkParallelTimer* log = svtkParallelTimer::GetGlobalInstance();
#endif

  // gather current geometry extents, compute the whole extent
  deque<deque<svtkPixelExtent> > allBlockExts;
  this->AllGatherExtents(this->BlockExts, allBlockExts, this->DataSetExt);

  if (this->Strategy == COMPOSITE_AUTO)
  {
    double commCost = this->EstimateCommunicationCost(allBlockExts, allBlockExts);
#ifdef svtkSurfaceLICPainterTIME
    log->GetHeader() << "in-place comm cost=" << commCost << "\n";
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
    cerr << "in-place comm cost=" << commCost << endl;
#endif
    if (commCost <= 0.3)
    {
      this->Strategy = COMPOSITE_INPLACE;
#ifdef svtkSurfaceLICPainterTIME
      log->GetHeader() << "using in-place composite\n";
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
      cerr << "using in-place composite" << endl;
#endif
    }
    else
    {
      this->Strategy = COMPOSITE_INPLACE_DISJOINT;
#ifdef svtkSurfaceLICPainterTIME
      log->GetHeader() << "using disjoint composite\n";
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
      cerr << "using disjoint composite" << endl;
#endif
    }
  }

  // decompose the screen
  deque<deque<svtkPixelExtent> > newExts;
  switch (this->Strategy)
  {
    case COMPOSITE_INPLACE:
      // make it locally disjoint to avoid redundant computation
      this->MakeDecompLocallyDisjoint(allBlockExts, newExts);
      break;

    case COMPOSITE_INPLACE_DISJOINT:
      this->MakeDecompDisjoint(allBlockExts, newExts, vectors);
      break;

    case COMPOSITE_BALANCED:
      this->DecomposeScreenExtent(newExts, vectors);
      break;

    default:
      return -1;
  }

#if defined(svtkSurfaceLICPainterTIME) || svtkPSurfaceLICCompositeDEBUG >= 2
  double commCost = this->EstimateCommunicationCost(allBlockExts, newExts);
#endif
#ifdef svtkSurfaceLICPainterTIME
  log->GetHeader() << "actual comm cost=" << commCost << "\n";
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "actual comm cost=" << commCost << endl;
#endif

  // save the local decomp
  // it's the valid region as no guard pixels were added
  this->CompositeExt = newExts[this->CommRank];

  int id = 0;
  this->ScatterProgram.clear();
  if (this->Strategy != COMPOSITE_INPLACE)
  {
    // construct program describing communication patterns that are
    // required to move data to geometry decomp from the new lic
    // decomp after LIC
    for (int srcRank = 0; srcRank < this->CommSize; ++srcRank)
    {
      deque<svtkPixelExtent>& srcBlocks = newExts[srcRank];
      int nSrcBlocks = static_cast<int>(srcBlocks.size());

      for (int sb = 0; sb < nSrcBlocks; ++sb)
      {
        const svtkPixelExtent& srcExt = srcBlocks[sb];

        for (int destRank = 0; destRank < this->CommSize; ++destRank)
        {
          int nBlocks = static_cast<int>(allBlockExts[destRank].size());
          for (int b = 0; b < nBlocks; ++b)
          {
            const svtkPixelExtent& destExt = allBlockExts[destRank][b];

            svtkPixelExtent sharedExt(destExt);
            sharedExt &= srcExt;

            if (!sharedExt.Empty())
            {
              this->ScatterProgram.push_back(svtkPPixelTransfer(
                srcRank, this->WindowExt, sharedExt, destRank, this->WindowExt, sharedExt, id));
            }
            id += 1;
          }
        }
      }
    }
  }

#if svtkPSurfaceLICCompositeDEBUG >= 1
  svtkPixelExtentIO::Write(this->CommRank, "ViewExtent.svtk", this->WindowExt);
  svtkPixelExtentIO::Write(this->CommRank, "GeometryDecomp.svtk", allBlockExts);
  svtkPixelExtentIO::Write(this->CommRank, "LICDecomp.svtk", newExts);
#endif

  // add guard cells to the new decomp that prevent artifacts
  deque<deque<svtkPixelExtent> > guardExts;
  deque<deque<svtkPixelExtent> > disjointGuardExts;
  this->AddGuardPixels(newExts, guardExts, disjointGuardExts, vectors);

#if svtkPSurfaceLICCompositeDEBUG >= 1
  svtkPixelExtentIO::Write(this->CommRank, "LICDecompGuard.svtk", guardExts);
  svtkPixelExtentIO::Write(this->CommRank, "LICDisjointDecompGuard.svtk", disjointGuardExts);
#endif

#if defined(svtkSurfaceLICPainterTIME) || svtkPSurfaceLICCompositeDEBUG >= 2
  double efficiency = this->EstimateDecompEfficiency(newExts, disjointGuardExts);
  size_t nNewExts = NumberOfExtents(newExts);
#endif
#if defined(svtkSurfaceLICPainterTIME)
  log->GetHeader() << "decompEfficiency=" << efficiency << "\n"
                   << "numberOfExtents=" << nNewExts << "\n";
#endif
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "decompEfficiency=" << efficiency << endl << "numberOfExtents=" << nNewExts << endl;
#endif

  // save the local decomp with guard cells
  this->GuardExt = guardExts[this->CommRank];
  this->DisjointGuardExt = disjointGuardExts[this->CommRank];

  // construct program describing communication patterns that are
  // required to move data from the geometry decomp to the new
  // disjoint decomp containing guard pixels
  this->GatherProgram.clear();
  id = 0;
  for (int destRank = 0; destRank < this->CommSize; ++destRank)
  {
    deque<svtkPixelExtent>& destBlocks = disjointGuardExts[destRank];
    int nDestBlocks = static_cast<int>(destBlocks.size());

    for (int db = 0; db < nDestBlocks; ++db)
    {
      const svtkPixelExtent& destExt = destBlocks[db];

      for (int srcRank = 0; srcRank < this->CommSize; ++srcRank)
      {
        int nBlocks = static_cast<int>(allBlockExts[srcRank].size());
        for (int b = 0; b < nBlocks; ++b)
        {
          const svtkPixelExtent& srcExt = allBlockExts[srcRank][b];

          svtkPixelExtent sharedExt(destExt);
          sharedExt &= srcExt;

          if (!sharedExt.Empty())
          {
            // to move vectors for the LIC decomp
            // into a contiguous recv buffer
            this->GatherProgram.push_back(
              svtkPPixelTransfer(srcRank, this->WindowExt, sharedExt, destRank,
                sharedExt, // dest ext
                sharedExt, id));
          }

          id += 1;
        }
      }
    }
  }

#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << *this << endl;
#endif

  return 0;
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::Gather(
  void* pSendPBO, int dataType, int nComps, svtkTextureObject*& newImage)
{
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkPSurfaceLICComposite::Composite" << endl;
#endif

  // two pipleines depending on if this process recv's or send's
  //
  // send:
  // tex -> pbo -> mpi_send
  //
  // recv:
  // mpi_recv -> pbo -> tex -> composite shader -> fbo

  // pass id is decoded into mpi tag form non-blocking comm
  this->Pass += 1;

  // validate inputs
  if (this->Pass >= maxNumPasses())
  {
    return -1;
  }
  if (pSendPBO == nullptr)
  {
    return -2;
  }
  if (this->Context == nullptr)
  {
    return -3;
  }
  if (this->CompositeShader == nullptr)
  {
    return -4;
  }

  // get the size of the array datatype
  int dataTypeSize = 0;
  switch (dataType)
  {
    svtkTemplateMacro(dataTypeSize = sizeof(SVTK_TT););
    default:
      return -5;
  }

  // initiate non-blocking comm
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  int nTransactions = static_cast<int>(this->GatherProgram.size());
  vector<MPI_Request> mpiRecvReqs;
  vector<MPI_Request> mpiSendReqs;
  deque<MPI_Datatype> mpiTypes;
#ifdef PBO_RECV_BUFFERS
  deque<svtkPixelBufferObject*> recvPBOs(nTransactions, static_cast<svtkPixelBufferObject*>(nullptr));
#else
  deque<void*> recvBufs(nTransactions, static_cast<void*>(nullptr));
#endif
  for (int j = 0; j < nTransactions; ++j)
  {
    svtkPPixelTransfer& transaction = this->GatherProgram[j];

    // postpone local transactions, they will be overlapped
    // with transactions requiring communication
    if (transaction.Local(this->CommRank))
    {
      continue;
    }

#ifdef PBO_RECV_BUFFERS
    void* pRecvPBO = nullptr;
#endif

    // encode transaction.
    int tag = encodeTag(j, this->Pass);

    if (transaction.Receiver(this->CommRank))
    {
      // allocate receive buffers
      const svtkPixelExtent& destExt = transaction.GetDestinationExtent();

      unsigned int pboSize = static_cast<unsigned int>(destExt.Size() * nComps);
      unsigned int bufSize = pboSize * dataTypeSize;

#ifdef PBO_RECV_BUFFERS
      svtkPixelBufferObject* pbo;
      pbo = svtkPixelBufferObject::New();
      pbo->SetContext(this->Context);
      pbo->SetType(dataType);
      pbo->SetComponents(nComps);
      pbo->SetSize(pboSize);
      recvPBOs[j] = pbo;

      pRecvPBO = pbo->MapUnpackedBuffer(bufSize);
#else
      recvBufs[j] = malloc(bufSize);
#endif
    }

    vector<MPI_Request>& mpiReqs = transaction.Receiver(this->CommRank) ? mpiRecvReqs : mpiSendReqs;

    // start send/recv data
    int iErr = 0;
    iErr = transaction.Execute(comm, this->CommRank, nComps, dataType, pSendPBO, dataType,
#ifdef PBO_RECV_BUFFERS
      pRecvPBO,
#else
      recvBufs[j],
#endif
      mpiReqs, mpiTypes, tag);
    if (iErr)
    {
      cerr << this->CommRank << " transaction " << j << ":" << tag << " failed " << iErr << endl
           << transaction << endl;
    }
  }

  // overlap framebuffer and shader config with communication
  unsigned int winExtSize[2];
  this->WindowExt.Size(winExtSize);

  if (newImage == nullptr)
  {
    newImage = svtkTextureObject::New();
    newImage->SetContext(this->Context);
    newImage->Create2D(winExtSize[0], winExtSize[1], nComps, dataType, false);
  }

  svtkOpenGLState* ostate = this->Context->GetState();
  ostate->PushFramebufferBindings();
  this->FBO->Bind(GL_FRAMEBUFFER);
  this->FBO->AddColorAttachment(0U, newImage);
  this->FBO->ActivateDrawBuffer(0U);

  svtkRenderbuffer* depthBuf = svtkRenderbuffer::New();
  depthBuf->SetContext(this->Context);
  depthBuf->CreateDepthAttachment(winExtSize[0], winExtSize[1]);
  this->FBO->AddDepthAttachment(depthBuf);

  svtkCheckFrameBufferStatusMacro(GL_FRAMEBUFFER);

  // the LIC'er requires all fragments in the vector
  // texture to be initialized to 0
  this->FBO->InitializeViewport(winExtSize[0], winExtSize[1]);

  ostate->svtkglEnable(GL_DEPTH_TEST);
  ostate->svtkglDisable(GL_SCISSOR_TEST);
  ostate->svtkglClearColor(0.0, 0.0, 0.0, 0.0);
  ostate->svtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  this->Context->GetShaderCache()->ReadyShaderProgram(this->CompositeShader->Program);

  // overlap compositing of local data with communication
  for (int j = 0; j < nTransactions; ++j)
  {
    svtkPPixelTransfer& transaction = this->GatherProgram[j];

    if (!transaction.Local(this->CommRank))
    {
      continue;
    }

#if svtkPSurfaceLICCompositeDEBUG >= 2
    cerr << this->CommRank << ":" << j << ":" << encodeTag(j, this->Pass) << " Local "
         << transaction << endl;
#endif

    const svtkPixelExtent& destExt = transaction.GetDestinationExtent();
    unsigned int pboSize = static_cast<unsigned int>(destExt.Size() * nComps);
    unsigned int bufSize = pboSize * dataTypeSize;

    svtkPixelBufferObject* pbo = svtkPixelBufferObject::New();
    pbo->SetContext(this->Context);
    pbo->SetType(dataType);
    pbo->SetComponents(nComps);
    pbo->SetSize(pboSize);

    void* pRecvPBO = pbo->MapUnpackedBuffer(bufSize);

    int iErr = transaction.Blit(nComps, dataType, pSendPBO, dataType, pRecvPBO);

    if (iErr)
    {
      cerr << this->CommRank << " local transaction " << j << ":" << this->Pass << " failed "
           << iErr << endl
           << transaction << endl;
    }

    pbo->UnmapUnpackedBuffer();

    unsigned int destDims[2];
    destExt.Size(destDims);

    svtkTextureObject* tex = svtkTextureObject::New();
    tex->SetContext(this->Context);
    tex->Create2D(destDims[0], destDims[1], nComps, pbo, false);

    pbo->Delete();

#if svtkPSurfaceLICCompositeDEBUG >= 2
    ostringstream oss;
    oss << j << ":" << this->Pass << "_localRecvdData.svtk";
    svtkTextureIO::Write(mpifn(this->CommRank, oss.str().c_str()), tex);
#endif

    // Compositing because of overlap in guard pixels
    this->ExecuteShader(destExt, tex);

    tex->Delete();
  }

  // composite inflight data as it arrives.
  int nRecvReqs = static_cast<int>(mpiRecvReqs.size());
  for (int i = 0; i < nRecvReqs; ++i)
  {
    // wait for the completion of one of the recvs
    MPI_Status stat;
    int reqId;
    int iErr = MPI_Waitany(nRecvReqs, &mpiRecvReqs[0], &reqId, &stat);
    if (iErr)
    {
      svtkErrorMacro("comm error in recv");
    }

    // decode transaction id
    int j = decodeTag(stat.MPI_TAG, this->Pass);
    svtkPPixelTransfer& transaction = this->GatherProgram[j];

#if svtkPSurfaceLICCompositeDEBUG >= 2
    cerr << this->CommRank << ":" << j << ":" << stat.MPI_TAG << " Recv " << transaction << endl;
#endif

    // move recv'd data from pbo to texture
    const svtkPixelExtent& destExt = transaction.GetDestinationExtent();

    unsigned int destDims[2];
    destExt.Size(destDims);

#ifdef PBO_RECV_BUFFERS
    svtkPixelBufferObject*& pbo = recvPBOs[j];
    pbo->UnmapUnpackedBuffer();
#else
    unsigned int pboSize = nComps * destExt.Size();
    unsigned int bufSize = pboSize * dataTypeSize;

    svtkPixelBufferObject* pbo = svtkPixelBufferObject::New();
    pbo->SetContext(this->Context);
    pbo->SetType(dataType);
    pbo->SetComponents(nComps);
    pbo->SetSize(pboSize);

    void* pbuf = pbo->MapUnpackedBuffer(bufSize);

    void*& rbuf = recvBufs[j];

    memcpy(pbuf, rbuf, bufSize);

    pbo->UnmapUnpackedBuffer();

    free(rbuf);
    rbuf = nullptr;
#endif

    svtkTextureObject* tex = svtkTextureObject::New();
    tex->SetContext(this->Context);
    tex->Create2D(destDims[0], destDims[1], nComps, pbo, false);

    pbo->Delete();
    pbo = nullptr;

#if svtkPSurfaceLICCompositeDEBUG >= 2
    ostringstream oss;
    oss << j << ":" << this->Pass << "_recvdData.svtk";
    svtkTextureIO::Write(mpifn(this->CommRank, oss.str().c_str()), tex);
#endif

    this->ExecuteShader(destExt, tex);

    tex->Delete();
  }

  this->FBO->DeactivateDrawBuffers();
  this->FBO->RemoveColorAttachment(0U);
  this->FBO->RemoveDepthAttachment();
  ostate->PopFramebufferBindings();
  depthBuf->Delete();

  // wait for sends to complete
  int nSendReqs = static_cast<int>(mpiSendReqs.size());
  if (nSendReqs)
  {
    int iErr = MPI_Waitall(nSendReqs, &mpiSendReqs[0], MPI_STATUSES_IGNORE);
    if (iErr)
    {
      svtkErrorMacro("comm error in send");
    }
  }

  MPITypeFree(mpiTypes);

  return 0;
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::ExecuteShader(const svtkPixelExtent& ext, svtkTextureObject* tex)
{
  // cell to node
  svtkPixelExtent next(ext);
  next.CellToNode();

  float fext[4];
  next.GetData(fext);

  float tcoords[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

  tex->Activate();
  this->CompositeShader->Program->SetUniformi("texData", tex->GetTextureUnit());

  unsigned int winExtSize[2];
  this->WindowExt.Size(winExtSize);

  float verts[] = { 2.0f * fext[0] / winExtSize[0] - 1.0f, 2.0f * fext[2] / winExtSize[1] - 1.0f,
    0.0f, 2.0f * (fext[1] + 1.0f) / winExtSize[0] - 1.0f, 2.0f * fext[2] / winExtSize[1] - 1.0f,
    0.0f, 2.0f * (fext[1] + 1.0f) / winExtSize[0] - 1.0f,
    2.0f * (fext[3] + 1.0f) / winExtSize[1] - 1.0f, 0.0f, 2.0f * fext[0] / winExtSize[0] - 1.0f,
    2.0f * (fext[3] + 1.0f) / winExtSize[1] - 1.0f, 0.0f };

  svtkOpenGLRenderUtilities::RenderQuad(
    verts, tcoords, this->CompositeShader->Program, this->CompositeShader->VAO);
  tex->Deactivate();

  return 0;
}

// ----------------------------------------------------------------------------
int svtkPSurfaceLICComposite::Scatter(
  void* pSendPBO, int dataType, int nComps, svtkTextureObject*& newImage)
{
#if svtkPSurfaceLICCompositeDEBUG >= 2
  cerr << "=====svtkPSurfaceLICComposite::Scatter" << endl;
#endif

  int iErr = 0;
  // two pipleines depending on if this process recv's or send's
  //
  // send:
  // tex -> pbo -> mpi_send
  //
  // recv:
  // mpi_recv -> pbo -> tex -> composite shader -> fbo

  // pass id is decoded into mpi tag form non-blocking comm
  this->Pass += 1;

  // validate inputs
  if (this->Pass >= maxNumPasses())
  {
    return -1;
  }
  if (pSendPBO == nullptr)
  {
    return -2;
  }
  if (this->Context == nullptr)
  {
    return -3;
  }

  // get the size of the array datatype
  int dataTypeSize = 0;
  switch (dataType)
  {
    svtkTemplateMacro(dataTypeSize = sizeof(SVTK_TT););
    default:
      return -4;
  }
  unsigned int pboSize = (unsigned int)this->WindowExt.Size() * nComps;
  unsigned int bufSize = pboSize * dataTypeSize;

#ifdef PBO_RECV_BUFFERS
  svtkPixelBufferObject* recvPBO;
  recvPBO = svtkPixelBufferObject::New();
  recvPBO->SetContext(this->Context);
  recvPBO->SetType(dataType);
  recvPBO->SetComponents(nComps);
  recvPBO->SetSize(pboSize);

  void* pRecvPBO = recvPBO->MapUnpackedBuffer(bufSize);
  memset(pRecvPBO, 0, bufSize);
#else
  void* pRecvBuf = malloc(bufSize);
  memset(pRecvBuf, 0, bufSize);
#endif

  // initiate non-blocking comm
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  int nTransactions = static_cast<int>(this->ScatterProgram.size());
  vector<MPI_Request> mpiRecvReqs;
  vector<MPI_Request> mpiSendReqs;
  deque<MPI_Datatype> mpiTypes;
  for (int j = 0; j < nTransactions; ++j)
  {
    svtkPPixelTransfer& transaction = this->ScatterProgram[j];

    // postpone local transactions, they will be overlapped
    // with transactions requiring communication
    if (transaction.Local(this->CommRank))
    {
      continue;
    }

    // encode transaction.
    int tag = encodeTag(j, this->Pass);

    vector<MPI_Request>& mpiReqs = transaction.Receiver(this->CommRank) ? mpiRecvReqs : mpiSendReqs;

    // start send/recv data
    iErr = transaction.Execute(comm, this->CommRank, nComps, dataType, pSendPBO, dataType,
#ifdef PBO_RECV_BUFFERS
      pRecvPBO,
#else
      pRecvBuf,
#endif
      mpiReqs, mpiTypes, tag);
    if (iErr)
    {
      svtkErrorMacro(<< this->CommRank << " transaction " << j << ":" << tag << " failed " << iErr
                    << endl
                    << transaction);
    }
  }

  // overlap transfer of local data with communication. compositing is not
  // needed since source blocks are disjoint.
  for (int j = 0; j < nTransactions; ++j)
  {
    svtkPPixelTransfer& transaction = this->ScatterProgram[j];

    if (!transaction.Local(this->CommRank))
    {
      continue;
    }

#if svtkPSurfaceLICCompositeDEBUG >= 2
    cerr << this->CommRank << ":" << j << ":" << encodeTag(j, this->Pass) << " Local "
         << transaction << endl;
#endif

    iErr = transaction.Blit(nComps, dataType, pSendPBO, dataType,
#ifdef PBO_RECV_BUFFERS
      pRecvPBO
#else
      pRecvBuf
#endif
    );
    if (iErr)
    {
      svtkErrorMacro(<< this->CommRank << " local transaction " << j << ":" << this->Pass
                    << " failed " << iErr << endl
                    << transaction);
    }
  }

  // recv remote data. compsiting is not needed since source blocks are
  // disjoint.
  int nRecvReqs = static_cast<int>(mpiRecvReqs.size());
  if (nRecvReqs)
  {
    iErr = MPI_Waitall(nRecvReqs, &mpiRecvReqs[0], MPI_STATUSES_IGNORE);
    if (iErr)
    {
      svtkErrorMacro("comm error in recv");
    }
  }

  unsigned int winExtSize[2];
  this->WindowExt.Size(winExtSize);

  if (newImage == nullptr)
  {
    newImage = svtkTextureObject::New();
    newImage->SetContext(this->Context);
    newImage->Create2D(winExtSize[0], winExtSize[1], nComps, dataType, false);
  }

// transfer received data to the icet/decomp.
#ifdef PBO_RECV_BUFFERS
  recvPBO->UnmapUnpackedBuffer();
  newImage->Create2D(winExtSize[0], winExtSize[1], nComps, recvPBO, false);
  recvPBO->Delete();
#else
  svtkPixelBufferObject* recvPBO;
  recvPBO = svtkPixelBufferObject::New();
  recvPBO->SetContext(this->Context);
  recvPBO->SetType(dataType);
  recvPBO->SetComponents(nComps);
  recvPBO->SetSize(pboSize);
  void* pRecvPBO = recvPBO->MapUnpackedBuffer(bufSize);
  memcpy(pRecvPBO, pRecvBuf, bufSize);
  recvPBO->UnmapUnpackedBuffer();
  newImage->Create2D(winExtSize[0], winExtSize[1], nComps, recvPBO, false);
  recvPBO->Delete();
#endif

  // wait for sends to complete
  int nSendReqs = static_cast<int>(mpiSendReqs.size());
  if (nSendReqs)
  {
    iErr = MPI_Waitall(nSendReqs, &mpiSendReqs[0], MPI_STATUSES_IGNORE);
    if (iErr)
    {
      svtkErrorMacro("comm error in send");
    }
  }

  MPITypeFree(mpiTypes);

  return 0;
}

// ----------------------------------------------------------------------------
void svtkPSurfaceLICComposite::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkObject::PrintSelf(os, indent);
  os << *this << endl;
}

// ****************************************************************************
ostream& operator<<(ostream& os, svtkPSurfaceLICComposite& ss)
{
  // this puts output in rank order
  MPI_Comm comm = *(static_cast<MPI_Comm*>(ss.PainterComm->GetCommunicator()));
  int rankBelow = ss.CommRank - 1;
  if (rankBelow >= 0)
  {
    MPI_Recv(nullptr, 0, MPI_BYTE, rankBelow, 13579, comm, MPI_STATUS_IGNORE);
  }
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
  os << "SuffleProgram:" << endl;
  size_t nTransactions = ss.GatherProgram.size();
  for (size_t j = 0; j < nTransactions; ++j)
  {
    os << "  " << ss.GatherProgram[j] << endl;
  }
  os << "UnSuffleProgram:" << endl;
  nTransactions = ss.ScatterProgram.size();
  for (size_t j = 0; j < nTransactions; ++j)
  {
    os << "  " << ss.ScatterProgram[j] << endl;
  }
  int rankAbove = ss.CommRank + 1;
  if (rankAbove < ss.CommSize)
  {
    MPI_Send(nullptr, 0, MPI_BYTE, rankAbove, 13579, comm);
  }
  return os;
}
