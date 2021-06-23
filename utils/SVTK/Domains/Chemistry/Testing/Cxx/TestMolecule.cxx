/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMolecule.h"
#include "svtkNew.h"
#include "svtkVector.h"
#include "svtkVectorOperators.h"

// Example code from the molecule documentation. If this breaks,
// update the docs in svtkMolecule.h
bool MoleculeExampleCode1()
{
  svtkNew<svtkMolecule> mol;
  svtkAtom h1 = mol->AppendAtom(1, 0.0, 0.0, -0.5);
  svtkAtom h2 = mol->AppendAtom(1, 0.0, 0.0, 0.5);
  svtkBond b = mol->AppendBond(h1, h2, 1);
  int errors(0);

  if (fabs(b.GetLength() - 1.0) > 1e-8)
  {
    cout << "Error bond length incorrect. Expected 1.0, but got " << b.GetLength() << endl;
    ++errors;
  }

  if (!h1.GetPosition().Compare(svtkVector3f(0.0, 0.0, -0.5), 1e-8))
  {
    cout << "Error atom position incorrect. Expected 0.0, 0.0, -0.5 but got " << h1.GetPosition()
         << endl;
    ++errors;
  }

  if (!h2.GetPosition().Compare(svtkVector3f(0.0, 0.0, 0.5), 1e-8))
  {
    cout << "Error atom position incorrect. Expected 0.0, 0.0, 0.5 but got " << h2.GetPosition()
         << endl;
    ++errors;
  }

  if (h1.GetAtomicNumber() != 1)
  {
    cout << "Error atomic number incorrect. Expected 1 but got " << h1.GetAtomicNumber() << endl;
    ++errors;
  }

  if (h2.GetAtomicNumber() != 1)
  {
    cout << "Error atomic number incorrect. Expected 1 but got " << h2.GetAtomicNumber() << endl;
    ++errors;
  }

  return errors == 0;
}

// Example code from the molecule documentation. If this breaks,
// update the docs in svtkMolecule.h
bool MoleculeExampleCode2()
{
  svtkNew<svtkMolecule> mol;

  svtkAtom h1 = mol->AppendAtom();
  h1.SetAtomicNumber(1);
  h1.SetPosition(0.0, 0.0, -0.5);

  svtkAtom h2 = mol->AppendAtom();
  h2.SetAtomicNumber(1);
  svtkVector3f displacement(0.0, 0.0, 1.0);
  h2.SetPosition(h1.GetPosition() + displacement);

  svtkBond b = mol->AppendBond(h1, h2, 1);

  int errors(0);

  if (fabs(b.GetLength() - 1.0) > 1e-8)
  {
    cout << "Error bond length incorrect. Expected 1.0, but got " << b.GetLength() << endl;
    ++errors;
  }

  if (!h1.GetPosition().Compare(svtkVector3f(0.0, 0.0, -0.5), 1e-8))
  {
    cout << "Error atom position incorrect. Expected 0.0, 0.0, -0.5 but got " << h1.GetPosition()
         << endl;
    ++errors;
  }

  if (!h2.GetPosition().Compare(svtkVector3f(0.0, 0.0, 0.5), 1e-8))
  {
    cout << "Error atom position incorrect. Expected 0.0, 0.0, 0.5 but got " << h2.GetPosition()
         << endl;
    ++errors;
  }

  if (h1.GetAtomicNumber() != 1)
  {
    cout << "Error atomic number incorrect. Expected 1 but got " << h1.GetAtomicNumber() << endl;
    ++errors;
  }

  if (h2.GetAtomicNumber() != 1)
  {
    cout << "Error atomic number incorrect. Expected 1 but got " << h2.GetAtomicNumber() << endl;
    ++errors;
  }

  return errors == 0;
}

int TestMolecule(int, char*[])
{
  // Check that the example code given in the molecule docs compiles:
  bool test1 = MoleculeExampleCode1();
  bool test2 = MoleculeExampleCode2();

  return (test1 && test2) ? 0 : 1;
}
