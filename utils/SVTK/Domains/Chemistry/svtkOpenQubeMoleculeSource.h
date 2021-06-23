/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenQubeMoleculeSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenQubeMoleculeSource
 * @brief   Read a OpenQube readable file and output
 * a svtkMolecule object
 *
 *
 */

#ifndef svtkOpenQubeMoleculeSource_h
#define svtkOpenQubeMoleculeSource_h

#include "svtkDataReader.h"
#include "svtkDomainsChemistryModule.h" // For export macro

class svtkMolecule;

namespace OpenQube
{
class Molecule;
class BasisSet;
}

class SVTKDOMAINSCHEMISTRY_EXPORT svtkOpenQubeMoleculeSource : public svtkDataReader
{
public:
  static svtkOpenQubeMoleculeSource* New();
  svtkTypeMacro(svtkOpenQubeMoleculeSource, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent);

  //@{
  /**
   * Get/Set the output (svtkMolecule) that the reader will fill
   */
  svtkMolecule* GetOutput();
  void SetOutput(svtkMolecule*);
  //@}

  //@{
  /**
   * Get/Set the name of the OpenQube readable file.
   * @note: If both a source OpenQube BasisSet object and a FileName
   * have been set with SetBasisSet and SetFileName, the object takes
   * precedence over the file and the file will not be read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set the OpenQube BasisSet object to read from.
   * @note: If both a source OpenQube BasisSet object and a FileName
   * have been set with SetBasisSet and SetFileName, the object takes
   * precedence over the file and the file will not be read.
   */
  virtual void SetBasisSet(OpenQube::BasisSet* b);
  svtkGetMacro(BasisSet, OpenQube::BasisSet*);
  //@}

  //@{
  /**
   * Get/Set whether or not to take ownership of the BasisSet object. Defaults
   * to false when SetBasisSet is used and true when the basis is read from a
   * file set by SetFileName. Destroying this class or setting a new BasisSet
   * or FileName will delete the BasisSet if true.
   */
  svtkSetMacro(CleanUpBasisSet, bool);
  svtkGetMacro(CleanUpBasisSet, bool);
  svtkBooleanMacro(CleanUpBasisSet, bool);
  //@}

protected:
  svtkOpenQubeMoleculeSource();
  ~svtkOpenQubeMoleculeSource();

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  int FillOutputPortInformation(int, svtkInformation*);

  char* FileName;
  OpenQube::BasisSet* BasisSet;
  bool CleanUpBasisSet;

  /**
   * Copy the OpenQube::Molecule object @a oqmol into the provided
   * svtkMolecule object @a mol.
   */
  void CopyOQMoleculeToVtkMolecule(const OpenQube::Molecule* oqmol, svtkMolecule* mol);

private:
  svtkOpenQubeMoleculeSource(const svtkOpenQubeMoleculeSource&) = delete;
  void operator=(const svtkOpenQubeMoleculeSource&) = delete;
};

#endif
