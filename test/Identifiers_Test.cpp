#include <doctest/doctest.h>
#include "DoIPIdentifiers.h"
#include <sstream>

#include "doctest_aux.h"

using namespace doip;

TEST_SUITE("GenericFixedId") {

    TEST_CASE("Default constructor creates empty VIN") {
        DoIpVin vin;

        // Verify all bytes are '0'
        for (size_t i = 0; i < 17; ++i) {
            CHECK(vin[i] == '0');
        }
    }

    TEST_CASE("Construction from string - exact length") {
        const std::string test_vin = "1HGBH41JXMN109186";
        DoIpVin vin(test_vin);

        CHECK_FALSE(vin.isEmpty());
        CHECK(vin.toString() == test_vin);

        // Verify individual characters
        CHECK(vin[0] == '1');
        CHECK(vin[16] == '6');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from string - shorter than 17 characters") {
        const std::string test_vin = "ABC12300000000000";
        DoIpVin vin(test_vin);

        CHECK_FALSE(vin.isEmpty());
        CHECK(vin.toString() == test_vin);

        // Verify padding with zeros
        CHECK(vin[0] == 'A');
        CHECK(vin[5] == '3');
        CHECK(vin[6] == '0');
        CHECK(vin[16] == '0');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from string - longer than 17 characters") {
        const std::string test_vin = "1HGBH41JXMN109186TOOLONGSTRING";
        DoIpVin vin(test_vin);

        CHECK_FALSE(vin.isEmpty());
        CHECK(vin.toString() == "1HGBH41JXMN109186");

        // Verify truncation
        CHECK(vin[0] == '1');
        CHECK(vin[16] == '6');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from empty string") {
        DoIpVin vin("");

        CHECK(vin.isEmpty());
        CHECK(vin.toString() == "00000000000000000");
        CHECK_BYTE_ARRAY_REF_EQ(vin.asByteArray(), DoIpVin::Zero.asByteArray());
    }

    TEST_CASE("Construction from C-style string") {
        const char* test_vin = "WVWZZZ1JZYW123456";
        DoIpVin vin(test_vin);

        CHECK(vin.toString() == test_vin);
        CHECK(vin[0] == 'W');
        CHECK(vin[16] == '6');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from nullptr C-style string") {
        const char* null_ptr = nullptr;
        DoIpVin vin(null_ptr);

        CHECK(vin.isEmpty());
        CHECK_BYTE_ARRAY_REF_EQ(vin.asByteArray(), DoIpVin::Zero.asByteArray());
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from byte sequence") {
        const uint8_t bytes[] = {'T', 'E', 'S', 'T', 'V', 'I', 'N', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
        DoIpVin vin(bytes, sizeof(bytes));

        CHECK(vin.toString() == "TESTVIN1234567890");
        CHECK(vin[0] == 'T');
        CHECK(vin[16] == vin.getPadByte());
        // I is illegal in VINs
        CHECK(isValidVin(vin) == false);
    }

    TEST_CASE("Construction from byte sequence - shorter") {
        const uint8_t bytes[] = {'S', 'H', 'O', 'R', 'T', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
        DoIpVin vin(bytes, sizeof(bytes));

        CHECK(vin.toString() == "SHORT000000000000");
        CHECK(vin[0] == 'S');
        CHECK(vin[4] == 'T');
        CHECK(vin[5] == vin.getPadByte());
        CHECK(vin[16] == vin.getPadByte());
        // O is illegal in VINs
        CHECK(isValidVin(vin) == false);
    }

    TEST_CASE("Construction from byte sequence - longer") {
        const uint8_t bytes[] = {'V', 'E', 'R', 'Y', 'L', 'O', 'N', 'G', 'V', 'I', 'N', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
        DoIpVin vin(bytes, sizeof(bytes));

        CHECK(vin.toString() == "VERYLONGVIN123456");
        CHECK(vin[16] == '6');
        // Note: This VIN contains 'I' and 'O' which are invalid per ISO 3779
        CHECK(isValidVin(vin) == false);
    }

    TEST_CASE("Construction from null byte sequence") {
        DoIpVin vin(nullptr, 10);

        CHECK(vin.isEmpty());
        CHECK(vin == DoIpVin::Zero);
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from ByteArray - exact length") {
        ByteArray bytes = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
        DoIpVin vin(bytes);

        CHECK(vin.toString() == "123456789ABCDEFGH");
        CHECK(vin[0] == '1');
        CHECK(vin[16] == 'H');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from ByteArray - shorter") {
        ByteArray bytes = {'X', 'Y', 'Z', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
        DoIpVin vin(bytes);

        CHECK(vin.toString() == "XYZ00000000000000");
        CHECK(vin[0] == 'X');
        CHECK(vin[2] == 'Z');
        CHECK(vin[3] == '0');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from ByteArray - longer") {
        ByteArray bytes = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'};
        DoIpVin vin(bytes);

        CHECK(vin.toString() == "ABCDEFGHIJKLMNOPQ");
        CHECK(vin[16] == 'Q');
    }

    TEST_CASE("Construction from empty ByteArray") {
        ByteArray bytes;
        DoIpVin vin(bytes);

        CHECK(vin.isEmpty());
        CHECK_BYTE_ARRAY_REF_EQ(vin.asByteArray(), DoIpVin::Zero.asByteArray());
    }

    TEST_CASE("Copy constructor") {
        DoIpVin vin1("ORIGINALVIN123456");
        DoIpVin vin2(vin1);

        CHECK(vin1 == vin2);
        CHECK(vin2.toString() == "ORIGINALVIN123456");
    }

    TEST_CASE("Move assignment") {
        DoIpVin vin1("MOVEASGN123456789");
        DoIpVin vin2;

        vin2 = std::move(vin1);

        CHECK(vin2.toString() == "MOVEASGN123456789");
    }

    TEST_CASE("toString method") {
        SUBCASE("Full VIN") {
            DoIpVin vin("FULLVIN1234567890");
            CHECK(vin.toString() == "FULLVIN1234567890");
        }

        SUBCASE("Partial VIN with padding") {
            DoIpVin vin("PART");
            CHECK(vin.toString() == "PART0000000000000");
        }

        SUBCASE("Empty VIN") {
            DoIpVin vin;
            CHECK(vin.toString() == "00000000000000000");
        }
    }

    TEST_CASE("getArray method") {
        DoIpVin vin("ARRAYTEST12345678");
        const auto& arr = vin.getArray();

        CHECK(arr.size() == 17);
        CHECK(arr[0] == 'A');
        CHECK(arr[16] == '8');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("data method") {
        DoIpVin vin("DATATEST123456789");
        const uint8_t* ptr = vin.data();

        CHECK(ptr != nullptr);
        CHECK(ptr[0] == 'D');
        CHECK(ptr[16] == '9');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("size method") {
        DoIpVin vin1;
        DoIpVin vin2("SHORT");
        DoIpVin vin3("EXACTSEVENTEENVIN");




        CHECK(isValidVin(vin1));
        CHECK(isValidVin(vin2) == false); // O is illegal in VINs
        CHECK(isValidVin(vin3) == false); // I is illegal in VINs
    }

    TEST_CASE("isEmpty method") {
        SUBCASE("Empty VIN") {
            DoIpVin vin;
            CHECK(vin.isEmpty());
        }

        SUBCASE("Empty string") {
            DoIpVin vin("");
            CHECK(vin.isEmpty());
        }

        SUBCASE("Zero instance") {
            CHECK(DoIpVin::Zero.isEmpty());
        }

        SUBCASE("Non-empty VIN") {
            DoIpVin vin("X");
            CHECK_FALSE(vin.isEmpty());
            CHECK(isValidVin(vin));
        }

        SUBCASE("Full VIN") {
            DoIpVin vin("FULLVXN1234567890");
            CHECK_FALSE(vin.isEmpty());
            CHECK(isValidVin(vin));
        }
    }

    TEST_CASE("Equality operator") {
        DoIpVin vin1("SAMEVIN1234567890");
        DoIpVin vin2("SAMEVIN1234567890");
        DoIpVin vin3("DIFFVIN1234567890");

        CHECK(vin1 == vin2);
        CHECK_FALSE(vin1 == vin3);

        DoIpVin vin4;
        DoIpVin vin5;
        CHECK(vin4 == vin5);
        CHECK_BYTE_ARRAY_REF_EQ(vin4.asByteArray(), DoIpVin::Zero.asByteArray());
    }

    TEST_CASE("Inequality operator") {
        DoIpVin vin1("VIN1_12345678901");
        DoIpVin vin2("VIN2_12345678901");
        DoIpVin vin3("VIN1_12345678901");

        CHECK(vin1 != vin2);
        CHECK_FALSE(vin1 != vin3);

        DoIpVin vin4;
        CHECK(vin1 != vin4);
    }

    TEST_CASE("Array subscript operator") {
        DoIpVin vin("SUBSCRIPT12345678");

        CHECK(vin[0] == 'S');
        CHECK(vin[1] == 'U');
        CHECK(vin[8] == 'T');
        CHECK(vin[16] == '8');
    }

    TEST_CASE("Array subscript with padding") {
        DoIpVin vin("PAD");

        CHECK(vin[0] == 'P');
        CHECK(vin[1] == 'A');
        CHECK(vin[2] == 'D');
        CHECK(vin[3] == '0');
        CHECK(vin[16] == '0');
    }

    TEST_CASE("VIN with special characters") {
        DoIpVin vin("VIN-WITH_SPEC.IAL");

        CHECK(vin.toString() == "VIN-WITH_SPEC.IAL");
        CHECK(vin[3] == '-');
        CHECK(vin[8] == '_');
        CHECK(vin[13] == '.');
    }

    TEST_CASE("VIN with numeric characters") {
        DoIpVin vin("12345678901234567");

        CHECK(vin.toString() == "12345678901234567");
        CHECK(vin[0] == '1');
        CHECK(vin[16] == '7');
    }

    TEST_CASE("VIN with lowercase characters") {
        DoIpVin vin("lowercase12345678");

        // String constructor converts to uppercase
        CHECK(vin.toString() == "LOWERCASE12345678");
        CHECK(vin[0] == 'L');
        // 'O' is invalid per ISO 3779
        CHECK(isValidVin(vin) == false);
    }

    TEST_CASE("VIN with mixed case") {
        DoIpVin vin("MxXeDcAsE12345678");

        // String constructor converts to uppercase
        CHECK(vin.toString() == "MXXEDCASE12345678");
        CHECK(vin[0] == 'M');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Real-world VIN examples") {
        SUBCASE("Honda VIN") {
            DoIpVin vin("1HGBH41JXMN109186");
            CHECK(vin.toString() == "1HGBH41JXMN109186");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("Volkswagen VIN") {
            DoIpVin vin("WVWZZZ1JZYW123456");
            CHECK(vin.toString() == "WVWZZZ1JZYW123456");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("BMW VIN") {
            DoIpVin vin("WBA3B1G59DNP26082");
            CHECK(vin.toString() == "WBA3B1G59DNP26082");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("Mercedes VIN") {
            DoIpVin vin("WDDUG8CB9DA123456");
            CHECK(vin.toString() == "WDDUG8CB9DA123456");
            CHECK_FALSE(vin.isEmpty());
        }
    }

    TEST_CASE("VIN conversion round-trip") {
        const std::string original = "ROUNDTRIP12345678";

        DoIpVin vin1(original);
        std::string str = vin1.toString();
        DoIpVin vin2(str);

        CHECK(vin1 == vin2);
        CHECK(vin2.toString() == original);
    }

    TEST_CASE("ByteArray conversion round-trip") {
        const std::string original = "BYTEARRAYTRIP1234";

        DoIpVin vin1(original);
        ByteArrayRef bytes = vin1.asByteArray();
        DoIpVin vin2(bytes.first, bytes.second);

        CHECK(vin1 == vin2);
        CHECK(bytes.second == 17);
    }

    TEST_CASE("VIN with null bytes in middle") {
        uint8_t data[17] = {'V', 'I', 'N', 0, 'N', 'U', 'L', 'L', 0, 'B', 'Y', 'T', 'E', 'S', '1', '2', '3'};
        DoIpVin vin(data, 17);

        // toString should stop at first null byte
        CHECK(vin.toString() == "VIN");

        // But the array should contain all data
        CHECK(vin[3] == 0);
        CHECK(vin[4] == 'N');
    }

    TEST_CASE("Constant correctness") {
        const DoIpVin vin("CONSTVIN123456789");

        CHECK(vin.toString() == "CONSTVIN123456789");
                CHECK_FALSE(vin.isEmpty());
        CHECK(vin[0] == 'C');

        const auto& arr = vin.getArray();
        CHECK(arr[0] == 'C');

        const uint8_t* ptr = vin.data();
        CHECK(ptr[0] == 'C');
    }

    TEST_CASE("DoIpEid - Entity Identifier (6 bytes)") {
        SUBCASE("Default constructor") {
            DoIpEid eid;
            CHECK(eid.isEmpty());
            CHECK(eid.size() == 6);
            CHECK(eid.toString().empty());
        }

        SUBCASE("Static Zero instance") {
            CHECK(DoIpEid::Zero.isEmpty());
            CHECK(DoIpEid::Zero.size() == 6);
        }

        SUBCASE("Construction from string - exact length") {
            DoIpEid eid("ABC123");
            CHECK(eid.toString() == "ABC123");
            CHECK(eid.size() == 6);
            CHECK_FALSE(eid.isEmpty());
        }

        SUBCASE("Construction from string - shorter") {
            DoIpEid eid("EID");
            CHECK(eid.toString() == "EID");
            CHECK(eid.size() == 6);
            CHECK(eid[0] == 'E');
            CHECK(eid[2] == 'D');
            CHECK(eid[3] == 0);
            CHECK(eid[5] == 0);
        }

        SUBCASE("Construction from string - longer") {
            DoIpEid eid("TOOLONGEID");
            CHECK(eid.toString() == "TOOLON");
            CHECK(eid.size() == 6);
        }

        SUBCASE("Construction from byte array") {
            const uint8_t bytes[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
            DoIpEid eid(bytes, sizeof(bytes));
            CHECK(eid.size() == 6);
            CHECK(eid[0] == 0x01);
            CHECK(eid[5] == 0x06);
            CHECK_FALSE(eid.isEmpty());
        }

        SUBCASE("Construction from ByteArray") {
            ByteArray bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
            DoIpEid eid(bytes);
            CHECK(eid.size() == 6);
            CHECK(eid[0] == 0xAA);
            CHECK(eid[5] == 0xFF);
        }

        SUBCASE("Equality and comparison") {
            DoIpEid eid1("EID001");
            DoIpEid eid2("EID001");
            DoIpEid eid3("EID002");

            CHECK(eid1 == eid2);
            CHECK(eid1 != eid3);
            CHECK(eid2 != eid3);
        }

        SUBCASE("asByteArray method") {
            DoIpEid eid("TEST12");
            ByteArrayRef result = eid.asByteArray();
            CHECK(result.second == 6);
            CHECK(result.first[0] == 'T');
            CHECK(result.first[5] == '2');
        }
    }

    TEST_CASE("DoIpGid - Group Identifier (6 bytes)") {
        SUBCASE("Default constructor") {
            DoIpGid gid;
            CHECK(gid.isEmpty());
            CHECK(gid.size() == 6);
            CHECK(gid.toString().empty());
        }

        SUBCASE("Static Zero instance") {
            CHECK(DoIpGid::Zero.isEmpty());
            CHECK(DoIpGid::Zero.size() == 6);
        }

        SUBCASE("Construction from string - exact length") {
            DoIpGid gid("GRP001");
            CHECK(gid.toString() == "GRP001");
            CHECK(gid.size() == 6);
            CHECK_FALSE(gid.isEmpty());
        }

        SUBCASE("Construction from string - shorter") {
            DoIpGid gid("GID");
            CHECK(gid.toString() == "GID");
            CHECK(gid.size() == 6);
            CHECK(gid[0] == 'G');
            CHECK(gid[2] == 'D');
            CHECK(gid[3] == 0);
            CHECK(gid[5] == 0);
        }

        SUBCASE("Construction from string - longer") {
            DoIpGid gid("TOOLONGGID");
            CHECK(gid.toString() == "TOOLON");
            CHECK(gid.size() == 6);
        }

        SUBCASE("Construction from uint32_t - longer") {
            uint32_t long_value = 0x544F4F4C; // ASCII for "TOOL"
            DoIpGid gid(long_value);
            CHECK(gid.toString() == "TOOL");
            CHECK(gid.size() == 6);
        }


        SUBCASE("Construction from uint64_t - longer") {
            uint64_t long_value = 0x544F4F4C4F4E47; // ASCII for "TOOLONG"
            DoIpGid gid(long_value);
            CHECK(gid.toString() == "OOLONG"); //
            CHECK(gid.size() == 6);
        }

        SUBCASE("Construction from byte array") {
            const uint8_t bytes[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
            DoIpGid gid(bytes, sizeof(bytes));
            CHECK(gid.size() == 6);
            CHECK(gid[0] == 0x10);
            CHECK(gid[5] == 0x60);
            CHECK_FALSE(gid.isEmpty());
        }

        SUBCASE("Construction from ByteArray") {
            ByteArray bytes = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
            DoIpGid gid(bytes);
            CHECK(gid.size() == 6);
            CHECK(gid[0] == 0x11);
            CHECK(gid[5] == 0x66);
        }

        SUBCASE("Equality and comparison") {
            DoIpGid gid1("GROUP1");
            DoIpGid gid2("GROUP1");
            DoIpGid gid3("GROUP2");

            CHECK(gid1 == gid2);
            CHECK(gid1 != gid3);
            CHECK(gid2 != gid3);
        }

        SUBCASE("asByteArray method") {
            DoIpGid gid("MYGRP1");
            ByteArrayRef result = gid.asByteArray();
            CHECK(result.second == 6);
            CHECK(result.first[0] == 'M');
            CHECK(result.first[5] == '1');
        }
    }

    TEST_CASE("Different identifier types are independent") {
        // Even though EID and GID have the same length, they're different types
        DoIpEid eid("ABC123");
        DoIpGid gid("ABC123");

        // They should have the same content but be different types
        CHECK(eid.toString() == gid.toString());
        CHECK(eid.size() == gid.size());

        // Verify they're truly independent instances
        DoIpEid eid2(eid);
        DoIpGid gid2(gid);
        CHECK(eid == eid2);
        CHECK(gid == gid2);
    }

    TEST_CASE("invalid VINs") {
        SUBCASE("VIN with invalid characters") {
            DoIpVin vin("INVALID#VIN$12345");

            CHECK(vin.toString() == "INVALID#VIN$12345");
            CHECK(isValidVin(vin) == false);
        }

        SUBCASE("VIN with small character ") {
            DoIpVin vin("isduds");

            CHECK(vin.toString() == "ISDUDS00000000000");
            CHECK(isValidVin(vin) == false);
        }
    }
}
