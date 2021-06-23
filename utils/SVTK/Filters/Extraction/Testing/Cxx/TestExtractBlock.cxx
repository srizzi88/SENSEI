/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests an extraction of a block using first svtkExtractBlock then
// a block selection from a svtkSelection and svtkExtractSelection

#include <svtkExtractBlock.h>
#include <svtkExtractSelection.h>
#include <svtkFieldData.h>
#include <svtkIntArray.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkNew.h>
#include <svtkSelectionSource.h>
#include <svtkSphereSource.h>

namespace
{
svtkSmartPointer<svtkDataObject> GetSphere(double x, double y, double z)
{
  svtkNew<svtkSphereSource> sphere;
  sphere->SetCenter(x, y, z);
  sphere->Update();
  return sphere->GetOutputDataObject(0);
}
}

int TestExtractBlock(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkNew<svtkMultiBlockDataSet> mb0;
  mb0->SetBlock(0, GetSphere(0, 0, -2));
  mb0->SetBlock(1, GetSphere(0, 0, 2));

  // Add a field data to the multiblock dataset
  svtkNew<svtkIntArray> fieldData;
  std::string globalIdName("GlobalID");
  fieldData->SetName(globalIdName.c_str());
  fieldData->SetNumberOfComponents(1);
  fieldData->SetNumberOfTuples(1);
  fieldData->SetValue(0, 5);
  mb0->GetFieldData()->AddArray(fieldData);

  // Test svtkExtractBlock
  svtkNew<svtkExtractBlock> extractBlock;
  extractBlock->AddIndex(2);
  extractBlock->SetPruneOutput(1);
  extractBlock->SetInputData(mb0);
  extractBlock->Update();

  auto output = svtkMultiBlockDataSet::SafeDownCast(extractBlock->GetOutput());
  if (output->GetBlock(0) == nullptr)
  {
    std::cerr << "Invalid block extracted by svtkExtractBlock. Should be block 0." << std::endl;
    std::cerr << *output << std::endl;
    return EXIT_FAILURE;
  }
  svtkIntArray* outputFDArray =
    svtkIntArray::SafeDownCast(output->GetFieldData()->GetArray(globalIdName.c_str()));
  if (outputFDArray == nullptr || outputFDArray->GetValue(0) != 5)
  {
    std::cerr << "Field data not copied to output. Should be." << std::endl;
    std::cerr << *output << std::endl;
    return EXIT_FAILURE;
  }

  // Now test a block selection
  svtkNew<svtkSelectionSource> selectionSource;
  selectionSource->SetContentType(svtkSelectionNode::BLOCKS);
  selectionSource->AddBlock(2);

  svtkNew<svtkExtractSelection> extract;
  extract->SetInputDataObject(mb0);
  extract->SetSelectionConnection(selectionSource->GetOutputPort());
  extract->Update();

  output = svtkMultiBlockDataSet::SafeDownCast(extract->GetOutput());
  if (output->GetBlock(0) != nullptr || output->GetBlock(1) == nullptr)
  {
    std::cerr << "Invalid block extracted. Should be block 1." << std::endl;
    std::cerr << *output << std::endl;
    return EXIT_FAILURE;
  }
  outputFDArray = svtkIntArray::SafeDownCast(output->GetFieldData()->GetArray(globalIdName.c_str()));
  if (outputFDArray == nullptr || outputFDArray->GetValue(0) != 5)
  {
    std::cerr << "Field data not copied to output. Should be." << std::endl;
    return EXIT_FAILURE;
  }

  // now extract non-leaf block.
  selectionSource->RemoveAllBlocks();
  selectionSource->AddBlock(1);

  svtkNew<svtkMultiBlockDataSet> mb1;
  mb1->SetBlock(0, mb0);
  mb1->SetBlock(1, GetSphere(0, 0, 3));

  extract->SetInputDataObject(mb1);
  extract->Update();

  output = svtkMultiBlockDataSet::SafeDownCast(extract->GetOutput());
  if (svtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0)) == nullptr ||
    svtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0))->GetBlock(0) == nullptr ||
    svtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0))->GetBlock(1) == nullptr ||
    output->GetBlock(1) != nullptr)
  {
    std::cerr << "Incorrect non-leaf block extraction!" << std::endl;
    std::cerr << *output << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
