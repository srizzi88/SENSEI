/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlueObeliskData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBlueObeliskData
 * @brief   Contains chemical data from the Blue
 * Obelisk Data Repository
 *
 *
 * The Blue Obelisk Data Repository is a free, open repository of
 * chemical information. This class is a container for this
 * information.
 *
 * \note This class contains only the raw arrays parsed from the
 * BODR. For more convenient access to this data, use the
 * svtkPeriodicTable class.
 *
 * \note If you must use this class directly, consider using the
 * static svtkBlueObeliskData object accessible through
 * svtkPeriodicTable::GetBlueObeliskData(). This object is
 * automatically populated on the first instantiation of
 * svtkPeriodicTable.
 */

#ifndef svtkBlueObeliskData_h
#define svtkBlueObeliskData_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkNew.h"                    // For svtkNew
#include "svtkObject.h"

class svtkAbstractArray;
class svtkFloatArray;
class svtkStringArray;
class svtkSimpleMutexLock;
class svtkUnsignedShortArray;

// Hidden STL reference: std::vector<svtkAbstractArray*>
class MyStdVectorOfVtkAbstractArrays;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkBlueObeliskData : public svtkObject
{
public:
  svtkTypeMacro(svtkBlueObeliskData, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkBlueObeliskData* New();

  /**
   * Fill this object using an internal svtkBlueObeliskDataParser
   * instance. Check that the svtkSimpleMutexLock GetWriteMutex() is
   * locked before calling this method on a static instance in a
   * multithreaded environment.
   */
  void Initialize();

  /**
   * Check if this object has been initialized yet.
   */
  bool IsInitialized() { return this->Initialized; }

  //@{
  /**
   * Access the mutex that protects the arrays during a call to
   * Initialize()
   */
  svtkGetObjectMacro(WriteMutex, svtkSimpleMutexLock);
  //@}

  //@{
  /**
   * Return the number of elements for which this svtkBlueObeliskData
   * instance contains information.
   */
  svtkGetMacro(NumberOfElements, unsigned short);
  //@}

  //@{
  /**
   * Access the raw arrays stored in this svtkBlueObeliskData.
   */
  svtkGetNewMacro(Symbols, svtkStringArray);
  svtkGetNewMacro(LowerSymbols, svtkStringArray);
  svtkGetNewMacro(Names, svtkStringArray);
  svtkGetNewMacro(LowerNames, svtkStringArray);
  svtkGetNewMacro(PeriodicTableBlocks, svtkStringArray);
  svtkGetNewMacro(ElectronicConfigurations, svtkStringArray);
  svtkGetNewMacro(Families, svtkStringArray);
  //@}

  svtkGetNewMacro(Masses, svtkFloatArray);
  svtkGetNewMacro(ExactMasses, svtkFloatArray);
  svtkGetNewMacro(IonizationEnergies, svtkFloatArray);
  svtkGetNewMacro(ElectronAffinities, svtkFloatArray);
  svtkGetNewMacro(PaulingElectronegativities, svtkFloatArray);
  svtkGetNewMacro(CovalentRadii, svtkFloatArray);
  svtkGetNewMacro(VDWRadii, svtkFloatArray);
  svtkGetNewMacro(DefaultColors, svtkFloatArray);
  svtkGetNewMacro(BoilingPoints, svtkFloatArray);
  svtkGetNewMacro(MeltingPoints, svtkFloatArray);

  svtkGetNewMacro(Periods, svtkUnsignedShortArray);
  svtkGetNewMacro(Groups, svtkUnsignedShortArray);

  /**
   * Static method to generate the data header file used by this class from the
   * BODR elements.xml. See the GenerateBlueObeliskHeader test in this module.
   */
  static bool GenerateHeaderFromXML(std::istream& xml, std::ostream& header);

protected:
  friend class svtkBlueObeliskDataParser;

  svtkBlueObeliskData();
  ~svtkBlueObeliskData() override;

  svtkSimpleMutexLock* WriteMutex;
  bool Initialized;

  /**
   * Allocate enough memory in each array for sz elements. ext is not
   * used.
   */
  virtual svtkTypeBool Allocate(svtkIdType sz, svtkIdType ext = 1000);

  /**
   * Reset each array.
   */
  virtual void Reset();

  /**
   * Free any unused memory in the member arrays
   */
  virtual void Squeeze();

  unsigned short NumberOfElements;

  // Lists all arrays
  MyStdVectorOfVtkAbstractArrays* Arrays;

  // Atomic Symbols
  svtkNew<svtkStringArray> Symbols;
  svtkNew<svtkStringArray> LowerSymbols;

  // Element Names
  svtkNew<svtkStringArray> Names;
  svtkNew<svtkStringArray> LowerNames;

  // Misc Strings
  svtkNew<svtkStringArray> PeriodicTableBlocks;
  svtkNew<svtkStringArray> ElectronicConfigurations;
  svtkNew<svtkStringArray> Families; // Non-Metal, Noblegas, Metalloids, etc

  // Misc Data
  svtkNew<svtkFloatArray> Masses;                     // amu
  svtkNew<svtkFloatArray> ExactMasses;                // amu
  svtkNew<svtkFloatArray> IonizationEnergies;         // eV
  svtkNew<svtkFloatArray> ElectronAffinities;         // eV
  svtkNew<svtkFloatArray> PaulingElectronegativities; // eV
  svtkNew<svtkFloatArray> CovalentRadii;              // Angstrom
  svtkNew<svtkFloatArray> VDWRadii;                   // Angstom
  svtkNew<svtkFloatArray> DefaultColors;              // rgb 3-tuples, [0.0,1.0]
  svtkNew<svtkFloatArray> BoilingPoints;              // K
  svtkNew<svtkFloatArray> MeltingPoints;              // K
  svtkNew<svtkUnsignedShortArray> Periods;            // Row of periodic table
  svtkNew<svtkUnsignedShortArray> Groups;             // Column of periodic table

  void PrintSelfIfExists(const char*, svtkObject*, ostream&, svtkIndent);

private:
  svtkBlueObeliskData(const svtkBlueObeliskData&) = delete;
  void operator=(const svtkBlueObeliskData&) = delete;
};

#endif
