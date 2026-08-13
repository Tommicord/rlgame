#ifndef RL_LOG_LOG_FORMAT_SPEC_H
#define RL_LOG_LOG_FORMAT_SPEC_H

#include <cstddef>

namespace rl
{

/** Enumeration of format types for log formatting */
enum class FormatType
{
        Int, /**< Signed integer */
        Uint, /**< Unsigned integer */
        Long, /**< Signed long */
        ULong, /**< Unsigned long */
        LongLong, /**< Signed long long */
        ULongLong, /**< Unsigned long long */
        intMax, /**< Maximum signed integer */
        UintMax, /**< Maximum unsigned integer */
        Size, /**< Size type */
        PtrDiff, /**< Pointer difference */
        Char, /**< character */
        Short, /**< Short integer */
        UShort, /**< Unsigned short */
        Float, /**< Floating point */
        Double, /**< Double precision */
        String, /**< String */
        Pointer, /**< Pointer address */
        Array /**< Array */
};

/** Format specification for log formatting */
struct FormatSpec
{
                FormatType type; /**< The format type */
                int        precision; /**< Precision for floating point */
                bool       isSigned; /**< Whether the value is signed */
                char       subType; /**< Subtype for array formatting */

                /** Default constructor */
                FormatSpec() : type(FormatType::Int), precision(6), isSigned(true), subType(0)
                {
                }
};

/** Parses a format specification from a format string
 * @param format The format string to parse
 * @param spec Output format specification
 * @return Pointer after the parsed spec */
const char* parseFormatSpec(const char* format, FormatSpec& spec) noexcept;

} // namespace rl

#endif // RL_LOG_LOG_FORMAT_SPEC_H
