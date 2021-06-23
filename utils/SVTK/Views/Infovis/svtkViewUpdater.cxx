/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewUpdater.cxx

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

#include "svtkViewUpdater.h"

#include "svtkAnnotationLink.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkRenderView.h"
#include "svtkView.h"

#include <algorithm>
#include <vector>

svtkStandardNewMacro(svtkViewUpdater);

class svtkViewUpdater::svtkViewUpdaterInternals : public svtkCommand
{
public:
  void Execute(svtkObject*, unsigned long, void*) override
  {
    for (unsigned int i = 0; i < this->Views.size(); ++i)
    {
      svtkRenderView* rv = svtkRenderView::SafeDownCast(this->Views[i]);
      if (rv)
      {
        rv->Render();
      }
      else
      {
        this->Views[i]->Update();
      }
    }
  }

  std::vector<svtkView*> Views;
};

svtkViewUpdater::svtkViewUpdater()
{
  this->Internals = new svtkViewUpdaterInternals();
}

svtkViewUpdater::~svtkViewUpdater()
{
  this->Internals->Delete();
}

void svtkViewUpdater::AddView(svtkView* view)
{
  this->Internals->Views.push_back(view);
  // view->AddObserver(svtkCommand::SelectionChangedEvent, this->Internals);
}
void svtkViewUpdater::RemoveView(svtkView* view)
{
  std::vector<svtkView*>::iterator p;
  p = std::find(this->Internals->Views.begin(), this->Internals->Views.end(), view);
  if (p == this->Internals->Views.end())
    return;
  this->Internals->Views.erase(p);
}

void svtkViewUpdater::AddAnnotationLink(svtkAnnotationLink* link)
{
  link->AddObserver(svtkCommand::AnnotationChangedEvent, this->Internals);
}

void svtkViewUpdater::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
