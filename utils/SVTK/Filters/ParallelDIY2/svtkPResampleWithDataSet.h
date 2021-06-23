/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPResampleWithDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPResampleWithDataSet
 * points from another dataset.
 *
 * svtkPResampleWithDataSet is the parallel version of svtkResampleWithDataSet
 * filter
 * @sa
 * svtkResampleWithDataSet svtkPResampleToImage
 */

#ifndef svtkPResampleWithDataSet_h
#define svtkPResampleWithDataSet_h

#include "svtkFiltersParallelDIY2Module.h" // For export macro
#include "svtkResampleWithDataSet.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLELDIY2_EXPORT svtkPResampleWithDataSet : public svtkResampleWithDataSet
{
public:
  svtkTypeMacro(svtkPResampleWithDataSet, svtkResampleWithDataSet);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPResampleWithDataSet* New();

  //@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  //@{
  /**
   * Set/Get if the filter should use Balanced Partitioning for fast lookup of
   * the input points. Balanced Partitioning partitions the points into similar
   * sized bins. It takes logarithmic time to search for the candidate bins, but
   * search inside border bins takes constant time.
   * The default is to use Regular Partitioning which partitions the space of the
   * points into regular sized bins. Based on their distribution, the bins may
   * contain widely varying number of points. It takes constant time to search
   * for the candidate bins but search within border bins can vary.
   * For most cases, both techniques perform the same with Regular Partitioning
   * being slightly better. Balanced Partitioning may perform better when the
   * points distribution is highly skewed.
   */
  svtkSetMacro(UseBalancedPartitionForPointsLookup, bool);
  svtkGetMacro(UseBalancedPartitionForPointsLookup, bool);
  svtkBooleanMacro(UseBalancedPartitionForPointsLookup, bool);
  //@}

protected:
  svtkPResampleWithDataSet();
  ~svtkPResampleWithDataSet() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMultiProcessController* Controller;
  bool UseBalancedPartitionForPointsLookup;

private:
  svtkPResampleWithDataSet(const svtkPResampleWithDataSet&) = delete;
  void operator=(const svtkPResampleWithDataSet&) = delete;
};

#endif // svtkPResampleWithDataSet_h
