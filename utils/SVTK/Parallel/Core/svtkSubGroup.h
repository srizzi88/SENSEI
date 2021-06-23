/*=========================================================================

  Program:   ParaView
  Module:    svtkSubGroup.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkSubGroup
 * @brief   scalable collective communication for a
 *      subset of members of a parallel SVTK application
 *
 *
 *     This class provides scalable broadcast, reduce, etc. using
 *     only a svtkMultiProcessController. It does not require MPI.
 *     Users are svtkPKdTree and svtkDistributedDataFilter.
 *
 * @attention
 * This class will be deprecated soon.  Instead of using this class, use the
 * collective and subgrouping operations now built into
 * svtkMultiProcessController.  The only reason this class is not deprecated
 * already is because svtkPKdTree relies heavily on this class in ways that are
 * not easy to work around.  Since svtkPKdTree is due for a major overhaul
 * anyway, we are leaving things the way they are for now.
 *
 * @sa
 *      svtkPKdTree svtkDistributedDataFilter
 */

#ifndef svtkSubGroup_h
#define svtkSubGroup_h

#include "svtkObject.h"
#include "svtkParallelCoreModule.h" // For export macro

class svtkMultiProcessController;
class svtkCommunicator;

class SVTKPARALLELCORE_EXPORT svtkSubGroup : public svtkObject
{
public:
  svtkTypeMacro(svtkSubGroup, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkSubGroup* New();

  // The wrapper gets confused here and falls down.
  enum
  {
    MINOP = 1,
    MAXOP = 2,
    SUMOP = 3
  };

  /**
   * Initialize a communication subgroup for the processes
   * with rank p0 through p1 of the given communicator.  (So
   * svtkSubGroup is limited to working with subgroups that
   * are identified by a contiguous set of rank IDs.)
   * The third argument is the callers rank, which must
   * in the range from p0 through p1.
   */

  int Initialize(int p0, int p1, int me, int tag, svtkCommunicator* c);

  int Gather(int* data, int* to, int length, int root);
  int Gather(char* data, char* to, int length, int root);
  int Gather(float* data, float* to, int length, int root);
#ifdef SVTK_USE_64BIT_IDS
  int Gather(svtkIdType* data, svtkIdType* to, int length, int root);
#endif
  int Broadcast(float* data, int length, int root);
  int Broadcast(double* data, int length, int root);
  int Broadcast(int* data, int length, int root);
  int Broadcast(char* data, int length, int root);
#ifdef SVTK_USE_64BIT_IDS
  int Broadcast(svtkIdType* data, int length, int root);
#endif
  int ReduceSum(int* data, int* to, int length, int root);
  int ReduceMax(float* data, float* to, int length, int root);
  int ReduceMax(double* data, double* to, int length, int root);
  int ReduceMax(int* data, int* to, int length, int root);
  int ReduceMin(float* data, float* to, int length, int root);
  int ReduceMin(double* data, double* to, int length, int root);
  int ReduceMin(int* data, int* to, int length, int root);

  int AllReduceUniqueList(int* list, int len, int** newList);
  int MergeSortedUnique(int* list1, int len1, int* list2, int len2, int** newList);

  void setGatherPattern(int root, int length);
  int getLocalRank(int processID);

  int Barrier();

  void PrintSubGroup() const;

  static int MakeSortedUnique(int* list, int len, int** newList);

  int tag;

protected:
  svtkSubGroup();
  ~svtkSubGroup() override;

private:
  int computeFanInTargets();
  void restoreRoot(int rootLoc);
  void moveRoot(int rootLoc);
  void setUpRoot(int root);

  int nFrom;
  int nTo;

  int sendId; // gather
  int sendOffset;
  int sendLength;

  int recvId[20];
  int recvOffset[20];
  int recvLength[20];
  int fanInFrom[20]; // reduce, broadcast

  int fanInTo;
  int nSend;
  int nRecv;
  int gatherRoot;
  int gatherLength;

  int* members;
  int nmembers;
  int myLocalRank;

  svtkCommunicator* comm;

  svtkSubGroup(const svtkSubGroup&) = delete;
  void operator=(const svtkSubGroup&) = delete;
};
#endif
