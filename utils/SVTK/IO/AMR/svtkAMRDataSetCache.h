/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRDataSetCache.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMRDataSetCache
 *
 *
 *  A concrete implementation of svtkObject that provides functionality for
 *  caching AMR blocks. The primary intent of this class is to be used by the
 *  AMR reader infrastructure for caching blocks/data in memory to minimize
 *  out-of-core operations.
 */

#ifndef svtkAMRDataSetCache_h
#define svtkAMRDataSetCache_h

#include "svtkIOAMRModule.h" // For export macro
#include "svtkObject.h"
#include <map> // For STL map used as the data-structure for the cache.

class svtkUniformGrid;
class svtkDataArray;

class SVTKIOAMR_EXPORT svtkAMRDataSetCache : public svtkObject
{
public:
  static svtkAMRDataSetCache* New();
  svtkTypeMacro(svtkAMRDataSetCache, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Inserts an AMR block to the cache
   */
  void InsertAMRBlock(int compositeIdx, svtkUniformGrid* amrGrid);

  /**
   * Inserts a point data array to an already cached block
   * NOTE: this->HasAMRBlock( compositeIdx ) == true
   */
  void InsertAMRBlockPointData(int compositeIdx, svtkDataArray* dataArray);

  /**
   * Inserts a cell data array to an already cached block
   * NOTE: this->HasAMRBlock( compositeIdx ) == true
   */
  void InsertAMRBlockCellData(int compositeIdx, svtkDataArray* dataArray);

  /**
   * Given the name of the cell array and AMR block composite index, this
   * method returns a pointer to the cell data array.
   * NOTE: Null is returned if the cell array and/or block is not cached.
   */
  svtkDataArray* GetAMRBlockCellData(int compositeIdx, const char* dataName);

  /**
   * Given the name of the point array and AMR block composite index, this
   * method returns a pointer to the point data array.
   * NOTE: Null is returned if the point array and /or block is not cached.
   */
  svtkDataArray* GetAMRBlockPointData(int compositeIdx, const char* dataName);

  /**
   * Given the composite index, this method returns the AMR block.
   * NOTE: Null is returned if the AMR block does not exist in the cache.
   */
  svtkUniformGrid* GetAMRBlock(int compositeIdx);

  /**
   * Checks if the cell data array, associated with the provided name, has
   * been cached for the AMR block with the given composite index.
   */
  bool HasAMRBlockCellData(int compositeIdx, const char* name);

  /**
   * Checks if the point data array, associated with the provided name, has
   * been cached for the AMR block with the given composite index.
   */
  bool HasAMRBlockPointData(int compositeIdx, const char* name);

  /**
   * Checks if the AMR block associated with the given composite is cached.
   */
  bool HasAMRBlock(const int compositeIdx);

protected:
  svtkAMRDataSetCache();
  ~svtkAMRDataSetCache() override;

  typedef std::map<int, svtkUniformGrid*> AMRCacheType;
  AMRCacheType Cache;

private:
  svtkAMRDataSetCache(const svtkAMRDataSetCache&) = delete;
  void operator=(const svtkAMRDataSetCache&) = delete;
};

#endif /* svtkAMRDataSetCache_h */
