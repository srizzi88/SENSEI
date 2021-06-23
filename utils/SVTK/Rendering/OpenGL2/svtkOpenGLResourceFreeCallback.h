/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkOpenGLResourceFreeCallback_h
#define svtkOpenGLResourceFreeCallback_h

// Description:
// Provide a mechanism for making sure graphics resources are
// freed properly.

class svtkOpenGLRenderWindow;
class svtkWindow;

class svtkGenericOpenGLResourceFreeCallback
{
public:
  svtkGenericOpenGLResourceFreeCallback()
  {
    this->SVTKWindow = nullptr;
    this->Releasing = false;
  }
  virtual ~svtkGenericOpenGLResourceFreeCallback() {}

  // Called when the event is invoked
  virtual void Release() = 0;

  virtual void RegisterGraphicsResources(svtkOpenGLRenderWindow* rw) = 0;

  bool IsReleasing() { return this->Releasing; }

protected:
  svtkOpenGLRenderWindow* SVTKWindow;
  bool Releasing;
};

// Description:
// Templated member callback.
template <class T>
class svtkOpenGLResourceFreeCallback : public svtkGenericOpenGLResourceFreeCallback
{
public:
  svtkOpenGLResourceFreeCallback(T* handler, void (T::*method)(svtkWindow*))
  {
    this->Handler = handler;
    this->Method = method;
  }

  ~svtkOpenGLResourceFreeCallback() override {}

  void RegisterGraphicsResources(svtkOpenGLRenderWindow* rw) override
  {
    if (this->SVTKWindow == rw)
    {
      return;
    }
    if (this->SVTKWindow)
    {
      this->Release();
    }
    this->SVTKWindow = rw;
    if (this->SVTKWindow)
    {
      this->SVTKWindow->RegisterGraphicsResources(this);
    }
  }

  // Called when the event is invoked
  void Release() override
  {
    if (this->SVTKWindow && this->Handler && !this->Releasing)
    {
      this->Releasing = true;
      this->SVTKWindow->PushContext();
      (this->Handler->*this->Method)(this->SVTKWindow);
      this->SVTKWindow->UnregisterGraphicsResources(this);
      this->SVTKWindow->PopContext();
      this->SVTKWindow = nullptr;
      this->Releasing = false;
    }
  }

protected:
  T* Handler;
  void (T::*Method)(svtkWindow*);
};

#endif
// SVTK-HeaderTest-Exclude: svtkOpenGLResourceFreeCallback.h
