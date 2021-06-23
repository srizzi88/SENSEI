/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStringToNumeric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkStringToNumeric
 * @brief   Converts string arrays to numeric arrays
 *
 *
 * svtkStringToNumeric is a filter for converting a string array
 * into a numeric arrays.
 */

#ifndef svtkStringToNumeric_h
#define svtkStringToNumeric_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class SVTKINFOVISCORE_EXPORT svtkStringToNumeric : public svtkDataObjectAlgorithm
{
public:
  static svtkStringToNumeric* New();
  svtkTypeMacro(svtkStringToNumeric, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Convert all numeric columns to svtkDoubleArray, even if they
   * contain only integer values. Default is off.
   */
  svtkSetMacro(ForceDouble, bool);
  svtkGetMacro(ForceDouble, bool);
  svtkBooleanMacro(ForceDouble, bool);
  //@}

  //@{
  /**
   * Set the default integer value assigned to arrays.  Default is 0.
   */
  svtkSetMacro(DefaultIntegerValue, int);
  svtkGetMacro(DefaultIntegerValue, int);
  //@}

  //@{
  /**
   * Set the default double value assigned to arrays.  Default is 0.0
   */
  svtkSetMacro(DefaultDoubleValue, double);
  svtkGetMacro(DefaultDoubleValue, double);
  //@}

  //@{
  /**
   * Whether to trim whitespace from strings prior to conversion to a numeric.
   * Default is false to preserve backward compatibility.

   * svtkVariant handles whitespace inconsistently, so trim it before we try to
   * convert it.  For example:

   * svtkVariant("  2.0").ToDouble() == 2.0 <-- leading whitespace is not a problem
   * svtkVariant("  2.0  ").ToDouble() == NaN <-- trailing whitespace is a problem
   * svtkVariant("  infinity  ").ToDouble() == NaN <-- any whitespace is a problem

   * In these cases, trimming the whitespace gives us the result we expect:
   * 2.0 and INF respectively.
   */
  svtkSetMacro(TrimWhitespacePriorToNumericConversion, bool);
  svtkGetMacro(TrimWhitespacePriorToNumericConversion, bool);
  svtkBooleanMacro(TrimWhitespacePriorToNumericConversion, bool);
  //@}

  //@{
  /**
   * Whether to detect and convert field data arrays.  Default is on.
   */
  svtkSetMacro(ConvertFieldData, bool);
  svtkGetMacro(ConvertFieldData, bool);
  svtkBooleanMacro(ConvertFieldData, bool);
  //@}

  //@{
  /**
   * Whether to detect and convert cell data arrays.  Default is on.
   */
  svtkSetMacro(ConvertPointData, bool);
  svtkGetMacro(ConvertPointData, bool);
  svtkBooleanMacro(ConvertPointData, bool);
  //@}

  //@{
  /**
   * Whether to detect and convert point data arrays.  Default is on.
   */
  svtkSetMacro(ConvertCellData, bool);
  svtkGetMacro(ConvertCellData, bool);
  svtkBooleanMacro(ConvertCellData, bool);
  //@}

  /**
   * Whether to detect and convert vertex data arrays.  Default is on.
   */
  virtual void SetConvertVertexData(bool b) { this->SetConvertPointData(b); }
  virtual bool GetConvertVertexData() { return this->GetConvertPointData(); }
  svtkBooleanMacro(ConvertVertexData, bool);

  /**
   * Whether to detect and convert edge data arrays.  Default is on.
   */
  virtual void SetConvertEdgeData(bool b) { this->SetConvertCellData(b); }
  virtual bool GetConvertEdgeData() { return this->GetConvertCellData(); }
  svtkBooleanMacro(ConvertEdgeData, bool);

  /**
   * Whether to detect and convert row data arrays.  Default is on.
   */
  virtual void SetConvertRowData(bool b) { this->SetConvertPointData(b); }
  virtual bool GetConvertRowData() { return this->GetConvertPointData(); }
  svtkBooleanMacro(ConvertRowData, bool);

  /**
   * This is required to capture REQUEST_DATA_OBJECT requests.
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkStringToNumeric();
  ~svtkStringToNumeric() override;

  /**
   * Creates the same output type as the input type.
   */
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Tries to convert string arrays to integer or double arrays.
   */
  void ConvertArrays(svtkFieldData* fieldData);

  bool ConvertFieldData;
  bool ConvertPointData;
  bool ConvertCellData;
  bool ForceDouble;
  int DefaultIntegerValue;
  double DefaultDoubleValue;
  bool TrimWhitespacePriorToNumericConversion;

  /**
   * Count the total number of items (array components) that will need
   * to be converted in the given svtkFieldData.  This lets us emit
   * ProgressEvent.
   */
  int CountItemsToConvert(svtkFieldData* fieldData);

  // These keep track of our progress
  int ItemsToConvert;
  int ItemsConverted;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkStringToNumeric(const svtkStringToNumeric&) = delete;
  void operator=(const svtkStringToNumeric&) = delete;
};

#endif
