/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDSPFilterDefinition.h

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
/**
 * @class   svtkDSPFilterDefinition
 * @brief   used by the Exodus readers
 *
 * svtkDSPFilterDefinition is used by svtkExodusReader, svtkExodusIIReader and
 * svtkPExodusReader to do temporal smoothing of data
 * @sa
 * svtkDSPFilterGroup svtkExodusReader svtkExodusIIReader svtkPExodusReader
 */

#ifndef svtkDSPFilterDefinition_h
#define svtkDSPFilterDefinition_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkObject.h"

class svtkDSPFilterDefinitionVectorDoubleSTLCloak;
class svtkDSPFilterDefinitionStringSTLCloak;

class SVTKFILTERSHYBRID_EXPORT svtkDSPFilterDefinition : public svtkObject
{
public:
  svtkTypeMacro(svtkDSPFilterDefinition, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkDSPFilterDefinition* New();

protected:
  svtkDSPFilterDefinition();
  svtkDSPFilterDefinition(svtkDSPFilterDefinition* other);
  ~svtkDSPFilterDefinition() override;

public:
  void Copy(svtkDSPFilterDefinition* other);
  void Clear();
  bool IsThisInputVariableInstanceNeeded(int a_timestep, int a_outputTimestep);

  void PushBackNumeratorWeight(double a_value);
  void PushBackDenominatorWeight(double a_value);
  void PushBackForwardNumeratorWeight(double a_value);
  void SetInputVariableName(const char* a_value);
  void SetOutputVariableName(const char* a_value);
  const char* GetInputVariableName();
  const char* GetOutputVariableName();

  int GetNumNumeratorWeights();
  int GetNumDenominatorWeights();
  int GetNumForwardNumeratorWeights();

  double GetNumeratorWeight(int a_which);
  double GetDenominatorWeight(int a_which);
  double GetForwardNumeratorWeight(int a_which);

  svtkDSPFilterDefinitionVectorDoubleSTLCloak* NumeratorWeights;
  svtkDSPFilterDefinitionVectorDoubleSTLCloak* DenominatorWeights;
  svtkDSPFilterDefinitionVectorDoubleSTLCloak* ForwardNumeratorWeights;

  svtkDSPFilterDefinitionStringSTLCloak* InputVariableName;
  svtkDSPFilterDefinitionStringSTLCloak* OutputVariableName;

protected:
private:
  svtkDSPFilterDefinition(const svtkDSPFilterDefinition&) = delete;
  void operator=(const svtkDSPFilterDefinition&) = delete;
};

#endif
