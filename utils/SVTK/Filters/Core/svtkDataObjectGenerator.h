/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataObjectGenerator
 * @brief   produces simple (composite or atomic) data
 * sets for testing.
 *
 * svtkDataObjectGenerator parses a string and produces dataobjects from the
 * dataobject template names it sees in the string. For example, if the string
 * contains "ID1" the generator will create a svtkImageData. "UF1", "RG1",
 * "SG1", "PD1", and "UG1" will produce svtkUniformGrid, svtkRectilinearGrid,
 * svtkStructuredGrid, svtkPolyData and svtkUnstructuredGrid respectively.
 * "PD2" will produce an alternate svtkPolyData. You
 * can compose composite datasets from the atomic ones listed above
 * by placing them within one of the two composite dataset identifiers
 * - "MB{}" or "HB[]". "MB{ ID1 PD1 MB{} }" for example will create a
 * svtkMultiBlockDataSet consisting of three blocks: image data, poly data,
 * multi-block (empty). Hierarchical Box data sets additionally require
 * the notion of groups, declared within "()" braces, to specify AMR depth.
 * "HB[ (UF1)(UF1)(UF1) ]" will create a svtkHierarchicalBoxDataSet representing
 * an octree that is three levels deep, in which the firstmost cell in each level
 * is refined.
 */

#ifndef svtkDataObjectGenerator_h
#define svtkDataObjectGenerator_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class svtkInternalStructureCache;

class SVTKFILTERSCORE_EXPORT svtkDataObjectGenerator : public svtkDataObjectAlgorithm
{
public:
  static svtkDataObjectGenerator* New();
  svtkTypeMacro(svtkDataObjectGenerator, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The string that will be parsed to specify a dataobject structure.
   */
  svtkSetStringMacro(Program);
  svtkGetStringMacro(Program);
  //@}

protected:
  svtkDataObjectGenerator();
  ~svtkDataObjectGenerator() override;

  int RequestData(
    svtkInformation* req, svtkInformationVector** inV, svtkInformationVector* outV) override;
  int RequestDataObject(
    svtkInformation* req, svtkInformationVector** inV, svtkInformationVector* outV) override;
  int RequestInformation(
    svtkInformation* req, svtkInformationVector** inV, svtkInformationVector* outV) override;
  int RequestUpdateExtent(
    svtkInformation* req, svtkInformationVector** inV, svtkInformationVector* outV) override;

  // the string to parse to create a structure
  char* Program;
  // a record of the structure
  svtkInternalStructureCache* Structure;

  // Helper for RequestDataObject
  svtkDataObject* CreateOutputDataObjects(svtkInternalStructureCache* structure);
  // Helper for RequestData
  svtkDataObject* FillOutputDataObjects(
    svtkInternalStructureCache* structure, int level, int stripe = 0);

  // to determine which composite data stripe to fill in
  svtkIdType Rank;
  svtkIdType Processors;

  // create the templated atomic data sets
  void MakeImageData1(svtkDataSet* ds);
  void MakeImageData2(svtkDataSet* ds);
  void MakeUniformGrid1(svtkDataSet* ds);
  void MakeRectilinearGrid1(svtkDataSet* ds);
  void MakeStructuredGrid1(svtkDataSet* ds);
  void MakePolyData1(svtkDataSet* ds);
  void MakePolyData2(svtkDataSet* ds);
  void MakeUnstructuredGrid1(svtkDataSet* ds);
  void MakeUnstructuredGrid2(svtkDataSet* ds);
  void MakeUnstructuredGrid3(svtkDataSet* ds);
  void MakeUnstructuredGrid4(svtkDataSet* ds);

  // used to spatially separate sub data sets within composites
  double XOffset; // increases for each dataset index
  double YOffset; // increases for each sub data set
  double ZOffset; // increases for each group index

  // used to filling in point and cell values with unique Ids
  svtkIdType CellIdCounter;
  svtkIdType PointIdCounter;

  // assign point and cell values to each point and cell
  void MakeValues(svtkDataSet* ds);

private:
  svtkDataObjectGenerator(const svtkDataObjectGenerator&) = delete;
  void operator=(const svtkDataObjectGenerator&) = delete;
};

#endif
