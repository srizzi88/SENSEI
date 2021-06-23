/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScenePicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkScenePicker.h"

#include "svtkCommand.h"
#include "svtkDataObject.h"
#include "svtkHardwareSelector.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

class svtkScenePickerSelectionRenderCommand : public svtkCommand
{
public:
  svtkScenePicker* m_Picker;
  static svtkScenePickerSelectionRenderCommand* New()
  {
    return new svtkScenePickerSelectionRenderCommand;
  }

  void Execute(svtkObject* svtkNotUsed(o), unsigned long event, void*) override
  {
    if (event == svtkCommand::StartInteractionEvent)
    {
      this->InteractiveRender = true;
    }
    else if (event == svtkCommand::EndInteractionEvent)
    {
      this->InteractiveRender = false;
    }
    else if (event == svtkCommand::EndEvent)
    {
      if (!this->InteractiveRender)
      {
        m_Picker->PickRender();
      }
      this->m_Picker->SetRenderer(this->m_Picker->Renderer);
    }
  }

protected:
  svtkScenePickerSelectionRenderCommand()
    : InteractiveRender(false)
  {
  }
  ~svtkScenePickerSelectionRenderCommand() override = default;
  bool InteractiveRender;
};

svtkStandardNewMacro(svtkScenePicker);

//----------------------------------------------------------------------------
svtkScenePicker::svtkScenePicker()
{
  this->EnableVertexPicking = 1;
  this->Renderer = nullptr;
  this->Interactor = nullptr;
  this->Selector = svtkHardwareSelector::New();
  this->NeedToUpdate = false;
  this->VertId = -1;
  this->CellId = -1;
  this->Prop = nullptr;
  this->SelectionRenderCommand = svtkScenePickerSelectionRenderCommand::New();
  this->SelectionRenderCommand->m_Picker = this;
}

//----------------------------------------------------------------------------
svtkScenePicker::~svtkScenePicker()
{
  this->SetRenderer(nullptr);
  this->Selector->Delete();
  this->SelectionRenderCommand->Delete();
}

//----------------------------------------------------------------------------
void svtkScenePicker::SetRenderer(svtkRenderer* r)
{
  svtkRenderWindowInteractor* rwi = nullptr;
  if (r && r->GetRenderWindow())
  {
    rwi = r->GetRenderWindow()->GetInteractor();
  }
  this->SetInteractor(rwi);

  if (this->Renderer == r)
  {
    return;
  }
  if (r && !r->GetRenderWindow())
  {
    svtkErrorMacro(<< "Renderer: " << this->Renderer << " does not have its render window set.");
    return;
  }

  if (this->Renderer)
  {
    this->Renderer->GetRenderWindow()->RemoveObserver(this->SelectionRenderCommand);
  }

  svtkSetObjectBodyMacro(Renderer, svtkRenderer, r);

  if (this->Renderer)
  {
    this->Renderer->GetRenderWindow()->AddObserver(
      svtkCommand::EndEvent, this->SelectionRenderCommand, 0.01);
  }

  this->Selector->SetRenderer(this->Renderer);
}

//----------------------------------------------------------------------------
void svtkScenePicker::SetInteractor(svtkRenderWindowInteractor* rwi)
{
  if (this->Interactor == rwi)
  {
    return;
  }
  if (this->Interactor)
  {
    this->Interactor->RemoveObserver(this->SelectionRenderCommand);
  }

  svtkSetObjectBodyMacro(Interactor, svtkRenderWindowInteractor, rwi);

  if (this->Interactor)
  {
    this->Interactor->AddObserver(
      svtkCommand::StartInteractionEvent, this->SelectionRenderCommand, 0.01);
    this->Interactor->AddObserver(
      svtkCommand::EndInteractionEvent, this->SelectionRenderCommand, 0.01);
  }
}

//----------------------------------------------------------------------------
// Do a selection render.. for caching object selection stuff.
// This is used for Object selection . We have to perform
// "select" and "mouse over" and "mouse out" as the mouse moves around the
// scene (or the mouse is clicked in the case of "select"). I do not want
// to do a conventional pick for this function because it's too darn slow.
// The Selector will be used here to pick-render the entire
// screen, store on a buffer the colored cells and read back as
// the mouse moves around the moused pick. This extra render from the
// Selector will be done only if the camera isn't in motion,
// otherwise motion would be too frickin slow.
//
void svtkScenePicker::PickRender()
{
  if (!this->Renderer || !this->Renderer->GetRenderWindow())
  {
    return;
  }

  double vp[4];
  this->Renderer->GetViewport(vp);
  int size[2] = { this->Renderer->GetRenderWindow()->GetSize()[0],
    this->Renderer->GetRenderWindow()->GetSize()[1] };

  int rx1 = static_cast<int>(vp[0] * (size[0] - 1));
  int ry1 = static_cast<int>(vp[1] * (size[1] - 1));
  int rx2 = static_cast<int>(vp[2] * (size[0] - 1));
  int ry2 = static_cast<int>(vp[3] * (size[1] - 1));

  this->PickRender(rx1, ry1, rx2, ry2);
}

// ----------------------------------------------------------------------------
// Do a selection render.. for caching object selection stuff.
void svtkScenePicker::PickRender(int x0, int y0, int x1, int y1)
{
  this->Renderer->GetRenderWindow()->RemoveObserver(this->SelectionRenderCommand);

  if (this->EnableVertexPicking)
  {
    this->Selector->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_POINTS);
  }
  else
  {
    this->Selector->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_CELLS);
  }
  cout << "Area: " << x0 << ", " << y0 << ", " << x1 << ", " << y1 << endl;
  this->Selector->SetArea(x0, y0, x1, y1);
  if (!this->Selector->CaptureBuffers())
  {
    svtkErrorMacro("Failed to capture buffers.");
  }
  this->NeedToUpdate = true;
  this->PickRenderTime.Modified();
  this->Renderer->GetRenderWindow()->AddObserver(
    svtkCommand::EndEvent, this->SelectionRenderCommand, 0.01);
}

//----------------------------------------------------------------------------
svtkIdType svtkScenePicker::GetCellId(int displayPos[2])
{
  if (this->EnableVertexPicking)
  {
    return -1;
  }
  this->Update(displayPos);
  return this->CellId;
}

//----------------------------------------------------------------------------
svtkProp* svtkScenePicker::GetViewProp(int displayPos[2])
{
  this->Update(displayPos);
  return this->Prop;
}

//----------------------------------------------------------------------------
svtkIdType svtkScenePicker::GetVertexId(int displayPos[2])
{
  if (!this->EnableVertexPicking)
  {
    return -1;
  }
  this->Update(displayPos);
  return this->CellId;
}

//----------------------------------------------------------------------------
void svtkScenePicker::Update(int displayPos[2])
{
  if (this->PickRenderTime <= this->GetMTime())
  {
    this->PickRender();
  }

  if (this->NeedToUpdate || this->LastQueriedDisplayPos[0] != displayPos[0] ||
    this->LastQueriedDisplayPos[1] != displayPos[1])
  {
    this->Prop = nullptr;
    unsigned int dpos[2] = { 0, 0 };
    if (displayPos[0] >= 0 && displayPos[1] >= 0)
    {
      dpos[0] = static_cast<unsigned int>(displayPos[0]);
      dpos[1] = static_cast<unsigned int>(displayPos[1]);
      svtkHardwareSelector::PixelInformation info = this->Selector->GetPixelInformation(dpos);
      this->CellId = info.AttributeID;
      this->Prop = info.Prop;
    }
    this->LastQueriedDisplayPos[0] = displayPos[0];
    this->LastQueriedDisplayPos[1] = displayPos[1];
    this->NeedToUpdate = false;
  }
}

//----------------------------------------------------------------------------
void svtkScenePicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "EnableVertexPicking: " << this->EnableVertexPicking << endl;
}
