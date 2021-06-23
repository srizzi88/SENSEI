/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCMLMoleculeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCMLMoleculeReader
 * @brief   Read a CML file and output a
 * svtkMolecule object
 *
 */

#ifndef svtkCMLMoleculeReader_h
#define svtkCMLMoleculeReader_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeAlgorithm.h"

class svtkMolecule;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkCMLMoleculeReader : public svtkMoleculeAlgorithm
{
public:
  static svtkCMLMoleculeReader* New();
  svtkTypeMacro(svtkCMLMoleculeReader, svtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the output (svtkMolecule) that the reader will fill
   */
  svtkMolecule* GetOutput();
  void SetOutput(svtkMolecule*) override;
  //@}

  //@{
  /**
   * Get/Set the name of the CML file
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkCMLMoleculeReader();
  ~svtkCMLMoleculeReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  char* FileName;

private:
  svtkCMLMoleculeReader(const svtkCMLMoleculeReader&) = delete;
  void operator=(const svtkCMLMoleculeReader&) = delete;
};

#endif
