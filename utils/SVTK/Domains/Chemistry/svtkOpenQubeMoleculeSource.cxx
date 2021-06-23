/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenQubeMoleculeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "svtkOpenQubeMoleculeSource.h"

#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkObjectFactory.h"
#include "svtkOpenQubeElectronicData.h"

#include <openqube/basisset.h>
#include <openqube/basissetloader.h>
#include <openqube/molecule.h>

#include <vector>

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenQubeMoleculeSource);

//----------------------------------------------------------------------------
svtkOpenQubeMoleculeSource::svtkOpenQubeMoleculeSource()
  : svtkDataReader()
  , FileName(nullptr)
  , CleanUpBasisSet(false)
{
}

//----------------------------------------------------------------------------
svtkOpenQubeMoleculeSource::~svtkOpenQubeMoleculeSource()
{
  this->SetFileName(nullptr);
  if (this->CleanUpBasisSet)
  {
    delete this->BasisSet;
    this->BasisSet = nullptr;
  }
}

//----------------------------------------------------------------------------
svtkMolecule* svtkOpenQubeMoleculeSource::GetOutput()
{
  return svtkMolecule::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void svtkOpenQubeMoleculeSource::SetOutput(svtkMolecule* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
void svtkOpenQubeMoleculeSource::SetBasisSet(OpenQube::BasisSet* b)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting BasisSet to " << b);
  if (this->BasisSet != b)
  {
    if (this->CleanUpBasisSet)
    {
      delete this->BasisSet;
    }
    this->BasisSet = b;
    this->CleanUpBasisSetOff();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkOpenQubeMoleculeSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkMolecule* output = svtkMolecule::SafeDownCast(svtkDataObject::GetData(outputVector));

  if (!output)
  {
    svtkWarningMacro(<< "svtkOpenQubeMoleculeSource does not have a svtkMolecule "
                       "as output.");
    return 1;
  }

  // Obtain basis set
  OpenQube::BasisSet* basisSet = 0;
  if (this->BasisSet)
  {
    basisSet = this->BasisSet;
  }
  else
  {
    if (!this->FileName)
    {
      svtkWarningMacro(<< "No FileName or OpenQube::BasisSet specified.");
      return 1;
    }
    // We're creating the BasisSet, so we need to clean it up
    this->CleanUpBasisSetOn();
    // Huge padding, better safe than sorry.
    char basisName[strlen(this->FileName) + 256];
    OpenQube::BasisSetLoader::MatchBasisSet(this->FileName, basisName);
    if (!basisName[0])
    {
      svtkErrorMacro(<< "OpenQube cannot find matching basis set file for '" << this->FileName
                    << "'");
      return 1;
    }
    basisSet = OpenQube::BasisSetLoader::LoadBasisSet(basisName);
    this->BasisSet = basisSet;
    svtkDebugMacro(<< "Loaded basis set file: " << basisName);
  }

  // Populate svtkMolecule
  const OpenQube::Molecule& oqmol = basisSet->moleculeRef();
  this->CopyOQMoleculeToVtkMolecule(&oqmol, output);

  // Add ElectronicData
  svtkNew<svtkOpenQubeElectronicData> oqed;
  oqed->SetBasisSet(basisSet);
  output->SetElectronicData(oqed);

  return 1;
}

//----------------------------------------------------------------------------
int svtkOpenQubeMoleculeSource::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMolecule");
  return 1;
}

//----------------------------------------------------------------------------
void svtkOpenQubeMoleculeSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << this->FileName << "\n";
}

//----------------------------------------------------------------------------
void svtkOpenQubeMoleculeSource::CopyOQMoleculeToVtkMolecule(
  const OpenQube::Molecule* oqmol, svtkMolecule* mol)
{
  mol->Initialize();
  // Copy atoms
  Eigen::Vector3d pos;
  for (size_t i = 0; i < oqmol->numAtoms(); ++i)
  {
    svtkAtom atom = mol->AppendAtom();
    pos = oqmol->atomPos(i);
    atom.SetPosition(svtkVector3d(pos.data()).Cast<float>().GetData());
    atom.SetAtomicNumber(oqmol->atomAtomicNumber(i));
  }

  // TODO copy bonds (OQ doesn't currently have bonds)
}
