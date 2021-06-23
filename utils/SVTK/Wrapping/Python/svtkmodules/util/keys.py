"""
Utility module to make it easier to create new keys.
"""
from svtkmodules.svtkCommonCore import svtkInformationDataObjectKey as DataaObjectKey
from svtkmodules.svtkCommonCore import svtkInformationDoubleKey as DoubleKey
from svtkmodules.svtkCommonCore import svtkInformationDoubleVectorKey as DoubleVectorKey
from svtkmodules.svtkCommonCore import svtkInformationIdTypeKey as IdTypeKey
from svtkmodules.svtkCommonCore import svtkInformationInformationKey as InformationKey
from svtkmodules.svtkCommonCore import svtkInformationInformationVectorKey as InformationVectorKey
from svtkmodules.svtkCommonCore import svtkInformationIntegerKey as IntegerKey
from svtkmodules.svtkCommonCore import svtkInformationIntegerVectorKey as IntegerVectorKey
from svtkmodules.svtkCommonCore import svtkInformationKeyVectorKey as KeyVectorKey
from svtkmodules.svtkCommonCore import svtkInformationObjectBaseKey as ObjectBaseKey
from svtkmodules.svtkCommonCore import svtkInformationObjectBaseVectorKey as ObjectBaseVectorKey
from svtkmodules.svtkCommonCore import svtkInformationRequestKey as RequestKey
from svtkmodules.svtkCommonCore import svtkInformationStringKey as StringKey
from svtkmodules.svtkCommonCore import svtkInformationStringVectorKey as StringVectorKey
from svtkmodules.svtkCommonCore import svtkInformationUnsignedLongKey as UnsignedLongKey
from svtkmodules.svtkCommonCore import svtkInformationVariantKey as VariantKey
from svtkmodules.svtkCommonCore import svtkInformationVariantVectorKey as VariantVectorKey
from svtkmodules.svtkCommonExecutionModel import svtkInformationDataObjectMetaDataKey as DataObjectMetaDataKey
from svtkmodules.svtkCommonExecutionModel import svtkInformationExecutivePortKey as ExecutivePortKey
from svtkmodules.svtkCommonExecutionModel import svtkInformationExecutivePortVectorKey as ExecutivePortVectorKey
from svtkmodules.svtkCommonExecutionModel import svtkInformationIntegerRequestKey as IntegerRequestKey

def MakeKey(key_type, name, location, *args):
    """Given a key type, make a new key of given name
    and location."""
    return key_type.MakeKey(name, location, *args)
