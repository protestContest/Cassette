#include "hash.h"
#include "math.h"

/* Random values to hash a byte */
static u32 byte_hash[256] = {
  0xEEAE7FDA, 0xD3395927, 0x4DD35D53, 0x30D67301, 0xBD397767, 0x16ADF05B, 0x436E841E, 0x4D4CEFEC,
  0x49BCFBB3, 0xFB7E1816, 0x5D60A901, 0xF22DDC89, 0x25CCADBE, 0x6FE4EE81, 0xEF42FD31, 0x57E385FF,
  0x3ED39511, 0xCF4CD8D4, 0x67D62B49, 0x39E8A08D, 0xCFCAF7C2, 0x2CED1929, 0x1CCCC1FB, 0x8B80EEF9,
  0x116B7BE7, 0xCCC3CF25, 0xEB5E6E0F, 0x1D6A7431, 0xCF79475C, 0x37A577DA, 0xD840F52F, 0xDD6E53B2,
  0x3D418231, 0xF7D4B6A3, 0xBAA9C2F9, 0xE63F8CB8, 0xF79FF9F0, 0xA483EC31, 0xA97B22AB, 0xB5E4B670,
  0xE742CFBD, 0xDC6A86A6, 0x8439AA08, 0x5E126F51, 0xEB8868CB, 0x4D49FF2D, 0x5A821FF7, 0x74BDA557,
  0x427EECFE, 0xA5EA7CC7, 0x617DE328, 0x263AEFB4, 0x627F69C0, 0x06FF54DC, 0x7683665C, 0x3FB86DD1,
  0xC7CF6434, 0x3F1E732F, 0xF757E2FE, 0x1CAEFF01, 0x123D1E23, 0xC14433E7, 0x5049FE4E, 0x74A8A9B0,
  0xD4F78FA1, 0x7DA2B0F8, 0xC790E436, 0x9EBD63E5, 0x8F2EDFDB, 0x8B9A5FC3, 0xDAA1E03A, 0x514F5BA7,
  0xE255EFF7, 0xCC392CF6, 0x536BEE85, 0x681B10C6, 0x8123DF1D, 0xDC7E29FA, 0x4887B4FF, 0xCC0F862C,
  0xE9B81C3B, 0x70B5F57C, 0x94FAE5E4, 0x599CAF5D, 0x69752EAE, 0x7DDD1BFD, 0x7A9F4A93, 0x0416CF76,
  0x26EA3D7B, 0x81115165, 0xBAAD1FBE, 0xA65CA2BF, 0x382AE3FB, 0xE604F959, 0xDBDC1A43, 0xFC813DF9,
  0x21C99FD9, 0x18AF57CC, 0x3777368D, 0xCFA15D62, 0x2FAE4BE8, 0x2A38BAEF, 0xF9315255, 0xAF8A3A3F,
  0xB11ADEDB, 0x9687E662, 0xFD04F32F, 0x23A901CA, 0xC9E3637C, 0x302A5D79, 0x4F864375, 0x6DB2680A,
  0x9F2CA5D3, 0xE2BA01D3, 0x7EF599F4, 0x99782F3A, 0xB4B612FD, 0xA4B4D51F, 0x0B0BC03F, 0xD616D516,
  0xBA937015, 0xB479A062, 0x524E46D2, 0xC0D991EE, 0x564A1FBF, 0x7CF3C948, 0xA4B37984, 0xAB7BD7DC,
  0x59A99CE8, 0x525D7D19, 0xFF54D6DF, 0xBF2D8A20, 0xA75AD6FD, 0x6D66A13D, 0xD299AD6E, 0x53C0FC7A,
  0x0E79357E, 0x5AD21E66, 0x58703079, 0x1EA4FFA7, 0xCFADCCD3, 0xCB66B728, 0x5ED447FE, 0xC9475BEC,
  0x6D843DBF, 0x657C76CC, 0xD9FFFAAE, 0x9B9D7839, 0x6022BFAB, 0x2C33F4D9, 0x0A76FF79, 0x12FFB354,
  0xD825C62F, 0x20F6A1DB, 0x49F36134, 0xA0ACECDD, 0xAA6100FD, 0xAEFF3E12, 0xEC39F716, 0x1859F595,
  0x4A7E112F, 0x0DB7628A, 0xC0637DFB, 0x02CA7492, 0x9E3CCC7A, 0xC5CD777E, 0x46946EF0, 0xCBFB7761,
  0x761BBDCA, 0x9BA3BDF4, 0x2AF77FAD, 0x67A2BBE3, 0xCA293C70, 0xA31829F8, 0xC96DCDBA, 0x5A181B2D,
  0xF116C9C0, 0x3FDC9E3E, 0x14EC5F93, 0x00B37CA4, 0x4EBD4A83, 0x9E8B8971, 0xF5DF633C, 0x9AFB4CDB,
  0x2F44CFB6, 0xE5DAF83B, 0x9AF13F2F, 0x835203EA, 0xB25C2EEA, 0x75071906, 0x8D3A47F7, 0x327C1BE5,
  0xEC7A5BFC, 0xC37E1F90, 0xCAFF5DCB, 0x6A9334B7, 0xBE96ED80, 0xBFA60639, 0xC1D6A1DA, 0x85855191,
  0xCDAB3C7E, 0x3D9AAB4A, 0xE16C8EA9, 0x1BDE28F5, 0xC623E578, 0x046C8D85, 0x2BCFD4F2, 0xFF7CC65D,
  0xEE343FEF, 0x3B870AF0, 0x6CF194A5, 0x1E03EEC9, 0x33FDE13D, 0x7FC6041C, 0x61CBEBFB, 0xC53E2BC2,
  0xFDDC9165, 0x028060A8, 0x2A27D908, 0x282AE930, 0x49319EC6, 0xDA9D1D49, 0xDE5E14BC, 0x07482362,
  0x5A1EE029, 0x1F9362CE, 0x9E7E10C6, 0x82E5E816, 0xED023B0C, 0x51F3A853, 0x3AB9FBA2, 0xF87647C1,
  0xA3417962, 0x9FA081C9, 0x8FE99CE9, 0xA8DD0894, 0x08585E61, 0x0F3CA1F5, 0x13A92FD5, 0x28A2B95E,
  0x5FECF8A2, 0xE7D9AF03, 0xB9296456, 0xD1439B1F, 0xBE43F197, 0xEFA69921, 0xD7EF180C, 0xBDB00401,
  0xB0FB7717, 0x26EB2FB1, 0xD3F7A737, 0xCC20DB21, 0xCE98D95A, 0xBB4FE919, 0xDBBDEE1B, 0x331E35FA,
};

/* Hashes are rotate and xor hashes of bytes, which allow them to be used as a
rolling hash */

u32 AppendHash(u32 hash, u8 byte)
{
  hash = (hash << 1) | (hash >> 31);
  return hash ^ byte_hash[byte];
}

u32 SkipHash(u32 hash, u8 byte, u32 size)
{
  u32 rotate = (size-1) % 32;
  u32 unhash = (byte_hash[byte] << rotate) | (byte_hash[byte] >> (32-rotate));
  return hash ^ unhash;
}

u32 Hash(void *data, u32 size)
{
  u32 i;
  u32 hash = 0;
  for (i = 0; i < size; i++) {
    hash = AppendHash(hash, ((u8*)data)[i]);
  }
  return hash;
}

u32 FoldHash(u32 hash, u32 bits)
{
  u32 mask = 0xFFFFFFFF >> (32 - bits);
  return (hash >> bits) ^ (hash & mask);
}

/* CRC Prime: 0x04C11DB7 */

static u32 table[] = {
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
  0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
  0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
  0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
  0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
  0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
  0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
  0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
  0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
  0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
  0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
  0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
  0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
  0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
  0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
  0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
  0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
  0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
  0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
  0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
  0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
  0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
  0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

u32 CRC32(u8 *buf, u32 len)
{
  u32 i;
  u32 crc = 0;

  crc = ~crc;
  for (i = 0; i < len; i++) crc = (crc >> 8) ^ table[(crc & 0xff) ^ buf[i]];
  return ~crc;
}

u32 VerifyCRC32(u8 *buf, u32 len)
{
  return CRC32(buf, len) == 0x2144DF1C;
}
