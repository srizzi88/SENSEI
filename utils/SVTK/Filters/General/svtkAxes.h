/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAxes
 * @brief   create an x-y-z axes
 *
 * svtkAxes creates three lines that form an x-y-z axes. The origin of the
 * axes is user specified (0,0,0 is default), and the size is specified with
 * a scale factor. Three scalar values are generated for the three lines and
 * can be used (via color map) to indicate a particular coordinate axis.
 */

#ifndef svtkAxes_h
#define svtkAxes_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkAxes : public svtkPolyDataAlgorithm
{
public:
  static svtkAxes* New();

  svtkTypeMacro(svtkAxes, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the origin of the axes.
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVectorMacro(Origin, double, 3);
  //@}

  //@{
  /**
   * Set the scale factor of the axes. Used to control size.
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * If Symmetric is on, the axis continue to negative values.
   */
  svtkSetMacro(Symmetric, svtkTypeBool);
  svtkGetMacro(Symmetric, svtkTypeBool);
  svtkBooleanMacro(Symmetric, svtkTypeBool);
  //@}

  //@{
  /**
   * Option for computing normals.  By default they are computed.
   */
  svtkSetMacro(ComputeNormals, svtkTypeBool);
  svtkGetMacro(ComputeNormals, svtkTypeBool);
  svtkBooleanMacro(ComputeNormals, svtkTypeBool);
  //@}

protected:
  svtkAxes();
  ~svtkAxes() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  // This source does not know how to generate pieces yet.
  int ComputeDivisionExtents(svtkDataObject* output, int idx, int numDivisions);

  double Origin[3];
  double ScaleFactor;

  svtkTypeBool Symmetric;
  svtkTypeBool ComputeNormals;

private:
  svtkAxes(const svtkAxes&) = delete;
  void operator=(const svtkAxes&) = delete;
};

#endif
