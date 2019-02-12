#include <stdio.h>
enum pac {REGISTER_REQ=0x00, REGISTER_ACK=0x01,
          REGISTER_NACK=0x02, REGISTER_REJ=0x03,
          ERROR=0x09,
          
          ALIVE_INF=0x10, ALIVE_ACK=0x11,
          ALIVE_NACK=0x12, ALIVE_REJ=0x13,
          
          SEND_FILE=0x20, SEND_ACK=0x21,
          SEND_NACK=0x22, SEND_REJ=0x23,
          SEND_DATA=0x24, SEND_END=0x25,
          
          GET_FILE=0x30, GET_ACK=0x31,
          GET_NACK=0x32, GET_REJ=0x33,
          GET_DATA=0x34, GET_END=0x35};

enum states {DISCONNECTED, WAIT_REG, REGISTERED,
             ALIVE};