"""
Utility functions to mimic the template support functions for svtkVariant
"""

from svtkmodules import svtkCommonCore
import sys

_variant_type_map = {
    'void' : svtkCommonCore.SVTK_VOID,
    'char' : svtkCommonCore.SVTK_CHAR,
    'unsigned char' : svtkCommonCore.SVTK_UNSIGNED_CHAR,
    'signed char' : svtkCommonCore.SVTK_SIGNED_CHAR,
    'short' : svtkCommonCore.SVTK_SHORT,
    'unsigned short' : svtkCommonCore.SVTK_UNSIGNED_SHORT,
    'int' : svtkCommonCore.SVTK_INT,
    'unsigned int' : svtkCommonCore.SVTK_UNSIGNED_INT,
    'long' : svtkCommonCore.SVTK_LONG,
    'unsigned long' : svtkCommonCore.SVTK_UNSIGNED_LONG,
    'long long' : svtkCommonCore.SVTK_LONG_LONG,
    'unsigned long long' : svtkCommonCore.SVTK_UNSIGNED_LONG_LONG,
    'float' : svtkCommonCore.SVTK_FLOAT,
    'double' : svtkCommonCore.SVTK_DOUBLE,
    'string' : svtkCommonCore.SVTK_STRING,
    'unicode string' : svtkCommonCore.SVTK_UNICODE_STRING,
    'svtkObjectBase' : svtkCommonCore.SVTK_OBJECT,
    'svtkObject' : svtkCommonCore.SVTK_OBJECT,
}

_variant_method_map = {
    svtkCommonCore.SVTK_VOID : '',
    svtkCommonCore.SVTK_CHAR : 'ToChar',
    svtkCommonCore.SVTK_UNSIGNED_CHAR : 'ToUnsignedChar',
    svtkCommonCore.SVTK_SIGNED_CHAR : 'ToSignedChar',
    svtkCommonCore.SVTK_SHORT : 'ToShort',
    svtkCommonCore.SVTK_UNSIGNED_SHORT : 'ToUnsignedShort',
    svtkCommonCore.SVTK_INT : 'ToInt',
    svtkCommonCore.SVTK_UNSIGNED_INT : 'ToUnsignedInt',
    svtkCommonCore.SVTK_LONG : 'ToLong',
    svtkCommonCore.SVTK_UNSIGNED_LONG : 'ToUnsignedLong',
    svtkCommonCore.SVTK_LONG_LONG : 'ToLongLong',
    svtkCommonCore.SVTK_UNSIGNED_LONG_LONG : 'ToUnsignedLongLong',
    svtkCommonCore.SVTK_FLOAT : 'ToFloat',
    svtkCommonCore.SVTK_DOUBLE : 'ToDouble',
    svtkCommonCore.SVTK_STRING : 'ToString',
    svtkCommonCore.SVTK_UNICODE_STRING : 'ToUnicodeString',
    svtkCommonCore.SVTK_OBJECT : 'ToSVTKObject',
}

_variant_check_map = {
    svtkCommonCore.SVTK_VOID : 'IsValid',
    svtkCommonCore.SVTK_CHAR : 'IsChar',
    svtkCommonCore.SVTK_UNSIGNED_CHAR : 'IsUnsignedChar',
    svtkCommonCore.SVTK_SIGNED_CHAR : 'IsSignedChar',
    svtkCommonCore.SVTK_SHORT : 'IsShort',
    svtkCommonCore.SVTK_UNSIGNED_SHORT : 'IsUnsignedShort',
    svtkCommonCore.SVTK_INT : 'IsInt',
    svtkCommonCore.SVTK_UNSIGNED_INT : 'IsUnsignedInt',
    svtkCommonCore.SVTK_LONG : 'IsLong',
    svtkCommonCore.SVTK_UNSIGNED_LONG : 'IsUnsignedLong',
    svtkCommonCore.SVTK_LONG_LONG : 'IsLongLong',
    svtkCommonCore.SVTK_UNSIGNED_LONG_LONG : 'IsUnsignedLongLong',
    svtkCommonCore.SVTK_FLOAT : 'IsFloat',
    svtkCommonCore.SVTK_DOUBLE : 'IsDouble',
    svtkCommonCore.SVTK_STRING : 'IsString',
    svtkCommonCore.SVTK_UNICODE_STRING : 'IsUnicodeString',
    svtkCommonCore.SVTK_OBJECT : 'IsSVTKObject',
}


def svtkVariantCreate(v, t):
    """
    Create a svtkVariant of the specified type, where the type is in the
    following format: 'int', 'unsigned int', etc. for numeric types,
    and 'string' or 'unicode string' for strings.  You can also use an
    integer SVTK type constant for the type.
    """
    if not issubclass(type(t), int):
        t = _variant_type_map[t]

    return svtkCommonCore.svtkVariant(v, t)


def svtkVariantExtract(v, t=None):
    """
    Extract the specified value type from the svtkVariant, where the type is
    in the following format: 'int', 'unsigned int', etc. for numeric types,
    and 'string' or 'unicode string' for strings.  You can also use an
    integer SVTK type constant for the type.  Set the type to 'None" to
    extract the value in its native type.
    """
    v = svtkCommonCore.svtkVariant(v)

    if t == None:
        t = v.GetType()
    elif not issubclass(type(t), int):
        t = _variant_type_map[t]

    if getattr(v, _variant_check_map[t])():
        return getattr(v, _variant_method_map[t])()
    else:
        return None


def svtkVariantCast(v, t):
    """
    Cast the svtkVariant to the specified value type, where the type is
    in the following format: 'int', 'unsigned int', etc. for numeric types,
    and 'string' or 'unicode string' for strings.  You can also use an
    integer SVTK type constant for the type.
    """
    if not issubclass(type(t), int):
        t = _variant_type_map[t]

    v = svtkCommonCore.svtkVariant(v, t)

    if v.IsValid():
        return getattr(v, _variant_method_map[t])()
    else:
        return None


def svtkVariantStrictWeakOrder(s1, s2):
    """
    Compare variants by type first, and then by value.  When called from
    within a Python 2 interpreter, the return values are -1, 0, 1 like the
    cmp() method, for compatibility with the Python 2 list sort() method.
    This is in contrast with the Python 3 version of this method (and the
    SVTK C++ version), which return true or false.
    """
    s1 = svtkCommonCore.svtkVariant(s1)
    s2 = svtkCommonCore.svtkVariant(s2)

    t1 = s1.GetType()
    t2 = s2.GetType()

    # define a cmp(x, y) for Python 3 that returns (x < y)
    def vcmp(x, y):
        if sys.hexversion >= 0x03000000:
            return (x < y)
        else:
            return cmp(x,y)

    # check based on type
    if t1 != t2:
        return vcmp(t1,t2)

    v1 = s1.IsValid()
    v2 = s2.IsValid()

    # check based on validity
    if (not v1) or (not v2):
        return vcmp(v1,v2)

    # extract and compare the values
    r1 = getattr(s1, _variant_method_map[t1])()
    r2 = getattr(s2, _variant_method_map[t2])()

    # compare svtk objects by classname, then address
    if t1 == svtkCommonCore.SVTK_OBJECT:
        c1 = r1.GetClassName()
        c2 = r2.GetClassName()
        if c1 != c2:
            return vcmp(c1,c2)
        else:
            return vcmp(r1.__this__,r2.__this__)

    return vcmp(r1, r2)


if sys.hexversion >= 0x03000000:
    class svtkVariantStrictWeakOrderKey:
        """A key method (class, actually) for use with sort()"""
        def __init__(self, obj, *args):
            self.obj = obj
        def __lt__(self, other):
            return svtkVariantStrictWeakOrder(self.obj, other)
else:
    class svtkVariantStrictWeakOrderKey:
        """A key method (class, actually) for use with sort()"""
        def __init__(self, obj, *args):
            self.obj = obj
        def __lt__(self, other):
            return svtkVariantStrictWeakOrder(self.obj, other) < 0
        def __gt__(self, other):
            return svtkVariantStrictWeakOrder(self.obj, other) > 0
        def __eq__(self, other):
            return svtkVariantStrictWeakOrder(self.obj, other) == 0
        def __le__(self, other):
            return svtkVariantStrictWeakOrder(self.obj, other) <= 0
        def __ge__(self, other):
            return svtkVariantStrictWeakOrder(self.obj, other) >= 0
        def __ne__(self, other):
            return svtkVariantStrictWeakOrder(self.obj, other) != 0


def svtkVariantStrictEquality(s1, s2):
    """
    Check two variants for strict equality of type and value.
    """
    s1 = svtkCommonCore.svtkVariant(s1)
    s2 = svtkCommonCore.svtkVariant(s2)

    t1 = s1.GetType()
    t2 = s2.GetType()

    # check based on type
    if t1 != t2:
        return False

    v1 = s1.IsValid()
    v2 = s2.IsValid()

    # check based on validity
    if (not v1) and (not v2):
        return True
    elif v1 != v2:
        return False

    # extract and compare the values
    r1 = getattr(s1, _variant_method_map[t1])()
    r2 = getattr(s2, _variant_method_map[t2])()

    return (r1 == r2)


def svtkVariantLessThan(s1, s2):
    """
    Return true if s1 < s2.
    """
    return (svtkCommonCore.svtkVariant(s1) < svtkCommonCore.svtkVariant(s2))


def svtkVariantEqual(s1, s2):
    """
    Return true if s1 == s2.
    """
    return (svtkCommonCore.svtkVariant(s1) == svtkCommonCore.svtkVariant(s2))
