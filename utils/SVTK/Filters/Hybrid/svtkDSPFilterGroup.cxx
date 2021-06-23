/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDSPFilterGroup.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkDSPFilterGroup.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDSPFilterDefinition.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStructuredGrid.h"

#include <cctype>
#include <string>
#include <vector>

svtkStandardNewMacro(svtkDSPFilterGroup);

class svtkDSPFilterGroupVectorIntSTLCloak
{
public:
  std::vector<int> m_vector;
};
class svtkDSPFilterGroupVectorVectorIntSTLCloak
{
public:
  std::vector<std::vector<int> > m_vector;
};

class svtkDSPFilterGroupVectorArraySTLCloak
{
public:
  std::vector<svtkFloatArray*> m_vector;
};
class svtkDSPFilterGroupVectorVectorArraySTLCloak
{
public:
  std::vector<std::vector<svtkFloatArray*> > m_vector;
};
class svtkDSPFilterGroupVectorStringSTLCloak
{
public:
  std::vector<std::string> m_vector;
};

class svtkDSPFilterGroupVectorDefinitionSTLCloak
{
public:
  std::vector<svtkDSPFilterDefinition*> m_vector;
};

//----------------------------------------------------------------------------
svtkDSPFilterGroup::svtkDSPFilterGroup()
{
  this->FilterDefinitions = new svtkDSPFilterGroupVectorDefinitionSTLCloak;
  this->CachedInputs = new svtkDSPFilterGroupVectorArraySTLCloak;
  this->CachedInputNames = new svtkDSPFilterGroupVectorStringSTLCloak;
  this->CachedInputTimesteps = new svtkDSPFilterGroupVectorIntSTLCloak;
  this->CachedOutputs = new svtkDSPFilterGroupVectorVectorArraySTLCloak;
  this->CachedOutputTimesteps = new svtkDSPFilterGroupVectorVectorIntSTLCloak;

  this->FilterDefinitions->m_vector.resize(0);
  this->CachedInputs->m_vector.resize(0);
  this->CachedInputNames->m_vector.resize(0);
  this->CachedInputTimesteps->m_vector.resize(0);
  this->CachedOutputs->m_vector.resize(0);
  this->CachedOutputTimesteps->m_vector.resize(0);
}

//----------------------------------------------------------------------------
svtkDSPFilterGroup::~svtkDSPFilterGroup()
{
  this->FilterDefinitions->m_vector.resize(0);
  this->CachedInputs->m_vector.resize(0);
  this->CachedInputNames->m_vector.resize(0);
  this->CachedInputTimesteps->m_vector.resize(0);
  this->CachedOutputs->m_vector.resize(0);
  this->CachedOutputTimesteps->m_vector.resize(0);

  delete this->FilterDefinitions;
  delete this->CachedInputs;
  delete this->CachedInputNames;
  delete this->CachedInputTimesteps;
  delete this->CachedOutputs;
  delete this->CachedOutputTimesteps;
}

//----------------------------------------------------------------------------
void svtkDSPFilterGroup::AddFilter(svtkDSPFilterDefinition* filter)
{
  // XXX can't just add this filter, need to check for duplicates and removals?

  svtkDSPFilterDefinition* thefilter = svtkDSPFilterDefinition::New();
  thefilter->Copy(filter);

  this->FilterDefinitions->m_vector.push_back(thefilter);

  std::vector<svtkFloatArray*> l_cachedOutsForThisFilter;
  l_cachedOutsForThisFilter.resize(0);
  this->CachedOutputs->m_vector.push_back(l_cachedOutsForThisFilter);

  std::vector<int> l_cachedOutTimesForThisFilter;
  l_cachedOutTimesForThisFilter.resize(0);
  this->CachedOutputTimesteps->m_vector.push_back(l_cachedOutTimesForThisFilter);

#if 0
  printf("**********************FILTERS AFTER ADDING FILTER***********************\n");
  for(int i=0;i<this->GetNumFilters();i++)
  {
      svtkDSPFilterDefinition *filterfromlist = this->FilterDefinitions->m_vector[i];
      printf("svtkDSPFilterGroup::AddFilter %d of %d  input=%s output=%s nums=%d dens=%d forwardnums=%d    this=%p\n",
       i,this->GetNumFilters(),
       filterfromlist->GetInputVariableName(),
       filterfromlist->GetOutputVariableName(),
       filterfromlist->GetNumNumeratorWeights(),
       filterfromlist->GetNumDenominatorWeights(),
       filterfromlist->GetNumForwardNumeratorWeights(),
       this);
  }
  printf("************************************************************************\n");
#endif
}

//----------------------------------------------------------------------------
void svtkDSPFilterGroup::RemoveFilter(const char* a_outputVariableName)
{
  std::vector<svtkDSPFilterDefinition*>::iterator l_iter;
  std::vector<std::vector<svtkFloatArray*> >::iterator l_cachedOutputsIter =
    this->CachedOutputs->m_vector.begin();
  std::vector<std::vector<int> >::iterator l_cachedOutputTimesIter =
    this->CachedOutputTimesteps->m_vector.begin();

  for (l_iter = this->FilterDefinitions->m_vector.begin();
       l_iter != this->FilterDefinitions->m_vector.end(); ++l_iter)
  {
    if (!strcmp(a_outputVariableName, (*l_iter)->GetOutputVariableName()))
    {
      // this is the filter to delete
      this->FilterDefinitions->m_vector.erase(l_iter);
      if (l_cachedOutputsIter != this->CachedOutputs->m_vector.end())
        this->CachedOutputs->m_vector.erase(l_cachedOutputsIter);
      if (l_cachedOutputTimesIter != this->CachedOutputTimesteps->m_vector.end())
        this->CachedOutputTimesteps->m_vector.erase(l_cachedOutputTimesIter);
      break;
    }
    ++l_cachedOutputsIter;
    ++l_cachedOutputTimesIter;
  }

#if 0
  printf("**********************FILTERS AFTER REMOVING FILTER*********************\n");
  for(int i=0;i<this->GetNumFilters();i++)
  {
      svtkDSPFilterDefinition *filterfromlist = this->FilterDefinitions[i];
      printf("svtkDSPFilterGroup::RemoveFilter %d of %d  input=%s output=%s nums=%d dens=%d    this=%p\n",
       i,this->GetNumFilters(),
       filterfromlist->GetInputVariableName(),
       filterfromlist->GetOutputVariableName(),
       filterfromlist->GetNumNumeratorWeights(),
       filterfromlist->GetNumDenominatorWeights(),
       this);
  }
  printf("************************************************************************\n");
#endif
}

//----------------------------------------------------------------------------
void svtkDSPFilterGroup::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* svtkDSPFilterGroup::GetInputVariableName(int a_whichFilter)
{
  return this->FilterDefinitions->m_vector[a_whichFilter]->GetInputVariableName();
}
//----------------------------------------------------------------------------
bool svtkDSPFilterGroup::IsThisInputVariableInstanceNeeded(
  const char* a_name, int a_timestep, int a_outputTimestep)
{
  for (int i = 0; i < this->GetNumFilters(); i++)
  {
    if (!strcmp(this->FilterDefinitions->m_vector[i]->GetInputVariableName(), a_name))
    {
      if (this->FilterDefinitions->m_vector[i]->IsThisInputVariableInstanceNeeded(
            a_timestep, a_outputTimestep))
      {
        return (true);
      }
    }
  }
  return (false);
}
//----------------------------------------------------------------------------
bool svtkDSPFilterGroup::IsThisInputVariableInstanceCached(const char* a_name, int a_timestep)
{
  for (int i = 0; i < (int)this->CachedInputTimesteps->m_vector.size(); i++)
  {
    if (this->CachedInputTimesteps->m_vector[i] == a_timestep)
    {
      if (this->CachedInputNames->m_vector[i] == a_name)
      {
        return (true);
      }
    }
  }
  return (false);
}
//----------------------------------------------------------------------------
void svtkDSPFilterGroup::AddInputVariableInstance(
  const char* a_name, int a_timestep, svtkFloatArray* a_data)
{
  // This assumes that the instance is not already cached! perhaps should check anyway?

  this->CachedInputTimesteps->m_vector.push_back(a_timestep);
  this->CachedInputNames->m_vector.push_back(a_name);

  svtkFloatArray* l_array = svtkFloatArray::New();
  l_array->DeepCopy(a_data);
  this->CachedInputs->m_vector.push_back(l_array);
}

//----------------------------------------------------------------------------
svtkFloatArray* svtkDSPFilterGroup::GetCachedInput(int a_whichFilter, int a_whichTimestep)
{
  std::string l_inputName =
    this->FilterDefinitions->m_vector[a_whichFilter]->GetInputVariableName();
  for (int i = 0; i < (int)this->CachedInputTimesteps->m_vector.size(); i++)
  {
    if (this->CachedInputTimesteps->m_vector[i] == a_whichTimestep)
    {
      if (this->CachedInputNames->m_vector[i] == l_inputName)
      {
        return (this->CachedInputs->m_vector[i]);
      }
    }
  }
  return (nullptr);
}

//----------------------------------------------------------------------------
svtkFloatArray* svtkDSPFilterGroup::GetCachedOutput(int a_whichFilter, int a_whichTimestep)
{
  for (int i = 0; i < (int)this->CachedOutputs->m_vector[a_whichFilter].size(); i++)
  {
    if (a_whichTimestep == this->CachedOutputTimesteps->m_vector[a_whichFilter][i])
    {
      svtkFloatArray* l_tmp = (this->CachedOutputs->m_vector[a_whichFilter])[i];
      if (!strcmp(l_tmp->GetName(),
            this->FilterDefinitions->m_vector[a_whichFilter]->GetOutputVariableName()))
      {
        // printf("svtkDSPFilterGroup::GetCachedOutput found time %d output in
        // cache\n",a_whichTimestep);
        return (l_tmp);
      }

      // else printf("svtkDSPFilterGroup::GetCachedOutput DID NOT FIND time %d output in cache %s
      // %s\n",a_whichTimestep,
      //        l_tmp->GetName(), this->FilterDefinitions[a_whichFilter]->OutputVariableName.c_str()
      //        );
    }
  }

  return (nullptr);
}

//----------------------------------------------------------------------------
void svtkDSPFilterGroup::Copy(svtkDSPFilterGroup* other)
{
  this->FilterDefinitions->m_vector = other->FilterDefinitions->m_vector;
}

//----------------------------------------------------------------------------
int svtkDSPFilterGroup::GetNumFilters()
{
  return static_cast<int>(this->FilterDefinitions->m_vector.size());
}

//----------------------------------------------------------------------------
svtkDSPFilterDefinition* svtkDSPFilterGroup::GetFilter(int a_whichFilter)
{
  return this->FilterDefinitions->m_vector[a_whichFilter];
}

//----------------------------------------------------------------------------
svtkFloatArray* svtkDSPFilterGroup::GetOutput(
  int a_whichFilter, int a_whichTimestep, int& a_instancesCalculated)
{
  int i, j, k;
  int l_numFilters = this->GetNumFilters();

  if ((int)this->CachedOutputs->m_vector.size() < l_numFilters)
  {
    // this shouldn't happen with saf. Should happen 1 time with exodus.
    // printf("svtkDSPFilterGroup::GetOutput resizing cache vector\n");

    int l_numNow = (int)this->CachedOutputs->m_vector.size();
    for (i = l_numNow; i < l_numFilters; i++)
    {
      std::vector<svtkFloatArray*> l_cachedOutsForThisFilter;
      l_cachedOutsForThisFilter.resize(0);
      this->CachedOutputs->m_vector.push_back(l_cachedOutsForThisFilter);

      std::vector<int> l_cachedOutTimesForThisFilter;
      l_cachedOutTimesForThisFilter.resize(0);
      this->CachedOutputTimesteps->m_vector.push_back(l_cachedOutTimesForThisFilter);
    }
  }

  // is this output array already cached?
  svtkFloatArray* l_tmp = this->GetCachedOutput(a_whichFilter, a_whichTimestep);
  if (l_tmp)
  {
    // printf("svtkDSPFilterGroup::GetOutput found time %d output in cache\n",a_whichTimestep);
    return (l_tmp);
  }
  // else printf("svtkDSPFilterGroup::GetOutput DID NOT FIND time %d output in cache (%d cache
  // slots)\n",
  //      a_whichTimestep,(int)this->CachedOutputs[a_whichFilter].size() );

  svtkFloatArray* l_output = svtkFloatArray::New();
  l_output->SetName(FilterDefinitions->m_vector[a_whichFilter]->GetOutputVariableName());

  int l_numNumerators = (int)FilterDefinitions->m_vector[a_whichFilter]->GetNumNumeratorWeights();
  int l_numForwardNumerators =
    (int)FilterDefinitions->m_vector[a_whichFilter]->GetNumForwardNumeratorWeights();
  if (!l_numNumerators && !l_numForwardNumerators)
  {
    printf("svtkDSPFilterGroup::GetOutput there are no numerator filter weights?\n");
    return (nullptr);
  }
  int l_numDenominators =
    (int)FilterDefinitions->m_vector[a_whichFilter]->GetNumDenominatorWeights();

  double l_a1 = 1.0;
  if (l_numDenominators)
  {
    l_a1 = FilterDefinitions->m_vector[a_whichFilter]->GetDenominatorWeight(0);
  }

  // printf("svtkDSPFilterGroup::GetOutput numerators=%d forwardnums=%d dens=%d\n",
  //   l_numNumerators,l_numForwardNumerators,l_numDenominators);

  // There should always be a valid input at the same time as an output
  svtkFloatArray* l_firstInput = this->GetCachedInput(a_whichFilter, a_whichTimestep);

  if (!l_firstInput)
  {
    printf("\n  svtkDSPFilterGroup::GetOutput error time %d has no input\n\n", a_whichTimestep);
    return (nullptr);
  }

  const int l_numEntries = l_firstInput->GetNumberOfTuples();
  const int l_numComponents = l_firstInput->GetNumberOfComponents();

  if (!l_numEntries || !l_numComponents)
  {
    printf(
      "\n  svtkDSPFilterGroup::GetOutput error time %d, l_numEntries=%d, l_numComponents=%d\n\n",
      a_whichTimestep, l_numEntries, l_numComponents);
    return (nullptr);
  }

  // printf("svtkDSPFilterGroup::GetOutput first input entries=%d
  // comps=%d\n",l_numEntries,l_numComponents);

  l_output->SetNumberOfComponents(l_numComponents);
  l_output->SetNumberOfTuples(l_numEntries);

  for (i = 0; i < l_numNumerators; i++)
  {
    int l_useThisTimestep = a_whichTimestep - i;
    double l_weight =
      this->FilterDefinitions->m_vector[a_whichFilter]->GetNumeratorWeight(i) / l_a1;

    if (l_useThisTimestep < 0)
      l_useThisTimestep = 0; // pre-time is considered infinite procession of input value at time 0

    // printf("svtkDSPFilterGroup::GetOutput numerator weight %d is %e (incl a1=%e)
    // time=%d\n",i,l_weight,l_a1,l_useThisTimestep);

    svtkFloatArray* l_input = this->GetCachedInput(a_whichFilter, l_useThisTimestep);
    float* l_outPtr = (float*)l_output->GetVoidPointer(0);

    if (!i)
    {
      for (j = 0; j < l_numEntries * l_numComponents; j++)
        l_outPtr[i] = 0;
    }

    if (l_input)
    {
      float* l_inPtr = (float*)l_input->GetVoidPointer(0);
      for (j = 0; j < l_numEntries; j++)
      {
        for (k = 0; k < l_numComponents; k++)
        {
          l_outPtr[0] += l_weight * l_inPtr[0];
          l_inPtr++;
          l_outPtr++;
        }
      }
    }
    else
    {
      printf("error svtkDSPFilterGroup::GetOutput can't get input %d\n", l_useThisTimestep);
    }
  }

  for (i = 1; i < l_numDenominators; i++)
  {
    double l_weight =
      this->FilterDefinitions->m_vector[a_whichFilter]->GetDenominatorWeight(i) / l_a1;

    if (a_whichTimestep - i < 0)
      break; // pre-time outputs are considered to be zero

    // printf("svtkDSPFilterGroup::GetOutput denominator weight %d is %e (incl a1=%e)
    // time=%d\n",i,l_weight,l_a1,a_whichTimestep-i);

    svtkFloatArray* l_input =
      this->GetOutput(a_whichFilter, a_whichTimestep - i, a_instancesCalculated);

    float* l_outPtr = (float*)l_output->GetVoidPointer(0);

    if (l_input)
    {
      float* l_inPtr = (float*)l_input->GetVoidPointer(0);
      for (j = 0; j < l_numEntries; j++)
      {
        for (k = 0; k < l_numComponents; k++)
        {
          l_outPtr[0] -= l_weight * l_inPtr[0];
          l_inPtr++;
          l_outPtr++;
        }
      }
    }
  }

  // Handle forward inputs
  for (i = 0; i < l_numForwardNumerators; i++)
  {
    int l_useThisTimestep = a_whichTimestep + i + 1;
    double l_weight =
      this->FilterDefinitions->m_vector[a_whichFilter]->GetForwardNumeratorWeight(i) / l_a1;

    float* l_outPtr = (float*)l_output->GetVoidPointer(0);

    svtkFloatArray* l_input = this->GetCachedInput(a_whichFilter, l_useThisTimestep);

    while (!l_input && l_useThisTimestep >= 0)
    {
      // printf("         time %d failed......trying prev time.....\n",l_useThisTimestep);

      // Try the timestep before: all post-time inputs are considered to be the same as the last
      // input
      l_useThisTimestep--;
      l_input = this->GetCachedInput(a_whichFilter, l_useThisTimestep);
    }

    if (l_input)
    {

      // printf("svtkDSPFilterGroup::GetOutput forward numerator weight %d is %e (incl a1=%e)
      // time=%d\n",i,l_weight,l_a1,l_useThisTimestep);

      float* l_inPtr = (float*)l_input->GetVoidPointer(0);
      for (j = 0; j < l_numEntries; j++)
      {
        for (k = 0; k < l_numComponents; k++)
        {
          l_outPtr[0] += l_weight * l_inPtr[0];
          l_inPtr++;
          l_outPtr++;
        }
      }
    }
    else
    {
      printf(
        "\nerror svtkDSPFilterGroup::GetOutput can't get forward input %d\n\n", l_useThisTimestep);
    }
  }

#if 0 // debug print
  {
     float *l_outPtr = (float *)l_output->GetVoidPointer(0);
     float *l_inPtr = (float *)l_firstInput->GetVoidPointer(0);
     float l_maxDiff=0;
     for(j=0;j<l_numEntries;j++)
     {
   for(k=0;k<l_numComponents;k++)
   {
       if( l_inPtr[0] - l_outPtr[0] )
       {
     if( fabs(l_inPtr[0] - l_outPtr[0]) > l_maxDiff ) l_maxDiff = fabs(l_inPtr[0] - l_outPtr[0]);


     printf("j=%d k=%d \t in=%f \t out=%f \t diff=%e   maxdiff=%e   diffperc=%f\n",j,k,
               l_inPtr[0],l_outPtr[0],l_inPtr[0] - l_outPtr[0],l_maxDiff,
      fabs(l_inPtr[0] - l_outPtr[0]) / fabs(l_inPtr[0]) );

       }
       l_inPtr++;
       l_outPtr++;
   }
     }
  }

#endif

  a_instancesCalculated++;

  // printf("****svtkDSPFilterGroup::GetOutput calculated  filter=%d time=%d entries=%d comps=%d***
  // out cache was %d slots\n",a_whichFilter,
  // a_whichTimestep,l_numEntries,l_numComponents,
  // this->CachedOutputs[a_whichFilter].size()  );

  this->CachedOutputs->m_vector[a_whichFilter].push_back(l_output);
  this->CachedOutputTimesteps->m_vector[a_whichFilter].push_back(a_whichTimestep);

  return (l_output);
}
