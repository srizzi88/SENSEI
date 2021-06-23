/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVASPTessellationReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkVASPTessellationReader
 * @brief   Read NPT_Z_TESSELLATE.out files.
 *
 *
 * Read NPT_Z_TESSELLATE.out files from VASP.
 */

#ifndef svtkVASPTessellationReader_h
#define svtkVASPTessellationReader_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeAlgorithm.h"

namespace svtksys
{
class RegularExpression;
}

class svtkUnstructuredGrid;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkVASPTessellationReader : public svtkMoleculeAlgorithm
{
public:
  static svtkVASPTessellationReader* New();
  svtkTypeMacro(svtkVASPTessellationReader, svtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name of the file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkVASPTessellationReader();
  ~svtkVASPTessellationReader() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inInfoVecs,
    svtkInformationVector* outInfoVec) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inInfoVecs,
    svtkInformationVector* outInfoVec) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Advance @a in to the start of the data for the next timestep. Parses the
   * "time = X" line, sets @a time to the timestamp, and returns true on
   * success. Returning false means either EOF was reached, or the timestamp
   * line could not be parsed.
   */
  bool NextTimeStep(std::istream& in, double& time);

  /**
   * Called by RequestData to determine which timestep to read. If both
   * UPDATE_TIME_STEP and TIME_STEPS are defined, return the index of the
   * timestep in TIME_STEPS closest to UPDATE_TIME_STEP. If either is undefined,
   * return 0.
   */
  size_t SelectTimeStepIndex(svtkInformation* info);

  bool ReadTimeStep(std::istream& in, svtkMolecule* molecule, svtkUnstructuredGrid* voronoi);

  char* FileName;

  svtksys::RegularExpression* TimeParser;
  svtksys::RegularExpression* LatticeParser;
  svtksys::RegularExpression* AtomCountParser;
  svtksys::RegularExpression* AtomParser;
  svtksys::RegularExpression* ParenExtract;

private:
  svtkVASPTessellationReader(const svtkVASPTessellationReader&) = delete;
  void operator=(const svtkVASPTessellationReader&) = delete;
};

#endif // svtkVASPTessellationReader_h
