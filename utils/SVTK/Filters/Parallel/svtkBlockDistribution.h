/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlockDistribution.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
/**
 * @class   svtkBlockDistribution
 * @brief   A helper class that manages a block distribution of N elements of data.
 *
 *
 *
 */

#ifndef svtkBlockDistribution_h
#define svtkBlockDistribution_h

class svtkBlockDistribution
{
public:
  /**
   * Create a block distribution with N elements on P processors.
   */
  svtkBlockDistribution(svtkIdType N, svtkIdType P);

  /**
   * Retrieves the number of elements for which this block distribution was
   * built.
   */
  svtkIdType GetNumElements() { return this->NumElements; }

  /**
   * Retrieves the number of processors for which this block
   * distribution was built.
   */
  svtkIdType GetNumProcessors() { return this->NumProcessors; }

  /**
   * Get the block size for the processor with the given rank. This is the
   * number of elements that the processor will store.
   */
  svtkIdType GetBlockSize(svtkIdType rank);

  /**
   * Retrieve the process number in [0, GetNumProcessors()) where the element
   * with the given global index will be located.
   */
  svtkIdType GetProcessorOfElement(svtkIdType globalIndex);

  /**
   * Retrieve the local index (offset) on the processor determined by
   * GetProcessorOfElement that refers to the given global index.
   */
  svtkIdType GetLocalIndexOfElement(svtkIdType globalIndex);

  /**
   * Retrieve the first global index stored on the processor with the given
   * rank.
   */
  svtkIdType GetFirstGlobalIndexOnProcessor(svtkIdType rank);

  /**
   * Retrieve the global index associated with the given local index on the
   * processor with the given rank.
   */
  svtkIdType GetGlobalIndex(svtkIdType localIndex, svtkIdType rank);

private:
  svtkIdType NumElements;
  svtkIdType NumProcessors;
};

// ----------------------------------------------------------------------

inline svtkBlockDistribution::svtkBlockDistribution(svtkIdType N, svtkIdType P)
  : NumElements(N)
  , NumProcessors(P)
{
}

// ----------------------------------------------------------------------

inline svtkIdType svtkBlockDistribution::GetBlockSize(svtkIdType rank)
{
  return (this->NumElements / this->NumProcessors) +
    (rank < this->NumElements % this->NumProcessors ? 1 : 0);
}

// ----------------------------------------------------------------------

inline svtkIdType svtkBlockDistribution::GetProcessorOfElement(svtkIdType globalIndex)
{
  svtkIdType smallBlockSize = this->NumElements / this->NumProcessors;
  svtkIdType cutoffProcessor = this->NumElements % this->NumProcessors;
  svtkIdType cutoffIndex = cutoffProcessor * (smallBlockSize + 1);

  if (globalIndex < cutoffIndex)
  {
    return globalIndex / (smallBlockSize + 1);
  }
  else
  {
    return cutoffProcessor + (globalIndex - cutoffIndex) / smallBlockSize;
  }
}

// ----------------------------------------------------------------------

inline svtkIdType svtkBlockDistribution::GetLocalIndexOfElement(svtkIdType globalIndex)
{
  svtkIdType rank = this->GetProcessorOfElement(globalIndex);
  return globalIndex - this->GetFirstGlobalIndexOnProcessor(rank);
}

// ----------------------------------------------------------------------

inline svtkIdType svtkBlockDistribution::GetFirstGlobalIndexOnProcessor(svtkIdType rank)
{
  svtkIdType estimate = rank * (this->NumElements / this->NumProcessors + 1);
  svtkIdType cutoffProcessor = this->NumElements % this->NumProcessors;
  if (rank < cutoffProcessor)
  {
    return estimate;
  }
  else
  {
    return estimate - (rank - cutoffProcessor);
  }
}

// ----------------------------------------------------------------------

inline svtkIdType svtkBlockDistribution::GetGlobalIndex(svtkIdType localIndex, svtkIdType rank)
{
  return this->GetFirstGlobalIndexOnProcessor(rank) + localIndex;
}

#endif
// SVTK-HeaderTest-Exclude: svtkBlockDistribution.h
