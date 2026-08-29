#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"

#if defined(_WIN32) || defined(_WIN64)
#define MICROBIT_SPIRV_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define MICROBIT_SPIRV_PLATFORM_LINUX 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#define MICROBIT_SPIRV_PLATFORM_MACOS 1
#endif
#elif defined(__ANDROID__)
#define MICROBIT_SPIRV_PLATFORM_ANDROID 1
#endif

#if defined(R_DEVMODE)
#define MICROBIT_SPIRV_DEBUG
#endif

#if defined(MICROBIT_SPIRV_DEBUG)
#include <assert.h>
#define MICROBIT_SPIRV_ASSERT(condition) assert (condition)
#else
#define MICROBIT_SPIRV_ASSERT(condition) ((void)0)
#endif

#if defined(_WIN32)
#ifdef MICROBIT_SPIRV_BUILDING_DLL
#define MICROBIT_SPIRV_API __declspec (dllexport)
#else
#define MICROBIT_SPIRV_API __declspec (dllimport)
#endif
#else
#define MICROBIT_SPIRV_API
#endif

enum R_Microbit_SpirvParserError
{
    MICROBIT_SPIRV_OK = 0,
    MICROBIT_SPIRV_ERROR_FAILED = -1,
    MICROBIT_SPIRV_ERROR_OUT_OF_MEMORY = -2,
    MICROBIT_SPIRV_ERROR_INVALID_ARGUMENT = -3,
    MICROBIT_SPIRV_ERROR_NULL_POINTER = -4,
    MICROBIT_SPIRV_ERROR_INVALID_MAGIC = -5,
    MICROBIT_SPIRV_ERROR_INVALID_VERSION = -6,
    MICROBIT_SPIRV_ERROR_INVALID_DATA = -7,
    MICROBIT_SPIRV_ERROR_UNSUPPORTED_OPCODE = -8,
    MICROBIT_SPIRV_ERROR_UNKNOWN = -99
};

MICROBIT_SPIRV_API const char* R_Microbit_SpirvParser_ErrorToString (enum R_Microbit_SpirvParserError error);

#define MICROBIT_SPIRV_MAGIC_NUMBER     0x07230203
#define MICROBIT_SPIRV_VERSION_1_5      0x00010500
#define MICROBIT_SPIRV_OPCODE_MASK      0xFFFF
#define MICROBIT_SPIRV_WORD_COUNT_SHIFT 16

enum R_Microbit_SpirvParserOpcode
{
    MICROBIT_SPIRV_OP_NOP = 0,
    MICROBIT_SPIRV_OP_NAME = 5,
    MICROBIT_SPIRV_OP_MEMBER_NAME = 6,
    MICROBIT_SPIRV_OP_STRING = 7,
    MICROBIT_SPIRV_OP_DECORATE = 71,
    MICROBIT_SPIRV_OP_MEMBER_DECORATE = 72,
    MICROBIT_SPIRV_OP_TYPE_VOID = 19,
    MICROBIT_SPIRV_OP_TYPE_BOOL = 20,
    MICROBIT_SPIRV_OP_TYPE_INT = 21,
    MICROBIT_SPIRV_OP_TYPE_FLOAT = 22,
    MICROBIT_SPIRV_OP_TYPE_VECTOR = 23,
    MICROBIT_SPIRV_OP_TYPE_MATRIX = 24,
    MICROBIT_SPIRV_OP_TYPE_IMAGE = 25,
    MICROBIT_SPIRV_OP_TYPE_SAMPLER = 26,
    MICROBIT_SPIRV_OP_TYPE_SAMPLED_IMAGE = 27,
    MICROBIT_SPIRV_OP_TYPE_ARRAY = 28,
    MICROBIT_SPIRV_OP_TYPE_RUNTIME_ARRAY = 29,
    MICROBIT_SPIRV_OP_TYPE_STRUCT = 30,
    MICROBIT_SPIRV_OP_TYPE_POINTER = 32,
    MICROBIT_SPIRV_OP_TYPE_FUNCTION = 33,
    MICROBIT_SPIRV_OP_CONSTANT = 43,
    MICROBIT_SPIRV_OP_CONSTANT_COMPOSITE = 44,
    MICROBIT_SPIRV_OP_VARIABLE = 59,
    MICROBIT_SPIRV_OP_FUNCTION = 54,
    MICROBIT_SPIRV_OP_FUNCTION_PARAMETER = 55,
    MICROBIT_SPIRV_OP_FUNCTION_END = 56,
    MICROBIT_SPIRV_OP_LABEL = 57,
    MICROBIT_SPIRV_OP_ACCESS_CHAIN = 65,
    MICROBIT_SPIRV_OP_ENTRY_POINT = 58,
    MICROBIT_SPIRV_OP_EXECUTION_MODE = 52,
    MICROBIT_SPIRV_OP_CAPABILITY = 17,
    MICROBIT_SPIRV_OP_EXTENSION = 10,
    MICROBIT_SPIRV_OP_EXT_INST_IMPORT = 11,
    MICROBIT_SPIRV_OP_MEMORY_MODEL = 14,
    MICROBIT_SPIRV_OP_DECORATION_BLOCK = 2,
    MICROBIT_SPIRV_OP_DECORATION_ROW_MAJOR = 4,
    MICROBIT_SPIRV_OP_DECORATION_COLUMN_MAJOR = 5,
    MICROBIT_SPIRV_OP_DECORATION_BUILTIN = 11,
    MICROBIT_SPIRV_OP_DECORATION_LOCATION = 30,
    MICROBIT_SPIRV_OP_DECORATION_BINDING = 33,
    MICROBIT_SPIRV_OP_DECORATION_DESCRIPTOR_SET = 34,
    MICROBIT_SPIRV_OP_DECORATION_OFFSET = 35,
    MICROBIT_SPIRV_OP_DECORATION_ARRAY_STRIDE = 36,
    MICROBIT_SPIRV_OP_DECORATION_MATRIX_STRIDE = 38,
    MICROBIT_SPIRV_OP_DECORATION_STD140 = 64,
    MICROBIT_SPIRV_OP_DECORATION_STD430 = 65,
    MICROBIT_SPIRV_OP_DECORATION_RELAXED_PRECISION = 44,
    MICROBIT_SPIRV_OP_RETURN = 9,
    MICROBIT_SPIRV_OP_RETURN_VALUE = 10,
    MICROBIT_SPIRV_OP_FUNCTION_CALL = 59,
    MICROBIT_SPIRV_OP_LOAD = 61,
    MICROBIT_SPIRV_OP_STORE = 62,
    MICROBIT_SPIRV_OP_COMPOSITE_EXTRACT = 79,
    MICROBIT_SPIRV_OP_COMPOSITE_INSERT = 80,
    MICROBIT_SPIRV_OP_COMPOSITE_CONSTRUCT = 80,
    MICROBIT_SPIRV_OP_VECTOR_EXTRACT_DYNAMIC = 77,
    MICROBIT_SPIRV_OP_VECTOR_INSERT_DYNAMIC = 78,
    MICROBIT_SPIRV_OP_COPY_MEMORY = 182,
    MICROBIT_SPIRV_OP_COPY_OBJECT = 182,
    MICROBIT_SPIRV_OP_CONVERT_F_TO_U = 113,
    MICROBIT_SPIRV_OP_CONVERT_F_TO_S = 114,
    MICROBIT_SPIRV_OP_CONVERT_S_TO_F = 115,
    MICROBIT_SPIRV_OP_CONVERT_U_TO_F = 116,
    MICROBIT_SPIRV_OP_U_CONVERT = 119,
    MICROBIT_SPIRV_OP_S_CONVERT = 120,
    MICROBIT_SPIRV_OP_F_CONVERT = 121,
    MICROBIT_SPIRV_OP_BITCAST = 50,
    MICROBIT_SPIRV_OP_SNORM_TO_U = 50,
    MICROBIT_SPIRV_OP_UNORM_TO_F = 50,
    MICROBIT_SPIRV_OP_FNegate = 127,
    MICROBIT_SPIRV_OP_FAdd = 124,
    MICROBIT_SPIRV_OP_FSub = 125,
    MICROBIT_SPIRV_OP_FMul = 126,
    MICROBIT_SPIRV_OP_FDiv = 128,
    MICROBIT_SPIRV_OP_FMod = 133,
    MICROBIT_SPIRV_OP_FRem = 129,
    MICROBIT_SPIRV_OP_IN_BOUNDS_ACCESS_CHAIN = 66,
    MICROBIT_SPIRV_OP_PTR_ACCESS_CHAIN = 67,
    MICROBIT_SPIRV_OP_IN_BOUNDS_PTR_ACCESS_CHAIN = 68,
    MICROBIT_SPIRV_OP_IAdd = 193,
    MICROBIT_SPIRV_OP_ISub = 194,
    MICROBIT_SPIRV_OP_IMul = 195,
    MICROBIT_SPIRV_OP_UDiv = 196,
    MICROBIT_SPIRV_OP_SDiv = 197,
    MICROBIT_SPIRV_OP_UMod = 198,
    MICROBIT_SPIRV_OP_SRem = 199,
    MICROBIT_SPIRV_OP_SMod = 200,
    MICROBIT_SPIRV_OP_FORD_EQUAL = 130,
    MICROBIT_SPIRV_OP_FORD_NOT_EQUAL = 131,
    MICROBIT_SPIRV_OP_FORD_LESS_THAN = 132,
    MICROBIT_SPIRV_OP_FORD_GREATER_THAN = 133,
    MICROBIT_SPIRV_OP_FORD_LESS_THAN_EQUAL = 134,
    MICROBIT_SPIRV_OP_FORD_GREATER_THAN_EQUAL = 135,
    MICROBIT_SPIRV_OP_FUNORDERED_EQUAL = 136,
    MICROBIT_SPIRV_OP_FUNORDERED_NOT_EQUAL = 137,
    MICROBIT_SPIRV_OP_FUNORDERED_LESS_THAN = 138,
    MICROBIT_SPIRV_OP_FUNORDERED_GREATER_THAN = 139,
    MICROBIT_SPIRV_OP_FUNORDERED_LESS_THAN_EQUAL = 140,
    MICROBIT_SPIRV_OP_FUNORDERED_GREATER_THAN_EQUAL = 141,
    MICROBIT_SPIRV_OP_LOGICAL_EQUAL = 171,
    MICROBIT_SPIRV_OP_LOGICAL_NOT_EQUAL = 172,
    MICROBIT_SPIRV_OP_LOGICAL_OR = 175,
    MICROBIT_SPIRV_OP_LOGICAL_AND = 174,
    MICROBIT_SPIRV_OP_LOGICAL_NOT = 173,
    MICROBIT_SPIRV_OP_SELECT = 162,
    MICROBIT_SPIRV_OP_I_EQUAL = 163,
    MICROBIT_SPIRV_OP_I_NOT_EQUAL = 164,
    MICROBIT_SPIRV_OP_U_LESS_THAN = 165,
    MICROBIT_SPIRV_OP_S_LESS_THAN = 166,
    MICROBIT_SPIRV_OP_U_GREATER_THAN = 167,
    MICROBIT_SPIRV_OP_S_GREATER_THAN = 168,
    MICROBIT_SPIRV_OP_U_LESS_THAN_EQUAL = 169,
    MICROBIT_SPIRV_OP_S_LESS_THAN_EQUAL = 170,
    MICROBIT_SPIRV_OP_U_GREATER_THAN_EQUAL = 171,
    MICROBIT_SPIRV_OP_S_GREATER_THAN_EQUAL = 172,
    MICROBIT_SPIRV_OP_SHIFT_LEFT_LOGICAL = 173,
    MICROBIT_SPIRV_OP_SHIFT_RIGHT_LOGICAL = 174,
    MICROBIT_SPIRV_OP_SHIFT_RIGHT_ARITHMETIC = 175,
    MICROBIT_SPIRV_OP_BITWISE_OR = 176,
    MICROBIT_SPIRV_OP_BITWISE_XOR = 177,
    MICROBIT_SPIRV_OP_BITWISE_AND = 178,
    MICROBIT_SPIRV_OP_NOT = 179,
    MICROBIT_SPIRV_OP_BIT_FIELD_INSERT = 180,
    MICROBIT_SPIRV_OP_BIT_FIELD_S_EXTRACT = 181,
    MICROBIT_SPIRV_OP_BIT_FIELD_U_EXTRACT = 182,
    MICROBIT_SPIRV_OP_BIT_REVERSE = 183,
    MICROBIT_SPIRV_OP_BIT_COUNT = 184,
    MICROBIT_SPIRV_OP_DPDX = 185,
    MICROBIT_SPIRV_OP_DPDX_COARSE = 186,
    MICROBIT_SPIRV_OP_DPDX_FINE = 187,
    MICROBIT_SPIRV_OP_DPDY = 188,
    MICROBIT_SPIRV_OP_DPDY_COARSE = 189,
    MICROBIT_SPIRV_OP_DPDY_FINE = 190,
    MICROBIT_SPIRV_OP_FWIDTH = 191,
    MICROBIT_SPIRV_OP_FWIDTH_COARSE = 192,
    MICROBIT_SPIRV_OP_FWIDTH_FINE = 193,
    MICROBIT_SPIRV_OP_SIN = 194,
    MICROBIT_SPIRV_OP_COS = 195,
    MICROBIT_SPIRV_OP_TAN = 196,
    MICROBIT_SPIRV_OP_ASIN = 197,
    MICROBIT_SPIRV_OP_ACOS = 198,
    MICROBIT_SPIRV_OP_ATAN = 199,
    MICROBIT_SPIRV_OP_ATAN2 = 200,
    MICROBIT_SPIRV_OP_SINH = 201,
    MICROBIT_SPIRV_OP_COSH = 202,
    MICROBIT_SPIRV_OP_TANH = 203,
    MICROBIT_SPIRV_OP_ASINH = 204,
    MICROBIT_SPIRV_OP_ACOSH = 205,
    MICROBIT_SPIRV_OP_ATANH = 206,
    MICROBIT_SPIRV_OP_SQRT = 207,
    MICROBIT_SPIRV_OP_INVERSE_SQRT = 208,
    MICROBIT_SPIRV_OP_EXP = 209,
    MICROBIT_SPIRV_OP_LOG = 210,
    MICROBIT_SPIRV_OP_EXP2 = 211,
    MICROBIT_SPIRV_OP_LOG2 = 212,
    MICROBIT_SPIRV_OP_POW = 213,
    MICROBIT_SPIRV_OP_FABS = 214,
    MICROBIT_SPIRV_OP_SIGN = 215,
    MICROBIT_SPIRV_OP_FLOOR = 216,
    MICROBIT_SPIRV_OP_CEIL = 217,
    MICROBIT_SPIRV_OP_FRACT = 218,
    MICROBIT_SPIRV_OP_RADIANS = 219,
    MICROBIT_SPIRV_OP_DEGREES = 220,
    MICROBIT_SPIRV_OP_FMIN = 221,
    MICROBIT_SPIRV_OP_FMAX = 222,
    MICROBIT_SPIRV_OP_FCLAMP = 223,
    MICROBIT_SPIRV_OP_FMA = 224,
    MICROBIT_SPIRV_OP_FMIX = 225,
    MICROBIT_SPIRV_OP_STEP = 226,
    MICROBIT_SPIRV_OP_SMOOTHSTEP = 227,
    MICROBIT_SPIRV_OP_LENGTH = 228,
    MICROBIT_SPIRV_OP_DISTANCE = 229,
    MICROBIT_SPIRV_OP_CROSS = 230,
    MICROBIT_SPIRV_OP_DOT = 231,
    MICROBIT_SPIRV_OP_NORMALIZE = 232,
    MICROBIT_SPIRV_OP_REFLECT = 233,
    MICROBIT_SPIRV_OP_REFRACT = 234,
    MICROBIT_SPIRV_OP_FACEFORWARD = 235,
    MICROBIT_SPIRV_OP_ANY = 236,
    MICROBIT_SPIRV_OP_ALL = 237,
    MICROBIT_SPIRV_OP_ISNAN = 238,
    MICROBIT_SPIRV_OP_ISINF = 239,
    MICROBIT_SPIRV_OP_ISFINITE = 240,
    MICROBIT_SPIRV_OP_UNPACK_HALF2X16 = 241,
    MICROBIT_SPIRV_OP_PACK_HALF2X16 = 242,
    MICROBIT_SPIRV_OP_IMAGE_READ = 247,
    MICROBIT_SPIRV_OP_IMAGE_WRITE = 248,
    MICROBIT_SPIRV_OP_IMAGE_SAMPLE_IMPLICIT_LOD = 251,
    MICROBIT_SPIRV_OP_IMAGE_SAMPLE_EXPLICIT_LOD = 252,
    MICROBIT_SPIRV_OP_IMAGE_SAMPLE_DREF_IMPLICIT_LOD = 253,
    MICROBIT_SPIRV_OP_IMAGE_SAMPLE_DREF_EXPLICIT_LOD = 254,
    MICROBIT_SPIRV_OP_IMAGE_SIZE = 257,
    MICROBIT_SPIRV_OP_IMAGE_FETCH = 258,
    MICROBIT_SPIRV_OP_CONTROL_BARRIER = 268,
    MICROBIT_SPIRV_OP_MEMORY_BARRIER = 269,
    MICROBIT_SPIRV_OP_BRANCH = 249,
    MICROBIT_SPIRV_OP_BRANCH_CONDITIONAL = 250,
    MICROBIT_SPIRV_OP_SWITCH = 251,
    MICROBIT_SPIRV_OP_KILL = 252,
    MICROBIT_SPIRV_OP_UNREACHABLE = 254,
    MICROBIT_SPIRV_OP_LOOP_MERGE = 246,
    MICROBIT_SPIRV_OP_SELECTION_MERGE = 247,
    MICROBIT_SPIRV_OP_LINE = 258,
    MICROBIT_SPIRV_OP_NO_LINE = 259,
    MICROBIT_SPIRV_OP_MAX_OPCODE = 0xFFFF
};

enum R_Microbit_SpirvParserStorageClass
{
    MICROBIT_SPIRV_STORAGE_CLASS_UNIFORM_CONSTANT = 0,
    MICROBIT_SPIRV_STORAGE_CLASS_INPUT = 1,
    MICROBIT_SPIRV_STORAGE_CLASS_UNIFORM = 2,
    MICROBIT_SPIRV_STORAGE_CLASS_OUTPUT = 3,
    MICROBIT_SPIRV_STORAGE_CLASS_WORKGROUP = 4,
    MICROBIT_SPIRV_STORAGE_CLASS_CROSS_WORKGROUP = 5,
    MICROBIT_SPIRV_STORAGE_CLASS_PRIVATE = 6,
    MICROBIT_SPIRV_STORAGE_CLASS_FUNCTION = 7,
    MICROBIT_SPIRV_STORAGE_CLASS_GENERIC = 8,
    MICROBIT_SPIRV_STORAGE_CLASS_PUSH_CONSTANT = 9,
    MICROBIT_SPIRV_STORAGE_CLASS_ATOMIC_COUNTER = 10,
    MICROBIT_SPIRV_STORAGE_CLASS_IMAGE = 11,
    MICROBIT_SPIRV_STORAGE_CLASS_STORAGE_BUFFER = 12,
    MICROBIT_SPIRV_STORAGE_CLASS_MAX = 0x7FFFFFFF
};

enum R_Microbit_SpirvParserExecutionModel
{
    MICROBIT_SPIRV_EXECUTION_MODEL_VERTEX = 0,
    MICROBIT_SPIRV_EXECUTION_MODEL_TESSELLATION_CONTROL = 1,
    MICROBIT_SPIRV_EXECUTION_MODEL_TESSELLATION_EVALUATION = 2,
    MICROBIT_SPIRV_EXECUTION_MODEL_GEOMETRY = 3,
    MICROBIT_SPIRV_EXECUTION_MODEL_FRAGMENT = 4,
    MICROBIT_SPIRV_EXECUTION_MODEL_GL_COMPUTE = 5,
    MICROBIT_SPIRV_EXECUTION_MODEL_KERNEL = 6,
    MICROBIT_SPIRV_EXECUTION_MODEL_MAX = 0x7FFFFFFF
};

enum R_Microbit_SpirvParserValueType
{
    MICROBIT_SPIRV_VALUE_TYPE_VOID = 0,
    MICROBIT_SPIRV_VALUE_TYPE_BOOL = 1,
    MICROBIT_SPIRV_VALUE_TYPE_INT = 2,
    MICROBIT_SPIRV_VALUE_TYPE_FLOAT = 3,
    MICROBIT_SPIRV_VALUE_TYPE_VECTOR = 4,
    MICROBIT_SPIRV_VALUE_TYPE_MATRIX = 5,
    MICROBIT_SPIRV_VALUE_TYPE_ARRAY = 6,
    MICROBIT_SPIRV_VALUE_TYPE_RUNTIME_ARRAY = 7,
    MICROBIT_SPIRV_VALUE_TYPE_STRUCT = 8,
    MICROBIT_SPIRV_VALUE_TYPE_IMAGE = 9,
    MICROBIT_SPIRV_VALUE_TYPE_SAMPLER = 10,
    MICROBIT_SPIRV_VALUE_TYPE_SAMPLED_IMAGE = 11,
    MICROBIT_SPIRV_VALUE_TYPE_POINTER = 12
};

enum R_Microbit_SpirvParserResultType
{
    MICROBIT_SPIRV_RESULT_TYPE_NONE = 0,
    MICROBIT_SPIRV_RESULT_TYPE_STRING = 1,
    MICROBIT_SPIRV_RESULT_TYPE_EXTENSION = 2,
    MICROBIT_SPIRV_RESULT_TYPE_FUNCTION_TYPE = 3,
    MICROBIT_SPIRV_RESULT_TYPE_TYPE = 4,
    MICROBIT_SPIRV_RESULT_TYPE_VARIABLE = 5,
    MICROBIT_SPIRV_RESULT_TYPE_CONSTANT = 6,
    MICROBIT_SPIRV_RESULT_TYPE_FUNCTION = 7,
    MICROBIT_SPIRV_RESULT_TYPE_ACCESS_CHAIN = 8,
    MICROBIT_SPIRV_RESULT_TYPE_FUNCTION_PARAMETER = 9,
    MICROBIT_SPIRV_RESULT_TYPE_LABEL = 10
};

struct R_Microbit_SpirvParserState;

struct R_Microbit_SpirvParserDecoration
{
        enum R_Microbit_SpirvParserOpcode type;
        uint32_t                          literal1;
        uint32_t                          literal2;
        uint32_t                          index;
};

struct R_Microbit_SpirvParserImageInfo
{
        enum R_Microbit_SpirvParserOpcode dim;
        uint8_t                           depth;
        uint8_t                           arrayed;
        uint8_t                           ms;
        uint8_t                           sampled;
        uint32_t                          format;
        uint32_t                          access;
};

struct R_Microbit_SpirvParserMember
{
        uint32_t type;
        union
        {
                float    f;
                double   d;
                int32_t  s;
                uint32_t u;
                uint64_t u64;
                int8_t   b;
                void*    pImage;
                void*    pSampler;
        } value;
        uint32_t                             memberCount;
        struct R_Microbit_SpirvParserMember* pMembers;
};

struct R_Microbit_SpirvParserResult
{
        enum R_Microbit_SpirvParserResultType    type;
        char*                                    pName;
        uint32_t                                 pointer;
        enum R_Microbit_SpirvParserStorageClass  storageClass;
        struct R_Microbit_SpirvParserResult*     owner;
        uint32_t                                 memberNameCount;
        char**                                   memberName;
        uint32_t                                 memberCount;
        struct R_Microbit_SpirvParserMember*     pMembers;
        uint32_t                                 decorationCount;
        struct R_Microbit_SpirvParserDecoration* decorations;
        uint32_t                                 returnType;
        uint32_t                                 extensionName;
        void*                                    pExtension;
        uint32_t*                                pParams;
        const uint32_t*                          pSourceLocation;
        uint32_t                                 sourceWordCount;
        enum R_Microbit_SpirvParserValueType     valueType;
        uint32_t                                 valueBitcount;
        int8_t                                   valueSign;
        struct R_Microbit_SpirvParserImageInfo*  pImageInfo;
};

struct R_Microbit_SpirvParserEntryPoint
{
        enum R_Microbit_SpirvParserExecutionModel execModel;
        uint32_t                                  id;
        char*                                     name;
        uint32_t                                  globalsCount;
        uint32_t*                                 pGlobals;
};

struct R_Microbit_SpirvParserFile
{
        char*    pName;
        uint32_t language;
        uint32_t languageVersion;
        char*    pSource;
};

struct R_Microbit_SpirvParserContext
{
        void (*pOpcodeExecute[0xFFFF]) (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState);
        void (*pOpcodeSetup[0xFFFF]) (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState);
};

struct R_Microbit_SpirvParserProgram
{
        struct R_Microbit_SpirvParserContext*    pContext;
        uint8_t                                  majorVersion;
        uint8_t                                  minorVersion;
        uint32_t                                 generatorId;
        uint32_t                                 generatorVersion;
        size_t                                   fileCount;
        struct R_Microbit_SpirvParserFile*       pFiles;
        uint32_t                                 bound;
        size_t                                   codeLength;
        const uint32_t*                          pCode;
        uint32_t                                 extensionCount;
        char**                                   ppExtensions;
        uint32_t                                 importCount;
        char**                                   ppImports;
        uint32_t                                 capabilityCount;
        uint32_t*                                pCapabilities;
        uint32_t                                 addressing;
        uint32_t                                 memoryModel;
        uint32_t                                 entryPointCount;
        struct R_Microbit_SpirvParserEntryPoint* pEntryPoints;
        uint32_t                                 localSizeX;
        uint32_t                                 localSizeY;
        uint32_t                                 localSizeZ;
        void (*pAllocateWorkgroupMemory) (
            struct R_Microbit_SpirvParserState* pState,
            uint32_t                            size,
            uint32_t                            id);
        void (
            *pWriteWorkgroupMemory) (struct R_Microbit_SpirvParserState* pState, uint32_t size, uint32_t id);
        void (*pAtomicOperation) (
            uint32_t                            operation,
            uint32_t                            scope,
            struct R_Microbit_SpirvParserState* pState);
        void* pUserData;
};

typedef struct R_Microbit_SpirvParserState* R_Microbit_SpirvParserStatePtr;

typedef void (*R_Microbit_SpirvParserImageReadCallback) (
    R_Microbit_SpirvParserStatePtr pState,
    void*                          pImage,
    int                            x,
    int                            y,
    int                            z,
    int                            layer,
    int                            level,
    float*                         pResult);

typedef void (*R_Microbit_SpirvParserImageWriteCallback) (
    R_Microbit_SpirvParserStatePtr pState,
    void*                          pImage,
    int                            x,
    int                            y,
    int                            z,
    int                            layer,
    int                            level,
    const float*                   pData);

struct R_Microbit_SpirvParserState
{
        struct R_Microbit_SpirvParserContext* pContext;
        struct R_Microbit_SpirvParserProgram* pOwner;
        const uint32_t*                       pCodeCurrent;
        struct R_Microbit_SpirvParserResult*  pResults;
        uint8_t                               didJump;
        uint8_t                               discarded;
        struct R_Microbit_SpirvParserResult*  pCurrentFunction;
        uint32_t                              currentParameter;
        uint32_t                              functionStackCurrent;
        uint32_t                              functionStackCount;
        const uint32_t**                      ppFunctionStack;
        struct R_Microbit_SpirvParserResult** ppFunctionStackInfo;
        uint32_t                              returnId;
        uint32_t*                             pFunctionStackReturns;
        uint32_t*                             pFunctionStackCfg;
        uint32_t*                             pFunctionStackCfgParent;
        void (*emitVertex) (R_Microbit_SpirvParserStatePtr pState, uint32_t stream);
        void (*endPrimitive) (R_Microbit_SpirvParserStatePtr pState, uint32_t stream);
        void (*controlBarrier) (
            R_Microbit_SpirvParserStatePtr pState,
            uint32_t                       execution,
            uint32_t                       memory,
            uint32_t                       semantics);
        R_Microbit_SpirvParserImageReadCallback  pReadImage;
        R_Microbit_SpirvParserImageWriteCallback pWriteImage;
        float                                    fragCoord[4];
        uint8_t                                  derivativeIsGroupMember;
        uint8_t                                  derivativeUsed;
        float                                    derivativeBufferX[16];
        float                                    derivativeBufferY[16];
        struct R_Microbit_SpirvParserState*      pDerivativeGroupX;
        struct R_Microbit_SpirvParserState*      pDerivativeGroupY;
        struct R_Microbit_SpirvParserState*      pDerivativeGroupD;
        char*                                    pCurrentFile;
        uint32_t                                 currentLine;
        uint32_t                                 currentColumn;
        uint32_t                                 instructionCount;
        void*                                    pAnalyzer;
        void*                                    pUserData;
};

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserContext* R_Microbit_NewSpirvParserContext (void);
MICROBIT_SPIRV_API void R_Microbit_DeleteSpirvParserContext (struct R_Microbit_SpirvParserContext* pCtx);

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserProgram* R_Microbit_NewSpirvParserProgram (
    struct R_Microbit_SpirvParserContext* pCtx,
    const uint32_t*                       pSpv,
    size_t                                spvLength);
MICROBIT_SPIRV_API char*
R_Microbit_SpirvParserProgramAddExtension (struct R_Microbit_SpirvParserProgram* pProg, uint32_t length);
MICROBIT_SPIRV_API struct R_Microbit_SpirvParserEntryPoint*
R_Microbit_SpirvParserProgramCreateEntryPoint (struct R_Microbit_SpirvParserProgram* pProg);
MICROBIT_SPIRV_API void
R_Microbit_SpirvParserProgramAddCapability (struct R_Microbit_SpirvParserProgram* pProg, uint32_t cap);
MICROBIT_SPIRV_API void R_Microbit_DeleteSpirvParserProgram (struct R_Microbit_SpirvParserProgram* pProg);

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserState*
                        R_Microbit_NewSpirvParserState (struct R_Microbit_SpirvParserProgram* pProg);
MICROBIT_SPIRV_API void R_Microbit_DeleteSpirvParserState (struct R_Microbit_SpirvParserState* pState);
MICROBIT_SPIRV_API void R_Microbit_SpirvParserStateSetExtension (
    struct R_Microbit_SpirvParserState* pState,
    const char*                         pName,
    void*                               pExt);
MICROBIT_SPIRV_API void R_Microbit_SpirvParserStateCallFunction (struct R_Microbit_SpirvParserState* pState);
MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStatePrepare (struct R_Microbit_SpirvParserState* pState, uint32_t fnLocation);
MICROBIT_SPIRV_API void R_Microbit_SpirvParserStateSetFragCoord (
    struct R_Microbit_SpirvParserState* pState,
    float                               x,
    float                               y,
    float                               z,
    float                               w);
MICROBIT_SPIRV_API void R_Microbit_SpirvParserStateStepOpcode (struct R_Microbit_SpirvParserState* pState);
MICROBIT_SPIRV_API void R_Microbit_SpirvParserStateStepInto (struct R_Microbit_SpirvParserState* pState);
MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStateJumpTo (struct R_Microbit_SpirvParserState* pState, uint32_t line);
MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStateJumpToInstruction (struct R_Microbit_SpirvParserState* pState, uint32_t inst);
MICROBIT_SPIRV_API uint32_t
R_Microbit_SpirvParserStateGetResultLocation (struct R_Microbit_SpirvParserState* pState, const char* pName);
MICROBIT_SPIRV_API struct R_Microbit_SpirvParserResult*
R_Microbit_SpirvParserStateGetResult (struct R_Microbit_SpirvParserState* pState, const char* pName);
MICROBIT_SPIRV_API struct R_Microbit_SpirvParserResult*
R_Microbit_SpirvParserStateGetResultWithValue (struct R_Microbit_SpirvParserState* pState, const char* pName);
MICROBIT_SPIRV_API struct R_Microbit_SpirvParserResult* R_Microbit_SpirvParserStateGetLocalResult (
    struct R_Microbit_SpirvParserState*  pState,
    struct R_Microbit_SpirvParserResult* pFn,
    const char*                          pName);
MICROBIT_SPIRV_API struct R_Microbit_SpirvParserMember* R_Microbit_SpirvParserStateGetObjectMember (
    struct R_Microbit_SpirvParserState*  pState,
    struct R_Microbit_SpirvParserResult* pVar,
    const char*                          pMemberName);

MICROBIT_SPIRV_API enum R_Microbit_SpirvParserError
R_Microbit_SpirvParserValidateHeader (const uint32_t* pData, size_t dataLength);
