/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkSQLDatabase.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkTextCodecFactory.h"

#include "svtkObjectFactory.h"
#include "svtkTextCodec.h"

#include "svtkASCIITextCodec.h"
#include "svtkUTF16TextCodec.h"
#include "svtkUTF8TextCodec.h"

#include <algorithm>
#include <vector>

svtkStandardNewMacro(svtkTextCodecFactory);

class svtkTextCodecFactory::CallbackVector : public std::vector<svtkTextCodecFactory::CreateFunction>
{
};

svtkTextCodecFactory::CallbackVector* svtkTextCodecFactory::Callbacks = nullptr;

// Ensures that there are no leaks when the application exits.
class svtkTextCodecCleanup
{
public:
  void Use() {}
  ~svtkTextCodecCleanup() { svtkTextCodecFactory::UnRegisterAllCreateCallbacks(); }
};

// Used to clean up the Callbacks
static svtkTextCodecCleanup svtkCleanupTextCodecGlobal;

void svtkTextCodecFactory::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "svtkTextCodecFactory (" << this << ") \n";
  indent = indent.GetNextIndent();
  if (nullptr != svtkTextCodecFactory::Callbacks)
  {
    os << svtkTextCodecFactory::Callbacks->size() << " Callbacks registered\n";
  }
  else
  {
    os << "No Callbacks registered.\n";
  }
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}

void svtkTextCodecFactory::RegisterCreateCallback(svtkTextCodecFactory::CreateFunction callback)
{
  if (!svtkTextCodecFactory::Callbacks)
  {
    Callbacks = new svtkTextCodecFactory::CallbackVector();
    svtkCleanupTextCodecGlobal.Use();
    svtkTextCodecFactory::Initialize();
  }

  if (find(svtkTextCodecFactory::Callbacks->begin(), svtkTextCodecFactory::Callbacks->end(),
        callback) == svtkTextCodecFactory::Callbacks->end())
  {
    svtkTextCodecFactory::Callbacks->push_back(callback);
  }
}

void svtkTextCodecFactory::UnRegisterCreateCallback(svtkTextCodecFactory::CreateFunction callback)
{
  // we don't know for sure what order we are called in so if the global ones goes first this is
  // nullptr
  if (svtkTextCodecFactory::Callbacks)
  {
    for (std::vector<svtkTextCodecFactory::CreateFunction>::iterator i =
           svtkTextCodecFactory::Callbacks->begin();
         i != svtkTextCodecFactory::Callbacks->end(); ++i)
    {
      if (*i == callback)
      {
        svtkTextCodecFactory::Callbacks->erase(i);
        break;
      }
    }

    if (svtkTextCodecFactory::Callbacks->empty())
    {
      delete svtkTextCodecFactory::Callbacks;
      svtkTextCodecFactory::Callbacks = nullptr;
    }
  }
}

void svtkTextCodecFactory::UnRegisterAllCreateCallbacks()
{
  delete svtkTextCodecFactory::Callbacks;
  svtkTextCodecFactory::Callbacks = nullptr;
}

svtkTextCodec* svtkTextCodecFactory::CodecForName(const char* codecName)
{
  if (!svtkTextCodecFactory::Callbacks)
  {
    svtkTextCodecFactory::Initialize();
  }
  std::vector<svtkTextCodecFactory::CreateFunction>::iterator CF_i;
  for (CF_i = Callbacks->begin(); CF_i != Callbacks->end(); ++CF_i)
  {
    svtkTextCodec* outCodec = (*CF_i)();
    if (outCodec)
    {
      if (outCodec->CanHandle(codecName))
      {
        return outCodec;
      }
      else
      {
        outCodec->Delete();
      }
    }
  }

  return nullptr;
}

svtkTextCodec* svtkTextCodecFactory::CodecToHandle(istream& SampleData)
{
  if (!svtkTextCodecFactory::Callbacks)
  {
    svtkTextCodecFactory::Initialize();
  }
  std::vector<svtkTextCodecFactory::CreateFunction>::iterator CF_i;
  for (CF_i = Callbacks->begin(); CF_i != Callbacks->end(); ++CF_i)
  {
    svtkTextCodec* outCodec = (*CF_i)();
    if (nullptr != outCodec)
    {
      if (outCodec->IsValid(SampleData))
      {
        return outCodec;
      }
      else
      {
        outCodec->Delete();
      }
    }
  }

  return nullptr;
}

static svtkTextCodec* svtkASCIITextCodecFromCallback()
{
  return svtkASCIITextCodec::New();
}

static svtkTextCodec* svtkUTF8TextCodecFromCallback()
{
  return svtkUTF8TextCodec::New();
}

static svtkTextCodec* svtkUTF16TextCodecFromCallback()
{
  return svtkUTF16TextCodec::New();
}

void svtkTextCodecFactory::Initialize()
{
  if (!svtkTextCodecFactory::Callbacks)
  {
    svtkTextCodecFactory::RegisterCreateCallback(svtkASCIITextCodecFromCallback);
    svtkTextCodecFactory::RegisterCreateCallback(svtkUTF8TextCodecFromCallback);
    svtkTextCodecFactory::RegisterCreateCallback(svtkUTF16TextCodecFromCallback);
  }
}

svtkTextCodecFactory::svtkTextCodecFactory() = default;

svtkTextCodecFactory::~svtkTextCodecFactory() = default;
