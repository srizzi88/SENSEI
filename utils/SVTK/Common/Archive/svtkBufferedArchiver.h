/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBufferedArchiver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBufferedArchiver
 * @brief   Writes an archive to a buffer for svtk-js datasets
 *
 * svtksvtkJSBufferedArchiver is a specialized archiver for writing datasets into
 * a memory buffer with zip compression.
 *
 * @sa
 * svtkArchiver
 */

#ifndef svtkBufferedArchiver_h
#define svtkBufferedArchiver_h

#include "svtkCommonArchiveModule.h" // For export macro

#include "svtkArchiver.h"

class SVTKCOMMONARCHIVE_EXPORT svtkBufferedArchiver : public svtkArchiver
{
public:
  static svtkBufferedArchiver* New();
  svtkTypeMacro(svtkBufferedArchiver, svtkArchiver);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Open the arhive for writing.
   */
  void OpenArchive() override;
  //@}

  //@{
  /**
   * Close the arhive.
   */
  void CloseArchive() override;
  //@}

  //@{
  /**
   * Insert \p data of size \p size into the archive at \p relativePath.
   */
  void InsertIntoArchive(
    const std::string& relativePath, const char* data, std::size_t size) override;
  //@}

  //@{
  /**
   * Checks if \p relativePath represents an entry in the archive.
   */
  bool Contains(const std::string& relativePath) override;
  //@}

  //@{
  /**
   * Access the buffer.
   */
  const char* GetBuffer();
  //@}

  //@{
  /**
   * Access the address of the buffer.
   */
  const void* GetBufferAddress();
  //@}

  //@{
  /**
   * Set/Get the allocated buffer size.
   */
  void SetAllocatedSize(std::size_t);
  std::size_t GetAllocatedSize();
  //@}

  //@{
  /**
   * Get the buffer used size.
   */
  std::size_t GetBufferSize();
  //@}

protected:
  svtkBufferedArchiver();
  ~svtkBufferedArchiver() override;

  struct Internal;
  Internal* Internals;

private:
  svtkBufferedArchiver(const svtkBufferedArchiver&) = delete;
  void operator=(const svtkBufferedArchiver&) = delete;
};

#endif
