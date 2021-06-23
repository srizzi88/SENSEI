/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkPParticleTracer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPParticleTracer.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkPParticleTracer);

svtkPParticleTracer::svtkPParticleTracer()
{
  this->IgnorePipelineTime = 0;
}

int svtkPParticleTracer::OutputParticles(svtkPolyData* poly)
{
  this->Output = poly;
  return 1;
}

void svtkPParticleTracer::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
