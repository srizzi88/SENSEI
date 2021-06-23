/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSortFileNames.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSortFileNames
 * @brief   Group and sort a set of filenames
 *
 * svtkSortFileNames will take a list of filenames (e.g. from
 * a file load dialog) and sort them into one or more series.  If
 * the input list of filenames contains any directories, these can
 * be removed before sorting using the SkipDirectories flag.  This
 * class should be used where information about the series groupings
 * can be determined by the filenames, but it might not be successful
 * in cases where the information about the series groupings is
 * stored in the files themselves (e.g DICOM).
 * @sa
 * svtkImageReader2
 */

#ifndef svtkSortFileNames_h
#define svtkSortFileNames_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkObject.h"

class svtkStringArray;

// this is a helper class defined in the .cxx file
class svtkStringArrayVector;

class SVTKIOCORE_EXPORT svtkSortFileNames : public svtkObject
{
public:
  svtkTypeMacro(svtkSortFileNames, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkSortFileNames* New();

  //@{
  /**
   * Sort the file names into groups, according to similarity in
   * filename name and path.  Files in different directories,
   * or with different extensions, or which do not fit into the same
   * numbered series will be placed into different groups.  This is
   * off by default.
   */
  svtkSetMacro(Grouping, svtkTypeBool);
  svtkGetMacro(Grouping, svtkTypeBool);
  svtkBooleanMacro(Grouping, svtkTypeBool);
  //@}

  //@{
  /**
   * Sort the files numerically, rather than lexicographically.
   * For filenames that contain numbers, this means the order will be
   * ["file8.dat", "file9.dat", "file10.dat"]
   * instead of the usual alphabetic sorting order
   * ["file10.dat" "file8.dat", "file9.dat"].
   * NumericSort is off by default.
   */
  svtkSetMacro(NumericSort, svtkTypeBool);
  svtkGetMacro(NumericSort, svtkTypeBool);
  svtkBooleanMacro(NumericSort, svtkTypeBool);
  //@}

  //@{
  /**
   * Ignore case when sorting.  This flag is honored by both
   * the sorting and the grouping. This is off by default.
   */
  svtkSetMacro(IgnoreCase, svtkTypeBool);
  svtkGetMacro(IgnoreCase, svtkTypeBool);
  svtkBooleanMacro(IgnoreCase, svtkTypeBool);
  //@}

  //@{
  /**
   * Skip directories. If this flag is set, any input item that
   * is a directory rather than a file will not be included in
   * the output.  This is off by default.
   */
  svtkSetMacro(SkipDirectories, svtkTypeBool);
  svtkGetMacro(SkipDirectories, svtkTypeBool);
  svtkBooleanMacro(SkipDirectories, svtkTypeBool);
  //@}

  //@{
  /**
   * Set a list of file names to group and sort.
   */
  void SetInputFileNames(svtkStringArray* input);
  svtkGetObjectMacro(InputFileNames, svtkStringArray);
  //@}

  /**
   * Get the full list of sorted filenames.
   */
  virtual svtkStringArray* GetFileNames();

  /**
   * Get the number of groups that the names were split into, if
   * grouping is on.  The filenames are automatically split into
   * groups, where the filenames in each group will be identical
   * except for their series numbers.  If grouping is not on, this
   * method will return zero.
   */
  virtual int GetNumberOfGroups();

  /**
   * Get the Nth group of file names.  This method should only
   * be used if grouping is on.  If grouping is off, it will always
   * return null.
   */
  virtual svtkStringArray* GetNthGroup(int i);

  /**
   * Update the output filenames from the input filenames.
   * This method is called automatically by GetFileNames()
   * and GetNumberOfGroups() if the input names have changed.
   */
  virtual void Update();

protected:
  svtkSortFileNames();
  ~svtkSortFileNames() override;

  svtkTypeBool NumericSort;
  svtkTypeBool IgnoreCase;
  svtkTypeBool Grouping;
  svtkTypeBool SkipDirectories;

  svtkTimeStamp UpdateTime;

  svtkStringArray* InputFileNames;
  svtkStringArray* FileNames;
  svtkStringArrayVector* Groups;

  /**
   * Fill the output.
   */
  virtual void Execute();

  /**
   * Sort the input string array, and append the results to the output.
   */
  virtual void SortFileNames(svtkStringArray* input, svtkStringArray* output);

  /**
   * Separate a string array into groups and append them to the output.
   */
  virtual void GroupFileNames(svtkStringArray* input, svtkStringArrayVector* output);

private:
  svtkSortFileNames(const svtkSortFileNames&) = delete;
  void operator=(const svtkSortFileNames&) = delete;
};

#endif
