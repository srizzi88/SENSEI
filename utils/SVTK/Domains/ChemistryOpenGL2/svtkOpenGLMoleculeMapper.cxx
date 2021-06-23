/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLMoleculeMapper.h"

#include "svtkOpenGLSphereMapper.h"
#include "svtkOpenGLStickMapper.h"

#include "svtkEventForwarderCommand.h"
#include "svtkGlyph3DMapper.h"
#include "svtkLookupTable.h"
#include "svtkMolecule.h"
#include "svtkObjectFactory.h"
#include "svtkPeriodicTable.h"
#include "svtkTrivialProducer.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLMoleculeMapper);

//----------------------------------------------------------------------------
svtkOpenGLMoleculeMapper::svtkOpenGLMoleculeMapper()
{
  // Setup glyph mappers
  this->FastAtomMapper->SetScalarRange(0, this->PeriodicTable->GetNumberOfElements());
  this->FastAtomMapper->SetColorModeToMapScalars();
  this->FastAtomMapper->SetScalarModeToUsePointFieldData();

  this->FastBondMapper->SetScalarRange(0, this->PeriodicTable->GetNumberOfElements());

  // Forward commands to instance mappers
  svtkNew<svtkEventForwarderCommand> cb;
  cb->SetTarget(this);

  this->FastAtomMapper->AddObserver(svtkCommand::StartEvent, cb);
  this->FastAtomMapper->AddObserver(svtkCommand::EndEvent, cb);
  this->FastAtomMapper->AddObserver(svtkCommand::ProgressEvent, cb);

  this->FastBondMapper->AddObserver(svtkCommand::StartEvent, cb);
  this->FastBondMapper->AddObserver(svtkCommand::EndEvent, cb);
  this->FastBondMapper->AddObserver(svtkCommand::ProgressEvent, cb);

  // Connect the trivial producers to forward the glyph polydata
  this->FastAtomMapper->SetInputConnection(this->AtomGlyphPointOutput->GetOutputPort());
  this->FastBondMapper->SetInputConnection(this->BondGlyphPointOutput->GetOutputPort());
}

svtkOpenGLMoleculeMapper::~svtkOpenGLMoleculeMapper() = default;

//----------------------------------------------------------------------------
void svtkOpenGLMoleculeMapper::Render(svtkRenderer* ren, svtkActor* act)
{
  // Update cached polydata if needed
  this->UpdateGlyphPolyData();

  // Pass rendering call on
  if (this->RenderAtoms)
  {
    this->FastAtomMapper->Render(ren, act);
  }

  if (this->RenderBonds)
  {
    this->FastBondMapper->Render(ren, act);
  }

  if (this->RenderLattice)
  {
    this->LatticeMapper->Render(ren, act);
  }
}

void svtkOpenGLMoleculeMapper::ProcessSelectorPixelBuffers(
  svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, svtkProp* prop)
{
  // forward to helper
  if (this->RenderAtoms)
  {
    this->FastAtomMapper->ProcessSelectorPixelBuffers(sel, pixeloffsets, prop);
  }

  if (this->RenderBonds)
  {
    this->FastBondMapper->ProcessSelectorPixelBuffers(sel, pixeloffsets, prop);
  }

  if (this->RenderLattice)
  {
    this->LatticeMapper->ProcessSelectorPixelBuffers(sel, pixeloffsets, prop);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLMoleculeMapper::ReleaseGraphicsResources(svtkWindow* w)
{
  this->FastAtomMapper->ReleaseGraphicsResources(w);
  this->FastBondMapper->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
// Generate scale and position information for each atom sphere
void svtkOpenGLMoleculeMapper::UpdateAtomGlyphPolyData()
{
  this->Superclass::UpdateAtomGlyphPolyData();
  this->FastAtomMapper->SetScalarMode(this->AtomGlyphMapper->GetScalarMode());
  this->FastAtomMapper->SetLookupTable(this->AtomGlyphMapper->GetLookupTable());
  this->FastAtomMapper->SetScaleArray("Scale Factors");

  // Copy the color array info:
  this->FastAtomMapper->SelectColorArray(this->AtomGlyphMapper->GetArrayId());
}

//----------------------------------------------------------------------------
// Generate position, scale, and orientation vectors for each bond cylinder
void svtkOpenGLMoleculeMapper::UpdateBondGlyphPolyData()
{
  this->Superclass::UpdateBondGlyphPolyData();

  this->FastBondMapper->SetLookupTable(this->BondGlyphMapper->GetLookupTable());
  this->FastBondMapper->SetScalarMode(this->BondGlyphMapper->GetScalarMode());
  this->FastBondMapper->SetColorMode(this->BondGlyphMapper->GetColorMode());
  this->FastBondMapper->SelectColorArray(this->BondGlyphMapper->GetArrayId());
  // Setup glypher
  this->FastBondMapper->SetScaleArray("Scale Factors");
  this->FastBondMapper->SetOrientationArray("Orientation Vectors");
  this->FastBondMapper->SetSelectionIdArray("Selection Ids");
}

//----------------------------------------------------------------------------
void svtkOpenGLMoleculeMapper::SetMapScalars(bool map)
{
  this->Superclass::SetMapScalars(map);
  this->FastAtomMapper->SetColorMode(
    map ? SVTK_COLOR_MODE_MAP_SCALARS : SVTK_COLOR_MODE_DIRECT_SCALARS);
  this->FastBondMapper->SetColorMode(
    map ? SVTK_COLOR_MODE_MAP_SCALARS : SVTK_COLOR_MODE_DIRECT_SCALARS);
}
