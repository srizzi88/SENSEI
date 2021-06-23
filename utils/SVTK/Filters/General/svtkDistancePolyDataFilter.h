/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistancePolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDistancePolyDataFilter
 *
 *
 * Computes the signed distance from one svtkPolyData to another. The
 * signed distance to the second input is computed at every point in
 * the first input using svtkImplicitPolyDataDistance. Optionally, the signed
 * distance to the first input at every point in the second input can
 * be computed. This may be enabled by calling
 * ComputeSecondDistanceOn().
 *
 * If the signed distance is not desired, the unsigned distance can be
 * computed by calling SignedDistanceOff(). The signed distance field
 * may be negated by calling NegateDistanceOn();
 *
 * This code was contributed in the SVTK Journal paper:
 * "Boolean Operations on Surfaces in SVTK Without External Libraries"
 * by Cory Quammen, Chris Weigle C., Russ Taylor
 * http://hdl.handle.net/10380/3262
 * http://www.midasjournal.org/browse/publication/797
 */

#ifndef svtkDistancePolyDataFilter_h
#define svtkDistancePolyDataFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkDistancePolyDataFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkDistancePolyDataFilter* New();
  svtkTypeMacro(svtkDistancePolyDataFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Enable/disable computation of the signed distance between
   * the first poly data and the second poly data. Defaults to on.
   */
  svtkSetMacro(SignedDistance, svtkTypeBool);
  svtkGetMacro(SignedDistance, svtkTypeBool);
  svtkBooleanMacro(SignedDistance, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable negation of the distance values. Defaults to
   * off. Has no effect if SignedDistance is off.
   */
  svtkSetMacro(NegateDistance, svtkTypeBool);
  svtkGetMacro(NegateDistance, svtkTypeBool);
  svtkBooleanMacro(NegateDistance, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable computation of a second output poly data with the
   * distance from the first poly data at each point. Defaults to on.
   */
  svtkSetMacro(ComputeSecondDistance, svtkTypeBool);
  svtkGetMacro(ComputeSecondDistance, svtkTypeBool);
  svtkBooleanMacro(ComputeSecondDistance, svtkTypeBool);
  //@}

  /**
   * Get the second output, which is a copy of the second input with an
   * additional distance scalar field.
   * Note this will return a valid data object only after this->Update() is
   * called.
   */
  svtkPolyData* GetSecondDistanceOutput();

  //@{
  /**
   * Enable/disable computation of cell-center distance to the
   * second poly data. Defaults to on for backwards compatibility.
   *
   * If the first poly data consists of just vertex cells,
   * computing point and cell-center distances is redundant.
   */
  svtkSetMacro(ComputeCellCenterDistance, svtkTypeBool);
  svtkGetMacro(ComputeCellCenterDistance, svtkTypeBool);
  svtkBooleanMacro(ComputeCellCenterDistance, svtkTypeBool);
  //@}

protected:
  svtkDistancePolyDataFilter();
  ~svtkDistancePolyDataFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void GetPolyDataDistance(svtkPolyData*, svtkPolyData*);

private:
  svtkDistancePolyDataFilter(const svtkDistancePolyDataFilter&) = delete;
  void operator=(const svtkDistancePolyDataFilter&) = delete;

  svtkTypeBool SignedDistance;
  svtkTypeBool NegateDistance;
  svtkTypeBool ComputeSecondDistance;
  svtkTypeBool ComputeCellCenterDistance;
};

#endif
