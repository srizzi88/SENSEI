/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPeriodicTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPeriodicTable
 * @brief   Access to information about the elements.
 *
 *
 * Sourced from the Blue Obelisk Data Repository
 *
 * @sa
 * svtkBlueObeliskData svtkBlueObeliskDataParser
 */

#ifndef svtkPeriodicTable_h
#define svtkPeriodicTable_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkNew.h"                    // Needed for the static data member
#include "svtkObject.h"

class svtkBlueObeliskData;
class svtkColor3f;
class svtkLookupTable;
class svtkStdString;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkPeriodicTable : public svtkObject
{
public:
  svtkTypeMacro(svtkPeriodicTable, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkPeriodicTable* New();

  //@{
  /**
   * Access the static svtkBlueObeliskData object for raw access to
   * BODR data.
   */
  svtkGetNewMacro(BlueObeliskData, svtkBlueObeliskData);
  //@}

  /**
   * Returns the number of elements in the periodic table.
   */
  unsigned short GetNumberOfElements();

  /**
   * Given an atomic number, returns the symbol associated with the
   * element
   */
  const char* GetSymbol(unsigned short atomicNum);

  /**
   * Given an atomic number, returns the name of the element
   */
  const char* GetElementName(unsigned short atomicNum);

  //@{
  /**
   * Given a case-insensitive string that contains the symbol or name
   * of an element, return the corresponding atomic number.
   */
  unsigned short GetAtomicNumber(const svtkStdString& str);
  unsigned short GetAtomicNumber(const char* str);
  //@}

  /**
   * Given an atomic number, return the covalent radius of the atom
   */
  float GetCovalentRadius(unsigned short atomicNum);

  /**
   * Given an atomic number, returns the van der Waals radius of the
   * atom
   */
  float GetVDWRadius(unsigned short atomicNum);

  /**
   * Given an atomic number, returns the van der Waals radius of the
   * atom
   */
  float GetMaxVDWRadius();

  /**
   * Fill the given svtkLookupTable to map atomic numbers to the
   * familiar RGB tuples provided by the Blue Obelisk Data Repository
   */
  void GetDefaultLUT(svtkLookupTable*);

  /**
   * Given an atomic number, return the familiar RGB tuple provided by
   * the Blue Obelisk Data Repository
   */
  void GetDefaultRGBTuple(unsigned short atomicNum, float rgb[3]);

  /**
   * Given an atomic number, return the familiar RGB tuple provided by
   * the Blue Obelisk Data Repository
   */
  svtkColor3f GetDefaultRGBTuple(unsigned short atomicNum);

protected:
  svtkPeriodicTable();
  ~svtkPeriodicTable() override;

  static svtkNew<svtkBlueObeliskData> BlueObeliskData;

private:
  svtkPeriodicTable(const svtkPeriodicTable&) = delete;
  void operator=(const svtkPeriodicTable&) = delete;
};

#endif
