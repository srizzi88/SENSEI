/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkHardwareSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkInteractorStyleRubberBandPick.h"
#include "svtkMolecule.h"
#include "svtkMoleculeMapper.h"
#include "svtkNew.h"
#include "svtkProp3DCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedAreaPicker.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkTrivialProducer.h"

class MoleculePickCommand : public svtkCommand
{
protected:
  svtkNew<svtkIdTypeArray> AtomIds;
  svtkNew<svtkIdTypeArray> BondIds;
  svtkRenderer* Renderer;
  svtkAreaPicker* Picker;
  svtkAlgorithm* MoleculeSource;
  svtkMoleculeMapper* MoleculeMapper;

public:
  static MoleculePickCommand* New() { return new MoleculePickCommand; }
  svtkTypeMacro(MoleculePickCommand, svtkCommand);

  MoleculePickCommand() = default;

  ~MoleculePickCommand() override = default;

  svtkIdTypeArray* GetAtomIds() { return this->AtomIds; }

  svtkIdTypeArray* GetBondIds() { return this->BondIds; }

  void SetRenderer(svtkRenderer* r) { this->Renderer = r; }

  void SetPicker(svtkAreaPicker* p) { this->Picker = p; }

  void SetMoleculeSource(svtkAlgorithm* m) { this->MoleculeSource = m; }

  void SetMoleculeMapper(svtkMoleculeMapper* m) { this->MoleculeMapper = m; }

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
      this->SetIdArrays(result);
      this->DumpMolSelection();
      result->Delete();
    }
  }

  // Set the ids for the atom/bond selection
  void SetIdArrays(svtkSelection* sel)
  {
    this->MoleculeMapper->GetSelectedAtomsAndBonds(sel, this->AtomIds, this->BondIds);
  }

  // Convenience function to print out the atom and bond ids that belong to
  // molMap and are contained in sel
  void DumpMolSelection()
  {
    svtkMolecule* mol = this->MoleculeMapper->GetInput();

    // Print selection
    cerr << "\n### Selection ###\n";
    cerr << "Atoms: ";
    for (svtkIdType i = 0; i < this->AtomIds->GetNumberOfTuples(); i++)
    {
      cerr << this->AtomIds->GetValue(i) << " ";
    }
    cerr << "\nBonds: ";
    for (svtkIdType i = 0; i < this->BondIds->GetNumberOfTuples(); i++)
    {
      svtkBond bond = mol->GetBond(this->BondIds->GetValue(i));
      cerr << bond.GetId() << " (" << bond.GetBeginAtomId() << "-" << bond.GetEndAtomId() << ") ";
    }
    cerr << endl;
  }
};

int TestMoleculeSelection(int argc, char* argv[])
{
  svtkNew<svtkMolecule> mol;

  // Use a trivial producer, since the molecule was created by hand
  svtkNew<svtkTrivialProducer> molSource;
  molSource->SetOutput(mol);

  // Create a 4x4 grid of atoms one angstrom apart
  svtkAtom a1 = mol->AppendAtom(1, 0.0, 0.0, 0.0);
  svtkAtom a2 = mol->AppendAtom(2, 0.0, 1.0, 0.0);
  svtkAtom a3 = mol->AppendAtom(3, 0.0, 2.0, 0.0);
  svtkAtom a4 = mol->AppendAtom(4, 0.0, 3.0, 0.0);
  svtkAtom a5 = mol->AppendAtom(5, 1.0, 0.0, 0.0);
  svtkAtom a6 = mol->AppendAtom(6, 1.0, 1.0, 0.0);
  svtkAtom a7 = mol->AppendAtom(7, 1.0, 2.0, 0.0);
  svtkAtom a8 = mol->AppendAtom(8, 1.0, 3.0, 0.0);
  svtkAtom a9 = mol->AppendAtom(9, 2.0, 0.0, 0.0);
  svtkAtom a10 = mol->AppendAtom(10, 2.0, 1.0, 0.0);
  svtkAtom a11 = mol->AppendAtom(11, 2.0, 2.0, 0.0);
  svtkAtom a12 = mol->AppendAtom(12, 2.0, 3.0, 0.0);
  svtkAtom a13 = mol->AppendAtom(13, 3.0, 0.0, 0.0);
  svtkAtom a14 = mol->AppendAtom(14, 3.0, 1.0, 0.0);
  svtkAtom a15 = mol->AppendAtom(15, 3.0, 2.0, 0.0);
  svtkAtom a16 = mol->AppendAtom(16, 3.0, 3.0, 0.0);

  // Add bonds along the grid
  mol->AppendBond(a1, a2, 1);
  mol->AppendBond(a2, a3, 1);
  mol->AppendBond(a3, a4, 1);
  mol->AppendBond(a5, a6, 1);
  mol->AppendBond(a6, a7, 1);
  mol->AppendBond(a7, a8, 1);
  mol->AppendBond(a9, a10, 1);
  mol->AppendBond(a10, a11, 1);
  mol->AppendBond(a11, a12, 1);
  mol->AppendBond(a13, a14, 1);
  mol->AppendBond(a14, a15, 1);
  mol->AppendBond(a15, a16, 1);
  mol->AppendBond(a1, a5, 1);
  mol->AppendBond(a2, a6, 1);
  mol->AppendBond(a3, a7, 1);
  mol->AppendBond(a4, a8, 1);
  mol->AppendBond(a5, a9, 1);
  mol->AppendBond(a6, a10, 1);
  mol->AppendBond(a7, a11, 1);
  mol->AppendBond(a8, a12, 1);
  mol->AppendBond(a9, a13, 1);
  mol->AppendBond(a10, a14, 1);
  mol->AppendBond(a11, a15, 1);
  mol->AppendBond(a12, a16, 1);

  // Set up render engine
  svtkNew<svtkMoleculeMapper> molmapper;
  molmapper->SetInputData(mol);
  molmapper->UseBallAndStickSettings();
  molmapper->SetAtomicRadiusTypeToUnitRadius();

  svtkNew<svtkActor> actor;
  actor->SetMapper(molmapper);

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
  // For easier debugging of clipping planes:
  ren->GetActiveCamera()->ParallelProjectionOn();
  ren->GetActiveCamera()->Zoom(2.2);

  // Setup picker
  svtkNew<svtkInteractorStyleRubberBandPick> pickerInt;
  iren->SetInteractorStyle(pickerInt);
  svtkNew<svtkRenderedAreaPicker> picker;
  iren->SetPicker(picker);

  // We'll follow up the cheap RenderedAreaPick with a detailed selection
  // to obtain the atoms and bonds.
  svtkNew<MoleculePickCommand> com;
  com->SetRenderer(ren);
  com->SetPicker(picker);
  com->SetMoleculeSource(molSource);
  com->SetMoleculeMapper(molmapper);
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
  if (com->GetAtomIds()->GetNumberOfTuples() < 4 || com->GetAtomIds()->GetValue(0) != 0 ||
    com->GetAtomIds()->GetValue(1) != 1 || com->GetAtomIds()->GetValue(2) != 4 ||
    com->GetAtomIds()->GetValue(3) != 5 || com->GetBondIds()->GetNumberOfTuples() < 8 ||
    com->GetBondIds()->GetValue(0) != 0 || com->GetBondIds()->GetValue(1) != 1 ||
    com->GetBondIds()->GetValue(2) != 3 || com->GetBondIds()->GetValue(3) != 4 ||
    com->GetBondIds()->GetValue(4) != 12 || com->GetBondIds()->GetValue(5) != 13 ||
    com->GetBondIds()->GetValue(6) != 16 || com->GetBondIds()->GetValue(7) != 17)
  {
    cerr << "Incorrect atoms/bonds picked! (if any picks were performed inter"
            "actively this could be ignored).\n";
    return EXIT_FAILURE;
  }

  return !retVal;
}
