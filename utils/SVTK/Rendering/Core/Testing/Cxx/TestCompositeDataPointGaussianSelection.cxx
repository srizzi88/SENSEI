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

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeDataSet.h"
#include "svtkHardwareSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointGaussianMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTrivialProducer.h"

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

#include "svtkCylinderSource.h"

int TestCompositeDataPointGaussianSelection(int argc, char* argv[])
{
  svtkNew<svtkRenderWindow> win;
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderer> ren;
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  svtkNew<svtkPointGaussianMapper> mapper;
  mapper->SetScaleFactor(0.01);

  int resolution = 18;
  svtkNew<svtkCylinderSource> cyl;
  cyl->CappingOn();
  cyl->SetRadius(0.2);
  cyl->SetResolution(resolution);

  // build a composite dataset
  svtkNew<svtkMultiBlockDataSet> data;
  int blocksPerLevel[3] = { 1, 16, 32 };
  std::vector<svtkSmartPointer<svtkMultiBlockDataSet> > blocks;
  blocks.push_back(data.GetPointer());
  unsigned levelStart = 0;
  unsigned levelEnd = 1;
  int numLevels = sizeof(blocksPerLevel) / sizeof(blocksPerLevel[0]);
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
          cyl->SetCenter(block * 0.25, 0.0, parent * 0.5);
          cyl->Update();
          child->DeepCopy(cyl->GetOutput(0));
          blocks[parent]->SetBlock(block, (block % 2) ? nullptr : child.GetPointer());
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

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  ren->AddActor(actor);
  win->SetSize(400, 400);

  ren->ResetCamera();

  ren->GetActiveCamera()->Elevation(40.0);
  ren->GetActiveCamera()->Zoom(3.2);
  ren->GetActiveCamera()->Roll(20.0);
  win->Render();

  svtkNew<svtkHardwareSelector> selector;
  selector->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_POINTS);
  selector->SetRenderer(ren);
  selector->SetArea(10, 10, 50, 50);
  svtkSelection* result = selector->Select();

  bool goodPick = false;

  cerr << "numnodes: " << result->GetNumberOfNodes() << "\n";
  if (result->GetNumberOfNodes() == 5)
  {
    for (unsigned int nodenum = 0; nodenum < result->GetNumberOfNodes(); ++nodenum)
    {
      svtkSelectionNode* node = result->GetNode(nodenum);

      cerr << "Node: " << nodenum
           << " comp: " << node->GetProperties()->Get(svtkSelectionNode::COMPOSITE_INDEX()) << "\n";

      svtkIdTypeArray* selIds = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
      if (selIds)
      {
        svtkIdType numIds = selIds->GetNumberOfTuples();
        for (svtkIdType i = 0; i < numIds; ++i)
        {
          svtkIdType curId = selIds->GetValue(i);
          cerr << curId << "\n";
        }
      }
    }

    svtkIdTypeArray* selIds =
      svtkArrayDownCast<svtkIdTypeArray>(result->GetNode(0)->GetSelectionList());
    if (result->GetNode(0)->GetProperties()->Has(svtkSelectionNode::PROP_ID()) &&
      result->GetNode(0)->GetProperties()->Get(svtkSelectionNode::PROP()) == actor.Get() &&
      result->GetNode(0)->GetProperties()->Get(svtkSelectionNode::COMPOSITE_INDEX()) == 305 &&
      result->GetNode(2)->GetProperties()->Get(svtkSelectionNode::COMPOSITE_INDEX()) == 340 &&
      selIds && selIds->GetNumberOfTuples() == 5 && selIds->GetValue(2) == 56)
    {
      goodPick = true;
    }
  }
  result->Delete();

  if (!goodPick)
  {
    cerr << "Incorrect splats picked!\n";
    return EXIT_FAILURE;
  }

  int retVal = svtkRegressionTestImageThreshold(win.GetPointer(), 15);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
