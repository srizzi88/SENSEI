/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCookieCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCookieCutter
 * @brief   cut svtkPolyData defined on the 2D plane with one or more polygons
 *
 * This filter crops an input svtkPolyData consisting of cells (i.e., points,
 * lines, polygons, and triangle strips) with loops specified by a second
 * input containing polygons. Note that this filter can handle concave
 * polygons and/or loops. It may produce multiple output polygons for each
 * polygon/loop interaction. Similarly, it may produce multiple line segments
 * and so on.
 *
 * @warning
 * The z-values of the input svtkPolyData and the points defining the loops are
 * assumed to lie at z=constant. In other words, this filter assumes that the data lies
 * in a plane orthogonal to the z axis.
 *
 */

#ifndef svtkCookieCutter_h
#define svtkCookieCutter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSMODELING_EXPORT svtkCookieCutter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods to instantiate, print and provide type information.
   */
  static svtkCookieCutter* New();
  svtkTypeMacro(svtkCookieCutter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify the a second svtkPolyData input which defines loops used to cut
   * the input polygonal data. These loops must be manifold, i.e., do not
   * self intersect. The loops are defined from the polygons defined in
   * this second input.
   */
  void SetLoopsConnection(svtkAlgorithmOutput* algOutput);
  svtkAlgorithmOutput* GetLoopsConnection();

  //@{
  /**
   * Specify the a second svtkPolyData input which defines loops used to cut
   * the input polygonal data. These loops must be manifold, i.e., do not
   * self intersect. The loops are defined from the polygons defined in
   * this second input.
   */
  void SetLoopsData(svtkDataObject* loops);
  svtkDataObject* GetLoops();
  //@}

  //@{
  /**
   * Specify a spatial locator for merging points. By default, an
   * instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

protected:
  svtkCookieCutter();
  ~svtkCookieCutter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  svtkIncrementalPointLocator* Locator;

private:
  svtkCookieCutter(const svtkCookieCutter&) = delete;
  void operator=(const svtkCookieCutter&) = delete;
};

#endif
