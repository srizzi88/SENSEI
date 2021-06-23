/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplineGraphEdges.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkSplineGraphEdges
 * @brief   subsample graph edges to make smooth curves
 *
 *
 * svtkSplineGraphEdges uses a svtkSpline to make edges into nicely sampled
 * splines. By default, the filter will use an optimized b-spline.
 * Otherwise, it will use a custom svtkSpline instance set by the user.
 */

#ifndef svtkSplineGraphEdges_h
#define svtkSplineGraphEdges_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkSmartPointer.h"        // For ivars

class svtkSpline;

class SVTKINFOVISLAYOUT_EXPORT svtkSplineGraphEdges : public svtkGraphAlgorithm
{
public:
  static svtkSplineGraphEdges* New();
  svtkTypeMacro(svtkSplineGraphEdges, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * If SplineType is CUSTOM, uses this spline.
   */
  virtual void SetSpline(svtkSpline* s);
  svtkGetObjectMacro(Spline, svtkSpline);
  //@}

  enum
  {
    BSPLINE = 0,
    CUSTOM
  };

  //@{
  /**
   * Spline type used by the filter.
   * BSPLINE (0) - Use optimized b-spline (default).
   * CUSTOM (1) - Use spline set with SetSpline.
   */
  svtkSetMacro(SplineType, int);
  svtkGetMacro(SplineType, int);
  //@}

  //@{
  /**
   * The number of subdivisions in the spline.
   */
  svtkSetMacro(NumberOfSubdivisions, svtkIdType);
  svtkGetMacro(NumberOfSubdivisions, svtkIdType);
  //@}

protected:
  svtkSplineGraphEdges();
  ~svtkSplineGraphEdges() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMTimeType GetMTime() override;

  void GeneratePoints(svtkGraph* g, svtkIdType e);
  void GenerateBSpline(svtkGraph* g, svtkIdType e);

  svtkSpline* Spline;

  int SplineType;

  svtkSmartPointer<svtkSpline> XSpline;
  svtkSmartPointer<svtkSpline> YSpline;
  svtkSmartPointer<svtkSpline> ZSpline;

  svtkIdType NumberOfSubdivisions;

private:
  svtkSplineGraphEdges(const svtkSplineGraphEdges&) = delete;
  void operator=(const svtkSplineGraphEdges&) = delete;
};

#endif
