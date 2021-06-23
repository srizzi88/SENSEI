//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ErrorBadAllocation.h>
#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/ErrorFilterExecution.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>

namespace svtkm
{
namespace cont
{
namespace detail
{

SVTKM_CONT_EXPORT void HandleTryExecuteException(svtkm::cont::DeviceAdapterId deviceId,
                                                svtkm::cont::RuntimeDeviceTracker& tracker,
                                                const std::string& functorName)
{
  try
  {
    //re-throw the last exception
    throw;
  }
  catch (svtkm::cont::ErrorBadAllocation& e)
  {
    SVTKM_LOG_TRYEXECUTE_DISABLE("Bad allocation (" << e.GetMessage() << ")", functorName, deviceId);
    //currently we only consider OOM errors worth disabling a device for
    //than we fallback to another device
    tracker.ReportAllocationFailure(deviceId, e);
  }
  catch (svtkm::cont::ErrorBadDevice& e)
  {
    SVTKM_LOG_TRYEXECUTE_DISABLE("Bad device (" << e.GetMessage() << ")", functorName, deviceId);
    tracker.ReportBadDeviceFailure(deviceId, e);
  }
  catch (svtkm::cont::ErrorBadType& e)
  {
    //should bad type errors should stop the execution, instead of
    //deferring to another device adapter?
    SVTKM_LOG_TRYEXECUTE_FAIL("ErrorBadType (" << e.GetMessage() << ")", functorName, deviceId);
  }
  catch (svtkm::cont::ErrorBadValue& e)
  {
    // Should bad values be deferred to another device? Seems unlikely they will succeed.
    // Re-throw instead.
    SVTKM_LOG_TRYEXECUTE_FAIL("ErrorBadValue (" << e.GetMessage() << ")", functorName, deviceId);
    throw;
  }
  catch (svtkm::cont::Error& e)
  {
    SVTKM_LOG_TRYEXECUTE_FAIL(e.GetMessage(), functorName, deviceId);
    if (e.GetIsDeviceIndependent())
    {
      // re-throw the exception as it's a device-independent exception.
      throw;
    }
  }
  catch (std::exception& e)
  {
    SVTKM_LOG_TRYEXECUTE_FAIL(e.what(), functorName, deviceId);
  }
  catch (...)
  {
    SVTKM_LOG_TRYEXECUTE_FAIL("Unknown exception", functorName, deviceId);
    std::cerr << "unknown exception caught" << std::endl;
  }
}
}
}
}
