/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPExtractSelectedArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPExtractSelectedArraysOverTime
 * @brief   extracts a selection over time.
 *
 * svtkPExtractSelectedArraysOverTime is a parallelized version of
 * svtkExtractSelectedArraysOverTime. It simply changes the types of internal
 * filters used to their parallelized versions. Thus instead of using
 * svtkExtractDataArraysOverTime over time, it's changed to
 * svtkPExtractDataArraysOverTime.
 *
 * @sa svtkExtractDataArraysOverTime, svtkPExtractDataArraysOverTime
 */

#ifndef svtkPExtractSelectedArraysOverTime_h
#define svtkPExtractSelectedArraysOverTime_h

#include "svtkExtractSelectedArraysOverTime.h"
#include "svtkFiltersParallelModule.h" // For export macro

class svtkMultiProcessController;
class SVTKFILTERSPARALLEL_EXPORT svtkPExtractSelectedArraysOverTime
  : public svtkExtractSelectedArraysOverTime
{
public:
  static svtkPExtractSelectedArraysOverTime* New();
  svtkTypeMacro(svtkPExtractSelectedArraysOverTime, svtkExtractSelectedArraysOverTime);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkMultiProcessController* GetController();
  //@}

protected:
  svtkPExtractSelectedArraysOverTime();
  ~svtkPExtractSelectedArraysOverTime() override;

private:
  svtkPExtractSelectedArraysOverTime(const svtkPExtractSelectedArraysOverTime&) = delete;
  void operator=(const svtkPExtractSelectedArraysOverTime&) = delete;
};

#endif
