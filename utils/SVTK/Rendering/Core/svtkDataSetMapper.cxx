/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataSetMapper.h"

#include "svtkDataSet.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkExecutive.h"
#include "svtkGarbageCollector.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkScalarsToColors.h"

svtkStandardNewMacro(svtkDataSetMapper);

//----------------------------------------------------------------------------
svtkDataSetMapper::svtkDataSetMapper()
{
  this->GeometryExtractor = nullptr;
  this->PolyDataMapper = nullptr;
}

//----------------------------------------------------------------------------
svtkDataSetMapper::~svtkDataSetMapper()
{
  // delete internally created objects.
  if (this->GeometryExtractor)
  {
    this->GeometryExtractor->Delete();
  }
  if (this->PolyDataMapper)
  {
    this->PolyDataMapper->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkDataSetMapper::SetInputData(svtkDataSet* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
svtkDataSet* svtkDataSetMapper::GetInput()
{
  return this->Superclass::GetInputAsDataSet();
}

//----------------------------------------------------------------------------
void svtkDataSetMapper::ReleaseGraphicsResources(svtkWindow* renWin)
{
  if (this->PolyDataMapper)
  {
    this->PolyDataMapper->ReleaseGraphicsResources(renWin);
  }
}

//----------------------------------------------------------------------------
// Receives from Actor -> maps data to primitives
//
void svtkDataSetMapper::Render(svtkRenderer* ren, svtkActor* act)
{
  // make sure that we've been properly initialized
  //
  if (!this->GetInput())
  {
    svtkErrorMacro(<< "No input!\n");
    return;
  }

  // Need a lookup table
  //
  if (this->LookupTable == nullptr)
  {
    this->CreateDefaultLookupTable();
  }
  this->LookupTable->Build();

  // Now can create appropriate mapper
  //
  if (this->PolyDataMapper == nullptr)
  {
    svtkDataSetSurfaceFilter* gf = svtkDataSetSurfaceFilter::New();
    svtkPolyDataMapper* pm = svtkPolyDataMapper::New();
    pm->SetInputConnection(gf->GetOutputPort());

    this->GeometryExtractor = gf;
    this->PolyDataMapper = pm;
  }

  // share clipping planes with the PolyDataMapper
  //
  if (this->ClippingPlanes != this->PolyDataMapper->GetClippingPlanes())
  {
    this->PolyDataMapper->SetClippingPlanes(this->ClippingPlanes);
  }

  // For efficiency: if input type is svtkPolyData, there's no need to
  // pass it through the geometry filter.
  //
  if (this->GetInput()->GetDataObjectType() == SVTK_POLY_DATA)
  {
    this->PolyDataMapper->SetInputConnection(this->GetInputConnection(0, 0));
  }
  else
  {
    this->GeometryExtractor->SetInputData(this->GetInput());
    this->PolyDataMapper->SetInputConnection(this->GeometryExtractor->GetOutputPort());
  }

  // update ourselves in case something has changed
  this->PolyDataMapper->SetLookupTable(this->GetLookupTable());
  this->PolyDataMapper->SetScalarVisibility(this->GetScalarVisibility());
  this->PolyDataMapper->SetUseLookupTableScalarRange(this->GetUseLookupTableScalarRange());
  this->PolyDataMapper->SetScalarRange(this->GetScalarRange());

  this->PolyDataMapper->SetColorMode(this->GetColorMode());
  this->PolyDataMapper->SetInterpolateScalarsBeforeMapping(
    this->GetInterpolateScalarsBeforeMapping());

  double f, u;
  this->GetRelativeCoincidentTopologyPolygonOffsetParameters(f, u);
  this->PolyDataMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(f, u);
  this->GetRelativeCoincidentTopologyLineOffsetParameters(f, u);
  this->PolyDataMapper->SetRelativeCoincidentTopologyLineOffsetParameters(f, u);
  this->GetRelativeCoincidentTopologyPointOffsetParameter(u);
  this->PolyDataMapper->SetRelativeCoincidentTopologyPointOffsetParameter(u);

  this->PolyDataMapper->SetScalarMode(this->GetScalarMode());
  if (this->ScalarMode == SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
    this->ScalarMode == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    if (this->ArrayAccessMode == SVTK_GET_ARRAY_BY_ID)
    {
      this->PolyDataMapper->ColorByArrayComponent(this->ArrayId, ArrayComponent);
    }
    else
    {
      this->PolyDataMapper->ColorByArrayComponent(this->ArrayName, ArrayComponent);
    }
  }

  this->PolyDataMapper->Render(ren, act);
  this->TimeToDraw = this->PolyDataMapper->GetTimeToDraw();
}

//----------------------------------------------------------------------------
void svtkDataSetMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->PolyDataMapper)
  {
    os << indent << "Poly Mapper: (" << this->PolyDataMapper << ")\n";
  }
  else
  {
    os << indent << "Poly Mapper: (none)\n";
  }

  if (this->GeometryExtractor)
  {
    os << indent << "Geometry Extractor: (" << this->GeometryExtractor << ")\n";
  }
  else
  {
    os << indent << "Geometry Extractor: (none)\n";
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkDataSetMapper::GetMTime()
{
  svtkMTimeType mTime = this->svtkMapper::GetMTime();
  svtkMTimeType time;

  if (this->LookupTable != nullptr)
  {
    time = this->LookupTable->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int svtkDataSetMapper::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkDataSetMapper::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  svtkGarbageCollectorReport(collector, this->GeometryExtractor, "GeometryExtractor");
  svtkGarbageCollectorReport(collector, this->PolyDataMapper, "PolyDataMapper");
}
