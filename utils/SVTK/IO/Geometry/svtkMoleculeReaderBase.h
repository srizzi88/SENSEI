/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeReaderBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMoleculeReaderBase
 * @brief   read Molecular Data files
 *
 * svtkMoleculeReaderBase is a source object that reads Molecule files
 * The FileName must be specified
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class
 */

#ifndef svtkMoleculeReaderBase_h
#define svtkMoleculeReaderBase_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCellArray;
class svtkFloatArray;
class svtkDataArray;
class svtkIdTypeArray;
class svtkUnsignedCharArray;
class svtkPoints;
class svtkStringArray;
class svtkMolecule;

class SVTKIOGEOMETRY_EXPORT svtkMoleculeReaderBase : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkMoleculeReaderBase, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

  //@{
  /**
   * A scaling factor to compute bonds between non-hydrogen atoms
   */
  svtkSetMacro(BScale, double);
  svtkGetMacro(BScale, double);
  //@}

  //@{
  /**
   * A scaling factor to compute bonds with hydrogen atoms.
   */
  svtkSetMacro(HBScale, double);
  svtkGetMacro(HBScale, double);
  //@}

  svtkGetMacro(NumberOfAtoms, int);

protected:
  svtkMoleculeReaderBase();
  ~svtkMoleculeReaderBase() override;

  char* FileName;
  double BScale;
  double HBScale;
  int NumberOfAtoms;

  int FillOutputPortInformation(int, svtkInformation*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int ReadMolecule(FILE* fp, svtkPolyData* output);
  int MakeAtomType(const char* atype);
  int MakeBonds(svtkPoints*, svtkIdTypeArray*, svtkCellArray*);

  svtkMolecule* Molecule;
  svtkPoints* Points;
  svtkUnsignedCharArray* RGB;
  svtkFloatArray* Radii;
  svtkIdTypeArray* AtomType;
  svtkStringArray* AtomTypeStrings;
  svtkIdTypeArray* Residue;
  svtkUnsignedCharArray* Chain;
  svtkUnsignedCharArray* SecondaryStructures;
  svtkUnsignedCharArray* SecondaryStructuresBegin;
  svtkUnsignedCharArray* SecondaryStructuresEnd;
  svtkUnsignedCharArray* IsHetatm;

  virtual void ReadSpecificMolecule(FILE* fp) = 0;

private:
  svtkMoleculeReaderBase(const svtkMoleculeReaderBase&) = delete;
  void operator=(const svtkMoleculeReaderBase&) = delete;
};

#endif
