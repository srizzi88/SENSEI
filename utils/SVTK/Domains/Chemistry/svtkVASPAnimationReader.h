/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVASPAnimationReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkVASPAnimationReader
 * @brief   Reader for VASP animation files.
 *
 *
 * Reads VASP animation files (e.g. NPT_Z_ANIMATE.out).
 */

#ifndef svtkVASPAnimationReader_h
#define svtkVASPAnimationReader_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeAlgorithm.h"
#include "svtkVector.h" // For svtkVector3f

namespace svtksys
{
class RegularExpression;
}

class SVTKDOMAINSCHEMISTRY_EXPORT svtkVASPAnimationReader : public svtkMoleculeAlgorithm
{
public:
  static svtkVASPAnimationReader* New();
  svtkTypeMacro(svtkVASPAnimationReader, svtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name of the file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkVASPAnimationReader();
  ~svtkVASPAnimationReader() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inInfoVecs,
    svtkInformationVector* outInfoVec) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inInfoVecs,
    svtkInformationVector* outInfoVec) override;

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

  bool ReadMolecule(std::istream& in, svtkMolecule* molecule);

  char* FileName;

  svtksys::RegularExpression* TimeParser;
  svtksys::RegularExpression* LatticeParser;
  svtksys::RegularExpression* AtomCountParser;
  svtksys::RegularExpression* AtomParser;

private:
  svtkVASPAnimationReader(const svtkVASPAnimationReader&) = delete;
  void operator=(const svtkVASPAnimationReader&) = delete;
};

#endif // svtkVASPAnimationReader_h
