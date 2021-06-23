/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositePolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <algorithm>

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositePolyDataMapper2.h"
#include "svtkCullerCollection.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointDataToCellData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTrivialProducer.h"

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

#include "svtkAppendPolyData.h"
#include "svtkAreaPicker.h"
#include "svtkCommand.h"
#include "svtkCylinderSource.h"
#include "svtkElevationFilter.h"
#include "svtkExtractEdges.h"
#include "svtkHardwareSelector.h"
#include "svtkInteractorStyleRubberBandPick.h"
#include "svtkPlaneSource.h"
#include "svtkProp3DCollection.h"
#include "svtkRenderedAreaPicker.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"

namespace
{

class PointPickCommand : public svtkCommand
{
protected:
  svtkRenderer* Renderer;
  svtkAreaPicker* Picker;
  svtkPolyDataMapper* Mapper;
  std::map<int, std::vector<int> > BlockPrims;

public:
  static PointPickCommand* New() { return new PointPickCommand; }

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
        int blockIndex = node->GetProperties()->Get(svtkSelectionNode::COMPOSITE_INDEX());
        cerr << "Block ID " << blockIndex << " with prim ids of: ";

        svtkIdTypeArray* selIds = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
        if (selIds)
        {
          svtkIdType numIds = selIds->GetNumberOfTuples();
          for (svtkIdType i = 0; i < numIds; ++i)
          {
            svtkIdType curId = selIds->GetValue(i);
            this->BlockPrims[blockIndex].push_back(curId);
            cerr << " " << curId;
          }
        }
        cerr << "\n";
      }
    }
  }

  std::map<int, std::vector<int> >& GetBlockPrims() { return this->BlockPrims; }

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
      // this->DumpPointSelection();
      result->Delete();
    }
  }
};

}

int TestCompositePolyDataMapper2Picking(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindow> win = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  win->SetMultiSamples(0);

  svtkSmartPointer<svtkCompositePolyDataMapper2> mapper =
    svtkSmartPointer<svtkCompositePolyDataMapper2>::New();
  svtkNew<svtkCompositeDataDisplayAttributes> cdsa;
  mapper->SetCompositeDataDisplayAttributes(cdsa.GetPointer());

  int resolution = 18;
  svtkNew<svtkPlaneSource> plane;
  plane->SetResolution(resolution, resolution);
  plane->SetOrigin(-0.2, -0.2, 0.0);
  plane->SetPoint1(0.2, -0.2, 0.0);
  plane->SetPoint2(-0.2, 0.2, 0.0);

  svtkNew<svtkExtractEdges> extract;
  extract->SetInputConnection(plane->GetOutputPort());

  svtkNew<svtkCylinderSource> cyl;
  cyl->CappingOn();
  cyl->SetRadius(0.2);
  cyl->SetResolution(resolution);

  svtkNew<svtkElevationFilter> elev;
  elev->SetInputConnection(cyl->GetOutputPort());

  svtkNew<svtkPointDataToCellData> p2c;
  p2c->SetInputConnection(elev->GetOutputPort());
  p2c->PassPointDataOff();

  // build a composite dataset
  svtkNew<svtkMultiBlockDataSet> data;
  int blocksPerLevel[3] = { 1, 8, 16 };
  std::vector<svtkSmartPointer<svtkMultiBlockDataSet> > blocks;
  blocks.push_back(data.GetPointer());
  unsigned levelStart = 0;
  unsigned levelEnd = 1;
  int numLevels = sizeof(blocksPerLevel) / sizeof(blocksPerLevel[0]);
  int numLeaves = 0;
  int numNodes = 0;
  svtkStdString blockName("Rolf");
  mapper->SetInputDataObject(data.GetPointer());
  for (int level = 1; level < numLevels; ++level)
  {
    int nblocks = blocksPerLevel[level];
    for (unsigned parent = levelStart; parent < levelEnd; ++parent)
    {
      blocks[parent]->SetNumberOfBlocks(nblocks);
      for (int block = 0; block < nblocks; ++block, ++numNodes)
      {
        if (level == numLevels - 1)
        {
          svtkNew<svtkPolyData> child;
          if ((block / 6) % 2)
          {
            cyl->SetCenter(block * 0.25, 0.0, parent * 0.5);
            plane->SetCenter(block * 0.25, 0.5, parent * 0.5);
            elev->SetLowPoint(block * 0.25 - 0.2 + 0.2 * block / nblocks, -0.02, 0.0);
            elev->SetHighPoint(block * 0.25 + 0.1 + 0.2 * block / nblocks, 0.02, 0.0);
            elev->Update();

            svtkPolyData* poly = svtkPolyData::SafeDownCast(elev->GetOutput(0));
            svtkNew<svtkCellArray> lines;
            lines->InsertNextCell(2);
            lines->InsertCellPoint(16);
            lines->InsertCellPoint(17);
            lines->InsertNextCell(2);
            lines->InsertCellPoint(18);
            lines->InsertCellPoint(19);
            poly->SetLines(lines);
            // note this strip is coincident with the cylinder and
            // with cell colors will resultin in some rendering
            // artifacts/flickering
            svtkNew<svtkCellArray> strips;
            strips->InsertNextCell(5);
            strips->InsertCellPoint(20);
            strips->InsertCellPoint(21);
            strips->InsertCellPoint(22);
            strips->InsertCellPoint(23);
            strips->InsertCellPoint(24);
            poly->SetStrips(strips);

            p2c->Update();
            child->DeepCopy(p2c->GetOutput(0));
          }
          else
          {
            plane->SetCenter(block * 0.25, 0.5, parent * 0.5);
            extract->Update();
            child->DeepCopy(extract->GetOutput(0));
          }
          blocks[parent]->SetBlock(block, (block % 2) ? nullptr : child.GetPointer());
          blocks[parent]->GetMetaData(block)->Set(svtkCompositeDataSet::NAME(), blockName.c_str());
          // test not setting it on some
          if (block % 11)
          {
            mapper->SetBlockVisibility(parent + numLeaves, (block % 7) != 0);
          }
          ++numLeaves;
        }
        else
        {
          svtkNew<svtkMultiBlockDataSet> child;
          blocks[parent]->SetBlock(block, child.GetPointer());
          blocks.push_back(child.GetPointer());
        }
      }
    }
    levelStart = levelEnd;
    levelEnd = static_cast<unsigned>(blocks.size());
  }

  mapper->SetScalarModeToUseCellData();

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetEdgeColor(1, 0, 0);
  // actor->GetProperty()->EdgeVisibilityOn();
  ren->AddActor(actor);
  win->SetSize(400, 400);

  ren->RemoveCuller(ren->GetCullers()->GetLastItem());
  ren->ResetCamera();
  win->Render(); // get the window up

  // modify the data to force a rebuild of OpenGL structs
  // after rendering set one cylinder to white
  mapper->SetBlockColor(80, 1.0, 1.0, 1.0);
  mapper->SetBlockOpacity(80, 1.0);
  mapper->SetBlockVisibility(80, 1.0);

  // Setup picker
  svtkNew<svtkInteractorStyleRubberBandPick> pickerInt;
  iren->SetInteractorStyle(pickerInt.GetPointer());
  svtkNew<svtkRenderedAreaPicker> picker;
  iren->SetPicker(picker.GetPointer());

  ren->GetActiveCamera()->Elevation(30.0);
  ren->GetActiveCamera()->Azimuth(-40.0);
  ren->GetActiveCamera()->Zoom(3.0);
  ren->GetActiveCamera()->Roll(10.0);
  win->Render();

  // We'll follow up the cheap RenderedAreaPick with a detailed selection
  svtkNew<PointPickCommand> com;
  com->SetRenderer(ren.GetPointer());
  com->SetPicker(picker.GetPointer());
  com->SetMapper(mapper.GetPointer());
  picker->AddObserver(svtkCommand::EndPickEvent, com.GetPointer());

  // Make pick
  win->Render();
  picker->AreaPick(250, 300, 380, 380, ren.GetPointer());
  win->Render();

  // Interact if desired
  int retVal = svtkRegressionTestImage(win.GetPointer());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Verify pick
  std::map<int, std::vector<int> >& bPrims = com->GetBlockPrims();
  if (bPrims.find(48) == bPrims.end() ||
    std::find(bPrims[48].begin(), bPrims[48].end(), 14) == bPrims[48].end() ||
    bPrims.find(82) == bPrims.end() ||
    std::find(bPrims[82].begin(), bPrims[82].end(), 114) == bPrims[82].end())
  {
    cerr << "Incorrect pick results (if any picks were performed inter"
            "actively this could be ignored).\n";
    return EXIT_FAILURE;
  }

  return !retVal;
}
