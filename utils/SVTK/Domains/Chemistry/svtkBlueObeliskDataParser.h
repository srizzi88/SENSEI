/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlueObeliskDataParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBlueObeliskDataParser
 * @brief   Fill a svtkBlueObeliskData
 * container with data from the BODR XML dataset.
 *
 *
 * The Blue Obelisk Data Repository is a free, open repository of
 * chemical information. This class extracts the BODR information into
 * svtk arrays, which are stored in a svtkBlueObeliskData object.
 *
 * \warning The svtkBlueObeliskDataParser class should never need to be
 * used directly. For convenient access to the BODR data, use
 * svtkPeriodicTable. For access to the raw arrays produced by this
 * parser, see the svtkBlueObeliskData class. A static
 * svtkBlueObeliskData object is accessible via
 * svtkPeriodicTable::GetBlueObeliskData().
 *
 * @sa
 * svtkPeriodicTable svtkBlueObeliskData
 */

#ifndef svtkBlueObeliskDataParser_h
#define svtkBlueObeliskDataParser_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkXMLParser.h"

#include "svtkSmartPointer.h" // For svtkSmartPointer

class svtkAbstractArray;
class svtkBlueObeliskData;
class svtkFloatArray;
class svtkStdString;
class svtkStringArray;
class svtkUnsignedShortArray;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkBlueObeliskDataParser : public svtkXMLParser
{
public:
  svtkTypeMacro(svtkBlueObeliskDataParser, svtkXMLParser);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkBlueObeliskDataParser* New();

  /**
   * Set the target svtkBlueObeliskData object that this parser will
   * populate
   */
  virtual void SetTarget(svtkBlueObeliskData* bodr);

  /**
   * Start parsing
   */
  int Parse() override;

  //@{
  /**
   * These are only implemented to prevent compiler warnings about hidden
   * virtual overloads. This function simply call Parse(); the arguments are
   * ignored.
   */
  int Parse(const char*) override;
  int Parse(const char*, unsigned int) override;
  //@}

protected:
  svtkBlueObeliskDataParser();
  ~svtkBlueObeliskDataParser() override;

  void StartElement(const char* name, const char** attr) override;
  void EndElement(const char* name) override;

  void CharacterDataHandler(const char* data, int length) override;

  void SetCurrentValue(const char* data, int length);
  void SetCurrentValue(const char* data);

  svtkBlueObeliskData* Target;

  bool IsProcessingAtom;
  void NewAtomStarted(const char** attr);
  void NewAtomFinished();

  bool IsProcessingValue;
  void NewValueStarted(const char** attr);
  void NewValueFinished();

  std::string CharacterDataValueBuffer;

  enum AtomValueType
  {
    None = 0,
    AtomicNumber,
    Symbol,
    Name,
    PeriodicTableBlock,
    ElectronicConfiguration,
    Family,
    Mass,
    ExactMass,
    IonizationEnergy,
    ElectronAffinity,
    PaulingElectronegativity,
    CovalentRadius,
    VDWRadius,
    DefaultColor,
    BoilingPoint,
    MeltingPoint,
    Period,
    Group
  } CurrentValueType;

  int CurrentAtomicNumber;
  svtkStdString* CurrentSymbol;
  svtkStdString* CurrentName;
  svtkStdString* CurrentPeriodicTableBlock;
  svtkStdString* CurrentElectronicConfiguration;
  svtkStdString* CurrentFamily;
  float CurrentMass;
  float CurrentExactMass;
  float CurrentIonizationEnergy;
  float CurrentElectronAffinity;
  float CurrentPaulingElectronegativity;
  float CurrentCovalentRadius;
  float CurrentVDWRadius;
  float CurrentDefaultColor[3];
  float CurrentBoilingPoint;
  float CurrentMeltingPoint;
  unsigned int CurrentPeriod;
  unsigned int CurrentGroup;

private:
  svtkBlueObeliskDataParser(const svtkBlueObeliskDataParser&) = delete;
  void operator=(const svtkBlueObeliskDataParser&) = delete;

  //@{
  /**
   * Resize array if needed and set the entry at ind to val.
   */
  static void ResizeArrayIfNeeded(svtkAbstractArray* arr, svtkIdType ind);
  static void ResizeAndSetValue(svtkStdString* val, svtkStringArray* arr, svtkIdType ind);
  static void ResizeAndSetValue(float val, svtkFloatArray* arr, svtkIdType ind);
  static void ResizeAndSetValue(unsigned short val, svtkUnsignedShortArray* arr, svtkIdType ind);
  //@}

  //@{
  /**
   * Parse types from const char *
   */
  static int parseInt(const char*);
  static float parseFloat(const char*);
  static void parseFloat3(const char* str, float[3]);
  static unsigned short parseUnsignedShort(const char*);
  //@}

  //@{
  /**
   * Convert a string to lower case. This will modify the input string
   * and return the input pointer.
   */
  static svtkStdString* ToLower(svtkStdString*);
  //@}
};

#endif
