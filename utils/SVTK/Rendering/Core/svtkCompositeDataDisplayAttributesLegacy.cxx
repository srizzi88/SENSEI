/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataDisplayAttributesLegacy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCompositeDataDisplayAttributesLegacy.h"

#include "svtkBoundingBox.h"
#include "svtkDataSet.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkCompositeDataDisplayAttributesLegacy);

svtkCompositeDataDisplayAttributesLegacy::svtkCompositeDataDisplayAttributesLegacy() = default;

svtkCompositeDataDisplayAttributesLegacy::~svtkCompositeDataDisplayAttributesLegacy() = default;

void svtkCompositeDataDisplayAttributesLegacy::SetBlockVisibility(
  unsigned int flat_index, bool visible)
{
  this->BlockVisibilities[flat_index] = visible;
}

bool svtkCompositeDataDisplayAttributesLegacy::GetBlockVisibility(unsigned int flat_index) const
{
  std::map<unsigned int, bool>::const_iterator iter = this->BlockVisibilities.find(flat_index);
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

bool svtkCompositeDataDisplayAttributesLegacy::HasBlockVisibilities() const
{
  return !this->BlockVisibilities.empty();
}

bool svtkCompositeDataDisplayAttributesLegacy::HasBlockVisibility(unsigned int flat_index) const
{
  return this->BlockVisibilities.count(flat_index) == size_t(1);
}

void svtkCompositeDataDisplayAttributesLegacy::RemoveBlockVisibility(unsigned int flat_index)
{
  this->BlockVisibilities.erase(flat_index);
}

void svtkCompositeDataDisplayAttributesLegacy::RemoveBlockVisibilities()
{
  this->BlockVisibilities.clear();
}

#ifndef SVTK_LEGACY_REMOVE
void svtkCompositeDataDisplayAttributesLegacy::RemoveBlockVisibilites()
{
  this->RemoveBlockVisibilities();
}
#endif

void svtkCompositeDataDisplayAttributesLegacy::SetBlockPickability(
  unsigned int flat_index, bool visible)
{
  this->BlockPickabilities[flat_index] = visible;
}

bool svtkCompositeDataDisplayAttributesLegacy::GetBlockPickability(unsigned int flat_index) const
{
  std::map<unsigned int, bool>::const_iterator iter = this->BlockPickabilities.find(flat_index);
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

bool svtkCompositeDataDisplayAttributesLegacy::HasBlockPickabilities() const
{
  return !this->BlockPickabilities.empty();
}

bool svtkCompositeDataDisplayAttributesLegacy::HasBlockPickability(unsigned int flat_index) const
{
  return this->BlockPickabilities.count(flat_index) == size_t(1);
}

void svtkCompositeDataDisplayAttributesLegacy::RemoveBlockPickability(unsigned int flat_index)
{
  this->BlockPickabilities.erase(flat_index);
}

void svtkCompositeDataDisplayAttributesLegacy::RemoveBlockPickabilities()
{
  this->BlockPickabilities.clear();
}

void svtkCompositeDataDisplayAttributesLegacy::SetBlockColor(
  unsigned int flat_index, const double color[3])
{
  this->BlockColors[flat_index] = svtkColor3d(color[0], color[1], color[2]);
}

void svtkCompositeDataDisplayAttributesLegacy::GetBlockColor(
  unsigned int flat_index, double color[3]) const
{
  std::map<unsigned int, svtkColor3d>::const_iterator iter = this->BlockColors.find(flat_index);
  if (iter != this->BlockColors.end())
  {
    std::copy(&iter->second[0], &iter->second[3], color);
  }
}

svtkColor3d svtkCompositeDataDisplayAttributesLegacy::GetBlockColor(unsigned int flat_index) const
{
  std::map<unsigned int, svtkColor3d>::const_iterator iter = this->BlockColors.find(flat_index);
  if (iter != this->BlockColors.end())
  {
    return iter->second;
  }
  return svtkColor3d();
}

bool svtkCompositeDataDisplayAttributesLegacy::HasBlockColors() const
{
  return !this->BlockColors.empty();
}

bool svtkCompositeDataDisplayAttributesLegacy::HasBlockColor(unsigned int flat_index) const
{
  return this->BlockColors.count(flat_index) == size_t(1);
}

void svtkCompositeDataDisplayAttributesLegacy::RemoveBlockColor(unsigned int flat_index)
{
  this->BlockColors.erase(flat_index);
}

void svtkCompositeDataDisplayAttributesLegacy::RemoveBlockColors()
{
  this->BlockColors.clear();
}

void svtkCompositeDataDisplayAttributesLegacy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkCompositeDataDisplayAttributesLegacy::SetBlockOpacity(
  unsigned int flat_index, double opacity)
{
  this->BlockOpacities[flat_index] = opacity;
}

double svtkCompositeDataDisplayAttributesLegacy::GetBlockOpacity(unsigned int flat_index) const
{
  std::map<unsigned int, double>::const_iterator iter = this->BlockOpacities.find(flat_index);

  if (iter != this->BlockOpacities.end())
  {
    return iter->second;
  }

  return 0;
}

bool svtkCompositeDataDisplayAttributesLegacy::HasBlockOpacities() const
{
  return !this->BlockOpacities.empty();
}

bool svtkCompositeDataDisplayAttributesLegacy::HasBlockOpacity(unsigned int flat_index) const
{
  return this->BlockOpacities.find(flat_index) != this->BlockOpacities.end();
}

void svtkCompositeDataDisplayAttributesLegacy::RemoveBlockOpacity(unsigned int flat_index)
{
  this->BlockOpacities.erase(flat_index);
}

void svtkCompositeDataDisplayAttributesLegacy::RemoveBlockOpacities()
{
  this->BlockOpacities.clear();
}

void svtkCompositeDataDisplayAttributesLegacy::ComputeVisibleBounds(
  svtkCompositeDataDisplayAttributesLegacy* cda, svtkDataObject* dobj, double bounds[6])
{
  svtkMath::UninitializeBounds(bounds);
  // computing bounds with only visible blocks
  svtkBoundingBox bbox;
  unsigned int flat_index = 0;
  svtkCompositeDataDisplayAttributesLegacy::ComputeVisibleBoundsInternal(
    cda, dobj, flat_index, &bbox);
  if (bbox.IsValid())
  {
    bbox.GetBounds(bounds);
  }
}

void svtkCompositeDataDisplayAttributesLegacy::ComputeVisibleBoundsInternal(
  svtkCompositeDataDisplayAttributesLegacy* cda, svtkDataObject* dobj, unsigned int& flat_index,
  svtkBoundingBox* bbox, bool parentVisible)
{
  if (!dobj || !bbox)
  {
    return;
  }

  // A block always *has* a visibility state, either explicitly set or inherited.
  bool blockVisible = (cda && cda->HasBlockVisibility(flat_index))
    ? cda->GetBlockVisibility(flat_index)
    : parentVisible;

  // Advance flat-index. After this point, flat_index no longer points to this block.
  flat_index++;

  svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::SafeDownCast(dobj);
  svtkMultiPieceDataSet* mpds = svtkMultiPieceDataSet::SafeDownCast(dobj);
  if (mbds || mpds)
  {
    unsigned int numChildren = mbds ? mbds->GetNumberOfBlocks() : mpds->GetNumberOfPieces();
    for (unsigned int cc = 0; cc < numChildren; cc++)
    {
      svtkDataObject* child = mbds ? mbds->GetBlock(cc) : mpds->GetPiece(cc);
      if (child == nullptr)
      {
        // speeds things up when dealing with nullptr blocks (which is common with AMRs).
        flat_index++;
        continue;
      }
      svtkCompositeDataDisplayAttributesLegacy::ComputeVisibleBoundsInternal(
        cda, child, flat_index, bbox, blockVisible);
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
