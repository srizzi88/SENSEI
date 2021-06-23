/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositePolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositePolyDataMapper.h"

#include "svtkActor.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkExecutive.h"
#include "svtkGarbageCollector.h"
#include "svtkInformation.h"
#include "svtkMapper.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"

#include <vector>

svtkStandardNewMacro(svtkCompositePolyDataMapper);

class svtkCompositePolyDataMapperInternals
{
public:
  std::vector<svtkPolyDataMapper*> Mappers;
};

svtkCompositePolyDataMapper::svtkCompositePolyDataMapper()
{
  this->Internal = new svtkCompositePolyDataMapperInternals;
}

svtkCompositePolyDataMapper::~svtkCompositePolyDataMapper()
{
  for (unsigned int i = 0; i < this->Internal->Mappers.size(); i++)
  {
    this->Internal->Mappers[i]->UnRegister(this);
  }
  this->Internal->Mappers.clear();

  delete this->Internal;
}

// Specify the type of data this mapper can handle. If we are
// working with a regular (not hierarchical) pipeline, then we
// need svtkPolyData. For composite data pipelines, then
// svtkCompositeDataSet is required, and we'll check when
// building our structure whether all the part of the composite
// data set are polydata.
int svtkCompositePolyDataMapper::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

// When the structure is out-of-date, recreate it by
// creating a mapper for each input data.
void svtkCompositePolyDataMapper::BuildPolyDataMapper()
{
  int warnOnce = 0;

  // Delete pdmappers if they already exist.
  for (unsigned int i = 0; i < this->Internal->Mappers.size(); i++)
  {
    this->Internal->Mappers[i]->UnRegister(this);
  }
  this->Internal->Mappers.clear();

  // Get the composite dataset from the input
  svtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  svtkCompositeDataSet* input =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // If it isn't hierarchical, maybe it is just a plain svtkPolyData
  if (!input)
  {
    svtkPolyData* pd = svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
    if (pd)
    {
      // Make a copy of the data to break the pipeline here
      svtkPolyData* newpd = svtkPolyData::New();
      newpd->ShallowCopy(pd);
      svtkPolyDataMapper* pdmapper = this->MakeAMapper();
      pdmapper->Register(this);
      pdmapper->SetInputData(newpd);
      this->Internal->Mappers.push_back(pdmapper);
      newpd->Delete();
      pdmapper->Delete();
    }
    else
    {
      svtkDataObject* tmpInp = this->GetExecutive()->GetInputData(0, 0);
      svtkErrorMacro("This mapper cannot handle input of type: " << (tmpInp ? tmpInp->GetClassName()
                                                                           : "(none)"));
    }
  }
  else
  {
    // for each data set build a svtkPolyDataMapper
    svtkCompositeDataIterator* iter = input->NewIterator();
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
    {
      svtkPolyData* pd = svtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        // Make a copy of the data to break the pipeline here
        svtkPolyData* newpd = svtkPolyData::New();
        newpd->ShallowCopy(pd);
        svtkPolyDataMapper* pdmapper = this->MakeAMapper();
        pdmapper->Register(this);
        pdmapper->SetInputData(newpd);
        this->Internal->Mappers.push_back(pdmapper);
        newpd->Delete();
        pdmapper->Delete();
      }
      // This is not polydata - warn the user that there are non-polydata
      // parts to this data set which will not be rendered by this mapper
      else
      {
        if (!warnOnce)
        {
          svtkErrorMacro("All data in the hierarchical dataset must be polydata.");
          warnOnce = 1;
        }
      }
      iter->GoToNextItem();
    }
    iter->Delete();
  }

  this->InternalMappersBuildTime.Modified();
}

void svtkCompositePolyDataMapper::Render(svtkRenderer* ren, svtkActor* a)
{
  // If the PolyDataMappers are not up-to-date then rebuild them
  svtkCompositeDataPipeline* executive =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());

  if (executive->GetPipelineMTime() > this->InternalMappersBuildTime.GetMTime())
  {
    this->BuildPolyDataMapper();
  }

  this->TimeToDraw = 0;
  // Call Render() on each of the PolyDataMappers
  for (unsigned int i = 0; i < this->Internal->Mappers.size(); i++)
  {
    // skip if we have a mismatch in opaque and translucent
    if (a->IsRenderingTranslucentPolygonalGeometry() ==
      this->Internal->Mappers[i]->HasOpaqueGeometry())
    {
      continue;
    }

    if (this->ClippingPlanes != this->Internal->Mappers[i]->GetClippingPlanes())
    {
      this->Internal->Mappers[i]->SetClippingPlanes(this->ClippingPlanes);
    }

    this->Internal->Mappers[i]->SetLookupTable(this->GetLookupTable());
    this->Internal->Mappers[i]->SetScalarVisibility(this->GetScalarVisibility());
    this->Internal->Mappers[i]->SetUseLookupTableScalarRange(this->GetUseLookupTableScalarRange());
    this->Internal->Mappers[i]->SetScalarRange(this->GetScalarRange());
    this->Internal->Mappers[i]->SetColorMode(this->GetColorMode());
    this->Internal->Mappers[i]->SetInterpolateScalarsBeforeMapping(
      this->GetInterpolateScalarsBeforeMapping());

    this->Internal->Mappers[i]->SetScalarMode(this->GetScalarMode());
    if (this->ScalarMode == SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
      this->ScalarMode == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
      if (this->ArrayAccessMode == SVTK_GET_ARRAY_BY_ID)
      {
        this->Internal->Mappers[i]->ColorByArrayComponent(this->ArrayId, ArrayComponent);
      }
      else
      {
        this->Internal->Mappers[i]->ColorByArrayComponent(this->ArrayName, ArrayComponent);
      }
    }

    this->Internal->Mappers[i]->Render(ren, a);
    this->TimeToDraw += this->Internal->Mappers[i]->GetTimeToDraw();
  }
}
svtkExecutive* svtkCompositePolyDataMapper::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

// Looks at each DataSet and finds the union of all the bounds
void svtkCompositePolyDataMapper::ComputeBounds()
{
  svtkMath::UninitializeBounds(this->Bounds);

  svtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  svtkCompositeDataSet* input =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if (!input)
  {
    svtkPolyData* pd = svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
    if (pd)
    {
      pd->GetBounds(this->Bounds);
    }
    this->BoundsMTime.Modified();
    return;
  }

  // We do have hierarchical data - so we need to loop over
  // it and get the total bounds.
  svtkCompositeDataIterator* iter = input->NewIterator();
  iter->GoToFirstItem();
  double bounds[6];
  int i;

  while (!iter->IsDoneWithTraversal())
  {
    svtkPolyData* pd = svtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
    if (pd)
    {
      // If this isn't the first time through, expand bounds
      // we've compute so far based on the bounds of this
      // block
      if (svtkMath::AreBoundsInitialized(this->Bounds))
      {
        pd->GetBounds(bounds);
        if (svtkMath::AreBoundsInitialized(bounds))
        {
          for (i = 0; i < 3; i++)
          {
            this->Bounds[i * 2] =
              (bounds[i * 2] < this->Bounds[i * 2]) ? (bounds[i * 2]) : (this->Bounds[i * 2]);
            this->Bounds[i * 2 + 1] = (bounds[i * 2 + 1] > this->Bounds[i * 2 + 1])
              ? (bounds[i * 2 + 1])
              : (this->Bounds[i * 2 + 1]);
          }
        }
      }
      // If this is our first time through, just get the bounds
      // of the data as the initial bounds
      else
      {
        pd->GetBounds(this->Bounds);
      }
    }
    iter->GoToNextItem();
  }
  iter->Delete();
  this->BoundsMTime.Modified();
}

double* svtkCompositePolyDataMapper::GetBounds()
{
  if (!this->GetExecutive()->GetInputData(0, 0))
  {
    svtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  else
  {
    this->Update();

    // only compute bounds when the input data has changed
    svtkCompositeDataPipeline* executive =
      svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());
    if (executive->GetPipelineMTime() > this->BoundsMTime.GetMTime())
    {
      this->ComputeBounds();
    }

    return this->Bounds;
  }
}

void svtkCompositePolyDataMapper::ReleaseGraphicsResources(svtkWindow* win)
{
  for (unsigned int i = 0; i < this->Internal->Mappers.size(); i++)
  {
    this->Internal->Mappers[i]->ReleaseGraphicsResources(win);
  }
}

void svtkCompositePolyDataMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

svtkPolyDataMapper* svtkCompositePolyDataMapper::MakeAMapper()
{
  svtkPolyDataMapper* m = svtkPolyDataMapper::New();
  // Copy our svtkMapper properties to the delegate
  m->svtkMapper::ShallowCopy(this);
  return m;
}

//-----------------------------------------------------------------------------
// look at children
bool svtkCompositePolyDataMapper::HasOpaqueGeometry()
{
  // If the PolyDataMappers are not up-to-date then rebuild them
  svtkCompositeDataPipeline* executive =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());

  if (executive->GetPipelineMTime() > this->InternalMappersBuildTime.GetMTime())
  {
    this->BuildPolyDataMapper();
  }

  bool hasOpaque = false;
  for (unsigned int i = 0; !hasOpaque && i < this->Internal->Mappers.size(); i++)
  {
    hasOpaque = hasOpaque || this->Internal->Mappers[i]->HasOpaqueGeometry();
  }
  return hasOpaque;
}

//-----------------------------------------------------------------------------
// look at children
bool svtkCompositePolyDataMapper::HasTranslucentPolygonalGeometry()
{
  // If the PolyDataMappers are not up-to-date then rebuild them
  svtkCompositeDataPipeline* executive =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());

  if (executive->GetPipelineMTime() > this->InternalMappersBuildTime.GetMTime())
  {
    this->BuildPolyDataMapper();
  }

  bool hasTrans = false;
  for (unsigned int i = 0; !hasTrans && i < this->Internal->Mappers.size(); i++)
  {
    hasTrans = hasTrans || this->Internal->Mappers[i]->HasTranslucentPolygonalGeometry();
  }
  return hasTrans;
}
