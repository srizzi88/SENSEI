/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkParticleTracer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkParticleTracer.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSetGet.h"

svtkObjectFactoryNewMacro(svtkParticleTracer);

svtkParticleTracer::svtkParticleTracer()
{
  this->IgnorePipelineTime = 0;
}

int svtkParticleTracer::OutputParticles(svtkPolyData* poly)
{
  this->Output = poly;
  return 1;
}

void svtkParticleTracer::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
