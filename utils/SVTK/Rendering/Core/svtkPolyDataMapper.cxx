/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyDataMapper.h"

#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkObjectFactoryNewMacro(svtkPolyDataMapper);

//----------------------------------------------------------------------------
svtkPolyDataMapper::svtkPolyDataMapper()
{
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->NumberOfSubPieces = 1;
  this->GhostLevel = 0;
  this->SeamlessU = false;
  this->SeamlessV = false;
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::Render(svtkRenderer* ren, svtkActor* act)
{
  if (this->Static)
  {
    this->RenderPiece(ren, act);
    return;
  }

  svtkInformation* inInfo = this->GetInputInformation();
  if (inInfo == nullptr)
  {
    svtkErrorMacro("Mapper has no input.");
    return;
  }

  int nPieces = this->NumberOfPieces * this->NumberOfSubPieces;
  for (int i = 0; i < this->NumberOfSubPieces; i++)
  {
    // If more than one pieces, render in loop.
    int currentPiece = this->NumberOfSubPieces * this->Piece + i;
    this->GetInputAlgorithm()->UpdateInformation();
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), currentPiece);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), nPieces);
    inInfo->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
    this->RenderPiece(ren, act);
  }
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::SetInputData(svtkPolyData* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
svtkPolyData* svtkPolyDataMapper::GetInput()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
svtkTypeBool svtkPolyDataMapper::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector*)
{
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    int currentPiece = this->NumberOfSubPieces * this->Piece;
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), currentPiece);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      this->NumberOfSubPieces * this->NumberOfPieces);
    inInfo->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
  }
  return 1;
}

//----------------------------------------------------------------------------
// Get the bounds for the input of this mapper as
// (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkPolyDataMapper::GetBounds()
{
  // do we have an input
  if (!this->GetNumberOfInputConnections(0))
  {
    svtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  else
  {
    if (!this->Static)
    {
      svtkInformation* inInfo = this->GetInputInformation();
      if (inInfo)
      {
        this->GetInputAlgorithm()->UpdateInformation();
        int currentPiece = this->NumberOfSubPieces * this->Piece;
        this->GetInputAlgorithm()->UpdatePiece(
          currentPiece, this->NumberOfSubPieces * this->NumberOfPieces, this->GhostLevel);
      }
    }
    this->ComputeBounds();

    // if the bounds indicate NAN and subpieces are being used then
    // return nullptr
    if (!svtkMath::AreBoundsInitialized(this->Bounds) && this->NumberOfSubPieces > 1)
    {
      return nullptr;
    }
    return this->Bounds;
  }
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::ComputeBounds()
{
  svtkPolyData* input = this->GetInput();
  if (input)
  {
    input->GetBounds(this->Bounds);
  }
  else
  {
    svtkMath::UninitializeBounds(this->Bounds);
  }
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::ShallowCopy(svtkAbstractMapper* mapper)
{
  svtkPolyDataMapper* m = svtkPolyDataMapper::SafeDownCast(mapper);
  if (m != nullptr)
  {
    this->SetInputConnection(m->GetInputConnection(0, 0));
    this->SetGhostLevel(m->GetGhostLevel());
    this->SetNumberOfPieces(m->GetNumberOfPieces());
    this->SetNumberOfSubPieces(m->GetNumberOfSubPieces());
    this->SetSeamlessU(m->GetSeamlessU());
    this->SetSeamlessV(m->GetSeamlessV());
  }

  // Now do superclass
  this->svtkMapper::ShallowCopy(mapper);
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::MapDataArrayToVertexAttribute(const char* svtkNotUsed(vertexAttributeName),
  const char* svtkNotUsed(dataArrayName), int svtkNotUsed(fieldAssociation),
  int svtkNotUsed(componentno))
{
  svtkErrorMacro("Not implemented at this level...");
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::MapDataArrayToMultiTextureAttribute(const char* svtkNotUsed(tname),
  const char* svtkNotUsed(dataArrayName), int svtkNotUsed(fieldAssociation),
  int svtkNotUsed(componentno))
{
  svtkErrorMacro("Not implemented at this level...");
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::RemoveVertexAttributeMapping(const char* svtkNotUsed(vertexAttributeName))
{
  svtkErrorMacro("Not implemented at this level...");
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::RemoveAllVertexAttributeMappings()
{
  svtkErrorMacro("Not implemented at this level...");
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Piece : " << this->Piece << endl;
  os << indent << "NumberOfPieces : " << this->NumberOfPieces << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "Number of sub pieces: " << this->NumberOfSubPieces << endl;
}

//----------------------------------------------------------------------------
int svtkPolyDataMapper::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::Update(int port)
{
  if (this->Static)
  {
    return;
  }
  this->Superclass::Update(port);
}

//----------------------------------------------------------------------------
void svtkPolyDataMapper::Update()
{
  if (this->Static)
  {
    return;
  }
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
svtkTypeBool svtkPolyDataMapper::Update(int port, svtkInformationVector* requests)
{
  if (this->Static)
  {
    return 1;
  }
  return this->Superclass::Update(port, requests);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkPolyDataMapper::Update(svtkInformation* requests)
{
  if (this->Static)
  {
    return 1;
  }
  return this->Superclass::Update(requests);
}
