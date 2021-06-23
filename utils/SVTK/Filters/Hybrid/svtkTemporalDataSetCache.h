/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalDataSetCache.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTemporalDataSetCache
 * @brief   cache time steps
 *
 * svtkTemporalDataSetCache cache time step requests of a temporal dataset,
 * when cached data is requested it is returned using a shallow copy.
 * @par Thanks:
 * Ken Martin (Kitware) and John Bidiscombe of
 * CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 * For related material, please refer to :
 * John Biddiscombe, Berk Geveci, Ken Martin, Kenneth Moreland, David Thompson,
 * "Time Dependent Processing in a Parallel Pipeline Architecture",
 * IEEE Visualization 2007.
 */

#ifndef svtkTemporalDataSetCache_h
#define svtkTemporalDataSetCache_h

#include "svtkFiltersHybridModule.h" // For export macro

#include "svtkAlgorithm.h"
#include <map> // used for the cache

class SVTKFILTERSHYBRID_EXPORT svtkTemporalDataSetCache : public svtkAlgorithm
{
public:
  static svtkTemporalDataSetCache* New();
  svtkTypeMacro(svtkTemporalDataSetCache, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * This is the maximum number of time steps that can be retained in memory.
   * it defaults to 10.
   */
  void SetCacheSize(int size);
  svtkGetMacro(CacheSize, int);
  //@}

protected:
  svtkTemporalDataSetCache();
  ~svtkTemporalDataSetCache() override;

  int CacheSize;

  typedef std::map<double, std::pair<unsigned long, svtkDataObject*> > CacheType;
  CacheType Cache;

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info) override;
  virtual int RequestDataObject(
    svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

private:
  svtkTemporalDataSetCache(const svtkTemporalDataSetCache&) = delete;
  void operator=(const svtkTemporalDataSetCache&) = delete;
};

#endif
