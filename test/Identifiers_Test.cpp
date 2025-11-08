#include <doctest/doctest.h>
#include "DoIPIdentifiers.h"
#include <sstream>

#include "doctest_aux.h"

using namespace doip;

TEST_SUITE("GenericFixedId") {

    TEST_CASE("Default constructor creates empty VIN") {
        DoIPVIN vin;

        // Verify all bytes are '0'
        for (size_t i = 0; i < 17; ++i) {
            CHECK(vin[i] == '0');
        }
    }

    TEST_CASE("Construction from string - exact length") {
        const std::string test_vin = "1HGBH41JXMN109186";
        DoIPVIN vin(test_vin);

        CHECK_FALSE(vin.isEmpty());
        CHECK(vin.toString() == test_vin);
        CHECK(vin.size() == 17);

        // Verify individual characters
        CHECK(vin[0] == '1');
        CHECK(vin[16] == '6');
    }

    TEST_CASE("Construction from string - shorter than 17 characters") {
        const std::string test_vin = "ABC12300000000000";
        DoIPVIN vin(test_vin);

        CHECK_FALSE(vin.isEmpty());
        CHECK(vin.toString() == test_vin);
        CHECK(vin.size() == 17);

        // Verify padding with zeros
        CHECK(vin[0] == 'A');
        CHECK(vin[5] == '3');
        CHECK(vin[6] == '0');
        CHECK(vin[16] == '0');
    }

    TEST_CASE("Construction from string - longer than 17 characters") {
        const std::string test_vin = "1HGBH41JXMN109186TOOLONGSTRING";
        DoIPVIN vin(test_vin);

        CHECK_FALSE(vin.isEmpty());
        CHECK(vin.toString() == "1HGBH41JXMN109186");
        CHECK(vin.size() == 17);

        // Verify truncation
        CHECK(vin[0] == '1');
        CHECK(vin[16] == '6');
    }

    TEST_CASE("Construction from empty string") {
        DoIPVIN vin("");

        CHECK(vin.isEmpty());
        CHECK(vin.toString() == "00000000000000000");
        CHECK_BYTE_ARRAY_EQ(vin.toByteArray(), DoIPVIN::Zero.toByteArray());
    }

    TEST_CASE("Construction from C-style string") {
        const char* test_vin = "WVWZZZ1JZYW123456";
        DoIPVIN vin(test_vin);

        CHECK(vin.toString() == test_vin);
        CHECK(vin[0] == 'W');
        CHECK(vin[16] == '6');
    }

    TEST_CASE("Construction from nullptr C-style string") {
        const char* null_ptr = nullptr;
        DoIPVIN vin(null_ptr);

        CHECK(vin.isEmpty());
        CHECK_BYTE_ARRAY_EQ(vin.toByteArray(), DoIPVIN::Zero.toByteArray());
    }

    TEST_CASE("Construction from byte sequence") {
        const uint8_t bytes[] = {'T', 'E', 'S', 'T', 'V', 'I', 'N', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
        DoIPVIN vin(bytes, sizeof(bytes));

        CHECK(vin.toString() == "TESTVIN1234567890");
        CHECK(vin[0] == 'T');
        CHECK(vin[16] == vin.getPadByte());
    }

    TEST_CASE("Construction from byte sequence - shorter") {
        const uint8_t bytes[] = {'S', 'H', 'O', 'R', 'T', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
        DoIPVIN vin(bytes, sizeof(bytes));

        CHECK(vin.toString() == "SHORT000000000000");
        CHECK(vin[0] == 'S');
        CHECK(vin[4] == 'T');
        CHECK(vin[5] == vin.getPadByte());
        CHECK(vin[16] == vin.getPadByte());
    }

    TEST_CASE("Construction from byte sequence - longer") {
        const uint8_t bytes[] = {'V', 'E', 'R', 'Y', 'L', 'O', 'N', 'G', 'V', 'I', 'N', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
        DoIPVIN vin(bytes, sizeof(bytes));

        CHECK(vin.toString() == "VERYLONGVIN123456");
        CHECK(vin[16] == '6');
    }

    TEST_CASE("Construction from null byte sequence") {
        DoIPVIN vin(nullptr, 10);

        CHECK(vin.isEmpty());
        CHECK(vin == DoIPVIN::Zero);
    }

    TEST_CASE("Construction from ByteArray - exact length") {
        ByteArray bytes = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
        DoIPVIN vin(bytes);

        CHECK(vin.toString() == "123456789ABCDEFGH");
        CHECK(vin[0] == '1');
        CHECK(vin[16] == 'H');
    }

    TEST_CASE("Construction from ByteArray - shorter") {
        ByteArray bytes = {'X', 'Y', 'Z', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
        DoIPVIN vin(bytes);

        CHECK(vin.toString() == "XYZ00000000000000");
        CHECK(vin[0] == 'X');
        CHECK(vin[2] == 'Z');
        CHECK(vin[3] == '0');
    }

    TEST_CASE("Construction from ByteArray - longer") {
        ByteArray bytes = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'};
        DoIPVIN vin(bytes);

        CHECK(vin.toString() == "ABCDEFGHIJKLMNOPQ");
        CHECK(vin[16] == 'Q');
    }

    TEST_CASE("Construction from empty ByteArray") {
        ByteArray bytes;
        DoIPVIN vin(bytes);

        CHECK(vin.isEmpty());
        CHECK_BYTE_ARRAY_EQ(vin.toByteArray(), DoIPVIN::Zero.toByteArray());
    }

    TEST_CASE("Copy constructor") {
        DoIPVIN vin1("ORIGINALVIN123456");
        DoIPVIN vin2(vin1);

        CHECK(vin1 == vin2);
        CHECK(vin2.toString() == "ORIGINALVIN123456");
    }

    TEST_CASE("Move assignment") {
        DoIPVIN vin1("MOVEASGN123456789");
        DoIPVIN vin2;

        vin2 = std::move(vin1);

        CHECK(vin2.toString() == "MOVEASGN123456789");
    }

    TEST_CASE("toString method") {
        SUBCASE("Full VIN") {
            DoIPVIN vin("FULLVIN1234567890");
            CHECK(vin.toString() == "FULLVIN1234567890");
        }

        SUBCASE("Partial VIN with padding") {
            DoIPVIN vin("PART");
            CHECK(vin.toString() == "PART0000000000000");
        }

        SUBCASE("Empty VIN") {
            DoIPVIN vin;
            CHECK(vin.toString() == "00000000000000000");
        }
    }

    TEST_CASE("toByteArray method") {
        DoIPVIN vin("TESTBYTEARRAY1234");
        ByteArray result = vin.toByteArray();

        CHECK(result.size() == 17);
        CHECK(result[0] == static_cast<uint8_t>('T'));
        CHECK(result[16] == static_cast<uint8_t>('4'));

        // Verify it's a copy, not a reference
        result[0] = static_cast<uint8_t>('X');
        CHECK(vin[0] == static_cast<uint8_t>('T'));
    }

    TEST_CASE("toByteArray with padding") {
        DoIPVIN vin("SHORT");
        ByteArray result = vin.toByteArray();

        CHECK(result.size() == 17);
        CHECK(result[0] == 'S');
        CHECK(result[4] == 'T');
        CHECK(result[5] == '0');
        CHECK(result[16] == '0');
    }

    TEST_CASE("getArray method") {
        DoIPVIN vin("ARRAYTEST12345678");
        const auto& arr = vin.getArray();

        CHECK(arr.size() == 17);
        CHECK(arr[0] == 'A');
        CHECK(arr[16] == '8');
    }

    TEST_CASE("data method") {
        DoIPVIN vin("DATATEST123456789");
        const uint8_t* ptr = vin.data();

        CHECK(ptr != nullptr);
        CHECK(ptr[0] == 'D');
        CHECK(ptr[16] == '9');
    }

    TEST_CASE("size method") {
        DoIPVIN vin1;
        DoIPVIN vin2("SHORT");
        DoIPVIN vin3("EXACTSEVENTEENVIN");

        CHECK(vin1.size() == 17);
        CHECK(vin2.size() == 17);
        CHECK(vin3.size() == 17);
    }

    TEST_CASE("isEmpty method") {
        SUBCASE("Empty VIN") {
            DoIPVIN vin;
            CHECK(vin.isEmpty());
        }

        SUBCASE("Empty string") {
            DoIPVIN vin("");
            CHECK(vin.isEmpty());
        }

        SUBCASE("Zero instance") {
            CHECK(DoIPVIN::Zero.isEmpty());
        }

        SUBCASE("Non-empty VIN") {
            DoIPVIN vin("X");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("Full VIN") {
            DoIPVIN vin("FULLVIN1234567890");
            CHECK_FALSE(vin.isEmpty());
        }
    }

    TEST_CASE("Equality operator") {
        DoIPVIN vin1("SAMEVIN1234567890");
        DoIPVIN vin2("SAMEVIN1234567890");
        DoIPVIN vin3("DIFFVIN1234567890");

        CHECK(vin1 == vin2);
        CHECK_FALSE(vin1 == vin3);

        DoIPVIN vin4;
        DoIPVIN vin5;
        CHECK(vin4 == vin5);
        CHECK_BYTE_ARRAY_EQ(vin4.toByteArray(), DoIPVIN::Zero.toByteArray());
    }

    TEST_CASE("Inequality operator") {
        DoIPVIN vin1("VIN1_12345678901");
        DoIPVIN vin2("VIN2_12345678901");
        DoIPVIN vin3("VIN1_12345678901");

        CHECK(vin1 != vin2);
        CHECK_FALSE(vin1 != vin3);

        DoIPVIN vin4;
        CHECK(vin1 != vin4);
    }

    TEST_CASE("Array subscript operator") {
        DoIPVIN vin("SUBSCRIPT12345678");

        CHECK(vin[0] == 'S');
        CHECK(vin[1] == 'U');
        CHECK(vin[8] == 'T');
        CHECK(vin[16] == '8');
    }

    TEST_CASE("Array subscript with padding") {
        DoIPVIN vin("PAD");

        CHECK(vin[0] == 'P');
        CHECK(vin[1] == 'A');
        CHECK(vin[2] == 'D');
        CHECK(vin[3] == '0');
        CHECK(vin[16] == '0');
    }

    TEST_CASE("VIN with special characters") {
        DoIPVIN vin("VIN-WITH_SPEC.IAL");

        CHECK(vin.toString() == "VIN-WITH_SPEC.IAL");
        CHECK(vin[3] == '-');
        CHECK(vin[8] == '_');
        CHECK(vin[13] == '.');
    }

    TEST_CASE("VIN with numeric characters") {
        DoIPVIN vin("12345678901234567");

        CHECK(vin.toString() == "12345678901234567");
        CHECK(vin[0] == '1');
        CHECK(vin[16] == '7');
    }

    TEST_CASE("VIN with lowercase characters") {
        DoIPVIN vin("lowercase12345678");

        CHECK(vin.toString() == "lowercase12345678");
        CHECK(vin[0] == 'l');
    }

    TEST_CASE("VIN with mixed case") {
        DoIPVIN vin("MiXeDcAsE12345678");

        CHECK(vin.toString() == "MiXeDcAsE12345678");
    }

    TEST_CASE("Real-world VIN examples") {
        SUBCASE("Honda VIN") {
            DoIPVIN vin("1HGBH41JXMN109186");
            CHECK(vin.toString() == "1HGBH41JXMN109186");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("Volkswagen VIN") {
            DoIPVIN vin("WVWZZZ1JZYW123456");
            CHECK(vin.toString() == "WVWZZZ1JZYW123456");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("BMW VIN") {
            DoIPVIN vin("WBA3B1G59DNP26082");
            CHECK(vin.toString() == "WBA3B1G59DNP26082");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("Mercedes VIN") {
            DoIPVIN vin("WDDUG8CB9DA123456");
            CHECK(vin.toString() == "WDDUG8CB9DA123456");
            CHECK_FALSE(vin.isEmpty());
        }
    }

    TEST_CASE("Comparison between different construction methods") {
        const char* vin_str = "COMPAREVIN1234567";

        DoIPVIN vin1{std::string(vin_str)};
        DoIPVIN vin2(vin_str);

        ByteArray bytes(vin_str, vin_str + 17);
        DoIPVIN vin3(bytes);

        uint8_t byte_arr[17];
        std::copy(vin_str, vin_str + 17, byte_arr);
        DoIPVIN vin4(byte_arr, 17);

        CHECK(vin1 == vin2);
        CHECK(vin2 == vin3);
        CHECK(vin3 == vin4);
    }

    TEST_CASE("VIN conversion round-trip") {
        const std::string original = "ROUNDTRIP12345678";

        DoIPVIN vin1(original);
        std::string str = vin1.toString();
        DoIPVIN vin2(str);

        CHECK(vin1 == vin2);
        CHECK(vin2.toString() == original);
    }

    TEST_CASE("ByteArray conversion round-trip") {
        const std::string original = "BYTEARRAYTRIP1234";

        DoIPVIN vin1(original);
        ByteArray bytes = vin1.toByteArray();
        DoIPVIN vin2(bytes);

        CHECK(vin1 == vin2);
        CHECK(bytes.size() == 17);
    }

    TEST_CASE("VIN with null bytes in middle") {
        uint8_t data[17] = {'V', 'I', 'N', 0, 'N', 'U', 'L', 'L', 0, 'B', 'Y', 'T', 'E', 'S', '1', '2', '3'};
        DoIPVIN vin(data, 17);

        // toString should stop at first null byte
        CHECK(vin.toString() == "VIN");

        // But the array should contain all data
        CHECK(vin[3] == 0);
        CHECK(vin[4] == 'N');
    }

    TEST_CASE("Constant correctness") {
        const DoIPVIN vin("CONSTVIN123456789");

        CHECK(vin.toString() == "CONSTVIN123456789");
        CHECK(vin.size() == 17);
        CHECK_FALSE(vin.isEmpty());
        CHECK(vin[0] == 'C');

        const auto& arr = vin.getArray();
        CHECK(arr[0] == 'C');

        const uint8_t* ptr = vin.data();
        CHECK(ptr[0] == 'C');
    }

    TEST_CASE("DoIPEID - Entity Identifier (6 bytes)") {
        SUBCASE("Default constructor") {
            DoIPEID eid;
            CHECK(eid.isEmpty());
            CHECK(eid.size() == 6);
            CHECK(eid.toString().empty());
        }

        SUBCASE("Static Zero instance") {
            CHECK(DoIPEID::Zero.isEmpty());
            CHECK(DoIPEID::Zero.size() == 6);
        }

        SUBCASE("Construction from string - exact length") {
            DoIPEID eid("ABC123");
            CHECK(eid.toString() == "ABC123");
            CHECK(eid.size() == 6);
            CHECK_FALSE(eid.isEmpty());
        }

        SUBCASE("Construction from string - shorter") {
            DoIPEID eid("EID");
            CHECK(eid.toString() == "EID");
            CHECK(eid.size() == 6);
            CHECK(eid[0] == 'E');
            CHECK(eid[2] == 'D');
            CHECK(eid[3] == 0);
            CHECK(eid[5] == 0);
        }

        SUBCASE("Construction from string - longer") {
            DoIPEID eid("TOOLONGEID");
            CHECK(eid.toString() == "TOOLON");
            CHECK(eid.size() == 6);
        }

        SUBCASE("Construction from byte array") {
            const uint8_t bytes[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
            DoIPEID eid(bytes, sizeof(bytes));
            CHECK(eid.size() == 6);
            CHECK(eid[0] == 0x01);
            CHECK(eid[5] == 0x06);
            CHECK_FALSE(eid.isEmpty());
        }

        SUBCASE("Construction from ByteArray") {
            ByteArray bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
            DoIPEID eid(bytes);
            CHECK(eid.size() == 6);
            CHECK(eid[0] == 0xAA);
            CHECK(eid[5] == 0xFF);
        }

        SUBCASE("Equality and comparison") {
            DoIPEID eid1("EID001");
            DoIPEID eid2("EID001");
            DoIPEID eid3("EID002");

            CHECK(eid1 == eid2);
            CHECK(eid1 != eid3);
            CHECK(eid2 != eid3);
        }

        SUBCASE("toByteArray method") {
            DoIPEID eid("TEST12");
            ByteArray result = eid.toByteArray();
            CHECK(result.size() == 6);
            CHECK(result[0] == 'T');
            CHECK(result[5] == '2');
        }
    }

    TEST_CASE("DoIPGID - Group Identifier (6 bytes)") {
        SUBCASE("Default constructor") {
            DoIPGID gid;
            CHECK(gid.isEmpty());
            CHECK(gid.size() == 6);
            CHECK(gid.toString().empty());
        }

        SUBCASE("Static Zero instance") {
            CHECK(DoIPGID::Zero.isEmpty());
            CHECK(DoIPGID::Zero.size() == 6);
        }

        SUBCASE("Construction from string - exact length") {
            DoIPGID gid("GRP001");
            CHECK(gid.toString() == "GRP001");
            CHECK(gid.size() == 6);
            CHECK_FALSE(gid.isEmpty());
        }

        SUBCASE("Construction from string - shorter") {
            DoIPGID gid("GID");
            CHECK(gid.toString() == "GID");
            CHECK(gid.size() == 6);
            CHECK(gid[0] == 'G');
            CHECK(gid[2] == 'D');
            CHECK(gid[3] == 0);
            CHECK(gid[5] == 0);
        }

        SUBCASE("Construction from string - longer") {
            DoIPGID gid("TOOLONGGID");
            CHECK(gid.toString() == "TOOLON");
            CHECK(gid.size() == 6);
        }

        SUBCASE("Construction from byte array") {
            const uint8_t bytes[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
            DoIPGID gid(bytes, sizeof(bytes));
            CHECK(gid.size() == 6);
            CHECK(gid[0] == 0x10);
            CHECK(gid[5] == 0x60);
            CHECK_FALSE(gid.isEmpty());
        }

        SUBCASE("Construction from ByteArray") {
            ByteArray bytes = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
            DoIPGID gid(bytes);
            CHECK(gid.size() == 6);
            CHECK(gid[0] == 0x11);
            CHECK(gid[5] == 0x66);
        }

        SUBCASE("Equality and comparison") {
            DoIPGID gid1("GROUP1");
            DoIPGID gid2("GROUP1");
            DoIPGID gid3("GROUP2");

            CHECK(gid1 == gid2);
            CHECK(gid1 != gid3);
            CHECK(gid2 != gid3);
        }

        SUBCASE("toByteArray method") {
            DoIPGID gid("MYGRP1");
            ByteArray result = gid.toByteArray();
            CHECK(result.size() == 6);
            CHECK(result[0] == 'M');
            CHECK(result[5] == '1');
        }
    }

    TEST_CASE("Different identifier types are independent") {
        // Even though EID and GID have the same length, they're different types
        DoIPEID eid("ABC123");
        DoIPGID gid("ABC123");

        // They should have the same content but be different types
        CHECK(eid.toString() == gid.toString());
        CHECK(eid.size() == gid.size());

        // Verify they're truly independent instances
        DoIPEID eid2(eid);
        DoIPGID gid2(gid);
        CHECK(eid == eid2);
        CHECK(gid == gid2);
    }
}
