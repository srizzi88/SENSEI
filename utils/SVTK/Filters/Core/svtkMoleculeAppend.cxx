/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeAppend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "svtkMoleculeAppend.h"

#include "svtkAlgorithmOutput.h"
#include "svtkDataArray.h"
#include "svtkDataSetAttributes.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkMolecule.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkUnsignedCharArray.h"

#include <set>
#include <utility>

svtkStandardNewMacro(svtkMoleculeAppend);

//----------------------------------------------------------------------------
svtkMoleculeAppend::svtkMoleculeAppend()
  : MergeCoincidentAtoms(true)
{
}

//----------------------------------------------------------------------------
svtkDataObject* svtkMoleculeAppend::GetInput(int idx)
{
  if (this->GetNumberOfInputConnections(0) <= idx)
  {
    return nullptr;
  }
  return svtkMolecule::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
int svtkMoleculeAppend::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkMolecule* output = svtkMolecule::GetData(outputVector, 0);
  svtkDataSetAttributes* outputAtomData = output->GetAtomData();
  svtkDataSetAttributes* outputBondData = output->GetBondData();

  // ********************
  // Create output data arrays following first input arrays.
  svtkMolecule* mol0 = svtkMolecule::SafeDownCast(this->GetInput());
  outputAtomData->CopyStructure(mol0->GetAtomData());
  outputBondData->CopyStructure(mol0->GetBondData());
  output->SetAtomicNumberArrayName(mol0->GetAtomicNumberArrayName());
  output->SetBondOrdersArrayName(mol0->GetBondOrdersArrayName());
  svtkUnsignedCharArray* outputGhostAtoms = output->GetAtomGhostArray();
  svtkUnsignedCharArray* outputGhostBonds = output->GetBondGhostArray();

  // ********************
  // Initialize unique atoms/bonds containers
  svtkNew<svtkMergePoints> uniquePoints;
  svtkNew<svtkPoints> uniquePointsList;
  double bounds[6] = { 0., 0., 0., 0., 0., 0. };
  uniquePoints->InitPointInsertion(uniquePointsList, bounds, 0);
  std::set<std::pair<svtkIdType, svtkIdType> > uniqueBonds;

  // ********************
  // Process each input
  for (int idx = 0; idx < this->GetNumberOfInputConnections(0); ++idx)
  {
    svtkMolecule* input = svtkMolecule::GetData(inputVector[0], idx);

    // --------------------
    // Sanity check on input
    int inputNbAtomArrays = input->GetAtomData()->GetNumberOfArrays();
    if (inputNbAtomArrays != outputAtomData->GetNumberOfArrays())
    {
      svtkErrorMacro(<< "Input " << idx << ": Wrong number of atom array. Has " << inputNbAtomArrays
                    << " instead of " << outputAtomData->GetNumberOfArrays());
      return 0;
    }

    int inputNbBondArrays = input->GetBondData()->GetNumberOfArrays();
    if (input->GetNumberOfBonds() > 0 && inputNbBondArrays != outputBondData->GetNumberOfArrays())
    {
      svtkErrorMacro(<< "Input " << idx << ": Wrong number of bond array. Has " << inputNbBondArrays
                    << " instead of " << outputBondData->GetNumberOfArrays());
      return 0;
    }

    for (svtkIdType ai = 0; ai < inputNbAtomArrays; ai++)
    {
      svtkAbstractArray* inArray = input->GetAtomData()->GetAbstractArray(ai);
      if (!this->CheckArrays(inArray, outputAtomData->GetAbstractArray(inArray->GetName())))
      {
        svtkErrorMacro(<< "Input " << idx << ": atoms arrays do not match with output");
        return 0;
      }
    }

    for (svtkIdType ai = 0; ai < inputNbBondArrays; ai++)
    {
      svtkAbstractArray* inArray = input->GetBondData()->GetAbstractArray(ai);
      if (!this->CheckArrays(inArray, outputBondData->GetAbstractArray(inArray->GetName())))
      {
        svtkErrorMacro(<< "Input " << idx << ": bonds arrays do not match with output");
        return 0;
      }
    }

    // --------------------
    // add atoms and bonds without duplication

    // map from 'input molecule atom ids' to 'output molecule atom ids'
    std::vector<svtkIdType> atomIdMap(input->GetNumberOfAtoms(), -1);

    svtkIdType previousNbOfAtoms = output->GetNumberOfAtoms();
    int nbOfAtoms = 0;
    for (svtkIdType i = 0; i < input->GetNumberOfAtoms(); i++)
    {
      double pt[3];
      input->GetAtomicPositionArray()->GetPoint(i, pt);
      bool addAtom = true;
      if (this->MergeCoincidentAtoms)
      {
        addAtom = uniquePoints->InsertUniquePoint(pt, atomIdMap[i]) == 1;
      }
      else
      {
        atomIdMap[i] = previousNbOfAtoms + nbOfAtoms;
      }

      if (addAtom)
      {
        nbOfAtoms++;
        svtkAtom atom = input->GetAtom(i);
        output->AppendAtom(atom.GetAtomicNumber(), atom.GetPosition()).GetId();
        if (outputGhostAtoms)
        {
          outputGhostAtoms->InsertValue(atomIdMap[i], 255);
        }
      }
    }
    svtkIdType previousNbOfBonds = output->GetNumberOfBonds();
    int nbOfBonds = 0;
    for (svtkIdType i = 0; i < input->GetNumberOfBonds(); i++)
    {
      svtkBond bond = input->GetBond(i);
      // as bonds are undirected, put min atom number at first to avoid duplication.
      svtkIdType atom1 = atomIdMap[bond.GetBeginAtomId()];
      svtkIdType atom2 = atomIdMap[bond.GetEndAtomId()];
      auto result =
        uniqueBonds.insert(std::make_pair(std::min(atom1, atom2), std::max(atom1, atom2)));
      if (result.second)
      {
        nbOfBonds++;
        output->AppendBond(atom1, atom2, bond.GetOrder());
      }
    }

    // --------------------
    // Reset arrays size (and allocation if needed)
    for (svtkIdType ai = 0; ai < input->GetAtomData()->GetNumberOfArrays(); ai++)
    {
      svtkAbstractArray* inArray = input->GetAtomData()->GetAbstractArray(ai);
      svtkAbstractArray* outArray = output->GetAtomData()->GetAbstractArray(inArray->GetName());
      outArray->Resize(previousNbOfAtoms + nbOfAtoms);
    }

    for (svtkIdType ai = 0; ai < input->GetBondData()->GetNumberOfArrays(); ai++)
    {
      // skip bond orders array as it is auto-filled by AppendBond method
      svtkAbstractArray* inArray = input->GetBondData()->GetAbstractArray(ai);
      if (!strcmp(inArray->GetName(), input->GetBondOrdersArrayName()))
      {
        continue;
      }
      svtkAbstractArray* outArray = output->GetBondData()->GetAbstractArray(inArray->GetName());
      outArray->Resize(previousNbOfBonds + nbOfBonds);
    }

    // --------------------
    // Fill DataArrays
    for (svtkIdType i = 0; i < input->GetNumberOfAtoms(); i++)
    {
      for (svtkIdType ai = 0; ai < input->GetAtomData()->GetNumberOfArrays(); ai++)
      {
        svtkAbstractArray* inArray = input->GetAtomData()->GetAbstractArray(ai);
        svtkAbstractArray* outArray = output->GetAtomData()->GetAbstractArray(inArray->GetName());
        // Use Value of non-ghost atom.
        if (outputGhostAtoms && outputGhostAtoms->GetValue(atomIdMap[i]) == 0)
        {
          continue;
        }
        outArray->InsertTuple(atomIdMap[i], i, inArray);
      }
    }
    for (svtkIdType i = 0; i < input->GetNumberOfBonds(); i++)
    {
      svtkBond bond = input->GetBond(i);
      svtkIdType outputBondId =
        output->GetBondId(atomIdMap[bond.GetBeginAtomId()], atomIdMap[bond.GetEndAtomId()]);

      for (svtkIdType ai = 0; ai < input->GetBondData()->GetNumberOfArrays(); ai++)
      {
        // skip bond orders array as it is auto-filled by AppendBond method
        svtkAbstractArray* inArray = input->GetBondData()->GetAbstractArray(ai);
        if (!strcmp(inArray->GetName(), input->GetBondOrdersArrayName()))
        {
          continue;
        }
        svtkAbstractArray* outArray = output->GetBondData()->GetAbstractArray(inArray->GetName());
        outArray->InsertTuple(outputBondId, i, inArray);
      }
    }
  }

  if (outputGhostBonds)
  {
    outputGhostBonds->SetNumberOfTuples(output->GetNumberOfBonds());
    outputGhostBonds->Fill(0);
    for (svtkIdType bondId = 0; bondId < output->GetNumberOfBonds(); bondId++)
    {
      svtkIdType atom1 = output->GetBond(bondId).GetBeginAtomId();
      svtkIdType atom2 = output->GetBond(bondId).GetEndAtomId();
      if (outputGhostAtoms->GetValue(atom1) == 1 || outputGhostAtoms->GetValue(atom2) == 1)
      {
        outputGhostBonds->SetValue(bondId, 1);
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkMoleculeAppend::FillInputPortInformation(int i, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(i, info);
}

//----------------------------------------------------------------------------
bool svtkMoleculeAppend::CheckArrays(svtkAbstractArray* array1, svtkAbstractArray* array2)
{
  if (strcmp(array1->GetName(), array2->GetName()))
  {
    svtkErrorMacro(<< "Execute: input name (" << array1->GetName() << "), must match output name ("
                  << array2->GetName() << ")");
    return false;
  }

  if (array1->GetDataType() != array2->GetDataType())
  {
    svtkErrorMacro(<< "Execute: input ScalarType (" << array1->GetDataType()
                  << "), must match output ScalarType (" << array2->GetDataType() << ")");
    return false;
  }

  if (array1->GetNumberOfComponents() != array2->GetNumberOfComponents())
  {
    svtkErrorMacro("Components of the inputs do not match");
    return false;
  }

  return true;
}
