/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridGhostCellsGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkUnstructuredGridGhostCellsGenerator
 * @brief   Builds ghost cells for a distributed unstructured grid dataset.
 *
 * This filter is a serial implementation of the svtkPUnstructuredGridGhostCellsGenerator
 * filter with the intent that it can be used in non-MPI builds. Both the serial and
 * parallel version act as a "pass-through" filter when run in serial. The combination
 * of these filters serves to unify the API for serial and parallel builds.
 *
 * @sa
 * svtkPDistributedDataFilter
 * svtkPUnstructuredGridGhostCellsGenerator
 *
 *
 */

#ifndef svtkUnstructuredGridGhostCellsGenerator_h
#define svtkUnstructuredGridGhostCellsGenerator_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkMultiProcessController;
class svtkUnstructuredGrid;
class svtkUnstructuredGridBase;

class SVTKFILTERSPARALLEL_EXPORT svtkUnstructuredGridGhostCellsGenerator
  : public svtkUnstructuredGridAlgorithm
{
  svtkTypeMacro(svtkUnstructuredGridGhostCellsGenerator, svtkUnstructuredGridAlgorithm);

public:
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkUnstructuredGridGhostCellsGenerator* New();

  //@{
  /**
   * Specify if the filter must take benefit of global point ids if they exist.
   * If false, point coordinates are used. Default is TRUE.
   */
  svtkSetMacro(UseGlobalPointIds, bool);
  svtkGetMacro(UseGlobalPointIds, bool);
  svtkBooleanMacro(UseGlobalPointIds, bool);
  //@}

  //@{
  /**
   * Specify the name of the global point ids data array if the GlobalIds
   * attribute array is not set. Default is "GlobalNodeIds".
   */
  svtkSetStringMacro(GlobalPointIdsArrayName);
  svtkGetStringMacro(GlobalPointIdsArrayName);
  //@}

  //@{
  /**
   * Specify if the data has global cell ids.
   * If more than one layer of ghost cells is needed, global cell ids are
   * necessary. If global cell ids are not provided, they will be computed
   * internally.
   * If false, global cell ids will be computed, then deleted afterwards.
   * Default is FALSE.
   */
  svtkSetMacro(HasGlobalCellIds, bool);
  svtkGetMacro(HasGlobalCellIds, bool);
  svtkBooleanMacro(HasGlobalCellIds, bool);
  //@}

  //@{
  /**
   * Specify the name of the global cell ids data array if the GlobalIds
   * attribute array is not set. Default is "GlobalNodeIds".
   */
  svtkSetStringMacro(GlobalCellIdsArrayName);
  svtkGetStringMacro(GlobalCellIdsArrayName);
  //@}

  //@{
  /**
   * Specify if the filter must generate the ghost cells only if required by
   * the pipeline.
   * If false, ghost cells are computed even if they are not required.
   * Default is TRUE.
   */
  svtkSetMacro(BuildIfRequired, bool);
  svtkGetMacro(BuildIfRequired, bool);
  svtkBooleanMacro(BuildIfRequired, bool);
  //@}

  //@{
  /**
   * When BuildIfRequired is `false`, this can be used to set the minimum number
   * of ghost levels to generate. Note, if the downstream pipeline requests more
   * ghost levels than the number specified here, then the filter will generate
   * those extra ghost levels as needed. Accepted values are in the interval
   * [1, SVTK_INT_MAX].
   */
  svtkSetClampMacro(MinimumNumberOfGhostLevels, int, 1, SVTK_INT_MAX);
  svtkGetMacro(MinimumNumberOfGhostLevels, int);
  //@}

protected:
  svtkUnstructuredGridGhostCellsGenerator();
  ~svtkUnstructuredGridGhostCellsGenerator() override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* GlobalPointIdsArrayName;
  bool UseGlobalPointIds;
  char* GlobalCellIdsArrayName;
  bool HasGlobalCellIds;
  bool BuildIfRequired;
  int MinimumNumberOfGhostLevels;

private:
  svtkUnstructuredGridGhostCellsGenerator(const svtkUnstructuredGridGhostCellsGenerator&) = delete;
  void operator=(const svtkUnstructuredGridGhostCellsGenerator&) = delete;
};

#endif
