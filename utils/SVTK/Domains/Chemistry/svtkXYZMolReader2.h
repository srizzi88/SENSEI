/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXYZMolReader2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXYZMolReader2
 * @brief   read Molecular Data files
 *
 * svtkXYZMolReader2 is a source object that reads Molecule files
 * The reader will detect multiple timesteps in an XYZ molecule file.
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class
 */

#ifndef svtkXYZMolReader2_h
#define svtkXYZMolReader2_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeAlgorithm.h"

#include <istream> // for std::istream
#include <vector>  // for std::vector

class svtkMolecule;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkXYZMolReader2 : public svtkMoleculeAlgorithm
{
public:
  static svtkXYZMolReader2* New();
  svtkTypeMacro(svtkXYZMolReader2, svtkMoleculeAlgorithm);
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
   * Get/Set the name of the XYZ Molecule file
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkXYZMolReader2();
  ~svtkXYZMolReader2() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;
  std::vector<istream::pos_type> file_positions; // to store beginning of each tstep
  std::vector<double> TimeSteps;

  int NumberOfTimeSteps;
  int NumberOfAtoms;

private:
  svtkXYZMolReader2(const svtkXYZMolReader2&) = delete;
  void operator=(const svtkXYZMolReader2&) = delete;
};

#endif
