/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlockIdScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBlockIdScalars
 * @brief   generates scalars from blocks.
 *
 * svtkBlockIdScalars is a filter that generates scalars using the block index
 * for each block. Note that all sub-blocks within a block get the same scalar.
 * The new scalars array is named \c BlockIdScalars.
 */

#ifndef svtkBlockIdScalars_h
#define svtkBlockIdScalars_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkBlockIdScalars : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkBlockIdScalars* New();
  svtkTypeMacro(svtkBlockIdScalars, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkBlockIdScalars();
  ~svtkBlockIdScalars() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkDataObject* ColorBlock(svtkDataObject* input, int group);

private:
  svtkBlockIdScalars(const svtkBlockIdScalars&) = delete;
  void operator=(const svtkBlockIdScalars&) = delete;
};

#endif
