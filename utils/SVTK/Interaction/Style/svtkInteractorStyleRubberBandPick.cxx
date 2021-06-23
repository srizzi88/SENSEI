/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleRubberBandPick.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleRubberBandPick.h"

#include "svtkAbstractPropPicker.h"
#include "svtkAreaPicker.h"
#include "svtkAssemblyPath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkInteractorStyleRubberBandPick);

#define SVTKISRBP_ORIENT 0
#define SVTKISRBP_SELECT 1

//--------------------------------------------------------------------------
svtkInteractorStyleRubberBandPick::svtkInteractorStyleRubberBandPick()
{
  this->CurrentMode = SVTKISRBP_ORIENT;
  this->StartPosition[0] = this->StartPosition[1] = 0;
  this->EndPosition[0] = this->EndPosition[1] = 0;
  this->Moving = 0;
  this->PixelArray = svtkUnsignedCharArray::New();
}

//--------------------------------------------------------------------------
svtkInteractorStyleRubberBandPick::~svtkInteractorStyleRubberBandPick()
{
  this->PixelArray->Delete();
}

//--------------------------------------------------------------------------
void svtkInteractorStyleRubberBandPick::StartSelect()
{
  this->CurrentMode = SVTKISRBP_SELECT;
}

//--------------------------------------------------------------------------
void svtkInteractorStyleRubberBandPick::OnChar()
{
  switch (this->Interactor->GetKeyCode())
  {
    case 'r':
    case 'R':
      // r toggles the rubber band selection mode for mouse button 1
      if (this->CurrentMode == SVTKISRBP_ORIENT)
      {
        this->CurrentMode = SVTKISRBP_SELECT;
      }
      else
      {
        this->CurrentMode = SVTKISRBP_ORIENT;
      }
      break;
    case 'p':
    case 'P':
    {
      svtkRenderWindowInteractor* rwi = this->Interactor;
      int* eventPos = rwi->GetEventPosition();
      this->FindPokedRenderer(eventPos[0], eventPos[1]);
      this->StartPosition[0] = eventPos[0];
      this->StartPosition[1] = eventPos[1];
      this->EndPosition[0] = eventPos[0];
      this->EndPosition[1] = eventPos[1];
      this->Pick();
      break;
    }
    default:
      this->Superclass::OnChar();
  }
}

//--------------------------------------------------------------------------
void svtkInteractorStyleRubberBandPick::OnLeftButtonDown()
{
  if (this->CurrentMode != SVTKISRBP_SELECT)
  {
    // if not in rubber band mode, let the parent class handle it
    this->Superclass::OnLeftButtonDown();
    return;
  }

  if (!this->Interactor)
  {
    return;
  }

  // otherwise record the rubber band starting coordinate

  this->Moving = 1;

  svtkRenderWindow* renWin = this->Interactor->GetRenderWindow();

  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
  this->EndPosition[0] = this->StartPosition[0];
  this->EndPosition[1] = this->StartPosition[1];

  this->PixelArray->Initialize();
  this->PixelArray->SetNumberOfComponents(4);
  const int* size = renWin->GetSize();
  this->PixelArray->SetNumberOfTuples(size[0] * size[1]);

  renWin->GetRGBACharPixelData(0, 0, size[0] - 1, size[1] - 1, 1, this->PixelArray);

  this->FindPokedRenderer(this->StartPosition[0], this->StartPosition[1]);
}

//--------------------------------------------------------------------------
void svtkInteractorStyleRubberBandPick::OnMouseMove()
{
  if (this->CurrentMode != SVTKISRBP_SELECT)
  {
    // if not in rubber band mode,  let the parent class handle it
    this->Superclass::OnMouseMove();
    return;
  }

  if (!this->Interactor || !this->Moving)
  {
    return;
  }

  this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
  this->EndPosition[1] = this->Interactor->GetEventPosition()[1];
  const int* size = this->Interactor->GetRenderWindow()->GetSize();
  if (this->EndPosition[0] > (size[0] - 1))
  {
    this->EndPosition[0] = size[0] - 1;
  }
  if (this->EndPosition[0] < 0)
  {
    this->EndPosition[0] = 0;
  }
  if (this->EndPosition[1] > (size[1] - 1))
  {
    this->EndPosition[1] = size[1] - 1;
  }
  if (this->EndPosition[1] < 0)
  {
    this->EndPosition[1] = 0;
  }
  this->RedrawRubberBand();
}

//--------------------------------------------------------------------------
void svtkInteractorStyleRubberBandPick::OnLeftButtonUp()
{
  if (this->CurrentMode != SVTKISRBP_SELECT)
  {
    // if not in rubber band mode,  let the parent class handle it
    this->Superclass::OnLeftButtonUp();
    return;
  }

  if (!this->Interactor || !this->Moving)
  {
    return;
  }

  // otherwise record the rubber band end coordinate and then fire off a pick
  if ((this->StartPosition[0] != this->EndPosition[0]) ||
    (this->StartPosition[1] != this->EndPosition[1]))
  {
    this->Pick();
  }
  this->Moving = 0;
  // this->CurrentMode = SVTKISRBP_ORIENT;
}

//--------------------------------------------------------------------------
void svtkInteractorStyleRubberBandPick::RedrawRubberBand()
{
  // update the rubber band on the screen
  const int* size = this->Interactor->GetRenderWindow()->GetSize();

  svtkUnsignedCharArray* tmpPixelArray = svtkUnsignedCharArray::New();
  tmpPixelArray->DeepCopy(this->PixelArray);
  unsigned char* pixels = tmpPixelArray->GetPointer(0);

  int min[2], max[2];

  min[0] =
    this->StartPosition[0] <= this->EndPosition[0] ? this->StartPosition[0] : this->EndPosition[0];
  if (min[0] < 0)
  {
    min[0] = 0;
  }
  if (min[0] >= size[0])
  {
    min[0] = size[0] - 1;
  }

  min[1] =
    this->StartPosition[1] <= this->EndPosition[1] ? this->StartPosition[1] : this->EndPosition[1];
  if (min[1] < 0)
  {
    min[1] = 0;
  }
  if (min[1] >= size[1])
  {
    min[1] = size[1] - 1;
  }

  max[0] =
    this->EndPosition[0] > this->StartPosition[0] ? this->EndPosition[0] : this->StartPosition[0];
  if (max[0] < 0)
  {
    max[0] = 0;
  }
  if (max[0] >= size[0])
  {
    max[0] = size[0] - 1;
  }

  max[1] =
    this->EndPosition[1] > this->StartPosition[1] ? this->EndPosition[1] : this->StartPosition[1];
  if (max[1] < 0)
  {
    max[1] = 0;
  }
  if (max[1] >= size[1])
  {
    max[1] = size[1] - 1;
  }

  int i;
  for (i = min[0]; i <= max[0]; i++)
  {
    pixels[4 * (min[1] * size[0] + i)] = 255 ^ pixels[4 * (min[1] * size[0] + i)];
    pixels[4 * (min[1] * size[0] + i) + 1] = 255 ^ pixels[4 * (min[1] * size[0] + i) + 1];
    pixels[4 * (min[1] * size[0] + i) + 2] = 255 ^ pixels[4 * (min[1] * size[0] + i) + 2];
    pixels[4 * (max[1] * size[0] + i)] = 255 ^ pixels[4 * (max[1] * size[0] + i)];
    pixels[4 * (max[1] * size[0] + i) + 1] = 255 ^ pixels[4 * (max[1] * size[0] + i) + 1];
    pixels[4 * (max[1] * size[0] + i) + 2] = 255 ^ pixels[4 * (max[1] * size[0] + i) + 2];
  }
  for (i = min[1] + 1; i < max[1]; i++)
  {
    pixels[4 * (i * size[0] + min[0])] = 255 ^ pixels[4 * (i * size[0] + min[0])];
    pixels[4 * (i * size[0] + min[0]) + 1] = 255 ^ pixels[4 * (i * size[0] + min[0]) + 1];
    pixels[4 * (i * size[0] + min[0]) + 2] = 255 ^ pixels[4 * (i * size[0] + min[0]) + 2];
    pixels[4 * (i * size[0] + max[0])] = 255 ^ pixels[4 * (i * size[0] + max[0])];
    pixels[4 * (i * size[0] + max[0]) + 1] = 255 ^ pixels[4 * (i * size[0] + max[0]) + 1];
    pixels[4 * (i * size[0] + max[0]) + 2] = 255 ^ pixels[4 * (i * size[0] + max[0]) + 2];
  }

  this->Interactor->GetRenderWindow()->SetRGBACharPixelData(
    0, 0, size[0] - 1, size[1] - 1, pixels, 0);
  this->Interactor->GetRenderWindow()->Frame();

  tmpPixelArray->Delete();
}

//--------------------------------------------------------------------------
void svtkInteractorStyleRubberBandPick::Pick()
{
  // find rubber band lower left, upper right and center
  double rbcenter[3];
  const int* size = this->Interactor->GetRenderWindow()->GetSize();
  int min[2], max[2];
  min[0] =
    this->StartPosition[0] <= this->EndPosition[0] ? this->StartPosition[0] : this->EndPosition[0];
  if (min[0] < 0)
  {
    min[0] = 0;
  }
  if (min[0] >= size[0])
  {
    min[0] = size[0] - 2;
  }

  min[1] =
    this->StartPosition[1] <= this->EndPosition[1] ? this->StartPosition[1] : this->EndPosition[1];
  if (min[1] < 0)
  {
    min[1] = 0;
  }
  if (min[1] >= size[1])
  {
    min[1] = size[1] - 2;
  }

  max[0] =
    this->EndPosition[0] > this->StartPosition[0] ? this->EndPosition[0] : this->StartPosition[0];
  if (max[0] < 0)
  {
    max[0] = 0;
  }
  if (max[0] >= size[0])
  {
    max[0] = size[0] - 2;
  }

  max[1] =
    this->EndPosition[1] > this->StartPosition[1] ? this->EndPosition[1] : this->StartPosition[1];
  if (max[1] < 0)
  {
    max[1] = 0;
  }
  if (max[1] >= size[1])
  {
    max[1] = size[1] - 2;
  }

  rbcenter[0] = (min[0] + max[0]) / 2.0;
  rbcenter[1] = (min[1] + max[1]) / 2.0;
  rbcenter[2] = 0;

  if (this->State == SVTKIS_NONE)
  {
    // tell the RenderWindowInteractor's picker to make it happen
    svtkRenderWindowInteractor* rwi = this->Interactor;

    svtkAssemblyPath* path = nullptr;
    rwi->StartPickCallback();
    svtkAbstractPropPicker* picker = svtkAbstractPropPicker::SafeDownCast(rwi->GetPicker());
    if (picker != nullptr)
    {
      svtkAreaPicker* areaPicker = svtkAreaPicker::SafeDownCast(picker);
      if (areaPicker != nullptr)
      {
        areaPicker->AreaPick(min[0], min[1], max[0], max[1], this->CurrentRenderer);
      }
      else
      {
        picker->Pick(rbcenter[0], rbcenter[1], 0.0, this->CurrentRenderer);
      }
      path = picker->GetPath();
    }
    if (path == nullptr)
    {
      this->HighlightProp(nullptr);
      this->PropPicked = 0;
    }
    else
    {
      // highlight the one prop that the picker saved in the path
      // this->HighlightProp(path->GetFirstNode()->GetViewProp());
      this->PropPicked = 1;
    }
    rwi->EndPickCallback();
  }

  this->Interactor->Render();
}

//--------------------------------------------------------------------------
void svtkInteractorStyleRubberBandPick::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
