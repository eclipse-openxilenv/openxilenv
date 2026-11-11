from enum import IntEnum


class ErrorCode(IntEnum):
    NO_FREE_WRFLAG = -801
    NO_FREE_VID = -802
    UNKNOWN_DTYPE = -803
    UNKNOWN_VARIABLE = -804
    WRONG_PARAMETER = -805
    WRONG_INI_ENTRY = -806
    NO_INI_ENTRY = -807
    UNKNOWN_PROCESS = -808
    NOT_INITIALIZED = -809
    PROCESS_RUNNING = -810
    PROCESS_INIT_ERROR = -811
    NO_FREE_PID = -812
    PROCESS_TIMEOUT = -813
    INI_WRITE_ERROR = -814
    NO_FREE_MEMORY = -815
    NO_PROCESS_RUNNING = -816
    EXCEPTION_ERROR = -817
    INI_READ_ERROR = -818
    TYPE_CLASH = -819
    NO_FREE_ACCESS_MASK = -820
    CANNOT_EXEC_PROGRAM = -821
    LABEL_NAME_TOO_LONG = -822
    ADDRESS_NO_VALID_LABEL = -823
    LABEL_NAME_NOT_VALID = -824

class DataType(IntEnum):
    BB_BYTE = 0       # Resolution: 8-bit; value range: -128..127
    BB_UBYTE = 1      # Resolution: 8-bit; value range: 0..255
    BB_WORD = 2       # Resolution: 16-bit; value range: -32768..32767
    BB_UWORD = 3      # Resolution: 16-bit; value range: 0..65535
    BB_DWORD = 4      # Resolution: 32-bit; value range: -2147483648..2147483647
    BB_UDWORD = 5     # Resolution: 32-bit; value range: 0..4294967295
    BB_QWORD = 34     # Resolution: 64-bit; value range: -9223372036854775808..9223372036854775807
    BB_UQWORD = 35    # Resolution: 64-bit; value range: 0..18446744073709551615
    BB_FLOAT = 6      # Resolution: 32-bit floating point
    BB_DOUBLE = 7     # Resolution: 64-bit floating point

class ConversionType(IntEnum):
    BB_CONV_NONE = 0
    BB_CONV_FORMULA = 1
    BB_CONV_TEXTREP = 2
    BB_CONV_FACTOFF = 3
    BB_CONV_OFFFACT = 4
    BB_CONV_TAB_INTP = 5
    BB_CONV_TAB_NOINTP = 6
    BB_CONV_RAT_FUNC = 7
    #BB_CONV_REF = 8 --> Eric says to ignore it for now