/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalDelimitedTextReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTemporalDelimitedTextReader
 * @brief   reads a delimited ascii or unicode text files and and output a
 * temporal svtkTable.
 *
 * This reader requires that FieldDelimiterCharacters is set before
 * the pipeline is executed, otherwise it will produce an empty output.
 *
 * A column can be selected as time step indicator using the SetTimeColumnName
 * or SetTimeColumnId functions. If so, for a given time step 's' only the
 * lines where the time step indicator column have the value 's' are present.
 * To control if the time step indicator column should be present in the
 * output, a RemoveTimeStepColumn option is available. If no time step
 * indicator column is given by the user, the whole file it outputted.
 *
 * This reader assume the time step column is numeric. A warning is
 * set otherwise. The DetectNumericColumns field is set to on,
 * do not change this field unless you really know what you are
 * doing.
 *
 * @see svtkDelimitedTextReader
 */

#ifndef svtkTemporalDelimitedTextReader_h
#define svtkTemporalDelimitedTextReader_h

#include "svtkDelimitedTextReader.h"

#include "svtkIOInfovisModule.h" // module export
#include "svtkNew.h"             // For ReadTable field

#include <map>    // To store the TimeMap
#include <vector> // To store the TimeMap

class SVTKIOINFOVIS_EXPORT svtkTemporalDelimitedTextReader : public svtkDelimitedTextReader
{
public:
  static svtkTemporalDelimitedTextReader* New();
  svtkTypeMacro(svtkTemporalDelimitedTextReader, svtkDelimitedTextReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the name of the column to use as time indicator.
   * Ignored if TimeColumnId is not equal to -1.
   * If no column has been chosen using either the TimeColumnId or the
   * TimeColumnName the whole input file is outputted.
   * Default to empty string.
   */
  svtkGetMacro(TimeColumnName, std::string);
  void SetTimeColumnName(const std::string name);
  //@}

  //@{
  /**
   * Get/Set the column to use as time indicator.
   * It the TimeColumnId is equal to -1, the TimeColumnName will be used
   * instead.
   * If no column has been chosen using either the TimeColumnId or the
   * TimeColumnName the whole input file is outputted.
   * Default to -1.
   */
  svtkGetMacro(TimeColumnId, int);
  void SetTimeColumnId(const int idx);
  //@}

  //@{
  /**
   * Set the RemoveTimeStepColumn flag
   * If this boolean is true, the output will not contain the Time step column.
   * Default to true.
   */
  svtkGetMacro(RemoveTimeStepColumn, bool);
  void SetRemoveTimeStepColumn(bool rts);
  //@}

  /** Internal fields of this reader use a specific MTime (InternalMTime).
   * This mechamism ensure the actual data is only re-read when necessary.
   * Here, we ensure the GetMTime of this reader stay consistent by returning
   * the latest between the MTime of this reader and the internal one.
   *
   * @see InternalModified
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkTemporalDelimitedTextReader();
  ~svtkTemporalDelimitedTextReader() override = default;

  /**
   * In order to fill the TIME_STEPS and TIME_RANGE keys, this method call the
   * ReadData function that actually read the full input file content (may be
   * slow!). Custom MTime management is used to ensure we do not re-read the
   * input file uselessly.
   */
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * This function checks if a user specified column has been set and check if
   * this input is valid. If an invalid input has been detected, return false.
   * Otherwise, InternalColumnName will be set to the name of the time column
   * or empty if none has been given by the user.
   */
  bool EnforceColumnName();

  /**
   * When parameters specific of this reader are modified, we do not want to
   * re-read the input file. Keep an internal time stamp to track them.
   */
  void InternalModified();

  // Time column fields
  std::string TimeColumnName = "";
  std::string InternalColumnName = "";
  svtkIdType TimeColumnId = -1;
  bool RemoveTimeStepColumn = true;
  std::map<double, std::vector<svtkIdType> > TimeMap;

  // Input file content and update
  svtkNew<svtkTable> ReadTable;
  svtkMTimeType LastReadTime = 0;
  svtkTimeStamp InternalMTime;

private:
  svtkTemporalDelimitedTextReader(const svtkTemporalDelimitedTextReader&) = delete;
  void operator=(const svtkTemporalDelimitedTextReader&) = delete;
};

#endif
