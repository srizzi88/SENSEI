/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDSPFilterDefinition.cxx

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

#include "svtkDSPFilterDefinition.h"
#include "svtkObjectFactory.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

svtkStandardNewMacro(svtkDSPFilterDefinition);

class svtkDSPFilterDefinitionVectorDoubleSTLCloak
{
public:
  std::vector<double> m_vector;
};
class svtkDSPFilterDefinitionStringSTLCloak
{
public:
  std::string m_string;
};

//----------------------------------------------------------------------------
svtkDSPFilterDefinition::svtkDSPFilterDefinition()
{
  // printf("    in svtkDSPFilterDefinition::svtkDSPFilterDefinition()\n");
  this->NumeratorWeights = new svtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->ForwardNumeratorWeights = new svtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->DenominatorWeights = new svtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->InputVariableName = new svtkDSPFilterDefinitionStringSTLCloak;
  this->OutputVariableName = new svtkDSPFilterDefinitionStringSTLCloak;

  this->NumeratorWeights->m_vector.resize(0);
  this->ForwardNumeratorWeights->m_vector.resize(0);
  this->DenominatorWeights->m_vector.resize(0);
  this->InputVariableName->m_string = "";
  this->OutputVariableName->m_string = "";
}
//----------------------------------------------------------------------------
svtkDSPFilterDefinition::svtkDSPFilterDefinition(svtkDSPFilterDefinition* other)
{
  // printf("    in svtkDSPFilterDefinition::svtkDSPFilterDefinition(svtkDSPFilterDefinition
  // *other)\n");
  this->NumeratorWeights = new svtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->ForwardNumeratorWeights = new svtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->DenominatorWeights = new svtkDSPFilterDefinitionVectorDoubleSTLCloak;
  this->InputVariableName = new svtkDSPFilterDefinitionStringSTLCloak;
  this->OutputVariableName = new svtkDSPFilterDefinitionStringSTLCloak;

  this->NumeratorWeights->m_vector = other->NumeratorWeights->m_vector;
  this->ForwardNumeratorWeights->m_vector = other->ForwardNumeratorWeights->m_vector;
  this->DenominatorWeights->m_vector = other->DenominatorWeights->m_vector;
  this->InputVariableName->m_string = other->InputVariableName->m_string;
  this->OutputVariableName->m_string = other->OutputVariableName->m_string;
}
//----------------------------------------------------------------------------
void svtkDSPFilterDefinition::Copy(svtkDSPFilterDefinition* other)
{
  // printf("    in svtkDSPFilterDefinition::Copy(svtkDSPFilterDefinition *other)\n");
  this->NumeratorWeights->m_vector = other->NumeratorWeights->m_vector;
  this->ForwardNumeratorWeights->m_vector = other->ForwardNumeratorWeights->m_vector;
  this->DenominatorWeights->m_vector = other->DenominatorWeights->m_vector;
  this->InputVariableName->m_string = other->InputVariableName->m_string;
  this->OutputVariableName->m_string = other->OutputVariableName->m_string;
}
//----------------------------------------------------------------------------
svtkDSPFilterDefinition::~svtkDSPFilterDefinition()
{
  this->NumeratorWeights->m_vector.resize(0);
  this->ForwardNumeratorWeights->m_vector.resize(0);
  this->DenominatorWeights->m_vector.resize(0);
  this->InputVariableName->m_string = "";
  this->OutputVariableName->m_string = "";

  delete this->NumeratorWeights;
  delete this->ForwardNumeratorWeights;
  delete this->DenominatorWeights;
  delete this->InputVariableName;
  delete this->OutputVariableName;
}
//----------------------------------------------------------------------------
void svtkDSPFilterDefinition::Clear()
{
  this->NumeratorWeights->m_vector.resize(0);
  this->ForwardNumeratorWeights->m_vector.resize(0);
  this->DenominatorWeights->m_vector.resize(0);
  this->InputVariableName->m_string = "";
  this->OutputVariableName->m_string = "";
}
//----------------------------------------------------------------------------
bool svtkDSPFilterDefinition::IsThisInputVariableInstanceNeeded(int a_timestep, int a_outputTimestep)
{
  if (a_outputTimestep < a_timestep)
  {
    int l_index = a_timestep - a_outputTimestep;
    if ((int)(this->ForwardNumeratorWeights->m_vector.size()) >= l_index)
    {
      // the filter does use this future input
      // printf("FILTER USES FUTURE INPUT %d for output %d\n",a_timestep,a_outputTimestep);
      return (true);
    }
    else
    {
      // future inputs not used for 1d filter
      // printf("FILTER doesn't use FUTURE INPUT %d for output %d\n",a_timestep,a_outputTimestep);
      return (false);
    }
  }
  if (this->DenominatorWeights->m_vector.size() > 1)
  {
    // with an iir filter, all prev outputs since the beginning of time are used,
    // therefore all prev inputs are used as well
    return (true);
  }

  // For an fir filter, only a certain number of past inputs are needed
  if (a_timestep < a_outputTimestep - ((int)(this->NumeratorWeights->m_vector.size()) - 1))
  {
    // this input is too far in the past
    return (false);
  }
  return (true);
}

//----------------------------------------------------------------------------
void svtkDSPFilterDefinition::PushBackNumeratorWeight(double a_value)
{
  this->NumeratorWeights->m_vector.push_back(a_value);
}
//----------------------------------------------------------------------------
void svtkDSPFilterDefinition::PushBackDenominatorWeight(double a_value)
{
  this->DenominatorWeights->m_vector.push_back(a_value);
}
//----------------------------------------------------------------------------
void svtkDSPFilterDefinition::PushBackForwardNumeratorWeight(double a_value)
{
  this->ForwardNumeratorWeights->m_vector.push_back(a_value);
}

//----------------------------------------------------------------------------
void svtkDSPFilterDefinition::SetInputVariableName(const char* a_value)
{
  this->InputVariableName->m_string = a_value;
}
//----------------------------------------------------------------------------
void svtkDSPFilterDefinition::SetOutputVariableName(const char* a_value)
{
  this->OutputVariableName->m_string = a_value;
}

//----------------------------------------------------------------------------
const char* svtkDSPFilterDefinition::GetInputVariableName()
{
  return this->InputVariableName->m_string.c_str();
}
//----------------------------------------------------------------------------
const char* svtkDSPFilterDefinition::GetOutputVariableName()
{
  return this->OutputVariableName->m_string.c_str();
}

//----------------------------------------------------------------------------
int svtkDSPFilterDefinition::GetNumNumeratorWeights()
{
  return static_cast<int>(this->NumeratorWeights->m_vector.size());
}
//----------------------------------------------------------------------------
int svtkDSPFilterDefinition::GetNumDenominatorWeights()
{
  return static_cast<int>(this->DenominatorWeights->m_vector.size());
}
//----------------------------------------------------------------------------
int svtkDSPFilterDefinition::GetNumForwardNumeratorWeights()
{
  return static_cast<int>(this->ForwardNumeratorWeights->m_vector.size());
}

//----------------------------------------------------------------------------
double svtkDSPFilterDefinition::GetNumeratorWeight(int a_which)
{
  return this->NumeratorWeights->m_vector[a_which];
}
//----------------------------------------------------------------------------
double svtkDSPFilterDefinition::GetDenominatorWeight(int a_which)
{
  return this->DenominatorWeights->m_vector[a_which];
}
//----------------------------------------------------------------------------
double svtkDSPFilterDefinition::GetForwardNumeratorWeight(int a_which)
{
  return this->ForwardNumeratorWeights->m_vector[a_which];
}

//----------------------------------------------------------------------------
void svtkDSPFilterDefinition::PrintSelf(ostream& os, svtkIndent indent)
{

  this->Superclass::PrintSelf(os, indent);
}
