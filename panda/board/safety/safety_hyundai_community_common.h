
int mdps12_checksum = -1;
int mdps12_count = 0;
int last_str_col_tq = 0;

void hyundai_mdps12_checksum(CANPacket_t *to_push) {

  int addr = GET_ADDR(to_push);

  if ((addr == 593) && (mdps12_checksum == -1)){
    int checksum_new = 0;
    uint8_t dat[8];
    for (int i=0; i<8; i++) {
      dat[i] = GET_BYTE(to_push, i);
    }
    int checksum = dat[3];
    dat[3] = 0;
    for (int i=0; i<8; i++) {
      checksum_new += dat[i];
    }
    checksum_new %= 256;
    if (checksum == checksum_new) {
      mdps12_checksum = 0;
    } else {
      mdps12_checksum = 1;
    }
  }
  
}

void hyundai_fwd_mdps12(CANPacket_t *to_fwd) {
  int addr = GET_ADDR(to_fwd);

  // Code for LKA/LFA/HDA anti-nagging.
  if (addr == 593) {
    uint8_t dat[8];
    int checksum_new = 0;
    for (int i=0; i<8; i++) {
      dat[i] = GET_BYTE(to_fwd, i);
    }
    if (mdps12_count > 330) {
      int StrColTq = dat[0] | (dat[1] & 0x7) << 8;
      int OutTq = dat[6] >> 4 | dat[7] << 4;
      if (mdps12_count == 331) {
        StrColTq -= 164;
      } else {
        StrColTq = last_str_col_tq + 34;
      }
      OutTq = 2058;
      dat[0] = StrColTq & 0xFF;
      dat[1] &= 0xF8;
      dat[1] |= StrColTq >> 8;
      dat[6] &= 0xF;
      dat[6] |= (OutTq & 0xF) << 4;
      dat[7] = OutTq >> 4;

      uint32_t* RDLR = (uint32_t*)&(to_fwd->data[0]);
      uint32_t* RDHR = (uint32_t*)&(to_fwd->data[4]);

      *RDLR &= 0xFFF800;
      *RDLR |= StrColTq;
      *RDHR &= 0xFFFFF;
      *RDHR |= OutTq << 20;

      last_str_col_tq = StrColTq;
      dat[3] = 0;
      if (!mdps12_checksum) {
        for (int i=0; i<8; i++) {
          checksum_new += dat[i];
        }
        checksum_new %= 256;
      } else if (mdps12_checksum) {
        uint8_t crc = 0xFF;
        uint8_t poly = 0x1D;
        int i, j;
        for (i=0; i<8; i++){
          if (i!=3){ //don't include CRC byte
            crc ^= dat[i];
            for (j=0; j<8; j++) {
              if ((crc & 0x80) != 0U) {
                crc = (crc << 1) ^ poly;
              } else {
                crc <<= 1;
              }
            }
          }
        }
        crc ^= 0xFF;
        crc %= 256;
        checksum_new = crc;
      }
      *RDLR |= checksum_new << 24;
    }
    mdps12_count += 1;
    mdps12_count %= 345;
  }
}
