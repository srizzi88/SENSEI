/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkLevelIdScalars.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkLevelIdScalars
 *
 *
 *  Empty class for backwards compatibility.
 */

#ifndef svtkLevelIdScalars_h
#define svtkLevelIdScalars_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkOverlappingAMRLevelIdScalars.h"

class SVTKFILTERSGENERAL_EXPORT svtkLevelIdScalars : public svtkOverlappingAMRLevelIdScalars
{
public:
  static svtkLevelIdScalars* New();
  svtkTypeMacro(svtkLevelIdScalars, svtkOverlappingAMRLevelIdScalars);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkLevelIdScalars();
  ~svtkLevelIdScalars() override;

private:
  svtkLevelIdScalars(const svtkLevelIdScalars&) = delete;
  void operator=(const svtkLevelIdScalars&) = delete;
};

#endif /* SVTKLEVELIDSCALARS_H_ */
