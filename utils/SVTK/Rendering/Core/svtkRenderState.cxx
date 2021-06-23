/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderState.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRenderState.h"
#include "svtkFrameBufferObjectBase.h"
#include "svtkRenderer.h"
#include <cassert>

// ----------------------------------------------------------------------------
// Description:
// Constructor. All values are initialized to 0 or nullptr.
// \pre renderer_exists: renderer!=0
// \post renderer_is_set: GetRenderer()==renderer.
// \post valid_state: IsValid()
svtkRenderState::svtkRenderState(svtkRenderer* renderer)
{
  assert("pre: renderer_exists" && renderer != nullptr);
  this->Renderer = renderer;
  this->FrameBuffer = nullptr;
  this->PropArray = nullptr;
  this->PropArrayCount = 0;
  this->RequiredKeys = nullptr;

  assert("post: renderer_is_set" && this->GetRenderer() == renderer);
  assert("post: is_valid" && this->IsValid());
}

// ----------------------------------------------------------------------------
// Description:
// Destructor. As a svtkRenderState does not own any of its variables,
// the destructor does nothing.
svtkRenderState::~svtkRenderState() = default;

// ----------------------------------------------------------------------------
// Description:
// Tells if the RenderState is a valid one (Renderer is not null).
bool svtkRenderState::IsValid() const
{
  return this->Renderer != nullptr;
}

// ----------------------------------------------------------------------------
// Description:
// Return the Renderer.
// \post result_exists: result!=0
svtkRenderer* svtkRenderState::GetRenderer() const
{
  assert("post: valid_result" && this->Renderer != nullptr);
  return this->Renderer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the FrameBuffer.
svtkFrameBufferObjectBase* svtkRenderState::GetFrameBuffer() const
{
  return this->FrameBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Set the FrameBuffer.
// \post is_set: GetFrameBuffer()==fbo
void svtkRenderState::SetFrameBuffer(svtkFrameBufferObjectBase* fbo)
{
  this->FrameBuffer = fbo;
  assert("post: is_set" && this->GetFrameBuffer() == fbo);
}

// ----------------------------------------------------------------------------
// Description:
// Get the window size of the state.
void svtkRenderState::GetWindowSize(int size[2]) const
{
  if (this->FrameBuffer == nullptr)
  {
    this->Renderer->GetTiledSize(&size[0], &size[1]);
  }
  else
  {
    this->FrameBuffer->GetLastSize(size);
  }
}

// ----------------------------------------------------------------------------
// Description:
// Return the array of filtered props
svtkProp** svtkRenderState::GetPropArray() const
{
  return this->PropArray;
}

// ----------------------------------------------------------------------------
// Description:
// Return the size of the array of filtered props.
// \post positive_result: result>=0
int svtkRenderState::GetPropArrayCount() const
{
  assert("post: positive_result" && this->PropArrayCount >= 0);
  return this->PropArrayCount;
}

// ----------------------------------------------------------------------------
// Description:
// Set the array of filtered props and its size.
// \pre positive_size: propArrayCount>=0
// \pre valid_null_array: propArray!=0 || propArrayCount==0
// \post is_set: GetPropArray()==propArray && GetPropArrayCount()==propArrayCount
void svtkRenderState::SetPropArrayAndCount(svtkProp** propArray, int propArrayCount)
{
  assert("pre: positive_size" && propArrayCount >= 0);
  assert("pre: valid_null_array" && (propArray != nullptr || propArrayCount == 0));

  this->PropArray = propArray;
  this->PropArrayCount = propArrayCount;

  assert("post: is_set" && this->GetPropArray() == propArray &&
    this->GetPropArrayCount() == propArrayCount);
}

// ----------------------------------------------------------------------------
// Description:
// Return the required property keys for the props.
svtkInformation* svtkRenderState::GetRequiredKeys() const
{
  return this->RequiredKeys;
}

// ----------------------------------------------------------------------------
// Description:
// Set the required property keys for the props.
// \post is_set: GetRequiredKeys()==keys
void svtkRenderState::SetRequiredKeys(svtkInformation* keys)
{
  this->RequiredKeys = keys;
  assert("post: is_set" && this->GetRequiredKeys() == keys);
}
