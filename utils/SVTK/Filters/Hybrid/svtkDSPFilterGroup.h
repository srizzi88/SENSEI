/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDSPFilterGroup.h

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
 * @class   svtkDSPFilterGroup
 * @brief   used by the Exodus readers
 *
 * svtkDSPFilterGroup is used by svtkExodusReader, svtkExodusIIReader and
 * svtkPExodusReader to do temporal smoothing of data
 * @sa
 * svtkDSPFilterDefinition svtkExodusReader svtkExodusIIReader svtkPExodusReader
 */

#ifndef svtkDSPFilterGroup_h
#define svtkDSPFilterGroup_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkObject.h"

class svtkDSPFilterGroupVectorIntSTLCloak;
class svtkDSPFilterGroupVectorVectorIntSTLCloak;
class svtkDSPFilterGroupVectorArraySTLCloak;
class svtkDSPFilterGroupVectorVectorArraySTLCloak;
class svtkDSPFilterGroupVectorStringSTLCloak;
class svtkDSPFilterGroupVectorDefinitionSTLCloak;
class svtkFloatArray;
class svtkDSPFilterDefinition;

class SVTKFILTERSHYBRID_EXPORT svtkDSPFilterGroup : public svtkObject
{
public:
  static svtkDSPFilterGroup* New();
  svtkTypeMacro(svtkDSPFilterGroup, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void AddFilter(svtkDSPFilterDefinition* filter);
  void RemoveFilter(const char* a_outputVariableName);

  bool IsThisInputVariableInstanceNeeded(const char* a_name, int a_timestep, int a_outputTimestep);
  bool IsThisInputVariableInstanceCached(const char* a_name, int a_timestep);
  void AddInputVariableInstance(const char* a_name, int a_timestep, svtkFloatArray* a_data);

  svtkFloatArray* GetOutput(int a_whichFilter, int a_whichTimestep, int& a_instancesCalculated);

  svtkFloatArray* GetCachedInput(int a_whichFilter, int a_whichTimestep);
  svtkFloatArray* GetCachedOutput(int a_whichFilter, int a_whichTimestep);

  const char* GetInputVariableName(int a_whichFilter);

  int GetNumFilters();

  void Copy(svtkDSPFilterGroup* other);

  svtkDSPFilterDefinition* GetFilter(int a_whichFilter);

  svtkDSPFilterGroupVectorDefinitionSTLCloak* /*std::vector<svtkDSPFilterDefinition *>*/
    FilterDefinitions;

protected:
  svtkDSPFilterGroup();
  ~svtkDSPFilterGroup() override;

  svtkDSPFilterGroupVectorArraySTLCloak* /*std::vector<svtkFloatArray *>*/ CachedInputs;
  svtkDSPFilterGroupVectorStringSTLCloak* /*std::vector<std::string>*/ CachedInputNames;
  svtkDSPFilterGroupVectorIntSTLCloak* /*std::vector<int>*/ CachedInputTimesteps;

  svtkDSPFilterGroupVectorVectorArraySTLCloak* /*std::vector< std::vector<svtkFloatArray *> >*/
    CachedOutputs;
  svtkDSPFilterGroupVectorVectorIntSTLCloak* /*std::vector< std::vector<int> >*/
    CachedOutputTimesteps;

private:
  svtkDSPFilterGroup(const svtkDSPFilterGroup&) = delete;
  void operator=(const svtkDSPFilterGroup&) = delete;
};

#endif
