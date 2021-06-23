/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRenderTimer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLRenderTimer.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderer.h" // For query allocation bug check

#include "svtk_glew.h"

// glQueryCounter unavailable in OpenGL ES:
#ifdef GL_ES_VERSION_3_0
#define NO_TIMESTAMP_QUERIES
#endif

//------------------------------------------------------------------------------
svtkOpenGLRenderTimer::svtkOpenGLRenderTimer()
  : StartReady(false)
  , EndReady(false)
  , StartQuery(0)
  , EndQuery(0)
  , StartTime(0)
  , EndTime(0)
  , ReusableStarted(false)
  , ReusableEnded(false)
{
}

//------------------------------------------------------------------------------
svtkOpenGLRenderTimer::~svtkOpenGLRenderTimer()
{
  if (this->StartQuery != 0 || this->EndQuery != 0)
  {
    this->Reset();
  }
}

//------------------------------------------------------------------------------
bool svtkOpenGLRenderTimer::IsSupported()
{
#ifdef NO_TIMESTAMP_QUERIES
  return false;
#else
  static const bool s = !svtkOpenGLRenderer::HaveAppleQueryAllocationBug();
  return s;
#endif
}

//------------------------------------------------------------------------------
void svtkOpenGLRenderTimer::Reset()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (this->StartQuery == 0 && this->EndQuery == 0)
  {
    // short-circuit to avoid checking if queries weren't initialized at all.
    // this is necessary since `IsSupported` may make OpenGL calls on APPLE
    // through `HaveAppleQueryAllocationBug` invocation and that may be not be
    // correct when timers are being destroyed.
    return;
  }

  if (!this->IsSupported())
  {
    return;
  }

  if (this->StartQuery != 0)
  {
    glDeleteQueries(1, static_cast<GLuint*>(&this->StartQuery));
    this->StartQuery = 0;
  }

  if (this->EndQuery != 0)
  {
    glDeleteQueries(1, static_cast<GLuint*>(&this->EndQuery));
    this->EndQuery = 0;
  }

  this->StartReady = false;
  this->EndReady = false;
  this->StartTime = 0;
  this->EndTime = 0;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void svtkOpenGLRenderTimer::Start()
{
  if (!this->IsSupported())
  {
    return;
  }

  this->Reset();

#ifndef NO_TIMESTAMP_QUERIES
  glGenQueries(1, static_cast<GLuint*>(&this->StartQuery));
  glQueryCounter(static_cast<GLuint>(this->StartQuery), GL_TIMESTAMP);
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void svtkOpenGLRenderTimer::Stop()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->IsSupported())
  {
    return;
  }

  if (this->EndQuery != 0)
  {
    svtkGenericWarningMacro("svtkOpenGLRenderTimer::Stop called before "
                           "resetting. Ignoring.");
    return;
  }

  if (this->StartQuery == 0)
  {
    svtkGenericWarningMacro("svtkOpenGLRenderTimer::Stop called before "
                           "svtkOpenGLRenderTimer::Start. Ignoring.");
    return;
  }

  glGenQueries(1, static_cast<GLuint*>(&this->EndQuery));
  glQueryCounter(static_cast<GLuint>(this->EndQuery), GL_TIMESTAMP);
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
bool svtkOpenGLRenderTimer::Started()
{
#ifndef NO_TIMESTAMP_QUERIES
  return this->StartQuery != 0;
#else  // NO_TIMESTAMP_QUERIES
  return false;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
bool svtkOpenGLRenderTimer::Stopped()
{
#ifndef NO_TIMESTAMP_QUERIES
  return this->EndQuery != 0;
#else  // NO_TIMESTAMP_QUERIES
  return false;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
bool svtkOpenGLRenderTimer::Ready()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->IsSupported())
  {
    return false;
  }

  if (!this->StartReady)
  {
    GLint ready;
    glGetQueryObjectiv(static_cast<GLuint>(this->StartQuery), GL_QUERY_RESULT_AVAILABLE, &ready);
    if (!ready)
    {
      return false;
    }

    this->StartReady = true;
    glGetQueryObjectui64v(static_cast<GLuint>(this->StartQuery), GL_QUERY_RESULT,
      reinterpret_cast<GLuint64*>(&this->StartTime));
  }

  if (!this->EndReady)
  {
    GLint ready;
    glGetQueryObjectiv(static_cast<GLuint>(this->EndQuery), GL_QUERY_RESULT_AVAILABLE, &ready);
    if (!ready)
    {
      return false;
    }

    this->EndReady = true;
    glGetQueryObjectui64v(static_cast<GLuint>(this->EndQuery), GL_QUERY_RESULT,
      reinterpret_cast<GLuint64*>(&this->EndTime));
  }
#endif // NO_TIMESTAMP_QUERIES

  return true;
}

//------------------------------------------------------------------------------
float svtkOpenGLRenderTimer::GetElapsedSeconds()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0.f;
  }

  return (this->EndTime - this->StartTime) * 1e-9f;
#else  // NO_TIMESTAMP_QUERIES
  return 0.f;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
float svtkOpenGLRenderTimer::GetElapsedMilliseconds()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0.f;
  }

  return (this->EndTime - this->StartTime) * 1e-6f;
#else  // NO_TIMESTAMP_QUERIES
  return 0.f;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
svtkTypeUInt64 svtkOpenGLRenderTimer::GetElapsedNanoseconds()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0;
  }

  return (this->EndTime - this->StartTime);
#else  // NO_TIMESTAMP_QUERIES
  return 0;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
svtkTypeUInt64 svtkOpenGLRenderTimer::GetStartTime()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0;
  }

  return this->StartTime;
#else  // NO_TIMESTAMP_QUERIES
  return 0;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
svtkTypeUInt64 svtkOpenGLRenderTimer::GetStopTime()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0;
  }

  return this->EndTime;
#else  // NO_TIMESTAMP_QUERIES
  return 0;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void svtkOpenGLRenderTimer::ReleaseGraphicsResources()
{
  this->Reset();
}

//------------------------------------------------------------------------------
void svtkOpenGLRenderTimer::ReusableStart()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->IsSupported())
  {
    return;
  }

  if (this->StartQuery == 0)
  {
    glGenQueries(1, static_cast<GLuint*>(&this->StartQuery));
    glQueryCounter(static_cast<GLuint>(this->StartQuery), GL_TIMESTAMP);
    this->ReusableStarted = true;
    this->ReusableEnded = false;
  }
  if (!this->ReusableStarted)
  {
    glQueryCounter(static_cast<GLuint>(this->StartQuery), GL_TIMESTAMP);
    this->ReusableStarted = true;
    this->ReusableEnded = false;
  }
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void svtkOpenGLRenderTimer::ReusableStop()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->IsSupported())
  {
    return;
  }

  if (!this->ReusableStarted)
  {
    svtkGenericWarningMacro("svtkOpenGLRenderTimer::ReusableStop called before "
                           "svtkOpenGLRenderTimer::ReusableStart. Ignoring.");
    return;
  }

  if (this->EndQuery == 0)
  {
    glGenQueries(1, static_cast<GLuint*>(&this->EndQuery));
    glQueryCounter(static_cast<GLuint>(this->EndQuery), GL_TIMESTAMP);
    this->ReusableEnded = true;
  }
  if (!this->ReusableEnded)
  {
    glQueryCounter(static_cast<GLuint>(this->EndQuery), GL_TIMESTAMP);
    this->ReusableEnded = true;
  }
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
float svtkOpenGLRenderTimer::GetReusableElapsedSeconds()
{
#ifndef NO_TIMESTAMP_QUERIES
  // we do not have an end query yet so we cannot have a time
  if (!this->EndQuery)
  {
    return 0.0;
  }

  if (this->ReusableStarted && !this->StartReady)
  {
    GLint ready;
    glGetQueryObjectiv(static_cast<GLuint>(this->StartQuery), GL_QUERY_RESULT_AVAILABLE, &ready);
    if (ready)
    {
      this->StartReady = true;
    }
  }

  if (this->StartReady && this->ReusableEnded && !this->EndReady)
  {
    GLint ready;
    glGetQueryObjectiv(static_cast<GLuint>(this->EndQuery), GL_QUERY_RESULT_AVAILABLE, &ready);
    if (ready)
    {
      this->EndReady = true;
    }
  }

  // if everything is ready read the times to get a new elapsed time
  // and then prep for a new flight. This also has the benefit that
  // if no one is getting the elapsed time then nothing is done
  // beyond the first flight.
  if (this->StartReady && this->EndReady)
  {
    glGetQueryObjectui64v(static_cast<GLuint>(this->StartQuery), GL_QUERY_RESULT,
      reinterpret_cast<GLuint64*>(&this->StartTime));
    glGetQueryObjectui64v(static_cast<GLuint>(this->EndQuery), GL_QUERY_RESULT,
      reinterpret_cast<GLuint64*>(&this->EndTime));
    // it was ready so prepare another flight
    this->ReusableStarted = false;
    this->ReusableEnded = false;
    this->StartReady = false;
    this->EndReady = false;
  }

  return (this->EndTime - this->StartTime) * 1e-9f;
#else  // NO_TIMESTAMP_QUERIES
  return 0.f;
#endif // NO_TIMESTAMP_QUERIES
}
