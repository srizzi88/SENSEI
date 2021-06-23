/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPOpenFOAMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPOpenFOAMReader
 * @brief   reads a decomposed dataset in OpenFOAM format
 *
 * svtkPOpenFOAMReader creates a multiblock dataset. It reads
 * parallel-decomposed mesh information and time dependent data.  The
 * polyMesh folders contain mesh information. The time folders contain
 * transient data for the cells. Each folder can contain any number of
 * data files.
 *
 * @par Thanks:
 * This class was developed by Takuya Oshima at Niigata University,
 * Japan (oshima@eng.niigata-u.ac.jp).
 */

#ifndef svtkPOpenFOAMReader_h
#define svtkPOpenFOAMReader_h

#include "svtkIOParallelModule.h" // For export macro
#include "svtkOpenFOAMReader.h"

class svtkDataArraySelection;
class svtkMultiProcessController;

class SVTKIOPARALLEL_EXPORT svtkPOpenFOAMReader : public svtkOpenFOAMReader
{
public:
  enum caseType
  {
    DECOMPOSED_CASE = 0,
    RECONSTRUCTED_CASE = 1
  };

  static svtkPOpenFOAMReader* New();
  svtkTypeMacro(svtkPOpenFOAMReader, svtkOpenFOAMReader);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set and get case type. 0 = decomposed case, 1 = reconstructed case.
   */
  void SetCaseType(const int t);
  svtkGetMacro(CaseType, caseType);
  //@}
  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPOpenFOAMReader();
  ~svtkPOpenFOAMReader() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkMultiProcessController* Controller;
  caseType CaseType;
  svtkMTimeType MTimeOld;
  int NumProcesses;
  int ProcessId;

  svtkPOpenFOAMReader(const svtkPOpenFOAMReader&) = delete;
  void operator=(const svtkPOpenFOAMReader&) = delete;

  void GatherMetaData();
  void BroadcastStatus(int&);
  void Broadcast(svtkStringArray*);
  void AllGather(svtkStringArray*);
  void AllGather(svtkDataArraySelection*);
};

#endif
