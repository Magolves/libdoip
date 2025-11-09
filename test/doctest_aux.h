#ifndef DOCTEST_AUX_H
#define DOCTEST_AUX_H

#include "ByteArray.h"
#include <doctest/doctest.h>
#include <iomanip>
#include <sstream>

namespace doip {
namespace test {

/**
 * @brief Compare two ByteArrays and generate detailed mismatch information
 *
 * @param lhs_got Left-hand side ByteArray
 * @param rhs_exp Right-hand side ByteArray
 * @return true if arrays are equal, false otherwise
 */
inline bool compareByteArrays(const ByteArray &lhs_got, const ByteArray &rhs_exp, std::ostream &os) {
    if (lhs_got.size() != rhs_exp.size()) {
        os << "Size mismatch: lhs_got.size()=" << lhs_got.size()
           << " != rhs_exp.size()=" << rhs_exp.size();
        return false;
    }

    bool all_match = true;
    std::ostringstream diff_stream;

    for (size_t i = 0; i < lhs_got.size(); ++i) {
        if (lhs_got[i] != rhs_exp[i]) {
            if (all_match) {
                diff_stream << "Byte differences found:\n";
                all_match = false;
            }

            diff_stream << "  Index " << std::setw(3) << i << ": "
                        << "lhs_got=0x" << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<unsigned int>(lhs_got[i])
                        << " ('" << (std::isprint(lhs_got[i]) ? static_cast<char>(lhs_got[i]) : '.') << "')"
                        << " != "
                        << "rhs_exp=0x" << std::setw(2) << std::setfill('0')
                        << static_cast<unsigned int>(rhs_exp[i])
                        << " ('" << (std::isprint(rhs_exp[i]) ? static_cast<char>(rhs_exp[i]) : '.') << "')"
                        << std::dec << std::setfill(' ') << "\n";
        }
    }

    if (!all_match) {
        os << diff_stream.str();
    }

    return all_match;
}

} // namespace test
} // namespace doip

/**
 * @brief Custom doctest macro for comparing ByteArrays with detailed mismatch reporting
 *
 * Usage: CHECK_BYTE_ARRAY_EQ(expected, actual)
 *
 * On mismatch, prints:
 * - Size differences if arrays have different lengths
 * - Index, hex value, and char representation for each differing byte
 */
#define CHECK_BYTE_ARRAY_EQ(lhs_got, rhs_exp)                                                \
    do {                                                                                     \
        std::ostringstream _doctest_os;                                                      \
        bool _doctest_result = doip::test::compareByteArrays(lhs_got, rhs_exp, _doctest_os); \
        if (!_doctest_result) {                                                              \
            INFO(_doctest_os.str());                                                         \
        }                                                                                    \
        CHECK_MESSAGE(_doctest_result, _doctest_os.str());                                   \
    } while (false)

#define CHECK_BYTE_ARRAY_REF_EQ(lhs_got, rhs_exp)                     \
    do {                                                              \
        std::ostringstream _doctest_os;                               \
        bool _doctest_result = doip::test::compareByteArrays(         \
            ByteArray(lhs_got.first, lhs_got.second), \
            ByteArray(rhs_exp.first, rhs_exp.second), \
            _doctest_os);                                             \
        if (!_doctest_result) {                                       \
            INFO(_doctest_os.str());                                  \
        }                                                             \
        CHECK_MESSAGE(_doctest_result, _doctest_os.str());            \
    } while (false)

/**
 * @brief REQUIRE variant of ByteArray comparison (stops test on failure)
 */
#define REQUIRE_BYTE_ARRAY_EQ(lhs_got, rhs_exp)                                              \
    do {                                                                                     \
        std::ostringstream _doctest_os;                                                      \
        bool _doctest_result = doip::test::compareByteArrays(lhs_got, rhs_exp, _doctest_os); \
        if (!_doctest_result) {                                                              \
            DOCTEST_INFO(_doctest_os.str());                                                 \
        }                                                                                    \
        REQUIRE(_doctest_result);                                                            \
    } while (false)

#endif /* DOCTEST_AUX_H */
