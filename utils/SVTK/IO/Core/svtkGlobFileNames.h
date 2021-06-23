/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGlobFileNames.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGlobFileNames
 * @brief   find files that match a wildcard pattern
 *
 * svtkGlobFileNames is a utility for finding files and directories
 * that match a given wildcard pattern.  Allowed wildcards are
 * *, ?, [...], [!...]. The "*" wildcard matches any substring,
 * the "?" matches any single character, the [...] matches any one of
 * the enclosed characters, e.g. [abc] will match one of a, b, or c,
 * while [0-9] will match any digit, and [!...] will match any single
 * character except for the ones within the brackets.  Special
 * treatment is given to "/" (or "\" on Windows) because these are
 * path separators.  These are never matched by a wildcard, they are
 * only matched with another file separator.
 * @warning
 * This function performs case-sensitive matches on UNIX and
 * case-insensitive matches on Windows.
 * @sa
 * svtkDirectory
 */

#ifndef svtkGlobFileNames_h
#define svtkGlobFileNames_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkObject.h"

class svtkStringArray;

class SVTKIOCORE_EXPORT svtkGlobFileNames : public svtkObject
{
public:
  //@{
  /**
   * Return the class name as a string.
   */
  svtkTypeMacro(svtkGlobFileNames, svtkObject);
  //@}

  /**
   * Create a new svtkGlobFileNames object.
   */
  static svtkGlobFileNames* New();

  /**
   * Print directory to stream.
   */
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Reset the glob by clearing the list of output filenames.
   */
  void Reset();

  //@{
  /**
   * Set the directory in which to perform the glob.  If this is
   * not set, then the current directory will be used.  Also, if
   * you use a glob pattern that contains absolute path (one that
   * starts with "/" or a drive letter) then that absolute path
   * will be used and Directory will be ignored.
   */
  svtkSetStringMacro(Directory);
  svtkGetStringMacro(Directory);
  //@}

  /**
   * Search for all files that match the given expression,
   * sort them, and add them to the output.  This method can
   * be called repeatedly to add files matching additional patterns.
   * Returns 1 if successful, otherwise returns zero.
   */
  int AddFileNames(const char* pattern);

  //@{
  /**
   * Recurse into subdirectories.
   */
  svtkSetMacro(Recurse, svtkTypeBool);
  svtkBooleanMacro(Recurse, svtkTypeBool);
  svtkGetMacro(Recurse, svtkTypeBool);
  //@}

  /**
   * Return the number of files found.
   */
  int GetNumberOfFileNames();

  /**
   * Return the file at the given index, the indexing is 0 based.
   */
  const char* GetNthFileName(int index);

  //@{
  /**
   * Get an array that contains all the file names.
   */
  svtkGetObjectMacro(FileNames, svtkStringArray);
  //@}

protected:
  //@{
  /**
   * Set the wildcard pattern.
   */
  svtkSetStringMacro(Pattern);
  svtkGetStringMacro(Pattern);
  //@}

  svtkGlobFileNames();
  ~svtkGlobFileNames() override;

private:
  char* Directory;           // Directory for search.
  char* Pattern;             // Wildcard pattern
  svtkTypeBool Recurse;       // Recurse into subdirectories
  svtkStringArray* FileNames; // SVTK array of files

private:
  svtkGlobFileNames(const svtkGlobFileNames&) = delete;
  void operator=(const svtkGlobFileNames&) = delete;
};

#endif
