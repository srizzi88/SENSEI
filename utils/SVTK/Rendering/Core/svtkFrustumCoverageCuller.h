/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFrustumCoverageCuller.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFrustumCoverageCuller
 * @brief   cull props based on frustum coverage
 *
 * svtkFrustumCoverageCuller will cull props based on the coverage in
 * the view frustum. The coverage is computed by enclosing the prop in
 * a bounding sphere, projecting that to the viewing coordinate system, then
 * taking a slice through the view frustum at the center of the sphere. This
 * results in a circle on the plane slice through the view frustum. This
 * circle is enclosed in a squared, and the fraction of the plane slice that
 * this square covers is the coverage. This is a number between 0 and 1.
 * If the number is less than the MinimumCoverage, the allocated render time
 * for that prop is set to zero. If it is greater than the MaximumCoverage,
 * the allocated render time is set to 1.0. In between, a linear ramp is used
 * to convert coverage into allocated render time.
 *
 * @sa
 * svtkCuller
 */

#ifndef svtkFrustumCoverageCuller_h
#define svtkFrustumCoverageCuller_h

#include "svtkCuller.h"
#include "svtkRenderingCoreModule.h" // For export macro

#define SVTK_CULLER_SORT_NONE 0
#define SVTK_CULLER_SORT_FRONT_TO_BACK 1
#define SVTK_CULLER_SORT_BACK_TO_FRONT 2

class svtkProp;
class svtkRenderer;

class SVTKRENDERINGCORE_EXPORT svtkFrustumCoverageCuller : public svtkCuller
{
public:
  static svtkFrustumCoverageCuller* New();
  svtkTypeMacro(svtkFrustumCoverageCuller, svtkCuller);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the minimum coverage - props with less coverage than this
   * are given no time to render (they are culled)
   */
  svtkSetMacro(MinimumCoverage, double);
  svtkGetMacro(MinimumCoverage, double);
  //@}

  //@{
  /**
   * Set/Get the maximum coverage - props with more coverage than this are
   * given an allocated render time of 1.0 (the maximum)
   */
  svtkSetMacro(MaximumCoverage, double);
  svtkGetMacro(MaximumCoverage, double);
  //@}

  //@{
  /**
   * Set the sorting style - none, front-to-back or back-to-front
   * The default is none
   */
  svtkSetClampMacro(SortingStyle, int, SVTK_CULLER_SORT_NONE, SVTK_CULLER_SORT_BACK_TO_FRONT);
  svtkGetMacro(SortingStyle, int);
  void SetSortingStyleToNone() { this->SetSortingStyle(SVTK_CULLER_SORT_NONE); }
  void SetSortingStyleToBackToFront() { this->SetSortingStyle(SVTK_CULLER_SORT_BACK_TO_FRONT); }
  void SetSortingStyleToFrontToBack() { this->SetSortingStyle(SVTK_CULLER_SORT_FRONT_TO_BACK); }
  const char* GetSortingStyleAsString(void);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * Perform the cull operation
   * This method should only be called by svtkRenderer as part of
   * the render process
   */
  double Cull(svtkRenderer* ren, svtkProp** propList, int& listLength, int& initialized) override;

protected:
  svtkFrustumCoverageCuller();
  ~svtkFrustumCoverageCuller() override {}

  double MinimumCoverage;
  double MaximumCoverage;
  int SortingStyle;

private:
  svtkFrustumCoverageCuller(const svtkFrustumCoverageCuller&) = delete;
  void operator=(const svtkFrustumCoverageCuller&) = delete;
};

#endif
