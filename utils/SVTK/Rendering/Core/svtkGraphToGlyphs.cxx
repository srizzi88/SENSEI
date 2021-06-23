/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphToGlyphs.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkGraphToGlyphs.h"

#include "svtkArrayCalculator.h"
#include "svtkDirectedGraph.h"
#include "svtkDistanceToCamera.h"
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkGraph.h"
#include "svtkGraphToPoints.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSphereSource.h"
#include "svtkTable.h"
#include "svtkUndirectedGraph.h"

svtkStandardNewMacro(svtkGraphToGlyphs);

svtkGraphToGlyphs::svtkGraphToGlyphs()
{
  this->GraphToPoints = svtkSmartPointer<svtkGraphToPoints>::New();
  this->Sphere = svtkSmartPointer<svtkSphereSource>::New();
  this->GlyphSource = svtkSmartPointer<svtkGlyphSource2D>::New();
  this->DistanceToCamera = svtkSmartPointer<svtkDistanceToCamera>::New();
  this->Glyph = svtkSmartPointer<svtkGlyph3D>::New();
  this->GlyphType = CIRCLE;
  this->Filled = true;
  this->ScreenSize = 10;
  this->Sphere->SetRadius(0.5);
  this->Sphere->SetPhiResolution(8);
  this->Sphere->SetThetaResolution(8);
  this->GlyphSource->SetScale(0.5);
  this->Glyph->SetScaleModeToScaleByScalar();
  this->Glyph->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");
  this->Glyph->FillCellDataOn();
  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "scale");
}

svtkGraphToGlyphs::~svtkGraphToGlyphs() = default;

int svtkGraphToGlyphs::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

void svtkGraphToGlyphs::SetRenderer(svtkRenderer* ren)
{
  this->DistanceToCamera->SetRenderer(ren);
  this->Modified();
}

svtkRenderer* svtkGraphToGlyphs::GetRenderer()
{
  return this->DistanceToCamera->GetRenderer();
}

void svtkGraphToGlyphs::SetScaling(bool b)
{
  this->DistanceToCamera->SetScaling(b);
  this->Modified();
}

bool svtkGraphToGlyphs::GetScaling()
{
  return this->DistanceToCamera->GetScaling();
}

svtkMTimeType svtkGraphToGlyphs::GetMTime()
{
  svtkMTimeType mtime = this->Superclass::GetMTime();
  if (this->GlyphType != VERTEX && this->DistanceToCamera->GetMTime() > mtime)
  {
    mtime = this->DistanceToCamera->GetMTime();
  }
  return mtime;
}

int svtkGraphToGlyphs::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!this->DistanceToCamera->GetRenderer())
  {
    svtkErrorMacro("Need renderer set before updating the filter.");
    return 0;
  }

  svtkSmartPointer<svtkGraph> inputCopy;
  if (svtkDirectedGraph::SafeDownCast(input))
  {
    inputCopy.TakeReference(svtkDirectedGraph::New());
  }
  else
  {
    inputCopy.TakeReference(svtkUndirectedGraph::New());
  }
  inputCopy->ShallowCopy(input);

  this->DistanceToCamera->SetScreenSize(this->ScreenSize);
  this->GlyphSource->SetFilled(this->Filled);

  this->GraphToPoints->SetInputData(inputCopy);
  svtkAbstractArray* arr = this->GetInputArrayToProcess(0, inputVector);
  if (arr)
  {
    this->DistanceToCamera->SetInputArrayToProcess(
      0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, arr->GetName());
  }
  this->DistanceToCamera->SetInputConnection(this->GraphToPoints->GetOutputPort());
  this->Glyph->SetInputConnection(0, this->DistanceToCamera->GetOutputPort());
  if (this->GlyphType == SPHERE)
  {
    this->Glyph->SetInputConnection(1, this->Sphere->GetOutputPort());
  }
  else
  {
    this->Glyph->SetInputConnection(1, this->GlyphSource->GetOutputPort());
    this->GlyphSource->SetGlyphType(this->GlyphType);
  }
  this->Glyph->Update();

  output->ShallowCopy(this->Glyph->GetOutput());

  return 1;
}

void svtkGraphToGlyphs::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Filled: " << this->Filled << endl;
  os << indent << "ScreenSize: " << this->ScreenSize << endl;
  os << indent << "GlyphType: " << this->GlyphType << endl;
}
