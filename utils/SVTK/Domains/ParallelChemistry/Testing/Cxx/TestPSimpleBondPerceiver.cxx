/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPSimpleBondPerceiver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkMolecule.h"
#include "svtkNew.h"
#include "svtkPSimpleBondPerceiver.h"
#include "svtkPeriodicTable.h"
#include "svtkUnsignedCharArray.h"

int TestPSimpleBondPerceiver(int argc, char* argv[])
{
  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  assert("pre: Controller should not be nullptr" && (controller != nullptr));
  svtkMultiProcessController::SetGlobalController(controller);

  const int rank = controller->GetLocalProcessId();

  svtkNew<svtkMolecule> mol;
  svtkNew<svtkPSimpleBondPerceiver> bonder;

  const svtkIdType atomicNb = 4;
  svtkNew<svtkPeriodicTable> periodicTable;
  // assert table is as expected when writing the test. cf values in svtkBlueObeliskDataInternal.h
  if (periodicTable->GetCovalentRadius(atomicNb) != 0.9f)
  {
    std::cout << "Warning : covalent radius from periodic table has changed since the test has "
                 "been written."
              << std::endl;
  }

  // First create a test molecule:
  mol->AppendAtom(atomicNb, 2.5 * rank, 0, 0);
  mol->AppendAtom(atomicNb, 2.5 * rank, 2, 0);
  mol->AppendAtom(atomicNb, 2.5 * rank, 0, 2);
  mol->AppendAtom(atomicNb, 2.5 * rank, 2, 2);
  bonder->SetInputData(mol);

  bonder->SetIsToleranceAbsolute(false);
  bonder->SetTolerance(1.15);
  bonder->Update();

  svtkIdType numOfBonds = bonder->GetOutput()->GetNumberOfBonds();
  // bonds only  between the 4 atoms of the rank. (no diagonals)
  svtkIdType expectedNumOfBonds = 4;
  if (numOfBonds != expectedNumOfBonds)
  {
    std::cout << "Case 1.15 relative : Wrong number of bonds for (have " << numOfBonds
              << " instead of " << expectedNumOfBonds << ")" << std::endl;
    return EXIT_FAILURE;
  }

  bonder->SetTolerance(1.5);
  bonder->Update();
  numOfBonds = bonder->GetOutput()->GetNumberOfBonds();
  // bonds between the 4 atoms of the rank AND between rank.
  expectedNumOfBonds = 8;
  if (numOfBonds != expectedNumOfBonds)
  {
    std::cout << "Case 1.5 relative : Wrong number of bonds ! (have " << numOfBonds
              << " instead of " << expectedNumOfBonds << ")" << std::endl;
    return EXIT_FAILURE;
  }

  bonder->SetIsToleranceAbsolute(true);
  bonder->SetTolerance(0.4);
  bonder->Update();
  numOfBonds = bonder->GetOutput()->GetNumberOfBonds();
  // bonds between the 4 atoms of the rank.
  expectedNumOfBonds = 4;
  if (numOfBonds != expectedNumOfBonds)
  {
    std::cout << "Case 0.4 absolute : Wrong number of bonds ! (have " << numOfBonds
              << " instead of " << expectedNumOfBonds << ")" << std::endl;
    return EXIT_FAILURE;
  }

  bonder->SetTolerance(0.8);
  bonder->Update();
  numOfBonds = bonder->GetOutput()->GetNumberOfBonds();
  // bonds between the 4 atoms of the rank AND between rank.
  expectedNumOfBonds = 8;
  if (numOfBonds != expectedNumOfBonds)
  {
    std::cout << "Case 0.8 absolute : Wrong number of bonds ! (have " << numOfBonds
              << " instead of " << expectedNumOfBonds << ")" << std::endl;
    return EXIT_FAILURE;
  }

  controller->Finalize();

  return EXIT_SUCCESS;
}
