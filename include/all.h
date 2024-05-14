

#define USEC_PER_SEC  1000000
#define USEC_PER_MSEC 1000

/*
  libgeneral: https://github.com/tihmstar/libgeneral
*/
#define cassure(a) do{ if ((a) == 0){err=__LINE__; goto error;} }while(0)
#define cretassure(cond, errstr ...) do{ if ((cond) == 0){err=__LINE__;error(LIBGENERAL_ERRSTR(errstr)); goto error;} }while(0)
#define creterror(errstr ...) do{error(LIBGENERAL_ERRSTR(errstr));err=__LINE__; goto error; }while(0)

#define MINI_VOLTAGE_CTRL_PIN 29

/*
  if optional solder says "PICO_CS" -> rev2
  if optional solder sys "PICO_CLK" -> rev3
*/
// #define REV2_LAYOUT 