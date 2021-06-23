/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCPExodusIIElementBlockCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCPExodusIIElementBlockCellIterator
 * @brief   svtkCellIterator subclass
 * specialized for svtkCPExodusIIElementBlock.
 */

#ifndef svtkCPExodusIIElementBlockCellIterator_h
#define svtkCPExodusIIElementBlockCellIterator_h

#include "svtkCellIterator.h"
#include "svtkIOExodusModule.h" // For export macro

#include "svtkSmartPointer.h" // For smart pointer

class svtkCPExodusIIElementBlock;
class svtkCPExodusIIElementBlockPrivate;

class SVTKIOEXODUS_EXPORT svtkCPExodusIIElementBlockCellIterator : public svtkCellIterator
{
public:
  typedef svtkCPExodusIIElementBlockPrivate StorageType;

  static svtkCPExodusIIElementBlockCellIterator* New();
  svtkTypeMacro(svtkCPExodusIIElementBlockCellIterator, svtkCellIterator);
  void PrintSelf(ostream& os, svtkIndent indent);

  bool IsValid();
  svtkIdType GetCellId();

protected:
  svtkCPExodusIIElementBlockCellIterator();
  ~svtkCPExodusIIElementBlockCellIterator();

  void ResetToFirstCell();
  void IncrementToNextCell();
  void FetchCellType();
  void FetchPointIds();
  void FetchPoints();

  friend class ::svtkCPExodusIIElementBlock;
  void SetStorage(svtkCPExodusIIElementBlock* eb);

private:
  svtkCPExodusIIElementBlockCellIterator(const svtkCPExodusIIElementBlockCellIterator&) = delete;
  void operator=(const svtkCPExodusIIElementBlockCellIterator&) = delete;

  svtkSmartPointer<StorageType> Storage;
  svtkSmartPointer<svtkPoints> DataSetPoints;
  svtkIdType CellId;
};

#endif // svtkCPExodusIIElementBlockCellIterator_h
