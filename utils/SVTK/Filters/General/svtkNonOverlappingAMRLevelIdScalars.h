/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNonOverlappingAMRLevelIdScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkNonOverlappingAMRLevelIdScalars
 * @brief   generate scalars from levels.
 *
 * svtkNonOverlappingAMRLevelIdScalars is a filter that generates scalars using
 * the level number for each level. Note that all datasets within a level get
 * the same scalar. The new scalars array is named \c LevelIdScalars.
 */

#ifndef svtkNonOverlappingAMRLevelIdScalars_h
#define svtkNonOverlappingAMRLevelIdScalars_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkNonOverlappingAMRAlgorithm.h"

class svtkUniformGrid;
class svtkUniformGridAMR;

class SVTKFILTERSGENERAL_EXPORT svtkNonOverlappingAMRLevelIdScalars
  : public svtkNonOverlappingAMRAlgorithm
{
public:
  static svtkNonOverlappingAMRLevelIdScalars* New();
  svtkTypeMacro(svtkNonOverlappingAMRLevelIdScalars, svtkNonOverlappingAMRAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent);

protected:
  svtkNonOverlappingAMRLevelIdScalars();
  ~svtkNonOverlappingAMRLevelIdScalars() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  void AddColorLevels(svtkUniformGridAMR* input, svtkUniformGridAMR* output);
  svtkUniformGrid* ColorLevel(svtkUniformGrid* input, int group);

private:
  svtkNonOverlappingAMRLevelIdScalars(const svtkNonOverlappingAMRLevelIdScalars&) = delete;
  void operator=(const svtkNonOverlappingAMRLevelIdScalars&) = delete;
};

#endif
