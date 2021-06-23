/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenQubeElectronicData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenQubeElectronicData
 * @brief   Provides access to and storage of
 * electronic data calculated by OpenQube.
 */

#ifndef svtkOpenQubeElectronicData_h
#define svtkOpenQubeElectronicData_h

#include "svtkAbstractElectronicData.h"
#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkNew.h"                    // for svtkNew

namespace OpenQube
{
class BasisSet;
class Cube;
}

class svtkImageData;
class svtkDataSetCollection;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkOpenQubeElectronicData : public svtkAbstractElectronicData
{
public:
  static svtkOpenQubeElectronicData* New();
  svtkTypeMacro(svtkOpenQubeElectronicData, svtkAbstractElectronicData);
  void PrintSelf(ostream& os, svtkIndent indent);

  /**
   * Returns the number of molecular orbitals in the OpenQube::BasisSet.
   */
  svtkIdType GetNumberOfMOs();

  /**
   * Returns the number of electrons in the molecule.
   */
  unsigned int GetNumberOfElectrons();

  /**
   * Returns the svtkImageData for the requested molecular orbital. The data
   * will be calculated when first requested, and cached for later requests.
   */
  svtkImageData* GetMO(svtkIdType orbitalNumber);

  /**
   * Returns svtkImageData for the molecule's electron density. The data
   * will be calculated when first requested, and cached for later requests.
   */
  svtkImageData* GetElectronDensity();

  //@{
  /**
   * Set/Get the OpenQube::BasisSet object used to generate the image data
   */
  svtkSetMacro(BasisSet, OpenQube::BasisSet*);
  svtkGetMacro(BasisSet, OpenQube::BasisSet*);
  //@}

  //@{
  /**
   * Set/Get the padding around the molecule used in determining the image
   * limits. Default: 2.0
   */
  svtkSetMacro(Padding, double);
  svtkGetMacro(Padding, double);
  //@}

  //@{
  /**
   * Set/Get the interval distance between grid points. Default: 0.1
   */
  svtkSetMacro(Spacing, double);
  svtkGetMacro(Spacing, double);
  //@}

  //@{
  /**
   * Get the collection of cached images
   */
  svtkGetNewMacro(Images, svtkDataSetCollection);
  //@}

  /**
   * Deep copies the data object into this.
   */
  virtual void DeepCopy(svtkDataObject* obj);

protected:
  svtkOpenQubeElectronicData();
  ~svtkOpenQubeElectronicData() override;

  //@{
  /**
   * Calculates and returns the requested svtkImageData. The data is added to
   * the cache, but the cache is not searched in this function.
   */
  svtkImageData* CalculateMO(svtkIdType orbitalNumber);
  svtkImageData* CalculateElectronDensity();
  //@}

  /**
   * Converts an OpenQube::Cube object into svtkImageData.
   */
  void FillImageDataFromQube(OpenQube::Cube* qube, svtkImageData* image);

  /**
   * Cache of calculated image data.
   */
  svtkNew<svtkDataSetCollection> Images;

  /**
   * The OpenQube::BasisSet object used to calculate the images.
   */
  OpenQube::BasisSet* BasisSet;

  /**
   * Used to determine the spacing of the image data.
   */
  double Spacing;

private:
  svtkOpenQubeElectronicData(const svtkOpenQubeElectronicData&) = delete;
  void operator=(const svtkOpenQubeElectronicData&) = delete;
};

#endif
