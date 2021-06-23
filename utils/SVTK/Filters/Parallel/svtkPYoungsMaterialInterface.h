/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPYoungsMaterialInterface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPYoungsMaterialInterface
 * @brief   parallel reconstruction of material interfaces
 *
 *
 * This is a subclass of svtkYoungsMaterialInterface, implementing the reconstruction
 * of material interfaces, for parallel data sets
 *
 * @par Thanks:
 * This file is part of the generalized Youngs material interface reconstruction algorithm
 * contributed by <br> CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
 * BP12, F-91297 Arpajon, France. <br>
 * Implementation by Thierry Carrard and Philippe Pebay
 *
 * @sa
 * svtkYoungsMaterialInterface
 */

#ifndef svtkPYoungsMaterialInterface_h
#define svtkPYoungsMaterialInterface_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkYoungsMaterialInterface.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkPYoungsMaterialInterface : public svtkYoungsMaterialInterface
{
public:
  static svtkPYoungsMaterialInterface* New();
  svtkTypeMacro(svtkPYoungsMaterialInterface, svtkYoungsMaterialInterface);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Parallel implementation of the material aggregation.
   */
  void Aggregate(int, int*) override;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPYoungsMaterialInterface();
  ~svtkPYoungsMaterialInterface() override;

  svtkMultiProcessController* Controller;

private:
  svtkPYoungsMaterialInterface(const svtkPYoungsMaterialInterface&) = delete;
  void operator=(const svtkPYoungsMaterialInterface&) = delete;
};

#endif /* SVTK_PYOUNGS_MATERIAL_INTERFACE_H */
