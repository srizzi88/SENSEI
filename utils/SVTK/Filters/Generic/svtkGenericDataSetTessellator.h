/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericDataSetTessellator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGenericDataSetTessellator
 * @brief   tessellates generic, higher-order datasets into linear cells
 *
 *
 * svtkGenericDataSetTessellator is a filter that subdivides a
 * svtkGenericDataSet into linear elements (i.e., linear SVTK
 * cells). Tetrahedras are produced from 3D cells; triangles from 2D cells;
 * and lines from 1D cells. The subdivision process depends on the cell
 * tessellator associated with the input generic dataset, and its associated
 * error metric. (These can be specified by the user if necessary.)
 *
 * This filter is typically used to convert a higher-order, complex dataset
 * represented by a svtkGenericDataSet into a conventional svtkDataSet that can
 * be operated on by linear SVTK graphics filters (end of pipeline for
 * rendering).
 *
 * @sa
 * svtkGenericCellTessellator svtkGenericSubdivisionErrorMetric
 */

#ifndef svtkGenericDataSetTessellator_h
#define svtkGenericDataSetTessellator_h

#include "svtkFiltersGenericModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkPointData;
class svtkIncrementalPointLocator;

class SVTKFILTERSGENERIC_EXPORT svtkGenericDataSetTessellator : public svtkUnstructuredGridAlgorithm
{
public:
  //@{
  /**
   * Standard SVTK methods.
   */
  static svtkGenericDataSetTessellator* New();
  svtkTypeMacro(svtkGenericDataSetTessellator, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Turn on/off generation of a cell centered attribute with ids of the
   * original cells (as an input cell is tessellated into several linear
   * cells).
   * The name of the data array is "OriginalIds". It is true by default.
   */
  svtkSetMacro(KeepCellIds, svtkTypeBool);
  svtkGetMacro(KeepCellIds, svtkTypeBool);
  svtkBooleanMacro(KeepCellIds, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off merging of coincident points. Note that is merging is
   * on, points with different point attributes (e.g., normals) are merged,
   * which may cause rendering artifacts.
   */
  svtkSetMacro(Merging, svtkTypeBool);
  svtkGetMacro(Merging, svtkTypeBool);
  svtkBooleanMacro(Merging, svtkTypeBool);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By
   * default an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return the MTime also considering the locator.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkGenericDataSetTessellator();
  ~svtkGenericDataSetTessellator() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int, svtkInformation*) override;

  // See Set/Get KeepCellIds() for explanations.
  svtkTypeBool KeepCellIds;

  // Used internal by svtkGenericAdaptorCell::Tessellate()
  svtkPointData* InternalPD;

  svtkTypeBool Merging;
  svtkIncrementalPointLocator* Locator;

private:
  svtkGenericDataSetTessellator(const svtkGenericDataSetTessellator&) = delete;
  void operator=(const svtkGenericDataSetTessellator&) = delete;
};

#endif
