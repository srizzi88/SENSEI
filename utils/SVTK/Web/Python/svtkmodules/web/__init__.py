import sys, re, hashlib, base64

py3 = sys.version_info >= (3,0)

arrayTypesMapping = [
  ' ', # SVTK_VOID            0
  ' ', # SVTK_BIT             1
  'b', # SVTK_CHAR            2
  'B', # SVTK_UNSIGNED_CHAR   3
  'h', # SVTK_SHORT           4
  'H', # SVTK_UNSIGNED_SHORT  5
  'i', # SVTK_INT             6
  'I', # SVTK_UNSIGNED_INT    7
  'l', # SVTK_LONG            8
  'L', # SVTK_UNSIGNED_LONG   9
  'f', # SVTK_FLOAT          10
  'd', # SVTK_DOUBLE         11
  'L', # SVTK_ID_TYPE        12
]

javascriptMapping = {
    'b': 'Int8Array',
    'B': 'Uint8Array',
    'h': 'Int16Array',
    'H': 'Int16Array',
    'i': 'Int32Array',
    'I': 'Uint32Array',
    'l': 'Int32Array',
    'L': 'Uint32Array',
    'f': 'Float32Array',
    'd': 'Float64Array'
}

if py3:
    def iteritems(d, **kwargs):
        return iter(d.items(**kwargs))
else:
    def iteritems(d, **kwargs):
        return d.iteritems(**kwargs)

if sys.version_info >= (2,7):
    buffer = memoryview
    base64Encode = lambda x: base64.b64encode(x).decode('utf-8')
else:
    buffer = buffer
    base64Encode = lambda x: x.encode('base64')

def hashDataArray(dataArray):
    hashedBit = base64Encode(hashlib.md5(buffer(dataArray)).digest()).strip()
    md5sum = re.sub('==$', '', hashedBit)
    typeCode = arrayTypesMapping[dataArray.GetDataType()]
    return '%s_%d%s' % (md5sum, dataArray.GetSize(), typeCode)

def getJSArrayType(dataArray):
    return javascriptMapping[arrayTypesMapping[dataArray.GetDataType()]]
