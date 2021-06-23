/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTrivialConsumer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkNew.h"
#include "svtkSphereSource.h"
#include "svtkTrivialConsumer.h"

int TestTrivialConsumer(int, char*[])
{
  svtkNew<svtkSphereSource> spheres;
  svtkNew<svtkTrivialConsumer> consumer;

  consumer->SetInputConnection(spheres->GetOutputPort());
  consumer->Update();

  return 0;
}
