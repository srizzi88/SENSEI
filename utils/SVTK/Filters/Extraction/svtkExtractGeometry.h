/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractGeometry.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractGeometry
 * @brief   extract cells that lie either entirely inside or outside of a specified implicit
 * function
 *
 *
 * svtkExtractGeometry extracts from its input dataset all cells that are either
 * completely inside or outside of a specified implicit function. Any type of
 * dataset can be input to this filter. On output the filter generates an
 * unstructured grid.
 *
 * To use this filter you must specify an implicit function. You must also
 * specify whether to extract cells laying inside or outside of the implicit
 * function. (The inside of an implicit function is the negative values
 * region.) An option exists to extract cells that are neither inside or
 * outside (i.e., boundary).
 *
 * A more efficient version of this filter is available for svtkPolyData input.
 * See svtkExtractPolyDataGeometry.
 *
 * @sa
 * svtkExtractPolyDataGeometry svtkGeometryFilter svtkExtractVOI
 */

#ifndef svtkExtractGeometry_h
#define svtkExtractGeometry_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkImplicitFunction;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractGeometry : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkExtractGeometry, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with ExtractInside turned on.
   */
  static svtkExtractGeometry* New();

  /**
   * Return the MTime taking into account changes to the implicit function
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify the implicit function for inside/outside checks.
   */
  virtual void SetImplicitFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(ImplicitFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * Boolean controls whether to extract cells that are inside of implicit
   * function (ExtractInside == 1) or outside of implicit function
   * (ExtractInside == 0).
   */
  svtkSetMacro(ExtractInside, svtkTypeBool);
  svtkGetMacro(ExtractInside, svtkTypeBool);
  svtkBooleanMacro(ExtractInside, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean controls whether to extract cells that are partially inside.
   * By default, ExtractBoundaryCells is off.
   */
  svtkSetMacro(ExtractBoundaryCells, svtkTypeBool);
  svtkGetMacro(ExtractBoundaryCells, svtkTypeBool);
  svtkBooleanMacro(ExtractBoundaryCells, svtkTypeBool);
  svtkSetMacro(ExtractOnlyBoundaryCells, svtkTypeBool);
  svtkGetMacro(ExtractOnlyBoundaryCells, svtkTypeBool);
  svtkBooleanMacro(ExtractOnlyBoundaryCells, svtkTypeBool);
  //@}

protected:
  svtkExtractGeometry(svtkImplicitFunction* f = nullptr);
  ~svtkExtractGeometry() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkImplicitFunction* ImplicitFunction;
  svtkTypeBool ExtractInside;
  svtkTypeBool ExtractBoundaryCells;
  svtkTypeBool ExtractOnlyBoundaryCells;

private:
  svtkExtractGeometry(const svtkExtractGeometry&) = delete;
  void operator=(const svtkExtractGeometry&) = delete;
};

#endif
