/*=========================================================================

  Program:   ParaView
  Module:    svtkAppendArcLength.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAppendArcLength
 * @brief   appends Arc length for input poly lines.
 *
 * svtkAppendArcLength is used for filter such as plot-over-line. In such cases,
 * we need to add an attribute array that is the arc_length over the length of
 * the probed line. That's when svtkAppendArcLength can be used. It adds a new
 * point-data array named "arc_length" with the computed arc length for each of
 * the polylines in the input. For all other cell types, the arc length is set
 * to 0.
 * @warning
 * This filter assumes that cells don't share points.
 */

#ifndef svtkAppendArcLength_h
#define svtkAppendArcLength_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkAppendArcLength : public svtkPolyDataAlgorithm
{
public:
  static svtkAppendArcLength* New();
  svtkTypeMacro(svtkAppendArcLength, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkAppendArcLength();
  ~svtkAppendArcLength() override;

  //@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkAppendArcLength(const svtkAppendArcLength&) = delete;
  void operator=(const svtkAppendArcLength&) = delete;
  //@}
};

#endif
