/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOverlappingAMRLevelIdScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOverlappingAMRLevelIdScalars
 * @brief   generate scalars from levels.
 *
 * svtkOverlappingAMRLevelIdScalars is a filter that generates scalars using
 * the level number for each level. Note that all datasets within a level get
 * the same scalar. The new scalars array is named \c LevelIdScalars.
 */

#ifndef svtkOverlappingAMRLevelIdScalars_h
#define svtkOverlappingAMRLevelIdScalars_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkOverlappingAMRAlgorithm.h"

class svtkUniformGrid;
class svtkUniformGridAMR;

class SVTKFILTERSGENERAL_EXPORT svtkOverlappingAMRLevelIdScalars : public svtkOverlappingAMRAlgorithm
{
public:
  static svtkOverlappingAMRLevelIdScalars* New();
  svtkTypeMacro(svtkOverlappingAMRLevelIdScalars, svtkOverlappingAMRAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkOverlappingAMRLevelIdScalars();
  ~svtkOverlappingAMRLevelIdScalars() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void AddColorLevels(svtkUniformGridAMR* input, svtkUniformGridAMR* output);
  svtkUniformGrid* ColorLevel(svtkUniformGrid* input, int group);

private:
  svtkOverlappingAMRLevelIdScalars(const svtkOverlappingAMRLevelIdScalars&) = delete;
  void operator=(const svtkOverlappingAMRLevelIdScalars&) = delete;
};

#endif
