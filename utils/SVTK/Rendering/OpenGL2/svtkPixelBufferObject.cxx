/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPixelBufferObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPixelBufferObject.h"

#include "svtk_glew.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"

#include "svtkOpenGLError.h"

//#define SVTK_PBO_DEBUG
//#define SVTK_PBO_TIMING

#ifdef SVTK_PBO_TIMING
#include "svtkTimerLog.h"
#endif

#include <cassert>

// Mapping from Usage values to OpenGL values.

static const GLenum OpenGLBufferObjectUsage[9] = { GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY,
  GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY, GL_DYNAMIC_DRAW, GL_DYNAMIC_READ,
  GL_DYNAMIC_COPY };

static const char* BufferObjectUsageAsString[9] = { "StreamDraw", "StreamRead", "StreamCopy",
  "StaticDraw", "StaticRead", "StaticCopy", "DynamicDraw", "DynamicRead", "DynamicCopy" };

// access modes
const GLenum OpenGLBufferObjectAccess[2] = {
#ifdef GL_ES_VERSION_3_0
  GL_MAP_WRITE_BIT, GL_MAP_READ_BIT
#else
  GL_WRITE_ONLY, GL_READ_ONLY
#endif
};

// targets
const GLenum OpenGLBufferObjectTarget[2] = { GL_PIXEL_UNPACK_BUFFER, GL_PIXEL_PACK_BUFFER };

#ifdef SVTK_PBO_DEBUG
#include <pthread.h> // for debugging with MPI, pthread_self()
#endif

// converting double to float behind the
// scene so we need sizeof(double)==4
template <class T>
class svtksizeof
{
public:
  static int GetSize() { return sizeof(T); }
};

template <>
class svtksizeof<double>
{
public:
  static int GetSize() { return sizeof(float); }
};

static int svtkGetSize(int type)
{
  switch (type)
  {
    svtkTemplateMacro(return ::svtksizeof<SVTK_TT>::GetSize(););
  }
  return 0;
}

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPixelBufferObject);

//----------------------------------------------------------------------------
svtkPixelBufferObject::svtkPixelBufferObject()
{
  this->Handle = 0;
  this->Context = nullptr;
  this->BufferTarget = 0;
  this->Components = 0;
  this->Size = 0;
  this->Type = SVTK_UNSIGNED_CHAR;
  this->Usage = StaticDraw;
}

//----------------------------------------------------------------------------
svtkPixelBufferObject::~svtkPixelBufferObject()
{
  this->DestroyBuffer();
}

//----------------------------------------------------------------------------
bool svtkPixelBufferObject::IsSupported(svtkRenderWindow*)
{
  return true;
}

//----------------------------------------------------------------------------
bool svtkPixelBufferObject::LoadRequiredExtensions(svtkRenderWindow* svtkNotUsed(renWin))
{
  return true;
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::SetContext(svtkRenderWindow* renWin)
{
  // avoid pointless re-assignment
  if (this->Context == renWin)
  {
    return;
  }
  // free resource allocations
  this->DestroyBuffer();
  this->Context = nullptr;
  this->Modified();
  // all done if assigned null
  if (!renWin)
  {
    return;
  }

  // update context
  this->Context = renWin;
  this->Context->MakeCurrent();
}

//----------------------------------------------------------------------------
svtkRenderWindow* svtkPixelBufferObject::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::SetSize(unsigned int nTups, int nComps)
{
  this->Size = nTups * nComps;
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::Bind(BufferType type)
{
  assert(this->Context);

  this->CreateBuffer();

  GLenum target;
  switch (type)
  {
    case svtkPixelBufferObject::PACKED_BUFFER:
      target = GL_PIXEL_PACK_BUFFER;
      break;

    case svtkPixelBufferObject::UNPACKED_BUFFER:
      target = GL_PIXEL_UNPACK_BUFFER;
      break;

    default:
      svtkErrorMacro("Impossible BufferType.");
      target = static_cast<GLenum>(this->BufferTarget);
      break;
  }

  if (this->BufferTarget && this->BufferTarget != target)
  {
    this->UnBind();
  }
  this->BufferTarget = target;
  glBindBuffer(static_cast<GLenum>(this->BufferTarget), this->Handle);
  svtkOpenGLCheckErrorMacro("failed at glBindBuffer");
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::UnBind()
{
  assert(this->Context);
  if (this->Handle && this->BufferTarget)
  {
    glBindBuffer(this->BufferTarget, 0);
    svtkOpenGLCheckErrorMacro("failed at glBindBuffer(0)");
    this->BufferTarget = 0;
  }
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::CreateBuffer()
{
  if (!this->Handle)
  {
    GLuint ioBuf;
    glGenBuffers(1, &ioBuf);
    svtkOpenGLCheckErrorMacro("failed at glGenBuffers");
    this->Handle = ioBuf;
  }
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::DestroyBuffer()
{
  // because we don't hold a reference to the render
  // context we don't have any control on when it is
  // destroyed. In fact it may be destroyed before
  // we are(eg smart pointers), in which case we should
  // do nothing.
  if (this->Context && this->Handle)
  {
    GLuint ioBuf = static_cast<GLuint>(this->Handle);
    glDeleteBuffers(1, &ioBuf);
    svtkOpenGLCheckErrorMacro("failed at glDeleteBuffers");
  }
  this->Handle = 0;
}

//----------------------------------------------------------------------------
template <class T>
class svtkUpload3D
{
public:
  static void Upload(void* pboPtr, T* inData, unsigned int dims[3], int numComponents,
    svtkIdType continuousIncrements[3], int components, int* componentList)
  {
    //  cout<<"incs[3]="<<continuousIncrements[0]<<" "<<continuousIncrements[1]
    //      <<" "<<continuousIncrements[2]<<endl;

    T* fIoMem = static_cast<T*>(pboPtr);

    int numComp;
    int* permutation = nullptr;
    if (components == 0)
    {
      numComp = numComponents;
      permutation = new int[numComponents];
      int i = 0;
      while (i < numComp)
      {
        permutation[i] = i;
        ++i;
      }
    }
    else
    {
      numComp = components;
      permutation = componentList;
    }

    svtkIdType tupleSize = static_cast<svtkIdType>(numComponents + continuousIncrements[0]);
    for (unsigned int zz = 0; zz < dims[2]; zz++)
    {
      for (unsigned int yy = 0; yy < dims[1]; yy++)
      {
        for (unsigned int xx = 0; xx < dims[0]; xx++)
        {
          for (int compNo = 0; compNo < numComp; compNo++)
          {
            *fIoMem = inData[permutation[compNo]];
            //              cout<<"upload[zz="<<zz<<"][yy="<<yy<<"][xx="<<xx<<"][compNo="<<
            //              compNo<<"] from inData to pbo="<<(double)(*fIoMem)<<endl;

            fIoMem++;
          }
          inData += tupleSize + continuousIncrements[0];
        }
        // Reached end of row, go to start of next row.
        inData += continuousIncrements[1] * tupleSize;
      }
      // Reached end of 2D plane.
      inData += continuousIncrements[2] * tupleSize;
    }

    if (components == 0)
    {
      delete[] permutation;
    }
  }
};

template <>
class svtkUpload3D<double>
{
public:
  static void Upload(void* pboPtr, double* inData, unsigned int dims[3], int numComponents,
    svtkIdType continuousIncrements[3], int components, int* componentList)
  {
    float* fIoMem = static_cast<float*>(pboPtr);

    int numComp;
    int* permutation = nullptr;
    if (components == 0)
    {
      numComp = numComponents;
      permutation = new int[numComponents];
      int i = 0;
      while (i < numComp)
      {
        permutation[i] = i;
        ++i;
      }
    }
    else
    {
      numComp = components;
      permutation = componentList;
    }

    svtkIdType tupleSize = static_cast<svtkIdType>(numComponents + continuousIncrements[0]);
    for (unsigned int zz = 0; zz < dims[2]; zz++)
    {
      for (unsigned int yy = 0; yy < dims[1]; yy++)
      {
        for (unsigned int xx = 0; xx < dims[0]; xx++)
        {
          for (int compNo = 0; compNo < numComponents; compNo++)
          {
            *fIoMem = static_cast<float>(inData[permutation[compNo]]);

            //        cout<<"upload specialized
            //        double[zz="<<zz<<"][yy="<<yy<<"][xx="<<xx<<"][compNo="<<compNo<<"] from
            //        inData="<<(*inData)<<" to pbo="<<(*fIoMem)<<endl;

            fIoMem++;
          }

          inData += tupleSize + continuousIncrements[0];
        }
        // Reached end of row, go to start of next row.
        inData += continuousIncrements[1] * tupleSize;
      }
      // Reached end of 2D plane.
      inData += continuousIncrements[2] * tupleSize;
    }
    if (components == 0)
    {
      delete[] permutation;
    }
  }
};

//----------------------------------------------------------------------------
void* svtkPixelBufferObject::MapBuffer(unsigned int nbytes, BufferType mode)
{
  // from svtk to opengl enums
  GLenum target = OpenGLBufferObjectTarget[mode];
  GLenum access = OpenGLBufferObjectAccess[mode];
  GLenum usage = OpenGLBufferObjectUsage[mode];
  GLuint size = static_cast<GLuint>(nbytes);
  GLuint ioBuf = static_cast<GLuint>(this->Handle);

  if (!ioBuf)
  {
    glGenBuffers(1, &ioBuf);
    svtkOpenGLCheckErrorMacro("failed at glGenBuffers");
    this->Handle = static_cast<unsigned int>(ioBuf);
  }
  this->BufferTarget = 0;

  // pointer to the mapped memory
  glBindBuffer(target, ioBuf);
  svtkOpenGLCheckErrorMacro("failed at glBindBuffer");

  glBufferData(target, size, nullptr, usage);
  svtkOpenGLCheckErrorMacro("failed at glBufferData");

#ifdef GL_ES_VERSION_3_0
  void* pPBO = glMapBufferRange(target, 0, size, access);
#else
  void* pPBO = glMapBuffer(target, access);
#endif
  svtkOpenGLCheckErrorMacro("failed at glMapBuffer");

  glBindBuffer(target, 0);

  return pPBO;
}

//----------------------------------------------------------------------------
void* svtkPixelBufferObject::MapBuffer(int type, unsigned int numtuples, int comps, BufferType mode)
{
  // from svtk to opengl enums
  this->Size = numtuples * comps;
  this->Type = type;
  this->Components = comps;
  unsigned int size = ::svtkGetSize(type) * this->Size;

  return this->MapBuffer(size, mode);
}

//----------------------------------------------------------------------------
void* svtkPixelBufferObject::MapBuffer(BufferType mode)
{
  // from svtk to opengl enum
  GLuint ioBuf = static_cast<GLuint>(this->Handle);
  if (!ioBuf)
  {
    svtkErrorMacro("Uninitialized object");
    return nullptr;
  }
  GLenum target = OpenGLBufferObjectTarget[mode];
  GLenum access = OpenGLBufferObjectAccess[mode];

  // pointer to the mnapped memory
  glBindBuffer(target, ioBuf);
  svtkOpenGLCheckErrorMacro("failed at glBindBuffer");

#ifdef GL_ES_VERSION_3_0
  void* pPBO = glMapBufferRange(this->BufferTarget, 0, this->Size, access);
#else
  void* pPBO = glMapBuffer(target, access);
#endif
  svtkOpenGLCheckErrorMacro("failed at glMapBuffer");

  glBindBuffer(target, 0);
  svtkOpenGLCheckErrorMacro("failed at glBindBuffer(0)");

  this->BufferTarget = 0;

  return pPBO;
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::UnmapBuffer(BufferType mode)
{
  GLuint ioBuf = static_cast<GLuint>(this->Handle);
  if (!ioBuf)
  {
    svtkErrorMacro("Uninitialized object");
    return;
  }
  GLenum target = OpenGLBufferObjectTarget[mode];

  glBindBuffer(target, ioBuf);
  svtkOpenGLCheckErrorMacro("failed at glBindBuffer");

  glUnmapBuffer(target);
  svtkOpenGLCheckErrorMacro("failed at glUnmapBuffer");

  glBindBuffer(target, 0);
  svtkOpenGLCheckErrorMacro("failed at glBindBuffer(0)");
}

//----------------------------------------------------------------------------
bool svtkPixelBufferObject::Upload3D(int type, void* data, unsigned int dims[3], int numComponents,
  svtkIdType continuousIncrements[3], int components, int* componentList)
{
#ifdef SVTK_PBO_TIMING
  svtkTimerLog* timer = svtkTimerLog::New();
  timer->StartTimer();
#endif
  assert(this->Context);

  this->CreateBuffer();
  this->Bind(svtkPixelBufferObject::UNPACKED_BUFFER);

  unsigned int size;

  if (components == 0)
  {
    size = dims[0] * dims[1] * dims[2] * static_cast<unsigned int>(numComponents);
  }
  else
  {
    size = dims[0] * dims[1] * dims[2] * static_cast<unsigned int>(components);
  }

  this->Components = numComponents;

  if (data != nullptr)
  {
    this->Usage = StreamDraw;
  }
  else
  {
    this->Usage = StreamRead;
  }

  glBufferData(this->BufferTarget, size * static_cast<unsigned int>(::svtkGetSize(type)), nullptr,
    OpenGLBufferObjectUsage[this->Usage]);
  svtkOpenGLCheckErrorMacro("failed at glBufferData");
  this->Type = type;
  if (this->Type == SVTK_DOUBLE)
  {
    this->Type = SVTK_FLOAT;
  }
  this->Size = size;

  if (data)
  {
#ifdef GL_ES_VERSION_3_0
    void* ioMem = glMapBufferRange(this->BufferTarget, 0, size, GL_MAP_WRITE_BIT);
#else
    void* ioMem = glMapBuffer(this->BufferTarget, GL_WRITE_ONLY);
#endif
    svtkOpenGLCheckErrorMacro("");
    switch (type)
    {
      svtkTemplateMacro(::svtkUpload3D<SVTK_TT>::Upload(ioMem, static_cast<SVTK_TT*>(data), dims,
        numComponents, continuousIncrements, components, componentList););
      default:
        svtkErrorMacro("unsupported svtk type");
        return false;
    }
    glUnmapBuffer(this->BufferTarget);
    svtkOpenGLCheckErrorMacro("failed at glUnmapBuffer");
  }

  this->UnBind();
#ifdef SVTK_PBO_TIMING
  timer->StopTimer();
  double time = timer->GetElapsedTime();
  timer->Delete();
  cout << "Upload data to PBO" << time << " seconds." << endl;
#endif
  return true;
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::Allocate(int type, unsigned int numtuples, int comps, BufferType mode)
{
  assert(this->Context);

  // from svtk to opengl enums
  this->Size = numtuples * comps;
  this->Type = type;
  this->Components = comps;
  unsigned int size = ::svtkGetSize(type) * this->Size;

  this->Allocate(size, mode);
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::Allocate(unsigned int nbytes, BufferType mode)
{
  assert(this->Context);

  // from svtk to opengl enums
  GLenum target = OpenGLBufferObjectTarget[mode];
  GLenum usage = OpenGLBufferObjectUsage[mode];
  GLuint size = static_cast<GLuint>(nbytes);
  GLuint ioBuf = static_cast<GLuint>(this->Handle);

  if (!ioBuf)
  {
    glGenBuffers(1, &ioBuf);
    svtkOpenGLCheckErrorMacro("failed at glGenBuffers");
    this->Handle = static_cast<unsigned int>(ioBuf);
  }
  this->BufferTarget = 0;

  glBindBuffer(target, ioBuf);
  svtkOpenGLCheckErrorMacro("failed at glBindBuffer");

  glBufferData(target, size, nullptr, usage);
  svtkOpenGLCheckErrorMacro("failed at glBufferData");

  glBindBuffer(target, 0);
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::ReleaseMemory()
{
  assert(this->Context);
  assert(this->Handle);

  this->Bind(svtkPixelBufferObject::PACKED_BUFFER);
  glBufferData(this->BufferTarget, 0, nullptr, GL_STREAM_DRAW);
  svtkOpenGLCheckErrorMacro("failed at glBufferData");
  this->Size = 0;
}

// ----------------------------------------------------------------------------
template <class TPBO, class TCPU>
void svtkDownload3D(
  TPBO* pboPtr, TCPU* cpuPtr, unsigned int dims[3], int numcomps, svtkIdType increments[3])
{
#ifdef SVTK_PBO_DEBUG
  cout << "template svtkDownload3D" << endl;
#endif
  svtkIdType tupleSize = static_cast<svtkIdType>(numcomps + increments[0]);
  for (unsigned int zz = 0; zz < dims[2]; zz++)
  {
    for (unsigned int yy = 0; yy < dims[1]; yy++)
    {
      for (unsigned int xx = 0; xx < dims[0]; xx++)
      {
        for (int comp = 0; comp < numcomps; comp++)
        {
          *cpuPtr = static_cast<TCPU>(*pboPtr);
          //          cout<<"download[zz="<<zz<<"][yy="<<yy<<"][xx="<<xx<<"][comp="<<comp<<"] from
          //          pbo="<<(*pboPtr)<<" to cpu="<<(*cpuPtr)<<endl;
          pboPtr++;
          cpuPtr++;
        }
        cpuPtr += increments[0];
      }
      // Reached end of row, go to start of next row.
      cpuPtr += increments[1] * tupleSize;
    }
    cpuPtr += increments[2] * tupleSize;
  }
}

// ----------------------------------------------------------------------------
template <class OType>
void svtkDownload3DSpe(
  int iType, void* iData, OType odata, unsigned int dims[3], int numcomps, svtkIdType increments[3])
{
#ifdef SVTK_PBO_DEBUG
  cout << "svtkDownload3DSpe" << endl;
#endif
  switch (iType)
  {
    svtkTemplateMacro(
      ::svtkDownload3D(static_cast<SVTK_TT*>(iData), odata, dims, numcomps, increments););
    default:
#ifdef SVTK_PBO_DEBUG
      cout << "d nested default." << endl;
#endif
      break;
  }
}

//----------------------------------------------------------------------------
bool svtkPixelBufferObject::Download3D(
  int type, void* data, unsigned int dims[3], int numcomps, svtkIdType increments[3])
{
#ifdef SVTK_PBO_TIMING
  svtkTimerLog* timer = svtkTimerLog::New();
  timer->StartTimer();
#endif
  assert(this->Context);

  if (!this->Handle)
  {
    svtkErrorMacro("No GPU data available.");
    return false;
  }

  if (this->Size < dims[0] * dims[1] * dims[2] * static_cast<unsigned int>(numcomps))
  {
    svtkErrorMacro("Size too small.");
    return false;
  }

  this->Bind(svtkPixelBufferObject::PACKED_BUFFER);

#ifdef GL_ES_VERSION_3_0
  void* ioMem = glMapBufferRange(this->BufferTarget, 0, this->Size, GL_MAP_READ_BIT);
#else
  void* ioMem = glMapBuffer(this->BufferTarget, GL_READ_ONLY);
#endif
  svtkOpenGLCheckErrorMacro("failed at glMapBuffer");

  switch (type)
  {
    svtkTemplateMacro(SVTK_TT* odata = static_cast<SVTK_TT*>(data);
                     ::svtkDownload3DSpe(this->Type, ioMem, odata, dims, numcomps, increments););
    default:
      svtkErrorMacro("unsupported svtk type");
      return false;
  }
  glUnmapBuffer(this->BufferTarget);
  svtkOpenGLCheckErrorMacro("failed at glUnmapBuffer");
  this->UnBind();

#ifdef SVTK_PBO_TIMING
  timer->StopTimer();
  double time = timer->GetElapsedTime();
  timer->Delete();
  cout << "dowmload data from PBO" << time << " seconds." << endl;
#endif

  return true;
}

//----------------------------------------------------------------------------
void svtkPixelBufferObject::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Context: " << this->Context << endl;
  os << indent << "Handle: " << this->Handle << endl;
  os << indent << "Size: " << this->Size << endl;
  os << indent << "SVTK Type: " << svtkImageScalarTypeNameMacro(this->Type) << endl;
  os << indent << "Usage:" << BufferObjectUsageAsString[this->Usage] << endl;
}
