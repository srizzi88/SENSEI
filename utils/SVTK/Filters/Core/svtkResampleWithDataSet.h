/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResampleWithDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResampleWithDataSet
 * @brief   sample point and cell data of a dataset on
 * points from another dataset.
 *
 * Similar to svtkCompositeDataProbeFilter, svtkResampleWithDataSet takes two
 * inputs - Input and Source, and samples the point and cell values of Source
 * on to the point locations of Input. The output has the same structure as
 * Input but its point data have the resampled values from Source. Unlike
 * svtkCompositeDataProbeFilter, this filter support composite datasets for both
 * Input and Source.
 * @sa
 * svtkCompositeDataProbeFilter svtkResampleToImage
 */

#ifndef svtkResampleWithDataSet_h
#define svtkResampleWithDataSet_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkNew.h"               // For svtkCompositeDataProbeFilter member variable
#include "svtkPassInputTypeAlgorithm.h"

class svtkAbstractCellLocator;
class svtkCompositeDataProbeFilter;
class svtkDataSet;

class SVTKFILTERSCORE_EXPORT svtkResampleWithDataSet : public svtkPassInputTypeAlgorithm
{
public:
  svtkTypeMacro(svtkResampleWithDataSet, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkResampleWithDataSet* New();

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceData(svtkDataObject* source);

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(svtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Control whether the source point data is to be treated as categorical. If
   * the data is categorical, then the resultant data will be determined by
   * a nearest neighbor interpolation scheme.
   */
  void SetCategoricalData(bool arg);
  bool GetCategoricalData();
  //@}

  //@{
  /**
   * Shallow copy the input cell data arrays to the output.
   * Off by default.
   */
  void SetPassCellArrays(bool arg);
  bool GetPassCellArrays();
  svtkBooleanMacro(PassCellArrays, bool);
  //@}

  //@{
  /**
   * Shallow copy the input point data arrays to the output
   * Off by default.
   */
  void SetPassPointArrays(bool arg);
  bool GetPassPointArrays();
  svtkBooleanMacro(PassPointArrays, bool);
  //@}

  //@{
  /**
   * Set whether to pass the field-data arrays from the Input i.e. the input
   * providing the geometry to the output. On by default.
   */
  void SetPassFieldArrays(bool arg);
  bool GetPassFieldArrays();
  svtkBooleanMacro(PassFieldArrays, bool);
  //@}

  //@{
  /**
   * Set the tolerance used to compute whether a point in the
   * source is in a cell of the input.  This value is only used
   * if ComputeTolerance is off.
   */
  void SetTolerance(double arg);
  double GetTolerance();
  //@}

  //@{
  /**
   * Set whether to use the Tolerance field or precompute the tolerance.
   * When on, the tolerance will be computed and the field
   * value is ignored. Off by default.
   */
  void SetComputeTolerance(bool arg);
  bool GetComputeTolerance();
  svtkBooleanMacro(ComputeTolerance, bool);
  //@}

  //@{
  /**
   * Set whether points without resampled values, and their corresponding cells,
   * should be marked as Blank. Default is On.
   */
  svtkSetMacro(MarkBlankPointsAndCells, bool);
  svtkGetMacro(MarkBlankPointsAndCells, bool);
  svtkBooleanMacro(MarkBlankPointsAndCells, bool);
  //@}

  //@{
  /*
   * Set/Get the prototype cell locator to use for probing the source dataset.
   * The value is forwarded to the underlying probe filter.
   */
  virtual void SetCellLocatorPrototype(svtkAbstractCellLocator*);
  virtual svtkAbstractCellLocator* GetCellLocatorPrototype() const;
  //@}

  svtkMTimeType GetMTime() override;

protected:
  svtkResampleWithDataSet();
  ~svtkResampleWithDataSet() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Get the name of the valid-points mask array.
   */
  const char* GetMaskArrayName() const;

  /**
   * Mark invalid points and cells of output DataSet as hidden
   */
  void SetBlankPointsAndCells(svtkDataSet* data);

  svtkNew<svtkCompositeDataProbeFilter> Prober;
  bool MarkBlankPointsAndCells;

private:
  svtkResampleWithDataSet(const svtkResampleWithDataSet&) = delete;
  void operator=(const svtkResampleWithDataSet&) = delete;
};

#endif // svtkResampleWithDataSet_h
