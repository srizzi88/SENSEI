/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <algorithm>

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkHardwareSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInteractorStyleRubberBandPick.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProp3DCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedAreaPicker.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSphereSource.h"

class PointPickCommand : public svtkCommand
{
protected:
  std::vector<int> PointIds;
  svtkRenderer* Renderer;
  svtkAreaPicker* Picker;
  svtkPolyDataMapper* Mapper;

public:
  static PointPickCommand* New() { return new PointPickCommand; }
  svtkTypeMacro(PointPickCommand, svtkCommand);

  PointPickCommand() = default;

  ~PointPickCommand() override = default;

  void SetPointIds(svtkSelection* selection)
  {
    // Find selection node that we're interested in:
    const svtkIdType numNodes = selection->GetNumberOfNodes();
    for (svtkIdType nodeId = 0; nodeId < numNodes; ++nodeId)
    {
      svtkSelectionNode* node = selection->GetNode(nodeId);

      // Check if the mapper is this instance of MoleculeMapper
      svtkActor* selActor =
        svtkActor::SafeDownCast(node->GetProperties()->Get(svtkSelectionNode::PROP()));
      if (selActor && (selActor->GetMapper() == this->Mapper))
      {
        // Separate the selection ids into atoms and bonds
        svtkIdTypeArray* selIds = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
        if (selIds)
        {
          svtkIdType numIds = selIds->GetNumberOfTuples();
          for (svtkIdType i = 0; i < numIds; ++i)
          {
            svtkIdType curId = selIds->GetValue(i);
            this->PointIds.push_back(curId);
          }
        }
      }
    }
  }

  std::vector<int>& GetPointIds() { return this->PointIds; }

  void SetMapper(svtkPolyDataMapper* m) { this->Mapper = m; }

  void SetRenderer(svtkRenderer* r) { this->Renderer = r; }

  void SetPicker(svtkAreaPicker* p) { this->Picker = p; }

  void Execute(svtkObject*, unsigned long, void*) override
  {
    svtkProp3DCollection* props = this->Picker->GetProp3Ds();
    if (props->GetNumberOfItems() != 0)
    {
      // If anything was picked during the fast area pick, do a more detailed
      // pick.
      svtkNew<svtkHardwareSelector> selector;
      selector->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_POINTS);
      selector->SetRenderer(this->Renderer);
      selector->SetArea(static_cast<unsigned int>(this->Renderer->GetPickX1()),
        static_cast<unsigned int>(this->Renderer->GetPickY1()),
        static_cast<unsigned int>(this->Renderer->GetPickX2()),
        static_cast<unsigned int>(this->Renderer->GetPickY2()));
      // Make the actual pick and pass the result to the convenience function
      // defined earlier
      svtkSelection* result = selector->Select();
      this->SetPointIds(result);
      this->DumpPointSelection();
      result->Delete();
    }
  }

  // Convenience function to print out the atom and bond ids that belong to
  // molMap and are contained in sel
  void DumpPointSelection()
  {
    // Print selection
    cerr << "\n### Selection ###\n";
    cerr << "Points: ";
    for (std::vector<int>::iterator i = this->PointIds.begin(); i != this->PointIds.end(); ++i)
    {
      cerr << *i << " ";
    }
    cerr << endl;
  }
};

int TestPointSelection(int argc, char* argv[])
{
  // create a line and a mesh
  svtkNew<svtkSphereSource> sphere;

  // Set up render engine
  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(sphereMapper);

  svtkNew<svtkRenderer> ren;
  ren->AddActor(actor);
  svtkNew<svtkRenderWindow> win;
  win->SetMultiSamples(0);
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(450, 450);
  win->Render();
  ren->GetActiveCamera()->Zoom(1.2);

  // Setup picker
  svtkNew<svtkInteractorStyleRubberBandPick> pickerInt;
  iren->SetInteractorStyle(pickerInt);
  svtkNew<svtkRenderedAreaPicker> picker;
  iren->SetPicker(picker);

  // We'll follow up the cheap RenderedAreaPick with a detailed selection
  // to obtain the atoms and bonds.
  svtkNew<PointPickCommand> com;
  com->SetRenderer(ren);
  com->SetPicker(picker);
  com->SetMapper(sphereMapper);
  picker->AddObserver(svtkCommand::EndPickEvent, com);

  // Make pick -- lower left quarter of renderer
  win->Render();
  picker->AreaPick(0, 0, 225, 225, ren);
  win->Render();

  // Interact if desired
  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Verify pick
  std::vector<int>& pIds = com->GetPointIds();
  if (pIds.size() < 7 || std::find(pIds.begin(), pIds.end(), 0) == pIds.end() ||
    std::find(pIds.begin(), pIds.end(), 26) == pIds.end() ||
    std::find(pIds.begin(), pIds.end(), 27) == pIds.end() ||
    std::find(pIds.begin(), pIds.end(), 32) == pIds.end() ||
    std::find(pIds.begin(), pIds.end(), 33) == pIds.end() ||
    std::find(pIds.begin(), pIds.end(), 38) == pIds.end() ||
    std::find(pIds.begin(), pIds.end(), 39) == pIds.end())
  {
    cerr << "Incorrect atoms/bonds picked! (if any picks were performed inter"
            "actively this could be ignored).\n";
    return EXIT_FAILURE;
  }

  return !retVal;
}
