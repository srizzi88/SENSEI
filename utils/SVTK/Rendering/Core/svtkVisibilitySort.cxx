/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVisibilitySort.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "svtkVisibilitySort.h"

#include "svtkCamera.h"
#include "svtkDataSet.h"
#include "svtkGarbageCollector.h"
#include "svtkIdList.h"
#include "svtkMatrix4x4.h"

//-----------------------------------------------------------------------------

svtkCxxSetObjectMacro(svtkVisibilitySort, Camera, svtkCamera);
svtkCxxSetObjectMacro(svtkVisibilitySort, Input, svtkDataSet);

//-----------------------------------------------------------------------------

svtkVisibilitySort::svtkVisibilitySort()
{
  this->ModelTransform = svtkMatrix4x4::New();
  this->ModelTransform->Identity();
  this->InverseModelTransform = svtkMatrix4x4::New();
  this->InverseModelTransform->Identity();

  this->Camera = nullptr;
  this->Input = nullptr;

  this->Direction = svtkVisibilitySort::BACK_TO_FRONT;

  this->MaxCellsReturned = SVTK_INT_MAX;
}

//-----------------------------------------------------------------------------

svtkVisibilitySort::~svtkVisibilitySort()
{
  this->ModelTransform->Delete();
  this->InverseModelTransform->Delete();

  this->SetCamera(nullptr);
  this->SetInput(nullptr);
}

//-----------------------------------------------------------------------------

void svtkVisibilitySort::Register(svtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

void svtkVisibilitySort::UnRegister(svtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

void svtkVisibilitySort::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->Input, "Input");
}

//-----------------------------------------------------------------------------

void svtkVisibilitySort::SetModelTransform(svtkMatrix4x4* mat)
{
  // Less efficient than svtkMatrix4x4::DeepCopy, but only sets Modified if
  // there is a real change.
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      this->ModelTransform->SetElement(i, j, mat->GetElement(i, j));
    }
  }

  if (this->ModelTransform->GetMTime() > this->InverseModelTransform->GetMTime())
  {
    this->InverseModelTransform->DeepCopy(this->ModelTransform);
    this->InverseModelTransform->Invert();
  }
}

//-----------------------------------------------------------------------------

void svtkVisibilitySort::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: (" << this->Input << ")" << endl;
  os << indent << "Direction: ";
  switch (this->Direction)
  {
    case svtkVisibilitySort::BACK_TO_FRONT:
      os << "back to front" << endl;
      break;
    case svtkVisibilitySort::FRONT_TO_BACK:
      os << "front to back" << endl;
      break;
    default:
      os << "unknown" << endl;
      break;
  }

  os << indent << "MaxCellsReturned: " << this->MaxCellsReturned << endl;

  os << indent << "ModelTransform:" << endl;
  this->ModelTransform->PrintSelf(os, indent.GetNextIndent());
  os << indent << "InverseModelTransform:" << endl;
  this->InverseModelTransform->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Camera: (" << this->Camera << ")" << endl;
}
