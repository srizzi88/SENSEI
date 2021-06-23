/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVtkJSViewNodeFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVtkJSViewNodeFactory.h"

#include <svtkActor.h>
#include <svtkActorNode.h>
#include <svtkCompositePolyDataMapper.h>
#include <svtkGlyph3DMapper.h>
#include <svtkMapper.h>
#include <svtkMapperNode.h>
#include <svtkObjectFactory.h>
#include <svtkRenderWindow.h>
#include <svtkRenderer.h>
#include <svtkRendererNode.h>
#include <svtkWindowNode.h>

#include "svtkVtkJSSceneGraphSerializer.h"

#if SVTK_MODULE_ENABLE_SVTK_RenderingOpenGL2
#include <svtkCompositePolyDataMapper2.h>
#endif

#include <type_traits>

namespace
{
// A template for performing a compile-time check if a scene element inherits
// from svtkAlgorithm and calling Update on it if it does.
class UpdateIfAlgorithm
{
  template <typename X>
  static typename std::enable_if<std::is_base_of<svtkAlgorithm, X>::value>::type MaybeUpdate(X* x)
  {
    x->Update();
  }

  template <typename X>
  static typename std::enable_if<!std::is_base_of<svtkAlgorithm, X>::value>::type MaybeUpdate(X*)
  {
  }

public:
  template <typename MaybeAlgorithm>
  static void Update(MaybeAlgorithm* maybeAlgorithm)
  {
    UpdateIfAlgorithm::MaybeUpdate(maybeAlgorithm);
  }
};

// A template for constructing view nodes associated with scene elements and
// their associated renderables.
template <typename Base, typename Renderable>
class svtkVtkJSViewNode : public Base
{
public:
  static svtkViewNode* New()
  {
    svtkVtkJSViewNode* result = new svtkVtkJSViewNode;
    result->InitializeObjectBase();
    return result;
  }

  void Synchronize(bool prepass) override
  {
    this->Base::Synchronize(prepass);
    if (prepass)
    {
      auto factory = svtkVtkJSViewNodeFactory::SafeDownCast(this->GetMyFactory());
      if (factory != nullptr)
      {
        factory->GetSerializer()->Add(this, Renderable::SafeDownCast(this->GetRenderable()));
      }
    }
  }

  void Render(bool prepass) override
  {
    this->Base::Render(prepass);
    UpdateIfAlgorithm::Update(Renderable::SafeDownCast(this->GetRenderable()));
  }
};
}

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkVtkJSViewNodeFactory);
svtkCxxSetObjectMacro(svtkVtkJSViewNodeFactory, Serializer, svtkVtkJSSceneGraphSerializer);

//----------------------------------------------------------------------------
svtkVtkJSViewNodeFactory::svtkVtkJSViewNodeFactory()
{
  this->Serializer = svtkVtkJSSceneGraphSerializer::New();

  // Since a view node is constructed if an override exists for one of its base
  // classes, we only need to span the set of base renderable types and provide
  // specializations when custom logic is required by svtk-js.

  // These overrides span the base renderable types.
  this->RegisterOverride("svtkActor", svtkVtkJSViewNode<svtkActorNode, svtkActor>::New);
  this->RegisterOverride("svtkMapper", svtkVtkJSViewNode<svtkMapperNode, svtkMapper>::New);
  this->RegisterOverride("svtkRenderWindow", svtkVtkJSViewNode<svtkWindowNode, svtkRenderWindow>::New);
  this->RegisterOverride("svtkRenderer", svtkVtkJSViewNode<svtkRendererNode, svtkRenderer>::New);

  // These overrides are necessary to accommodate custom logic that must be
  // performed when converting these renderables to svtk-js.
  this->RegisterOverride(
    "svtkCompositePolyDataMapper", svtkVtkJSViewNode<svtkMapperNode, svtkCompositePolyDataMapper>::New);
#if SVTK_MODULE_ENABLE_SVTK_RenderingOpenGL2
  this->RegisterOverride("svtkCompositePolyDataMapper2",
    svtkVtkJSViewNode<svtkMapperNode, svtkCompositePolyDataMapper2>::New);
#endif
  this->RegisterOverride(
    "svtkGlyph3DMapper", svtkVtkJSViewNode<svtkMapperNode, svtkGlyph3DMapper>::New);
}

//----------------------------------------------------------------------------
svtkVtkJSViewNodeFactory::~svtkVtkJSViewNodeFactory()
{
  this->SetSerializer(nullptr);
}

//----------------------------------------------------------------------------
void svtkVtkJSViewNodeFactory::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
