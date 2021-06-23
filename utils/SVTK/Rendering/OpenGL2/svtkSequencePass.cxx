/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSequencePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSequencePass.h"
#include "svtkObjectFactory.h"
#include "svtkRenderPassCollection.h"
#include <cassert>

svtkStandardNewMacro(svtkSequencePass);
svtkCxxSetObjectMacro(svtkSequencePass, Passes, svtkRenderPassCollection);

// ----------------------------------------------------------------------------
svtkSequencePass::svtkSequencePass()
{
  this->Passes = nullptr;
}

// ----------------------------------------------------------------------------
svtkSequencePass::~svtkSequencePass()
{
  if (this->Passes)
  {
    this->Passes->Delete();
  }
}

// ----------------------------------------------------------------------------
void svtkSequencePass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Passes:";
  if (this->Passes != nullptr)
  {
    this->Passes->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkSequencePass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;
  if (this->Passes)
  {
    this->Passes->InitTraversal();
    svtkRenderPass* p = this->Passes->GetNextRenderPass();
    while (p)
    {
      p->Render(s);
      this->NumberOfRenderedProps += p->GetNumberOfRenderedProps();
      p = this->Passes->GetNextRenderPass();
    }
  }
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkSequencePass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  if (this->Passes)
  {
    this->Passes->InitTraversal();
    svtkRenderPass* p = this->Passes->GetNextRenderPass();
    while (p)
    {
      p->ReleaseGraphicsResources(w);
      p = this->Passes->GetNextRenderPass();
    }
  }
}
