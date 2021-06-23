/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointSetToMoleculeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkMolecule.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPointSetToMoleculeFilter.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkUnsignedShortArray.h"

#define CheckNumbers(name, first, second)                                                          \
  if (first != second)                                                                             \
  {                                                                                                \
    cerr << "Error : wrong number of " << #name << ". Got " << first << " but expects " << second  \
         << endl;                                                                                  \
    return EXIT_FAILURE;                                                                           \
  }

int TestPointSetToMoleculeFilter(int, char*[])
{
  svtkNew<svtkPolyData> polyData;
  svtkNew<svtkPoints> points;
  const int numberOfAtoms = 4;
  points->SetNumberOfPoints(numberOfAtoms);
  points->SetPoint(0, 0, 0, 0);
  points->SetPoint(1, 1, 1, 1);
  points->SetPoint(2, 2, 2, 2);
  points->SetPoint(3, 3, 3, 3);
  polyData->SetPoints(points);

  svtkNew<svtkUnsignedShortArray> scalars;
  scalars->SetNumberOfValues(numberOfAtoms);
  const unsigned short firstAtomicNb = 42;
  scalars->SetValue(0, firstAtomicNb);
  scalars->SetValue(1, firstAtomicNb + 1);
  scalars->SetValue(2, firstAtomicNb + 2);
  scalars->SetValue(3, firstAtomicNb + 3);
  scalars->SetName("scalarsData");
  polyData->GetPointData()->SetScalars(scalars);

  svtkNew<svtkDoubleArray> extraData;
  extraData->SetNumberOfValues(numberOfAtoms);
  extraData->SetValue(0, 0.0);
  extraData->SetValue(1, 0.1);
  extraData->SetValue(2, 0.2);
  extraData->SetValue(3, 0.3);
  extraData->SetName("ExtraData");
  polyData->GetPointData()->AddArray(extraData);

  svtkNew<svtkCellArray> cells;
  cells->InsertNextCell(2);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertNextCell(2);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(2);
  polyData->SetLines(cells);
  svtkNew<svtkUnsignedShortArray> cellData;
  cellData->SetNumberOfValues(2);
  cellData->SetValue(0, 2);
  cellData->SetValue(1, 2);
  cellData->SetName("Bond Orders Bis");
  polyData->GetCellData()->SetScalars(cellData);

  svtkNew<svtkPointSetToMoleculeFilter> filter;
  filter->SetInputData(polyData.Get());
  filter->Update();
  svtkMolecule* molecule = filter->GetOutput();

  if (!molecule)
  {
    cerr << "Output molecule was not initialized !" << endl;
    return EXIT_FAILURE;
  }

  CheckNumbers("atoms", molecule->GetNumberOfAtoms(), numberOfAtoms);
  CheckNumbers("bonds", molecule->GetNumberOfBonds(), polyData->GetNumberOfLines());

  // all arrays are copied + atomic number created from scalars
  int nbExpectedArrays = polyData->GetPointData()->GetNumberOfArrays() + 1;
  CheckNumbers("atom data arrays", molecule->GetAtomData()->GetNumberOfArrays(), nbExpectedArrays);

  // all arrays are copied + bond orders array
  nbExpectedArrays = polyData->GetCellData()->GetNumberOfArrays() + 1;
  CheckNumbers("bond data arrays", molecule->GetBondData()->GetNumberOfArrays(), nbExpectedArrays);

  svtkDataArray* atomicNumbers = molecule->GetAtomicNumberArray();
  if (!atomicNumbers)
  {
    cerr << "Error: No atomic numbers array was found." << endl;
    return EXIT_FAILURE;
  }
  CheckNumbers("atomic number value", atomicNumbers->GetTuple1(0), firstAtomicNb)

    filter->ConvertLinesIntoBondsOff();
  filter->Update();
  molecule = filter->GetOutput();

  CheckNumbers("bonds", molecule->GetNumberOfBonds(), 0);
  CheckNumbers("bond data arrays", molecule->GetBondData()->GetNumberOfArrays(), 1);

  return EXIT_SUCCESS;
}
