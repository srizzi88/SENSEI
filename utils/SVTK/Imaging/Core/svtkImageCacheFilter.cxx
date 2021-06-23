/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageCacheFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageCacheFilter.h"

#include "svtkCachedStreamingDemandDrivenPipeline.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkImageCacheFilter);

//----------------------------------------------------------------------------
svtkImageCacheFilter::svtkImageCacheFilter()
{
  svtkExecutive* exec = this->CreateDefaultExecutive();
  this->SetExecutive(exec);
  exec->Delete();

  this->SetCacheSize(10);
}

//----------------------------------------------------------------------------
svtkImageCacheFilter::~svtkImageCacheFilter() = default;

//----------------------------------------------------------------------------
svtkExecutive* svtkImageCacheFilter::CreateDefaultExecutive()
{
  return svtkCachedStreamingDemandDrivenPipeline::New();
}

//----------------------------------------------------------------------------
void svtkImageCacheFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CacheSize: " << this->GetCacheSize() << endl;
}

//----------------------------------------------------------------------------
void svtkImageCacheFilter::SetCacheSize(int size)
{
  svtkCachedStreamingDemandDrivenPipeline* csddp =
    svtkCachedStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (csddp)
  {
    csddp->SetCacheSize(size);
  }
}

//----------------------------------------------------------------------------
int svtkImageCacheFilter::GetCacheSize()
{
  svtkCachedStreamingDemandDrivenPipeline* csddp =
    svtkCachedStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (csddp)
  {
    return csddp->GetCacheSize();
  }
  return 0;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void svtkImageCacheFilter::ExecuteData(svtkDataObject*)
{
  // do nothing just override superclass to prevent warning
}
