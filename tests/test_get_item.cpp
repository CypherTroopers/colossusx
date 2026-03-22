#include "core/get_item.h"
#include "tests/test_helpers.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {
using colossusx::DatasetItem;
using colossusx::Hash256;
using colossusx::Profile;

Hash256 MakeSeed(std::uint8_t start) {
  Hash256 seed{};
  for (std::size_t i = 0; i < seed.size(); ++i) {
    seed[i] = static_cast<std::uint8_t>(start + i);
  }
  return seed;
}

std::string ItemHex(const DatasetItem& item) {
  std::ostringstream stream;
  stream << std::hex << std::setfill('0');
  for (std::uint8_t byte : item) {
    stream << std::setw(2) << static_cast<unsigned>(byte);
  }
  return stream.str();
}

void TestItemCountMatchesProfileGeometry() {
  EXPECT_EQ(50331648ULL, colossusx::dataset_item_count(Profile::kCx18));
  EXPECT_EQ(100663296ULL, colossusx::dataset_item_count(Profile::kCx32));
}

void TestRequiredItemVectors() {
  const Hash256 seed = MakeSeed(0x55U);
  EXPECT_EQ(std::string("3f8a7af31f2cbf29a5943538e5560ae3236800acb096917715fa69b35bd18a5748046b5bd63a283166501aa213cd39a54add5689ab2bf1da104fd4ba7d07c4a593ea2bed505159d987c0387f30f806812b32083d3039adccb15d39bace801c9c7296f8e89f2b4e890702501343314e765d9ac9cae2c4f95077738e8456fefbd409a416c55a93910da3f5ec625ce93c83beba3689821b58880ff75a926ba0013a0da4f61d927c377adccc2021a30883aab9e05aaa1b26ffccb8447131b3fa9e4b71640beb5005ab00cdaee4e6a31753e52c72853890082d830f587a116bc8da979d49c9212119334c6065694b1a9891b7970679f59edcdf3f7f33b80a3d6f65d4"),
            ItemHex(colossusx::get_item(Profile::kCx18, seed, 0)));
  EXPECT_EQ(std::string("5eb9ed2d9041a843ef3e7a06a0c308e56942efb2af794c8aa5e46aa96a5d5bc50dc6bf49cc9acf8c9f4178ab27690e70b5a2c67d4d4ab81508ba08f8a8b7b7704a971b480d372ba0f6118f16d9de1216facc0ee04ebe53c7d3c11ac2a6a61a72df73e7078fe8a8e7949237bb1d87ef634dd19737713a02ff40158088596fac6571efced40d69113e4e055f548a26c40b644290be3dad5c50ded98a795799cccf3743b173c5661f6d676e6859df59b315c0f41bbf1e70d44875f810236ffc8e771301e6a02fbbbf45f8d0cebe704a6bb2a0cff176fdb4a9fc16f93bf068d0f0749d04386ee722c67f9c63e84721c24836a00d727b5c0692f995c89d597e458d3d"),
            ItemHex(colossusx::get_item(Profile::kCx18, seed, 17)));
  EXPECT_EQ(std::string("1b880c89f1b79768ba12b6de1d5af8df5b3e698be3677c07a02091c327e183ba8baea293812bcdb65aa42ef622b2720fe6d3d63e310a6d33391e36cd918f7a1e5d7afd5ebd2adacfb0cf08846a6c17f6e3faaa5efd33586fc39cdb0b31a9d3cfde3cc0fb694cf4bcbd7bd373ad3b62d49f23cbb2f00f258ca263eef9266a68b7be8313b1ddaeac0d3a36e4622dfd10d4f937206679cd7af5dce7170012e9f75fb256b2b19a906a73a775c9db8b50d0e9c28130c47c2c9bd7a4ace37246fa2d0dba64552b46d9048d207bb7ff5120e4c2cae3c09fb024ff672e45d7e5b7c35b38f1d074d9938ce3bcc7c4cbefd882b1557cfb8f68247cd4a0ba34dcb8ba772094"),
            ItemHex(colossusx::get_item(Profile::kCx18, seed, colossusx::dataset_item_count(Profile::kCx18) / 2ULL)));
  EXPECT_EQ(std::string("e93482ee38e545db28dcf856e153c00c5d7d78c07df4b605d5d02b1684fd77f1a69e39d1ff50cef5b0be165a4ae24577e377be2334744e68bbd1afdfe5cd22dc5a6216490a4ede88252876d7e5b193aae1fbb4585d53ac7261ce856fa61930fd1084749b3c2f8211a6da7cc141e8e2d7b96ac85b80f9684049b78dc55e8f22af516671101e0cb0b085f9a2f337c8b86239f17dcce768e58083fc09e357255500a36ae59ca6a0356d5913df5e29ee8fdc523de44735a52c58265a78faea9ccabbae16c8ab95aa00e0c839cb3ca688d02a93a6c3c3b3dbe0e6b3c51bf8dce8f4018146bbcb5f755ce8c53fe47f2ea4b0802d685bafac11b871aa599c54cc9acae0"),
            ItemHex(colossusx::get_item(Profile::kCx18, seed, colossusx::dataset_item_count(Profile::kCx18) - 1ULL)));
}

void TestGetItemDeterminismAndProfileSeparation() {
  const Hash256 seed = MakeSeed(0x70U);
  const DatasetItem first = colossusx::get_item(Profile::kCx18, seed, 123456);
  EXPECT_EQ(ItemHex(first), ItemHex(colossusx::get_item(Profile::kCx18, seed, 123456)));
  EXPECT_TRUE(ItemHex(first) != ItemHex(colossusx::get_item(Profile::kCx18, seed, 123457)));
  EXPECT_TRUE(ItemHex(first) != ItemHex(colossusx::get_item(Profile::kCx32, seed, 123456)));
}

void RunGetItemTests() {
  TestItemCountMatchesProfileGeometry();
  TestRequiredItemVectors();
  TestGetItemDeterminismAndProfileSeparation();
}

}  // namespace

void run_get_item_tests() { RunGetItemTests(); }
