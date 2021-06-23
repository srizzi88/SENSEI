/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenerateIndexArray.h

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
 * @class   svtkGenerateIndexArray
 *
 *
 * Generates a new svtkIdTypeArray containing zero-base indices.
 *
 * svtkGenerateIndexArray operates in one of two distinct "modes".
 * By default, it simply generates an index array containing
 * monotonically-increasing integers in the range [0, N), where N
 * is appropriately sized for the field type that will store the
 * results.  This mode is useful for generating a unique ID field
 * for datasets that have none.
 *
 * The second "mode" uses an existing array from the input data
 * object as a "reference".  Distinct values from the reference
 * array are sorted in ascending order, and an integer index in
 * the range [0, N) is assigned to each.  The resulting map is
 * used to populate the output index array, mapping each value
 * in the reference array to its corresponding index and storing
 * the result in the output array.  This mode is especially
 * useful when generating tensors, since it allows us to "map"
 * from an array with arbitrary contents to an index that can
 * be used as tensor coordinates.
 */

#ifndef svtkGenerateIndexArray_h
#define svtkGenerateIndexArray_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class SVTKINFOVISCORE_EXPORT svtkGenerateIndexArray : public svtkDataObjectAlgorithm
{
public:
  static svtkGenerateIndexArray* New();

  svtkTypeMacro(svtkGenerateIndexArray, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Control the output index array name.  Default: "index".
   */
  svtkSetStringMacro(ArrayName);
  svtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Control the location where the index array will be stored.
   */
  svtkSetMacro(FieldType, int);
  svtkGetMacro(FieldType, int);
  //@}

  //@{
  /**
   * Specifies an optional reference array for index-generation.
   */
  svtkSetStringMacro(ReferenceArrayName);
  svtkGetStringMacro(ReferenceArrayName);
  //@}

  //@{
  /**
   * Specifies whether the index array should be marked as
   * pedigree ids.  Default: false.
   */
  svtkSetMacro(PedigreeID, int);
  svtkGetMacro(PedigreeID, int);
  //@}

  enum
  {
    ROW_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4
  };

protected:
  svtkGenerateIndexArray();
  ~svtkGenerateIndexArray() override;

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* ArrayName;
  int FieldType;
  char* ReferenceArrayName;
  int PedigreeID;

private:
  svtkGenerateIndexArray(const svtkGenerateIndexArray&) = delete;
  void operator=(const svtkGenerateIndexArray&) = delete;
};

#endif
