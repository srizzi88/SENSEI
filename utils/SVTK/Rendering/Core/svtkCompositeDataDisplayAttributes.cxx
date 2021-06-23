/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataDisplayAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkBoundingBox.h"
#include "svtkDataObjectTree.h"
#include "svtkDataObjectTreeRange.h"
#include "svtkDataSet.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkCompositeDataDisplayAttributes);

svtkCompositeDataDisplayAttributes::svtkCompositeDataDisplayAttributes() = default;

svtkCompositeDataDisplayAttributes::~svtkCompositeDataDisplayAttributes() = default;

void svtkCompositeDataDisplayAttributes::SetBlockVisibility(svtkDataObject* data_object, bool visible)
{
  if (this->HasBlockVisibility(data_object) && this->GetBlockVisibility(data_object) == visible)
  {
    return;
  }
  this->BlockVisibilities[data_object] = visible;
  this->Modified();
}

bool svtkCompositeDataDisplayAttributes::GetBlockVisibility(svtkDataObject* data_object) const
{
  BoolMap::const_iterator iter = this->BlockVisibilities.find(data_object);
  if (iter != this->BlockVisibilities.end())
  {
    return iter->second;
  }
  else
  {
    // default to true
    return true;
  }
}

bool svtkCompositeDataDisplayAttributes::HasBlockVisibilities() const
{
  return !this->BlockVisibilities.empty();
}

bool svtkCompositeDataDisplayAttributes::HasBlockVisibility(svtkDataObject* data_object) const
{
  return this->BlockVisibilities.count(data_object) == size_t(1);
}

void svtkCompositeDataDisplayAttributes::RemoveBlockVisibility(svtkDataObject* data_object)
{
  this->BlockVisibilities.erase(data_object);
}

void svtkCompositeDataDisplayAttributes::RemoveBlockVisibilities()
{
  if (this->HasBlockVisibilities())
  {
    this->Modified();
  }
  this->BlockVisibilities.clear();
}

#ifndef SVTK_LEGACY_REMOVE
void svtkCompositeDataDisplayAttributes::RemoveBlockVisibilites()
{
  SVTK_LEGACY_REPLACED_BODY(svtkCompositeDataDisplayAttributes::RemoveBlockVisibilites, "SVTK 8.1",
    svtkCompositeDataDisplayAttributes::RemoveBlockVisibilities());
  this->RemoveBlockVisibilities();
}
#endif

void svtkCompositeDataDisplayAttributes::SetBlockPickability(
  svtkDataObject* data_object, bool visible)
{
  if (this->HasBlockPickability(data_object) && this->GetBlockPickability(data_object) == visible)
  {
    return;
  }
  this->BlockPickabilities[data_object] = visible;
  this->Modified();
}

bool svtkCompositeDataDisplayAttributes::GetBlockPickability(svtkDataObject* data_object) const
{
  BoolMap::const_iterator iter = this->BlockPickabilities.find(data_object);
  if (iter != this->BlockPickabilities.end())
  {
    return iter->second;
  }
  else
  {
    // default to true
    return true;
  }
}

bool svtkCompositeDataDisplayAttributes::HasBlockPickabilities() const
{
  return !this->BlockPickabilities.empty();
}

bool svtkCompositeDataDisplayAttributes::HasBlockPickability(svtkDataObject* data_object) const
{
  return this->BlockPickabilities.count(data_object) == size_t(1);
}

void svtkCompositeDataDisplayAttributes::RemoveBlockPickability(svtkDataObject* data_object)
{
  this->BlockPickabilities.erase(data_object);
}

void svtkCompositeDataDisplayAttributes::RemoveBlockPickabilities()
{
  if (this->HasBlockPickabilities())
  {
    this->Modified();
  }
  this->BlockPickabilities.clear();
}

void svtkCompositeDataDisplayAttributes::SetBlockColor(
  svtkDataObject* data_object, const double color[3])
{
  if (this->HasBlockColor(data_object))
  {
    double currentColor[3];
    this->GetBlockColor(data_object, currentColor);
    if (color[0] == currentColor[0] && color[1] == currentColor[1] && color[2] == currentColor[2])
    {
      return;
    }
  }
  this->BlockColors[data_object] = svtkColor3d(color[0], color[1], color[2]);
  this->Modified();
}

void svtkCompositeDataDisplayAttributes::GetBlockColor(
  svtkDataObject* data_object, double color[3]) const
{
  ColorMap::const_iterator iter = this->BlockColors.find(data_object);
  if (iter != this->BlockColors.end())
  {
    std::copy(&iter->second[0], &iter->second[3], color);
  }
}

svtkColor3d svtkCompositeDataDisplayAttributes::GetBlockColor(svtkDataObject* data_object) const
{
  ColorMap::const_iterator iter = this->BlockColors.find(data_object);
  if (iter != this->BlockColors.end())
  {
    return iter->second;
  }
  return svtkColor3d();
}

bool svtkCompositeDataDisplayAttributes::HasBlockColors() const
{
  return !this->BlockColors.empty();
}

bool svtkCompositeDataDisplayAttributes::HasBlockColor(svtkDataObject* data_object) const
{
  return this->BlockColors.count(data_object) == size_t(1);
}

void svtkCompositeDataDisplayAttributes::RemoveBlockColor(svtkDataObject* data_object)
{
  this->BlockColors.erase(data_object);
}

void svtkCompositeDataDisplayAttributes::RemoveBlockColors()
{
  if (this->HasBlockColors())
  {
    this->Modified();
  }
  this->BlockColors.clear();
}

void svtkCompositeDataDisplayAttributes::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkCompositeDataDisplayAttributes::SetBlockOpacity(svtkDataObject* data_object, double opacity)
{
  if (this->HasBlockOpacity(data_object) && this->GetBlockOpacity(data_object) == opacity)
  {
    return;
  }
  this->BlockOpacities[data_object] = opacity;
  this->Modified();
}

double svtkCompositeDataDisplayAttributes::GetBlockOpacity(svtkDataObject* data_object) const
{
  DoubleMap::const_iterator iter = this->BlockOpacities.find(data_object);

  if (iter != this->BlockOpacities.end())
  {
    return iter->second;
  }

  return 0;
}

bool svtkCompositeDataDisplayAttributes::HasBlockOpacities() const
{
  return !this->BlockOpacities.empty();
}

bool svtkCompositeDataDisplayAttributes::HasBlockOpacity(svtkDataObject* data_object) const
{
  return this->BlockOpacities.find(data_object) != this->BlockOpacities.end();
}

void svtkCompositeDataDisplayAttributes::RemoveBlockOpacity(svtkDataObject* data_object)
{
  this->BlockOpacities.erase(data_object);
}

void svtkCompositeDataDisplayAttributes::RemoveBlockOpacities()
{
  if (this->HasBlockOpacities())
  {
    this->Modified();
  }
  this->BlockOpacities.clear();
}

void svtkCompositeDataDisplayAttributes::SetBlockMaterial(
  svtkDataObject* data_object, const std::string& material)
{
  if (this->HasBlockMaterial(data_object) && this->GetBlockMaterial(data_object) == material)
  {
    return;
  }
  this->BlockMaterials[data_object] = material;
  this->Modified();
}

const std::string& svtkCompositeDataDisplayAttributes::GetBlockMaterial(
  svtkDataObject* data_object) const
{
  StringMap::const_iterator iter = this->BlockMaterials.find(data_object);

  if (iter != this->BlockMaterials.end())
  {
    return iter->second;
  }

  static const std::string nomat;
  return nomat;
}

bool svtkCompositeDataDisplayAttributes::HasBlockMaterials() const
{
  return !this->BlockMaterials.empty();
}

bool svtkCompositeDataDisplayAttributes::HasBlockMaterial(svtkDataObject* data_object) const
{
  return this->BlockMaterials.find(data_object) != this->BlockMaterials.end();
}

void svtkCompositeDataDisplayAttributes::RemoveBlockMaterial(svtkDataObject* data_object)
{
  this->BlockMaterials.erase(data_object);
}

void svtkCompositeDataDisplayAttributes::RemoveBlockMaterials()
{
  if (this->HasBlockMaterials())
  {
    this->Modified();
  }
  this->BlockMaterials.clear();
}

void svtkCompositeDataDisplayAttributes::ComputeVisibleBounds(
  svtkCompositeDataDisplayAttributes* cda, svtkDataObject* dobj, double bounds[6])
{
  svtkMath::UninitializeBounds(bounds);
  // computing bounds with only visible blocks
  svtkBoundingBox bbox;
  svtkCompositeDataDisplayAttributes::ComputeVisibleBoundsInternal(cda, dobj, &bbox);
  if (bbox.IsValid())
  {
    bbox.GetBounds(bounds);
  }
}

void svtkCompositeDataDisplayAttributes::ComputeVisibleBoundsInternal(
  svtkCompositeDataDisplayAttributes* cda, svtkDataObject* dobj, svtkBoundingBox* bbox,
  bool parentVisible)
{
  if (!dobj || !bbox)
  {
    return;
  }

  // A block always *has* a visibility state, either explicitly set or inherited.
  bool blockVisible =
    (cda && cda->HasBlockVisibility(dobj)) ? cda->GetBlockVisibility(dobj) : parentVisible;

  svtkDataObjectTree* dObjTree = svtkDataObjectTree::SafeDownCast(dobj);
  if (dObjTree)
  {
    using Opts = svtk::DataObjectTreeOptions;
    for (svtkDataObject* child : svtk::Range(dObjTree, Opts::SkipEmptyNodes))
    {
      svtkCompositeDataDisplayAttributes::ComputeVisibleBoundsInternal(
        cda, child, bbox, blockVisible);
    }
  }
  else if (dobj && blockVisible == true)
  {
    svtkDataSet* ds = svtkDataSet::SafeDownCast(dobj);
    if (ds)
    {
      double bounds[6];
      ds->GetBounds(bounds);
      bbox->AddBounds(bounds);
    }
  }
}

svtkDataObject* svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
  const unsigned int flat_index, svtkDataObject* parent_obj, unsigned int& current_flat_index)
{
  if (current_flat_index == flat_index)
  {
    return parent_obj;
  }
  current_flat_index++;

  // for leaf types quick continue, otherwise it recurses which
  // calls two more SafeDownCast which are expensive
  int dotype = parent_obj->GetDataObjectType();
  if (dotype < SVTK_COMPOSITE_DATA_SET) // see svtkType.h
  {
    return nullptr;
  }

  svtkDataObjectTree* dObjTree = svtkDataObjectTree::SafeDownCast(parent_obj);
  if (dObjTree)
  {
    using Opts = svtk::DataObjectTreeOptions;
    for (svtkDataObject* child : svtk::Range(dObjTree, Opts::None))
    {
      if (child)
      {
        const auto data = svtkCompositeDataDisplayAttributes::DataObjectFromIndex(
          flat_index, child, current_flat_index);
        if (data)
        {
          return data;
        }
      }
      else
      {
        ++current_flat_index;
      }
    }
  }

  return nullptr;
}
