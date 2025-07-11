// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// the following code are modified from RocksDB:
// https://github.com/facebook/rocksdb/blob/master/util/crc32c.cc

#include "util/crc32c.h"
#ifdef __SSE4_2__
#include <nmmintrin.h>
#endif
#if defined(__ARM_NEON) && defined(__aarch64__)
#include <arm_acle.h>
#include <arm_neon.h>
#endif
#include "util/coding.h"

namespace starrocks::crc32c {

static const uint32_t table0_[256] = {
        0x00000000, 0xf26b8303, 0xe13b70f7, 0x1350f3f4, 0xc79a971f, 0x35f1141c, 0x26a1e7e8, 0xd4ca64eb, 0x8ad958cf,
        0x78b2dbcc, 0x6be22838, 0x9989ab3b, 0x4d43cfd0, 0xbf284cd3, 0xac78bf27, 0x5e133c24, 0x105ec76f, 0xe235446c,
        0xf165b798, 0x030e349b, 0xd7c45070, 0x25afd373, 0x36ff2087, 0xc494a384, 0x9a879fa0, 0x68ec1ca3, 0x7bbcef57,
        0x89d76c54, 0x5d1d08bf, 0xaf768bbc, 0xbc267848, 0x4e4dfb4b, 0x20bd8ede, 0xd2d60ddd, 0xc186fe29, 0x33ed7d2a,
        0xe72719c1, 0x154c9ac2, 0x061c6936, 0xf477ea35, 0xaa64d611, 0x580f5512, 0x4b5fa6e6, 0xb93425e5, 0x6dfe410e,
        0x9f95c20d, 0x8cc531f9, 0x7eaeb2fa, 0x30e349b1, 0xc288cab2, 0xd1d83946, 0x23b3ba45, 0xf779deae, 0x05125dad,
        0x1642ae59, 0xe4292d5a, 0xba3a117e, 0x4851927d, 0x5b016189, 0xa96ae28a, 0x7da08661, 0x8fcb0562, 0x9c9bf696,
        0x6ef07595, 0x417b1dbc, 0xb3109ebf, 0xa0406d4b, 0x522bee48, 0x86e18aa3, 0x748a09a0, 0x67dafa54, 0x95b17957,
        0xcba24573, 0x39c9c670, 0x2a993584, 0xd8f2b687, 0x0c38d26c, 0xfe53516f, 0xed03a29b, 0x1f682198, 0x5125dad3,
        0xa34e59d0, 0xb01eaa24, 0x42752927, 0x96bf4dcc, 0x64d4cecf, 0x77843d3b, 0x85efbe38, 0xdbfc821c, 0x2997011f,
        0x3ac7f2eb, 0xc8ac71e8, 0x1c661503, 0xee0d9600, 0xfd5d65f4, 0x0f36e6f7, 0x61c69362, 0x93ad1061, 0x80fde395,
        0x72966096, 0xa65c047d, 0x5437877e, 0x4767748a, 0xb50cf789, 0xeb1fcbad, 0x197448ae, 0x0a24bb5a, 0xf84f3859,
        0x2c855cb2, 0xdeeedfb1, 0xcdbe2c45, 0x3fd5af46, 0x7198540d, 0x83f3d70e, 0x90a324fa, 0x62c8a7f9, 0xb602c312,
        0x44694011, 0x5739b3e5, 0xa55230e6, 0xfb410cc2, 0x092a8fc1, 0x1a7a7c35, 0xe811ff36, 0x3cdb9bdd, 0xceb018de,
        0xdde0eb2a, 0x2f8b6829, 0x82f63b78, 0x709db87b, 0x63cd4b8f, 0x91a6c88c, 0x456cac67, 0xb7072f64, 0xa457dc90,
        0x563c5f93, 0x082f63b7, 0xfa44e0b4, 0xe9141340, 0x1b7f9043, 0xcfb5f4a8, 0x3dde77ab, 0x2e8e845f, 0xdce5075c,
        0x92a8fc17, 0x60c37f14, 0x73938ce0, 0x81f80fe3, 0x55326b08, 0xa759e80b, 0xb4091bff, 0x466298fc, 0x1871a4d8,
        0xea1a27db, 0xf94ad42f, 0x0b21572c, 0xdfeb33c7, 0x2d80b0c4, 0x3ed04330, 0xccbbc033, 0xa24bb5a6, 0x502036a5,
        0x4370c551, 0xb11b4652, 0x65d122b9, 0x97baa1ba, 0x84ea524e, 0x7681d14d, 0x2892ed69, 0xdaf96e6a, 0xc9a99d9e,
        0x3bc21e9d, 0xef087a76, 0x1d63f975, 0x0e330a81, 0xfc588982, 0xb21572c9, 0x407ef1ca, 0x532e023e, 0xa145813d,
        0x758fe5d6, 0x87e466d5, 0x94b49521, 0x66df1622, 0x38cc2a06, 0xcaa7a905, 0xd9f75af1, 0x2b9cd9f2, 0xff56bd19,
        0x0d3d3e1a, 0x1e6dcdee, 0xec064eed, 0xc38d26c4, 0x31e6a5c7, 0x22b65633, 0xd0ddd530, 0x0417b1db, 0xf67c32d8,
        0xe52cc12c, 0x1747422f, 0x49547e0b, 0xbb3ffd08, 0xa86f0efc, 0x5a048dff, 0x8ecee914, 0x7ca56a17, 0x6ff599e3,
        0x9d9e1ae0, 0xd3d3e1ab, 0x21b862a8, 0x32e8915c, 0xc083125f, 0x144976b4, 0xe622f5b7, 0xf5720643, 0x07198540,
        0x590ab964, 0xab613a67, 0xb831c993, 0x4a5a4a90, 0x9e902e7b, 0x6cfbad78, 0x7fab5e8c, 0x8dc0dd8f, 0xe330a81a,
        0x115b2b19, 0x020bd8ed, 0xf0605bee, 0x24aa3f05, 0xd6c1bc06, 0xc5914ff2, 0x37faccf1, 0x69e9f0d5, 0x9b8273d6,
        0x88d28022, 0x7ab90321, 0xae7367ca, 0x5c18e4c9, 0x4f48173d, 0xbd23943e, 0xf36e6f75, 0x0105ec76, 0x12551f82,
        0xe03e9c81, 0x34f4f86a, 0xc69f7b69, 0xd5cf889d, 0x27a40b9e, 0x79b737ba, 0x8bdcb4b9, 0x988c474d, 0x6ae7c44e,
        0xbe2da0a5, 0x4c4623a6, 0x5f16d052, 0xad7d5351};
static const uint32_t table1_[256] = {
        0x00000000, 0x13a29877, 0x274530ee, 0x34e7a899, 0x4e8a61dc, 0x5d28f9ab, 0x69cf5132, 0x7a6dc945, 0x9d14c3b8,
        0x8eb65bcf, 0xba51f356, 0xa9f36b21, 0xd39ea264, 0xc03c3a13, 0xf4db928a, 0xe7790afd, 0x3fc5f181, 0x2c6769f6,
        0x1880c16f, 0x0b225918, 0x714f905d, 0x62ed082a, 0x560aa0b3, 0x45a838c4, 0xa2d13239, 0xb173aa4e, 0x859402d7,
        0x96369aa0, 0xec5b53e5, 0xfff9cb92, 0xcb1e630b, 0xd8bcfb7c, 0x7f8be302, 0x6c297b75, 0x58ced3ec, 0x4b6c4b9b,
        0x310182de, 0x22a31aa9, 0x1644b230, 0x05e62a47, 0xe29f20ba, 0xf13db8cd, 0xc5da1054, 0xd6788823, 0xac154166,
        0xbfb7d911, 0x8b507188, 0x98f2e9ff, 0x404e1283, 0x53ec8af4, 0x670b226d, 0x74a9ba1a, 0x0ec4735f, 0x1d66eb28,
        0x298143b1, 0x3a23dbc6, 0xdd5ad13b, 0xcef8494c, 0xfa1fe1d5, 0xe9bd79a2, 0x93d0b0e7, 0x80722890, 0xb4958009,
        0xa737187e, 0xff17c604, 0xecb55e73, 0xd852f6ea, 0xcbf06e9d, 0xb19da7d8, 0xa23f3faf, 0x96d89736, 0x857a0f41,
        0x620305bc, 0x71a19dcb, 0x45463552, 0x56e4ad25, 0x2c896460, 0x3f2bfc17, 0x0bcc548e, 0x186eccf9, 0xc0d23785,
        0xd370aff2, 0xe797076b, 0xf4359f1c, 0x8e585659, 0x9dface2e, 0xa91d66b7, 0xbabffec0, 0x5dc6f43d, 0x4e646c4a,
        0x7a83c4d3, 0x69215ca4, 0x134c95e1, 0x00ee0d96, 0x3409a50f, 0x27ab3d78, 0x809c2506, 0x933ebd71, 0xa7d915e8,
        0xb47b8d9f, 0xce1644da, 0xddb4dcad, 0xe9537434, 0xfaf1ec43, 0x1d88e6be, 0x0e2a7ec9, 0x3acdd650, 0x296f4e27,
        0x53028762, 0x40a01f15, 0x7447b78c, 0x67e52ffb, 0xbf59d487, 0xacfb4cf0, 0x981ce469, 0x8bbe7c1e, 0xf1d3b55b,
        0xe2712d2c, 0xd69685b5, 0xc5341dc2, 0x224d173f, 0x31ef8f48, 0x050827d1, 0x16aabfa6, 0x6cc776e3, 0x7f65ee94,
        0x4b82460d, 0x5820de7a, 0xfbc3faf9, 0xe861628e, 0xdc86ca17, 0xcf245260, 0xb5499b25, 0xa6eb0352, 0x920cabcb,
        0x81ae33bc, 0x66d73941, 0x7575a136, 0x419209af, 0x523091d8, 0x285d589d, 0x3bffc0ea, 0x0f186873, 0x1cbaf004,
        0xc4060b78, 0xd7a4930f, 0xe3433b96, 0xf0e1a3e1, 0x8a8c6aa4, 0x992ef2d3, 0xadc95a4a, 0xbe6bc23d, 0x5912c8c0,
        0x4ab050b7, 0x7e57f82e, 0x6df56059, 0x1798a91c, 0x043a316b, 0x30dd99f2, 0x237f0185, 0x844819fb, 0x97ea818c,
        0xa30d2915, 0xb0afb162, 0xcac27827, 0xd960e050, 0xed8748c9, 0xfe25d0be, 0x195cda43, 0x0afe4234, 0x3e19eaad,
        0x2dbb72da, 0x57d6bb9f, 0x447423e8, 0x70938b71, 0x63311306, 0xbb8de87a, 0xa82f700d, 0x9cc8d894, 0x8f6a40e3,
        0xf50789a6, 0xe6a511d1, 0xd242b948, 0xc1e0213f, 0x26992bc2, 0x353bb3b5, 0x01dc1b2c, 0x127e835b, 0x68134a1e,
        0x7bb1d269, 0x4f567af0, 0x5cf4e287, 0x04d43cfd, 0x1776a48a, 0x23910c13, 0x30339464, 0x4a5e5d21, 0x59fcc556,
        0x6d1b6dcf, 0x7eb9f5b8, 0x99c0ff45, 0x8a626732, 0xbe85cfab, 0xad2757dc, 0xd74a9e99, 0xc4e806ee, 0xf00fae77,
        0xe3ad3600, 0x3b11cd7c, 0x28b3550b, 0x1c54fd92, 0x0ff665e5, 0x759baca0, 0x663934d7, 0x52de9c4e, 0x417c0439,
        0xa6050ec4, 0xb5a796b3, 0x81403e2a, 0x92e2a65d, 0xe88f6f18, 0xfb2df76f, 0xcfca5ff6, 0xdc68c781, 0x7b5fdfff,
        0x68fd4788, 0x5c1aef11, 0x4fb87766, 0x35d5be23, 0x26772654, 0x12908ecd, 0x013216ba, 0xe64b1c47, 0xf5e98430,
        0xc10e2ca9, 0xd2acb4de, 0xa8c17d9b, 0xbb63e5ec, 0x8f844d75, 0x9c26d502, 0x449a2e7e, 0x5738b609, 0x63df1e90,
        0x707d86e7, 0x0a104fa2, 0x19b2d7d5, 0x2d557f4c, 0x3ef7e73b, 0xd98eedc6, 0xca2c75b1, 0xfecbdd28, 0xed69455f,
        0x97048c1a, 0x84a6146d, 0xb041bcf4, 0xa3e32483};
static const uint32_t table2_[256] = {
        0x00000000, 0xa541927e, 0x4f6f520d, 0xea2ec073, 0x9edea41a, 0x3b9f3664, 0xd1b1f617, 0x74f06469, 0x38513ec5,
        0x9d10acbb, 0x773e6cc8, 0xd27ffeb6, 0xa68f9adf, 0x03ce08a1, 0xe9e0c8d2, 0x4ca15aac, 0x70a27d8a, 0xd5e3eff4,
        0x3fcd2f87, 0x9a8cbdf9, 0xee7cd990, 0x4b3d4bee, 0xa1138b9d, 0x045219e3, 0x48f3434f, 0xedb2d131, 0x079c1142,
        0xa2dd833c, 0xd62de755, 0x736c752b, 0x9942b558, 0x3c032726, 0xe144fb14, 0x4405696a, 0xae2ba919, 0x0b6a3b67,
        0x7f9a5f0e, 0xdadbcd70, 0x30f50d03, 0x95b49f7d, 0xd915c5d1, 0x7c5457af, 0x967a97dc, 0x333b05a2, 0x47cb61cb,
        0xe28af3b5, 0x08a433c6, 0xade5a1b8, 0x91e6869e, 0x34a714e0, 0xde89d493, 0x7bc846ed, 0x0f382284, 0xaa79b0fa,
        0x40577089, 0xe516e2f7, 0xa9b7b85b, 0x0cf62a25, 0xe6d8ea56, 0x43997828, 0x37691c41, 0x92288e3f, 0x78064e4c,
        0xdd47dc32, 0xc76580d9, 0x622412a7, 0x880ad2d4, 0x2d4b40aa, 0x59bb24c3, 0xfcfab6bd, 0x16d476ce, 0xb395e4b0,
        0xff34be1c, 0x5a752c62, 0xb05bec11, 0x151a7e6f, 0x61ea1a06, 0xc4ab8878, 0x2e85480b, 0x8bc4da75, 0xb7c7fd53,
        0x12866f2d, 0xf8a8af5e, 0x5de93d20, 0x29195949, 0x8c58cb37, 0x66760b44, 0xc337993a, 0x8f96c396, 0x2ad751e8,
        0xc0f9919b, 0x65b803e5, 0x1148678c, 0xb409f5f2, 0x5e273581, 0xfb66a7ff, 0x26217bcd, 0x8360e9b3, 0x694e29c0,
        0xcc0fbbbe, 0xb8ffdfd7, 0x1dbe4da9, 0xf7908dda, 0x52d11fa4, 0x1e704508, 0xbb31d776, 0x511f1705, 0xf45e857b,
        0x80aee112, 0x25ef736c, 0xcfc1b31f, 0x6a802161, 0x56830647, 0xf3c29439, 0x19ec544a, 0xbcadc634, 0xc85da25d,
        0x6d1c3023, 0x8732f050, 0x2273622e, 0x6ed23882, 0xcb93aafc, 0x21bd6a8f, 0x84fcf8f1, 0xf00c9c98, 0x554d0ee6,
        0xbf63ce95, 0x1a225ceb, 0x8b277743, 0x2e66e53d, 0xc448254e, 0x6109b730, 0x15f9d359, 0xb0b84127, 0x5a968154,
        0xffd7132a, 0xb3764986, 0x1637dbf8, 0xfc191b8b, 0x595889f5, 0x2da8ed9c, 0x88e97fe2, 0x62c7bf91, 0xc7862def,
        0xfb850ac9, 0x5ec498b7, 0xb4ea58c4, 0x11abcaba, 0x655baed3, 0xc01a3cad, 0x2a34fcde, 0x8f756ea0, 0xc3d4340c,
        0x6695a672, 0x8cbb6601, 0x29faf47f, 0x5d0a9016, 0xf84b0268, 0x1265c21b, 0xb7245065, 0x6a638c57, 0xcf221e29,
        0x250cde5a, 0x804d4c24, 0xf4bd284d, 0x51fcba33, 0xbbd27a40, 0x1e93e83e, 0x5232b292, 0xf77320ec, 0x1d5de09f,
        0xb81c72e1, 0xccec1688, 0x69ad84f6, 0x83834485, 0x26c2d6fb, 0x1ac1f1dd, 0xbf8063a3, 0x55aea3d0, 0xf0ef31ae,
        0x841f55c7, 0x215ec7b9, 0xcb7007ca, 0x6e3195b4, 0x2290cf18, 0x87d15d66, 0x6dff9d15, 0xc8be0f6b, 0xbc4e6b02,
        0x190ff97c, 0xf321390f, 0x5660ab71, 0x4c42f79a, 0xe90365e4, 0x032da597, 0xa66c37e9, 0xd29c5380, 0x77ddc1fe,
        0x9df3018d, 0x38b293f3, 0x7413c95f, 0xd1525b21, 0x3b7c9b52, 0x9e3d092c, 0xeacd6d45, 0x4f8cff3b, 0xa5a23f48,
        0x00e3ad36, 0x3ce08a10, 0x99a1186e, 0x738fd81d, 0xd6ce4a63, 0xa23e2e0a, 0x077fbc74, 0xed517c07, 0x4810ee79,
        0x04b1b4d5, 0xa1f026ab, 0x4bdee6d8, 0xee9f74a6, 0x9a6f10cf, 0x3f2e82b1, 0xd50042c2, 0x7041d0bc, 0xad060c8e,
        0x08479ef0, 0xe2695e83, 0x4728ccfd, 0x33d8a894, 0x96993aea, 0x7cb7fa99, 0xd9f668e7, 0x9557324b, 0x3016a035,
        0xda386046, 0x7f79f238, 0x0b899651, 0xaec8042f, 0x44e6c45c, 0xe1a75622, 0xdda47104, 0x78e5e37a, 0x92cb2309,
        0x378ab177, 0x437ad51e, 0xe63b4760, 0x0c158713, 0xa954156d, 0xe5f54fc1, 0x40b4ddbf, 0xaa9a1dcc, 0x0fdb8fb2,
        0x7b2bebdb, 0xde6a79a5, 0x3444b9d6, 0x91052ba8};
static const uint32_t table3_[256] = {
        0x00000000, 0xdd45aab8, 0xbf672381, 0x62228939, 0x7b2231f3, 0xa6679b4b, 0xc4451272, 0x1900b8ca, 0xf64463e6,
        0x2b01c95e, 0x49234067, 0x9466eadf, 0x8d665215, 0x5023f8ad, 0x32017194, 0xef44db2c, 0xe964b13d, 0x34211b85,
        0x560392bc, 0x8b463804, 0x924680ce, 0x4f032a76, 0x2d21a34f, 0xf06409f7, 0x1f20d2db, 0xc2657863, 0xa047f15a,
        0x7d025be2, 0x6402e328, 0xb9474990, 0xdb65c0a9, 0x06206a11, 0xd725148b, 0x0a60be33, 0x6842370a, 0xb5079db2,
        0xac072578, 0x71428fc0, 0x136006f9, 0xce25ac41, 0x2161776d, 0xfc24ddd5, 0x9e0654ec, 0x4343fe54, 0x5a43469e,
        0x8706ec26, 0xe524651f, 0x3861cfa7, 0x3e41a5b6, 0xe3040f0e, 0x81268637, 0x5c632c8f, 0x45639445, 0x98263efd,
        0xfa04b7c4, 0x27411d7c, 0xc805c650, 0x15406ce8, 0x7762e5d1, 0xaa274f69, 0xb327f7a3, 0x6e625d1b, 0x0c40d422,
        0xd1057e9a, 0xaba65fe7, 0x76e3f55f, 0x14c17c66, 0xc984d6de, 0xd0846e14, 0x0dc1c4ac, 0x6fe34d95, 0xb2a6e72d,
        0x5de23c01, 0x80a796b9, 0xe2851f80, 0x3fc0b538, 0x26c00df2, 0xfb85a74a, 0x99a72e73, 0x44e284cb, 0x42c2eeda,
        0x9f874462, 0xfda5cd5b, 0x20e067e3, 0x39e0df29, 0xe4a57591, 0x8687fca8, 0x5bc25610, 0xb4868d3c, 0x69c32784,
        0x0be1aebd, 0xd6a40405, 0xcfa4bccf, 0x12e11677, 0x70c39f4e, 0xad8635f6, 0x7c834b6c, 0xa1c6e1d4, 0xc3e468ed,
        0x1ea1c255, 0x07a17a9f, 0xdae4d027, 0xb8c6591e, 0x6583f3a6, 0x8ac7288a, 0x57828232, 0x35a00b0b, 0xe8e5a1b3,
        0xf1e51979, 0x2ca0b3c1, 0x4e823af8, 0x93c79040, 0x95e7fa51, 0x48a250e9, 0x2a80d9d0, 0xf7c57368, 0xeec5cba2,
        0x3380611a, 0x51a2e823, 0x8ce7429b, 0x63a399b7, 0xbee6330f, 0xdcc4ba36, 0x0181108e, 0x1881a844, 0xc5c402fc,
        0xa7e68bc5, 0x7aa3217d, 0x52a0c93f, 0x8fe56387, 0xedc7eabe, 0x30824006, 0x2982f8cc, 0xf4c75274, 0x96e5db4d,
        0x4ba071f5, 0xa4e4aad9, 0x79a10061, 0x1b838958, 0xc6c623e0, 0xdfc69b2a, 0x02833192, 0x60a1b8ab, 0xbde41213,
        0xbbc47802, 0x6681d2ba, 0x04a35b83, 0xd9e6f13b, 0xc0e649f1, 0x1da3e349, 0x7f816a70, 0xa2c4c0c8, 0x4d801be4,
        0x90c5b15c, 0xf2e73865, 0x2fa292dd, 0x36a22a17, 0xebe780af, 0x89c50996, 0x5480a32e, 0x8585ddb4, 0x58c0770c,
        0x3ae2fe35, 0xe7a7548d, 0xfea7ec47, 0x23e246ff, 0x41c0cfc6, 0x9c85657e, 0x73c1be52, 0xae8414ea, 0xcca69dd3,
        0x11e3376b, 0x08e38fa1, 0xd5a62519, 0xb784ac20, 0x6ac10698, 0x6ce16c89, 0xb1a4c631, 0xd3864f08, 0x0ec3e5b0,
        0x17c35d7a, 0xca86f7c2, 0xa8a47efb, 0x75e1d443, 0x9aa50f6f, 0x47e0a5d7, 0x25c22cee, 0xf8878656, 0xe1873e9c,
        0x3cc29424, 0x5ee01d1d, 0x83a5b7a5, 0xf90696d8, 0x24433c60, 0x4661b559, 0x9b241fe1, 0x8224a72b, 0x5f610d93,
        0x3d4384aa, 0xe0062e12, 0x0f42f53e, 0xd2075f86, 0xb025d6bf, 0x6d607c07, 0x7460c4cd, 0xa9256e75, 0xcb07e74c,
        0x16424df4, 0x106227e5, 0xcd278d5d, 0xaf050464, 0x7240aedc, 0x6b401616, 0xb605bcae, 0xd4273597, 0x09629f2f,
        0xe6264403, 0x3b63eebb, 0x59416782, 0x8404cd3a, 0x9d0475f0, 0x4041df48, 0x22635671, 0xff26fcc9, 0x2e238253,
        0xf36628eb, 0x9144a1d2, 0x4c010b6a, 0x5501b3a0, 0x88441918, 0xea669021, 0x37233a99, 0xd867e1b5, 0x05224b0d,
        0x6700c234, 0xba45688c, 0xa345d046, 0x7e007afe, 0x1c22f3c7, 0xc167597f, 0xc747336e, 0x1a0299d6, 0x782010ef,
        0xa565ba57, 0xbc65029d, 0x6120a825, 0x0302211c, 0xde478ba4, 0x31035088, 0xec46fa30, 0x8e647309, 0x5321d9b1,
        0x4a21617b, 0x9764cbc3, 0xf54642fa, 0x2803e842};

// Used to fetch a naturally-aligned 32-bit word in little endian byte-order
static inline uint32_t LE_LOAD32(const uint8_t* p) {
    return decode_fixed32_le(p);
}

#if defined(__SSE4_2__) && (defined(__LP64__) || defined(_WIN64))
static inline uint64_t LE_LOAD64(const uint8_t* p) {
    return decode_fixed64_le(p);
}
#endif

[[maybe_unused]] static inline void Slow_CRC32(uint64_t* l, uint8_t const** p) {
    auto c = static_cast<uint32_t>(*l ^ LE_LOAD32(*p));
    *p += 4;
    *l = table3_[c & 0xff] ^ table2_[(c >> 8) & 0xff] ^ table1_[(c >> 16) & 0xff] ^ table0_[c >> 24];
    // DO it twice.
    c = static_cast<uint32_t>(*l ^ LE_LOAD32(*p));
    *p += 4;
    *l = table3_[c & 0xff] ^ table2_[(c >> 8) & 0xff] ^ table1_[(c >> 16) & 0xff] ^ table0_[c >> 24];
}

static inline void Fast_CRC32(uint64_t* l, uint8_t const** p) {
#ifndef __SSE4_2__
#if defined(__ARM_NEON) && defined(__aarch64__)
    *l = __crc32cw(static_cast<unsigned int>(*l), LE_LOAD32(*p));
    *p += 4;
    *l = __crc32cw(static_cast<unsigned int>(*l), LE_LOAD32(*p));
    *p += 4;
#else
    Slow_CRC32(l, p);
#endif // defined(__ARM_NEON) && defined(__aarch64__)
#elif defined(__LP64__) || defined(_WIN64)
    *l = _mm_crc32_u64(*l, LE_LOAD64(*p));
    *p += 8;
#else
    *l = _mm_crc32_u32(static_cast<unsigned int>(*l), LE_LOAD32(*p));
    *p += 4;
    *l = _mm_crc32_u32(static_cast<unsigned int>(*l), LE_LOAD32(*p));
    *p += 4;
#endif
}

template <void (*CRC32)(uint64_t*, uint8_t const**)>
uint32_t ExtendImpl(uint32_t crc, const char* buf, size_t size) {
    const auto* p = reinterpret_cast<const uint8_t*>(buf);
    const uint8_t* e = p + size;
    uint64_t l = crc ^ 0xffffffffu;

// Align n to (1 << m) byte boundary
#define ALIGN(n, m) ((n + ((1 << m) - 1)) & ~((1 << m) - 1))

#define STEP1                      \
    do {                           \
        int c = (l & 0xff) ^ *p++; \
        l = table0_[c] ^ (l >> 8); \
    } while (0)

    // Point x at first 16-byte aligned byte in string.  This might be
    // just past the end of the string.
    const auto pval = reinterpret_cast<uintptr_t>(p);
    const auto* x = reinterpret_cast<const uint8_t*>(ALIGN(pval, 4)); // NOLINT
    if (x <= e) {
        // Process bytes until finished or p is 16-byte aligned
        while (p != x) {
            STEP1;
        }
    }
    // Process bytes 16 at a time
    while ((e - p) >= 16) {
        CRC32(&l, &p);
        CRC32(&l, &p);
    }
    // Process bytes 8 at a time
    while ((e - p) >= 8) {
        CRC32(&l, &p);
    }
    // Process the last few bytes
    while (p != e) {
        STEP1;
    }
#undef STEP1
#undef ALIGN
    return static_cast<uint32_t>(l ^ 0xffffffffu);
}

#if defined(__SSE4_2__) && defined(__PCLMUL__)
uint32_t crc32c_sse42_simd(uint32_t crc, const char* buf, size_t len);
#endif

uint32_t Extend(uint32_t crc, const char* buf, size_t size) {
#if defined(__SSE4_2__) && defined(__PCLMUL__)
    constexpr size_t CRC32C_SSE42_CHUNKSIZE_MASK = ((1 << 4) - 1);
    constexpr size_t CRC32C_SSE42_MINIMUM_LENGTH = (1 << 6);
    if (size >= CRC32C_SSE42_MINIMUM_LENGTH) {
        size_t chunk_size = size & ~CRC32C_SSE42_CHUNKSIZE_MASK;
        size &= CRC32C_SSE42_CHUNKSIZE_MASK;
        crc = ~crc32c_sse42_simd(~crc, buf, chunk_size);
        if (!size) return crc;
        buf += chunk_size;
    }
#endif

    return ExtendImpl<Fast_CRC32>(crc, buf, size);
}

} // namespace starrocks::crc32c
