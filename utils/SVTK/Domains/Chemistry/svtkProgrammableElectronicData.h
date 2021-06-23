/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgrammableElectronicData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProgrammableElectronicData
 * @brief   Provides access to and storage of
 * user-generated svtkImageData that describes electrons.
 */

#ifndef svtkProgrammableElectronicData_h
#define svtkProgrammableElectronicData_h

#include "svtkAbstractElectronicData.h"
#include "svtkDomainsChemistryModule.h" // For export macro

class svtkImageData;

class StdVectorOfImageDataPointers;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkProgrammableElectronicData : public svtkAbstractElectronicData
{
public:
  static svtkProgrammableElectronicData* New();
  svtkTypeMacro(svtkProgrammableElectronicData, svtkAbstractElectronicData);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the number of molecular orbitals. Setting this will resize this
   * internal array of MOs.
   */
  svtkIdType GetNumberOfMOs() override;
  virtual void SetNumberOfMOs(svtkIdType);
  //@}

  //@{
  /**
   * Get/Set the number of electrons in the molecule. Needed for HOMO/LUMO
   * convenience functions
   */
  svtkIdType GetNumberOfElectrons() override { return this->NumberOfElectrons; }
  svtkSetMacro(NumberOfElectrons, svtkIdType);
  //@}

  //@{
  /**
   * Get/Set the svtkImageData for the requested molecular orbital.
   */
  svtkImageData* GetMO(svtkIdType orbitalNumber) override;
  void SetMO(svtkIdType orbitalNumber, svtkImageData* data);
  //@}

  //@{
  /**
   * Get/Set the svtkImageData for the molecule's electron density.
   */
  svtkImageData* GetElectronDensity() override { return this->ElectronDensity; }
  virtual void SetElectronDensity(svtkImageData*);
  //@}

  //@{
  /**
   * Set the padding around the molecule to which the cube extends. This
   * is used to determine the dataset bounds.
   */
  svtkSetMacro(Padding, double);
  //@}

  /**
   * Deep copies the data object into this.
   */
  void DeepCopy(svtkDataObject* obj) override;

protected:
  svtkProgrammableElectronicData();
  ~svtkProgrammableElectronicData() override;

  /**
   * Electronic data set property
   */
  svtkIdType NumberOfElectrons;

  //@{
  /**
   * Storage for the svtkImageData objects
   */
  StdVectorOfImageDataPointers* MOs;
  svtkImageData* ElectronDensity;
  //@}

private:
  svtkProgrammableElectronicData(const svtkProgrammableElectronicData&) = delete;
  void operator=(const svtkProgrammableElectronicData&) = delete;
};

#endif
