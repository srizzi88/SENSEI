/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedAreaPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRenderedAreaPicker.h"
#include "svtkAbstractMapper3D.h"
#include "svtkAbstractVolumeMapper.h"
#include "svtkActor.h"
#include "svtkAssemblyPath.h"
#include "svtkCommand.h"
#include "svtkImageMapper3D.h"
#include "svtkLODProp3D.h"
#include "svtkMapper.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPlanes.h"
#include "svtkPoints.h"
#include "svtkProp.h"
#include "svtkProp3DCollection.h"
#include "svtkPropCollection.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkVolume.h"

svtkStandardNewMacro(svtkRenderedAreaPicker);

//--------------------------------------------------------------------------
svtkRenderedAreaPicker::svtkRenderedAreaPicker() = default;

//--------------------------------------------------------------------------
svtkRenderedAreaPicker::~svtkRenderedAreaPicker() = default;

//--------------------------------------------------------------------------
// Does what this class is meant to do.
int svtkRenderedAreaPicker::AreaPick(
  double x0, double y0, double x1, double y1, svtkRenderer* renderer)
{
  int picked = 0;
  svtkProp* propCandidate;
  svtkAbstractMapper3D* mapper = nullptr;
  int pickable;

  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  this->SelectionPoint[0] = (x0 + x1) * 0.5;
  this->SelectionPoint[1] = (y0 + y1) * 0.5;
  this->SelectionPoint[2] = 0.0;

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  this->DefineFrustum(x0, y0, x1, y1, renderer);

  // Ask the renderer do the hardware pick
  svtkPropCollection* pickList = nullptr;
  if (this->PickFromList)
  {
    pickList = this->PickList;
  }

  this->SetPath(renderer->PickPropFrom(x0, y0, x1, y1, pickList));

  // Software pick resulted in a hit.
  if (this->Path)
  {
    picked = 1;

    // invoke the pick event
    propCandidate = this->Path->GetLastNode()->GetViewProp();

    // find the mapper and dataset corresponding to the picked prop
    pickable = this->TypeDecipher(propCandidate, &mapper);
    if (pickable)
    {
      if (mapper)
      {
        this->Mapper = mapper;
        svtkMapper* map1;
        svtkAbstractVolumeMapper* vmap;
        svtkImageMapper3D* imap;
        if ((map1 = svtkMapper::SafeDownCast(mapper)) != nullptr)
        {
          this->DataSet = map1->GetInput();
          this->Mapper = map1;
        }
        else if ((vmap = svtkAbstractVolumeMapper::SafeDownCast(mapper)) != nullptr)
        {
          this->DataSet = vmap->GetDataSetInput();
          this->Mapper = vmap;
        }
        else if ((imap = svtkImageMapper3D::SafeDownCast(mapper)) != nullptr)
        {
          this->DataSet = imap->GetDataSetInput();
          this->Mapper = imap;
        }
        else
        {
          this->DataSet = nullptr;
        }
      } // mapper
    }   // pickable

    // go through list of props the renderer got for us and put only
    // the prop3Ds into this->Prop3Ds
    svtkPropCollection* pProps = renderer->GetPickResultProps();
    pProps->InitTraversal();

    svtkProp* prop;
    svtkAssemblyPath* path;
    while ((prop = pProps->GetNextProp()))
    {
      for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
      {
        propCandidate = path->GetLastNode()->GetViewProp();
        pickable = this->TypeDecipher(propCandidate, &mapper);
        if (pickable && !this->Prop3Ds->IsItemPresent(prop))
        {
          this->Prop3Ds->AddItem(static_cast<svtkProp3D*>(prop));
        }
      }
    }

    // Invoke pick method if one defined - prop goes first
    this->Path->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(svtkCommand::PickEvent, nullptr);
  }

  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);

  return picked;
}

//----------------------------------------------------------------------------
void svtkRenderedAreaPicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
