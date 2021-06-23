/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAreaPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAreaPicker.h"
#include "svtkAbstractMapper3D.h"
#include "svtkAbstractVolumeMapper.h"
#include "svtkActor.h"
#include "svtkAssemblyPath.h"
#include "svtkCommand.h"
#include "svtkExtractSelectedFrustum.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkImageSlice.h"
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

svtkStandardNewMacro(svtkAreaPicker);

//--------------------------------------------------------------------------
svtkAreaPicker::svtkAreaPicker()
{
  this->FrustumExtractor = svtkExtractSelectedFrustum::New();
  this->Frustum = this->FrustumExtractor->GetFrustum();
  this->Frustum->Register(this);

  this->ClipPoints = this->FrustumExtractor->GetClipPoints();
  this->ClipPoints->Register(this);

  this->Prop3Ds = svtkProp3DCollection::New();
  this->Mapper = nullptr;
  this->DataSet = nullptr;

  this->X0 = 0.0;
  this->Y0 = 0.0;
  this->X1 = 0.0;
  this->Y1 = 0.0;
}

//--------------------------------------------------------------------------
svtkAreaPicker::~svtkAreaPicker()
{
  this->Prop3Ds->Delete();
  this->ClipPoints->Delete();
  this->Frustum->Delete();
  this->FrustumExtractor->Delete();
}

//--------------------------------------------------------------------------
// Initialize the picking process.
void svtkAreaPicker::Initialize()
{
  this->svtkAbstractPropPicker::Initialize();
  this->Prop3Ds->RemoveAllItems();
  this->Mapper = nullptr;
}

//--------------------------------------------------------------------------
void svtkAreaPicker::SetRenderer(svtkRenderer* renderer)
{
  this->Renderer = renderer;
}
//--------------------------------------------------------------------------
void svtkAreaPicker::SetPickCoords(double x0, double y0, double x1, double y1)
{
  this->X0 = x0;
  this->Y0 = y0;
  this->X1 = x1;
  this->Y1 = y1;
}
//--------------------------------------------------------------------------
int svtkAreaPicker::Pick()
{
  return this->AreaPick(this->X0, this->Y0, this->X1, this->Y1, this->Renderer);
}

//--------------------------------------------------------------------------
// Does what this class is meant to do.
int svtkAreaPicker::AreaPick(double x0, double y0, double x1, double y1, svtkRenderer* renderer)
{
  this->Initialize();
  this->X0 = x0;
  this->Y0 = y0;
  this->X1 = x1;
  this->Y1 = y1;
  if (renderer)
  {
    this->Renderer = renderer;
  }

  this->SelectionPoint[0] = (this->X0 + this->X1) * 0.5;
  this->SelectionPoint[1] = (this->Y0 + this->Y1) * 0.5;
  this->SelectionPoint[2] = 0.0;

  if (this->Renderer == nullptr)
  {
    svtkErrorMacro(<< "Must specify renderer!");
    return 0;
  }

  this->DefineFrustum(this->X0, this->Y0, this->X1, this->Y1, this->Renderer);

  return this->PickProps(this->Renderer);
}

//--------------------------------------------------------------------------
// Converts the given screen rectangle into a selection frustum.
// Saves the results in ClipPoints and Frustum.
void svtkAreaPicker::DefineFrustum(double x0, double y0, double x1, double y1, svtkRenderer* renderer)
{
  this->X0 = (x0 < x1) ? x0 : x1;
  this->Y0 = (y0 < y1) ? y0 : y1;
  this->X1 = (x0 > x1) ? x0 : x1;
  this->Y1 = (y0 > y1) ? y0 : y1;

  if (this->X0 == this->X1)
  {
    this->X1 += 1.0;
  }
  if (this->Y0 == this->Y1)
  {
    this->Y1 += 1.0;
  }

  // compute world coordinates of the pick volume
  double verts[32];
  renderer->SetDisplayPoint(this->X0, this->Y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[0]);

  renderer->SetDisplayPoint(this->X0, this->Y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[4]);

  renderer->SetDisplayPoint(this->X0, this->Y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[8]);

  renderer->SetDisplayPoint(this->X0, this->Y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[12]);

  renderer->SetDisplayPoint(this->X1, this->Y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[16]);

  renderer->SetDisplayPoint(this->X1, this->Y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[20]);

  renderer->SetDisplayPoint(this->X1, this->Y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[24]);

  renderer->SetDisplayPoint(this->X1, this->Y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[28]);

  // a pick point is required by svtkAbstractPicker
  // return center for now until a better meaning is desired
  double sum[3] = { 0.0, 0.0, 0.0 };
  for (int i = 0; i < 8; i++)
  {
    sum[0] += verts[i * 3 + 0];
    sum[1] += verts[i * 3 + 1];
    sum[2] += verts[i * 3 + 2];
  }
  this->PickPosition[0] = sum[0] / 8.0;
  this->PickPosition[1] = sum[1] / 8.0;
  this->PickPosition[2] = sum[2] / 8.0;

  this->FrustumExtractor->CreateFrustum(verts);
}

//--------------------------------------------------------------------------
// Decides which props are within the frustum.
// Adds each to the prop3d list and fires pick events.
// Remembers the dataset, mapper, and assembly path for the nearest.
int svtkAreaPicker::PickProps(svtkRenderer* renderer)
{
  svtkProp* prop;
  int pickable;
  double bounds[6];

  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  if (renderer == nullptr)
  {
    svtkErrorMacro(<< "Must specify renderer!");
    return 0;
  }

  //  Loop over all props.
  //
  svtkPropCollection* props;
  svtkProp* propCandidate;
  if (this->PickFromList)
  {
    props = this->GetPickList();
  }
  else
  {
    props = renderer->GetViewProps();
  }

  svtkAbstractMapper3D* mapper = nullptr;
  svtkAssemblyPath* path;

  double mindist = SVTK_DOUBLE_MAX;

  svtkCollectionSimpleIterator pit;
  for (props->InitTraversal(pit); (prop = props->GetNextProp(pit));)
  {
    for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
    {
      propCandidate = path->GetLastNode()->GetViewProp();
      pickable = this->TypeDecipher(propCandidate, &mapper);

      //  If actor can be picked, see if it is within the pick frustum.
      if (pickable)
      {
        if (mapper)
        {
          propCandidate->PokeMatrix(path->GetLastNode()->GetMatrix());
          const double* bds = propCandidate->GetBounds();
          propCandidate->PokeMatrix(nullptr);
          for (int i = 0; i < 6; i++)
          {
            bounds[i] = bds[i];
          }

          double dist;
          if (this->ABoxFrustumIsect(bounds, dist))
          {
            if (!this->Prop3Ds->IsItemPresent(prop))
            {
              this->Prop3Ds->AddItem(static_cast<svtkProp3D*>(prop));
              if (dist < mindist) // new nearest, remember it
              {
                mindist = dist;
                this->SetPath(path);
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
              }
            }
          }
        } // mapper
      }   // pickable

    } // for all parts
  }   // for all props

  int picked = 0;

  if (this->Path)
  {
    // Invoke pick method if one defined - prop goes first
    this->Path->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(svtkCommand::PickEvent, nullptr);
    picked = 1;
  }

  // Invoke end pick method if defined
  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);

  return picked;
}

//------------------------------------------------------------------------------
// converts the propCandidate into a svtkAbstractMapper3D
// and returns its pickability
int svtkAreaPicker::TypeDecipher(svtkProp* propCandidate, svtkAbstractMapper3D** mapper)
{
  int pickable = 0;
  *mapper = nullptr;

  svtkActor* actor;
  svtkLODProp3D* prop3D;
  svtkProperty* tempProperty;
  svtkVolume* volume;
  svtkImageSlice* imageSlice;

  if (propCandidate->GetPickable() && propCandidate->GetVisibility())
  {
    pickable = 1;
    if ((actor = svtkActor::SafeDownCast(propCandidate)) != nullptr)
    {
      *mapper = actor->GetMapper();
      if (actor->GetProperty()->GetOpacity() <= 0.0)
      {
        pickable = 0;
      }
    }
    else if ((prop3D = svtkLODProp3D::SafeDownCast(propCandidate)) != nullptr)
    {
      int LODId = prop3D->GetPickLODID();
      *mapper = prop3D->GetLODMapper(LODId);
      if (svtkMapper::SafeDownCast(*mapper) != nullptr)
      {
        prop3D->GetLODProperty(LODId, &tempProperty);
        if (tempProperty->GetOpacity() <= 0.0)
        {
          pickable = 0;
        }
      }
    }
    else if ((volume = svtkVolume::SafeDownCast(propCandidate)) != nullptr)
    {
      *mapper = volume->GetMapper();
    }
    else if ((imageSlice = svtkImageSlice::SafeDownCast(propCandidate)) != nullptr)
    {
      *mapper = imageSlice->GetMapper();
    }
    else
    {
      pickable = 0; // only svtkProp3D's (actors and volumes) can be picked
    }
  }
  return pickable;
}

//--------------------------------------------------------------------------
// Intersect the bbox represented by the bounds with the clipping frustum.
// Return true if partially inside.
// Also return a distance to the near plane.
int svtkAreaPicker::ABoxFrustumIsect(double* bounds, double& mindist)
{
  if (bounds[0] > bounds[1] || bounds[2] > bounds[3] || bounds[4] > bounds[5])
  {
    return 0;
  }

  double verts[8][3];
  int x, y, z;
  int vid = 0;
  for (x = 0; x < 2; x++)
  {
    for (y = 0; y < 2; y++)
    {
      for (z = 0; z < 2; z++)
      {
        verts[vid][0] = bounds[0 + x];
        verts[vid][1] = bounds[2 + y];
        verts[vid][2] = bounds[4 + z];
        vid++;
      }
    }
  }

  // find distance to the corner nearest the near plane for 'closest' prop
  mindist = -SVTK_DOUBLE_MAX;
  svtkPlane* plane = this->Frustum->GetPlane(4); // near plane
  for (vid = 0; vid < 8; vid++)
  {
    double dist = plane->EvaluateFunction(verts[vid]);
    if (dist < 0 && dist > mindist)
    {
      mindist = dist;
    }
  }
  mindist = -mindist;

  // leave the intersection test to the frustum extractor class
  return this->FrustumExtractor->OverallBoundsTest(bounds);
}

//--------------------------------------------------------------------------
void svtkAreaPicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Frustum: " << this->Frustum << "\n";
  os << indent << "ClipPoints: " << this->ClipPoints << "\n";
  os << indent << "Mapper: " << this->Mapper << "\n";
  os << indent << "DataSet: " << this->DataSet << "\n";
}
