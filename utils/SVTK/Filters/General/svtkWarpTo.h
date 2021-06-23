/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWarpTo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWarpTo
 * @brief   deform geometry by warping towards a point
 *
 * svtkWarpTo is a filter that modifies point coordinates by moving the
 * points towards a user specified position.
 */

#ifndef svtkWarpTo_h
#define svtkWarpTo_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkWarpTo : public svtkPointSetAlgorithm
{
public:
  static svtkWarpTo* New();
  svtkTypeMacro(svtkWarpTo, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the value to scale displacement.
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Set/Get the position to warp towards.
   */
  svtkGetVectorMacro(Position, double, 3);
  svtkSetVector3Macro(Position, double);
  //@}

  //@{
  /**
   * Set/Get the Absolute ivar. Turning Absolute on causes scale factor
   * of the new position to be one unit away from Position.
   */
  svtkSetMacro(Absolute, svtkTypeBool);
  svtkGetMacro(Absolute, svtkTypeBool);
  svtkBooleanMacro(Absolute, svtkTypeBool);
  //@}

  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkWarpTo();
  ~svtkWarpTo() override {}

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double ScaleFactor;
  double Position[3];
  svtkTypeBool Absolute;

private:
  svtkWarpTo(const svtkWarpTo&) = delete;
  void operator=(const svtkWarpTo&) = delete;
};

#endif
