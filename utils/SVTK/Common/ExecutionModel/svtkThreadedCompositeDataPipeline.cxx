/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThreadedCompositeDataPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkThreadedCompositeDataPipeline.h"

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationExecutivePortKey.h"
#include "svtkInformationExecutivePortVectorKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationObjectBaseKey.h"
#include "svtkInformationRequestKey.h"
#include "svtkInformationVector.h"

#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDebugLeaks.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkTimerLog.h"

#include "svtkSMPProgressObserver.h"
#include "svtkSMPThreadLocal.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"

#include <cassert>
#include <vector>

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkThreadedCompositeDataPipeline);

//----------------------------------------------------------------------------
namespace
{
static svtkInformationVector** Clone(svtkInformationVector** src, int n)
{
  svtkInformationVector** dst = new svtkInformationVector*[n];
  for (int i = 0; i < n; ++i)
  {
    dst[i] = svtkInformationVector::New();
    dst[i]->Copy(src[i], 1);
  }
  return dst;
}
static void DeleteAll(svtkInformationVector** dst, int n)
{
  for (int i = 0; i < n; ++i)
  {
    dst[i]->Delete();
  }
  delete[] dst;
}
};

//----------------------------------------------------------------------------
class ProcessBlockData : public svtkObjectBase
{
public:
  svtkBaseTypeMacro(ProcessBlockData, svtkObjectBase);
  svtkInformationVector** In;
  svtkInformationVector* Out;
  int InSize;

  static ProcessBlockData* New()
  {
    // Can't use object factory macros, this is not a svtkObject.
    ProcessBlockData* ret = new ProcessBlockData;
    ret->InitializeObjectBase();
    return ret;
  }

  void Construct(
    svtkInformationVector** inInfoVec, int inInfoVecSize, svtkInformationVector* outInfoVec)
  {
    this->InSize = inInfoVecSize;
    this->In = Clone(inInfoVec, inInfoVecSize);
    this->Out = svtkInformationVector::New();
    this->Out->Copy(outInfoVec, 1);
  }

  ~ProcessBlockData() override
  {
    DeleteAll(this->In, this->InSize);
    this->Out->Delete();
  }

protected:
  ProcessBlockData()
    : In(nullptr)
    , Out(nullptr)
  {
  }
};
//----------------------------------------------------------------------------
class ProcessBlock
{
public:
  ProcessBlock(svtkThreadedCompositeDataPipeline* exec, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec, int compositePort, int connection, svtkInformation* request,
    const std::vector<svtkDataObject*>& inObjs, std::vector<svtkDataObject*>& outObjs)
    : Exec(exec)
    , InInfoVec(inInfoVec)
    , OutInfoVec(outInfoVec)
    , CompositePort(compositePort)
    , Connection(connection)
    , Request(request)
    , InObjs(inObjs)
  {
    int numInputPorts = this->Exec->GetNumberOfInputPorts();
    this->OutObjs = &outObjs[0];
    this->InfoPrototype = svtkSmartPointer<ProcessBlockData>::New();
    this->InfoPrototype->Construct(this->InInfoVec, numInputPorts, this->OutInfoVec);
  }

  ~ProcessBlock()
  {
    svtkSMPThreadLocal<svtkInformationVector**>::iterator itr1 = this->InInfoVecs.begin();
    svtkSMPThreadLocal<svtkInformationVector**>::iterator end1 = this->InInfoVecs.end();
    while (itr1 != end1)
    {
      DeleteAll(*itr1, this->InfoPrototype->InSize);
      ++itr1;
    }

    svtkSMPThreadLocal<svtkInformationVector*>::iterator itr2 = this->OutInfoVecs.begin();
    svtkSMPThreadLocal<svtkInformationVector*>::iterator end2 = this->OutInfoVecs.end();
    while (itr2 != end2)
    {
      (*itr2)->Delete();
      ++itr2;
    }
  }

  void Initialize()
  {
    svtkInformationVector**& inInfoVec = this->InInfoVecs.Local();
    svtkInformationVector*& outInfoVec = this->OutInfoVecs.Local();

    inInfoVec = Clone(this->InfoPrototype->In, this->InfoPrototype->InSize);
    outInfoVec = svtkInformationVector::New();
    outInfoVec->Copy(this->InfoPrototype->Out, 1);

    svtkInformation*& request = this->Requests.Local();
    request->Copy(this->Request, 1);
  }

  void operator()(svtkIdType begin, svtkIdType end)
  {
    svtkInformationVector** inInfoVec = this->InInfoVecs.Local();
    svtkInformationVector* outInfoVec = this->OutInfoVecs.Local();
    svtkInformation* request = this->Requests.Local();

    svtkInformation* inInfo = inInfoVec[this->CompositePort]->GetInformationObject(this->Connection);

    for (svtkIdType i = begin; i < end; ++i)
    {
      std::vector<svtkDataObject*> outObjList = this->Exec->ExecuteSimpleAlgorithmForBlock(
        &inInfoVec[0], outInfoVec, inInfo, request, this->InObjs[i]);
      for (int j = 0; j < outInfoVec->GetNumberOfInformationObjects(); ++j)
      {
        this->OutObjs[i * outInfoVec->GetNumberOfInformationObjects() + j] = outObjList[j];
      }
    }
  }

  void Reduce() {}

protected:
  svtkThreadedCompositeDataPipeline* Exec;
  svtkInformationVector** InInfoVec;
  svtkInformationVector* OutInfoVec;
  svtkSmartPointer<ProcessBlockData> InfoPrototype;
  int CompositePort;
  int Connection;
  svtkInformation* Request;
  const std::vector<svtkDataObject*>& InObjs;
  svtkDataObject** OutObjs;

  svtkSMPThreadLocal<svtkInformationVector**> InInfoVecs;
  svtkSMPThreadLocal<svtkInformationVector*> OutInfoVecs;
  svtkSMPThreadLocalObject<svtkInformation> Requests;
};

//----------------------------------------------------------------------------
svtkThreadedCompositeDataPipeline::svtkThreadedCompositeDataPipeline() = default;

//----------------------------------------------------------------------------
svtkThreadedCompositeDataPipeline::~svtkThreadedCompositeDataPipeline() = default;

//-------------------------------------------------------------------------
void svtkThreadedCompositeDataPipeline::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
void svtkThreadedCompositeDataPipeline::ExecuteEach(svtkCompositeDataIterator* iter,
  svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec, int compositePort,
  int connection, svtkInformation* request,
  std::vector<svtkSmartPointer<svtkCompositeDataSet> >& compositeOutput)
{
  // from input data objects  itr -> (inObjs, indices)
  // inObjs are the non-null objects that we will loop over.
  // indices map the input objects to inObjs
  std::vector<svtkDataObject*> inObjs;
  std::vector<int> indices;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataObject* dobj = iter->GetCurrentDataObject();
    if (dobj)
    {
      inObjs.push_back(dobj);
      indices.push_back(static_cast<int>(inObjs.size()) - 1);
    }
    else
    {
      indices.push_back(-1);
    }
  }

  // instantiate outObjs, the output objects that will be created from inObjs
  std::vector<svtkDataObject*> outObjs;
  outObjs.resize(indices.size() * outInfoVec->GetNumberOfInformationObjects(), nullptr);

  // create the parallel task processBlock
  ProcessBlock processBlock(
    this, inInfoVec, outInfoVec, compositePort, connection, request, inObjs, outObjs);

  svtkSmartPointer<svtkProgressObserver> origPo(this->Algorithm->GetProgressObserver());
  svtkNew<svtkSMPProgressObserver> po;
  this->Algorithm->SetProgressObserver(po);
  svtkSMPTools::For(0, static_cast<svtkIdType>(inObjs.size()), processBlock);
  this->Algorithm->SetProgressObserver(origPo);

  int i = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), i++)
  {
    int j = indices[i];
    if (j >= 0)
    {
      for (int k = 0; k < outInfoVec->GetNumberOfInformationObjects(); ++k)
      {
        svtkDataObject* outObj = outObjs[j * outInfoVec->GetNumberOfInformationObjects() + k];
        compositeOutput[k]->SetDataSet(iter, outObj);
        if (outObj)
        {
          outObj->FastDelete();
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
int svtkThreadedCompositeDataPipeline::CallAlgorithm(svtkInformation* request, int direction,
  svtkInformationVector** inInfo, svtkInformationVector* outInfo)
{
  // Copy default information in the direction of information flow.
  this->CopyDefaultInformation(request, direction, inInfo, outInfo);

  // Invoke the request on the algorithm.
  int result = this->Algorithm->ProcessRequest(request, inInfo, outInfo);

  // If the algorithm failed report it now.
  if (!result)
  {
    svtkErrorMacro("Algorithm " << this->Algorithm->GetClassName() << "(" << this->Algorithm
                               << ") returned failure for request: " << *request);
  }

  return result;
}
