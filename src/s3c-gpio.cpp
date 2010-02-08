/*
    GPIO machine specific interface
    Copyright (C) 2009 Tanguy Pruvot

    For conditions of use see file COPYING
*/

#include <time.h>

#include "xtypes.h"
#include "s3c-gpio.h"
#include "memory.h"
#include "output.h"
#include "script.h" // REG_VAR_BITSET
#include "arch-s3.h" // S3Cx4xx

// Which GPIO changes to ignore during watch
//uint32 s3c_gpioIgnore [3] = { 0, 0, 0 };

//REG_VAR_BITSET(testS3C64xx, "IGPIO", s3c_gpioIgnore, 187
//               , "The list of GPIOs to ignore during WGPIO")

//bank, first gpio n, con addr (base), con padding, data reg addr, pull up/down addr, sleep con, sleep pull up/down
#define S3C_BANK_NAME   0
#define S3C_BANK_GPIO   1
#define S3C_BANK_COUNT  2
#define S3C_BANK_CONREG 3
#define S3C_BANK_CONSZ  4
#define S3C_BANK_CONPAD 5
#define S3C_BANK_DATREG 6
#define S3C_BANK_PUDREG 7
#define S3C_BANK_SCONREG 8
#define S3C_BANK_SPUDREG 9

#define S3C_BANK_COLS 10

const uint32 S3C6410_BANKS[S3C_BANK_COLS * 17] = {
'A', S3C64XX_GPA(0), S3C64XX_GPIO_A_NR, S3C64XX_GPACON,  1, 4, S3C64XX_GPADAT, S3C64XX_GPAPUD, S3C64XX_GPACONSLP, S3C64XX_GPAPUDSLP,
'B', S3C64XX_GPB(0), S3C64XX_GPIO_B_NR, S3C64XX_GPBCON,  1, 4, S3C64XX_GPBDAT, S3C64XX_GPBPUD, S3C64XX_GPBCONSLP, S3C64XX_GPBPUDSLP,
'C', S3C64XX_GPC(0), S3C64XX_GPIO_C_NR, S3C64XX_GPCCON,  1, 4, S3C64XX_GPCDAT, S3C64XX_GPCPUD, S3C64XX_GPCCONSLP, S3C64XX_GPCPUDSLP,
'D', S3C64XX_GPD(0), S3C64XX_GPIO_D_NR, S3C64XX_GPDCON,  1, 4, S3C64XX_GPDDAT, S3C64XX_GPDPUD, S3C64XX_GPDCONSLP, S3C64XX_GPDPUDSLP,
'E', S3C64XX_GPE(0), S3C64XX_GPIO_E_NR, S3C64XX_GPECON,  1, 4, S3C64XX_GPEDAT, S3C64XX_GPEPUD, S3C64XX_GPECONSLP, S3C64XX_GPEPUDSLP,
'F', S3C64XX_GPF(0), S3C64XX_GPIO_F_NR, S3C64XX_GPFCON,  1, 2, S3C64XX_GPFDAT, S3C64XX_GPFPUD, S3C64XX_GPFCONSLP, S3C64XX_GPFPUDSLP,
'G', S3C64XX_GPG(0), S3C64XX_GPIO_G_NR, S3C64XX_GPGCON,  1, 4, S3C64XX_GPGDAT, S3C64XX_GPGPUD, S3C64XX_GPGCONSLP, S3C64XX_GPGPUDSLP,
'H', S3C64XX_GPH(0), S3C64XX_GPIO_H_NR, S3C64XX_GPHCON0, 2, 4, S3C64XX_GPHDAT, S3C64XX_GPHPUD, S3C64XX_GPHCONSLP, S3C64XX_GPHPUDSLP,
'I', S3C64XX_GPI(0), S3C64XX_GPIO_I_NR, S3C64XX_GPICON,  1, 2, S3C64XX_GPIDAT, S3C64XX_GPIPUD, S3C64XX_GPICONSLP, S3C64XX_GPIPUDSLP,
'J', S3C64XX_GPJ(0), S3C64XX_GPIO_J_NR, S3C64XX_GPJCON,  1, 2, S3C64XX_GPJDAT, S3C64XX_GPJPUD, S3C64XX_GPJCONSLP, S3C64XX_GPJPUDSLP,
'K', S3C64XX_GPK(0), S3C64XX_GPIO_K_NR, S3C64XX_GPKCON0, 2, 4, S3C64XX_GPKDAT, S3C64XX_GPKPUD, 0, 0,
'L', S3C64XX_GPL(0), S3C64XX_GPIO_L_NR, S3C64XX_GPLCON0, 2, 4, S3C64XX_GPLDAT, S3C64XX_GPLPUD, 0, 0,
'M', S3C64XX_GPM(0), S3C64XX_GPIO_M_NR, S3C64XX_GPMCON,  1, 4, S3C64XX_GPMDAT, S3C64XX_GPMPUD, 0, 0,
'N', S3C64XX_GPN(0), S3C64XX_GPIO_N_NR, S3C64XX_GPNCON,  1, 2, S3C64XX_GPNDAT, S3C64XX_GPNPUD, 0, 0,
'O', S3C64XX_GPO(0), S3C64XX_GPIO_O_NR, S3C64XX_GPOCON,  1, 2, S3C64XX_GPODAT, S3C64XX_GPOPUD, S3C64XX_GPOCONSLP, S3C64XX_GPOPUDSLP,
'P', S3C64XX_GPP(0), S3C64XX_GPIO_P_NR, S3C64XX_GPPCON,  1, 2, S3C64XX_GPPDAT, S3C64XX_GPPPUD, S3C64XX_GPPCONSLP, S3C64XX_GPPPUDSLP,
'Q', S3C64XX_GPQ(0), S3C64XX_GPIO_Q_NR, S3C64XX_GPQCON,  1, 2, S3C64XX_GPQDAT, S3C64XX_GPQPUD, S3C64XX_GPQCONSLP, S3C64XX_GPQPUDSLP,
};
           
/*
I: Input O: Output r: Reserved X: Ext Interrupt ' ': Pin specific, see doc..
*/

const unsigned char S3C6410_BANKS_STATES[8 * 17] = {
/*     0    1    2    3     4    5    6    7 */
/*A*/ 'I', 'O', ' ', 'r',  'r', 'r', 'r', 'X', //2:UART
/*B*/ 'I', 'O', ' ', ' ',  ' ', ' ', ' ', 'X', //2-4:UART/IrDA/CF/Ext DMA 5:CF 6:I²C
/*C*/ 'I', 'O', ' ', ' ',  'r', ' ', 'r', 'X', //2:SPI 3:MMC 5:I2S
/*D*/ 'I', 'O', ' ', ' ',  ' ', 'r', 'r', 'X', //2:PCM 3:I2S 4:AC97
/*E*/ 'I', 'O', ' ', ' ',  ' ', 'r', 'r', 'r', //2:PCM 3:I2S 4:AC97 
/*F*/ 'I', 'O', ' ', 'X',  '-', '-', '-', '-', //2:CAMIF/PWM 3:X/CLKOUT
/*G*/ 'I', 'O', ' ', ' ',  'r', 'r', 'r', 'X', //2:MMC 3:MMC
/*H*/ 'I', 'O', ' ', ' ',  ' ', ' ', ' ', 'X', //3: 4: 5: 6:
/*I*/ 'I', 'O', ' ', 'r',  '-', '-', '-', '-', //2:LCD VD
/*J*/ 'I', 'O', ' ', 'r',  '-', '-', '-', '-', //2:LCD    
/*K*/ 'I', 'O', ' ', ' ',  'r', ' ', 'r', 'r', //2:HostIF 3: 5:CF
/*L*/ 'I', 'O', ' ', ' ',  'r', ' ', ' ', 'r', //2:HostIF 3:Keypad Col/Ext Irq 5: 6:
/*M*/ 'I', 'O', ' ', ' ',  ' ', 'r', ' ', 'r',
/*N*/ 'I', 'O', 'X', ' ',  '-', '-', '-', '-', //3:Keypad Rows (input)
/*O*/ 'I', 'O', ' ', 'X',  '-', '-', '-', '-', //2:MEM0
/*P*/ 'I', 'O', ' ', 'X',  '-', '-', '-', '-', //2:MEM0
/*Q*/ 'I', 'O', ' ', 'X',  '-', '-', '-', '-', //2:MEM0 
};
        
//Sample: get 'A' for 0<=num<=7, 'B' for 8<=num<=14 
uint32 s3c_int2bank(uint32 num)
{
  if (num >= S3C64XX_GPIO_END)
        return 0;
  
  unsigned int B;
  for (int reg=17-1; reg>=0; reg--)
  {
        B=reg*S3C_BANK_COLS;
        if (num >= S3C6410_BANKS[B + S3C_BANK_GPIO])
                return S3C6410_BANKS[B + S3C_BANK_NAME];
  }
  
  return 0;
}

int s3c_gpioGetDir(int num)
{
  uint32 gpio_offset;
  uint32 con_addr,con_dwords;
  uint32 con_data0,con_data1;
  uint32 con_mask0=0,con_mask1=0;
  uint32 pad_mask=0x3; //0b0011
  
  uint32 bank = s3c_int2bank(num);  
  if (bank==0)
        return -1;    

  uint32 B = (bank - (uint32) 'A') * S3C_BANK_COLS;
  gpio_offset = num - S3C6410_BANKS[B + S3C_BANK_GPIO];
  con_dwords  = S3C6410_BANKS[B + S3C_BANK_CONSZ];
  
  if (S3C6410_BANKS[B + S3C_BANK_CONPAD] == 4)
        pad_mask=0xF; //0b1111
        
  if (con_dwords == 2 && gpio_offset >= 8)
        con_mask1   = pad_mask << (gpio_offset-8) * S3C6410_BANKS[B + S3C_BANK_CONPAD]; 
  else
	con_mask0   = pad_mask << gpio_offset * S3C6410_BANKS[B + S3C_BANK_CONPAD];

  con_addr    = S3C6410_BANKS[B + S3C_BANK_CONREG];
  con_addr /= 4;
  
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);  

  //CON0
  if (con_dwords == 1 || gpio_offset < 8) {
    con_data0 = GPIO[con_addr];
    return (con_data0 & con_mask0) >> gpio_offset * S3C6410_BANKS[B + S3C_BANK_CONPAD];
  }
  
  //CON1 (for banks H,K,L)
  if (con_dwords == 2 && gpio_offset >= 8) {
    con_data1 = GPIO[con_addr+1];
    return (con_data1 & con_mask1) >> (gpio_offset-8) * S3C6410_BANKS[B + S3C_BANK_CONPAD]; 
  }
  
  return -1;
}

void s3c_gpioSetDir(int num, int dir)
{
  uint32 gpio_offset;
  uint32 con_addr,con_dwords;
  uint32 con_data0,con_data1;
  uint32 con_mask0=0,con_mask1=0;
  uint32 pad_mask=0x3; //0b0011
   
  uint32 bank = s3c_int2bank(num);  
  if (bank==0)
        return;

  uint32 B = (bank - (uint32) 'A') * S3C_BANK_COLS;
  gpio_offset = num - S3C6410_BANKS[B + S3C_BANK_GPIO];
  con_dwords  = S3C6410_BANKS[B + S3C_BANK_CONSZ];
  
  if (S3C6410_BANKS[B + S3C_BANK_CONPAD] == 4)
        pad_mask=0xF; //0b1111
  
  if (gpio_offset < 8)
        con_mask0   = pad_mask << gpio_offset * S3C6410_BANKS[B + S3C_BANK_CONPAD];
  if (con_dwords == 2 && gpio_offset >= 8)
        con_mask1   = pad_mask << (gpio_offset-8) * S3C6410_BANKS[B + S3C_BANK_CONPAD];
  
  con_addr    = S3C6410_BANKS[B + S3C_BANK_CONREG];
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);  
  con_addr /= 4;
  
  dir &= pad_mask;
  
  //CON0
  if (con_dwords == 1 || gpio_offset < 8) {
    con_data0 = GPIO[con_addr];    
    
    con_data0 &= ~con_mask0;
    con_data0 |= (dir << (gpio_offset * S3C6410_BANKS[B + S3C_BANK_CONPAD]));

    GPIO[con_addr] = con_data0;  
  }
  
  //CON1 (for banks H,K,L)
  if (con_dwords == 2 && gpio_offset >= 8) {
    con_data1 = GPIO[con_addr+1];
    
    con_data1 &= ~con_mask1;
    con_data1 |= (dir << ((gpio_offset-8) * S3C6410_BANKS[B + S3C_BANK_CONPAD]));
    
    GPIO[con_addr+1] = con_data1;
  }
}

int s3c_gpioGetPUD(int num)
{
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);

  uint B, bank;
  uint32 addr,data,mask;
  
  bank = s3c_int2bank(num);
  if (bank==0)
     return -1;
            
  B = (bank - 'A')*S3C_BANK_COLS;
  addr = S3C6410_BANKS[B + S3C_BANK_PUDREG];
  data = GPIO[addr / 4];
      
  mask = 0x3 << ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);
  data = (data & mask) >> ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);

  return data;
}

void s3c_gpioSetPUD(int num, int state)
{	
  state &= 0x3;
  
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);

  uint B, bank;
  uint32 addr,data,mask;
  
  bank = s3c_int2bank(num);
  if (bank==0)
     return;
            
  B = (bank - 'A')*S3C_BANK_COLS;
  addr = S3C6410_BANKS[B + S3C_BANK_PUDREG];
  data = GPIO[addr / 4];
  
  mask = 0x3 << ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);
  data &= ~mask;
  
  mask = state << ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);
  data |= mask;
  
  GPIO[addr / 4] = data;

}

int s3c_gpioGetState(int num)
{
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);

  uint B, bank;
  uint32 addr,data,mask;
  
  bank = s3c_int2bank(num);
  if (bank==0)
     return -1;
            
  B = (bank - 'A')*S3C_BANK_COLS;
  addr = S3C6410_BANKS[B + S3C_BANK_DATREG];
  data = GPIO[addr / 4];
      
  mask = 1 << (num - S3C6410_BANKS[B + S3C_BANK_GPIO]);
  data = ((data & mask) > 0);

  return data;
}

void s3c_gpioSetState(int num, int state)
{
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);

  uint B, bank;
  uint32 addr,data,mask;
  
  bank = s3c_int2bank(num);
  if (bank==0)
     return;
            
  B = (bank - 'A')*S3C_BANK_COLS;
  addr = S3C6410_BANKS[B + S3C_BANK_DATREG];
  data = GPIO[addr / 4];

  mask = 1 << (num - S3C6410_BANKS[B + S3C_BANK_GPIO]);
  
  state = (state != 0);
  
  if (state)
    data = data | mask;
  else
    data = data & ~mask;
  
  GPIO[addr / 4] = data;
}

int s3c_gpioGetSleepDir (int num)
{
  uint B, bank;
  uint32 addr,data,mask;
  
  bank = s3c_int2bank(num);
  if (bank==0)
     return -1;
     
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);
            
  B = (bank - 'A')*S3C_BANK_COLS;
  addr = S3C6410_BANKS[B + S3C_BANK_SCONREG];
  if (addr != 0) {
    data = GPIO[addr / 4];
      
    mask = 0x3 << ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);
    data = (data & mask) >> ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);

    return data;
  }
  
  return 0;
}

void s3c_gpioSetSleepDir (int num, int state)
{
  state &=0x3;
	
  uint B, bank;
  uint32 addr,data,mask;
  
  bank = s3c_int2bank(num);
  if (bank==0)
     return;

  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);
            
  B = (bank - 'A')*S3C_BANK_COLS;
  addr = S3C6410_BANKS[B + S3C_BANK_SCONREG];
  if (addr != 0) {  
    data = GPIO[addr / 4];
      
    mask = 0x3 << ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);
    data &= ~mask;
  
    mask = state << ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);
    data |= mask;
    
    GPIO[addr / 4] = data;
  }

}

int s3c_gpioGetSleepPUD(int num)
{
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);

  uint B, bank;
  uint32 addr,data,mask;
  
  bank = s3c_int2bank(num);
  if (bank==0)
     return -1;
            
  B = (bank - 'A')*S3C_BANK_COLS;
  addr = S3C6410_BANKS[B + S3C_BANK_SPUDREG];
  if (addr != 0) {
    data = GPIO[addr / 4];
      
    mask = 0x3 << ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);
    data = (data & mask) >> ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);

    return data;
  }
  
  return 0;
}

void s3c_gpioSetSleepPUD(int num, int state)
{	
  state &= 0x3;
  
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);

  uint B, bank;
  uint32 addr,data,mask;
  
  bank = s3c_int2bank(num);
  if (bank==0)
     return;
            
  B = (bank - 'A')*S3C_BANK_COLS;
  addr = S3C6410_BANKS[B + S3C_BANK_SPUDREG];
  if (addr != 0) {
    data = GPIO[addr / 4];
  
    mask = 0x3 << ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);
    data &= ~mask;
  
    mask = state << ((num - S3C6410_BANKS[B + S3C_BANK_GPIO])*2);
    data |= mask;
  
    GPIO[addr / 4] = data;
  }
}

// Watch for given number of seconds which GPIO pins change
static void s3c_gpioWatch (uint seconds)
{
  if (seconds > 60)
  {
    Output(C_INFO "Number of seconds trimmed to 60");
    seconds = 60;
  }

  int cur_time = time (NULL);
  int fin_time = cur_time + seconds;

  uint B, bank;
  uint32 addr,data; //,mask;

  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);
  uint32 old_gpio[17];
  for (bank = 0; bank < 17; bank++) {
  
    B = bank*S3C_BANK_COLS;
    addr = S3C6410_BANKS[B + S3C_BANK_DATREG];
    data = GPIO[addr / 4];
      
//    mask = 1 << (num - S3C6410_BANKS[B + S3C_BANK_GPIO]);
//    data = ((data & mask) > 0);

    old_gpio[bank] = data;
  }

  while (cur_time <= fin_time)
  {
    GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);
    for (bank = 0; bank < 17; bank++) {

      B = bank*S3C_BANK_COLS;
      addr = S3C6410_BANKS[B + S3C_BANK_DATREG];
      data = GPIO[addr / 4];
        
      if (old_gpio[bank] != data)
      {
        Screen("BANK %c %04x to %04x", S3C6410_BANKS[B + S3C_BANK_NAME], old_gpio[bank], data);
        old_gpio[bank] = data;
      }
    }

    cur_time = time (NULL);
  }
}

static void
s3c_cmd_wgpio(const char *cmd, const char *x)
{
    uint32 sec;
    if (!get_expression(&x, &sec)) {
        ScriptError("Expected <seconds>");
        return;
    }
    s3c_gpioWatch(sec);
}
REG_CMD(testS3C64xx, "WB|ANK", s3c_cmd_wgpio,
        "WBANK <seconds>\n"
        "  Alt. method to watch GPIO pins for given period of time and report changes, prefer WATCH GPIOS <seconds>.")

static uint32 s3c_gpioScrGPLR (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > 187)
  {
    Output("Valid GPIO indexes are 0..187, not %d", args [0]);
    return -1;
  }

  if (setval)
  {
    int oldstate=s3c_gpioGetState (args [0]);
    s3c_gpioSetState (args [0], val != 0);
    Output(" set gpio(%d) state from %d to %d, new value:%d", args[0], oldstate, val != 0, s3c_gpioGetState(args[0]));
    return 0;
  }

  return s3c_gpioGetState (args [0]);
}
REG_VAR_RWFUNC(testS3C64xx, "GPLR", s3c_gpioScrGPLR, 1,
               "General Purpose I/O Level Register")

static uint32 s3c_gpioScrGPDR (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > 187)
  {
    Output("Valid GPIO indexes are 0..187, not %d", args [0]);
    return -1;
  }

  if (setval)
  {
    s3c_gpioSetDir (args [0], val);
    return 0;
  }

  return s3c_gpioGetDir (args [0]);
}
REG_VAR_RWFUNC(testS3C64xx, "GPDR", s3c_gpioScrGPDR, 1
               , "General Purpose I/O Direction Register")
               
static uint32 s3c_gpioScrGPUD (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > 187)
  {
    Output("Valid GPIO indexes are 0..187, not %d", args [0]);
    return -1;
  }

  if (setval)
  {
    s3c_gpioSetPUD (args [0], val);
    return 0;
  }

  return s3c_gpioGetPUD (args [0]);
}
REG_VAR_RWFUNC(testS3C64xx, "GPPUD", s3c_gpioScrGPUD, 1
               , "General Purpose I/O Pull Up/Down")
               
static uint32 s3c_gpioScrGPSDR (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > 187)
  {
    Output("Valid GPIO indexes are 0..187, not %d", args [0]);
    return -1;
  }

  if (setval)
  {
    s3c_gpioSetSleepDir (args [0], val);
    return 0;
  }

  return s3c_gpioGetSleepDir (args [0]);
}
REG_VAR_RWFUNC(testS3C64xx, "GPSDR", s3c_gpioScrGPSDR, 1
               , "General Purpose I/O Sleep Direction Register")
               
static uint32 s3c_gpioScrGPSPUD (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > 187)
  {
    Output("Valid GPIO indexes are 0..187, not %d", args [0]);
    return -1;
  }

  if (setval)
  {
    s3c_gpioSetSleepPUD (args [0], val);
    return 0;
  }

  return s3c_gpioGetSleepPUD (args [0]);
}
REG_VAR_RWFUNC(testS3C64xx, "GPSPUD", s3c_gpioScrGPSPUD, 1
               , "General Purpose I/O Sleep Pull Up/Down")

// Dump the overall GPIO state
static void
s3c_gpioDump(const char *tok, const char *args)
{
  const uint rows = 187/4 +1;
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);
  int direction, pullupdown;
  char dir, sdir, pud, spud;
  uint B, bank;
  uint32 addr,data,mask,gpio;
  
  Output("GPIO B## V D S P S |GPIO B## V D S P S |GPIO B## V D S P S |GPIO B## V D S P S");
  Output("-------------------+-------------------+-------------------+------------------");
  for (uint i = 0; i < rows; i++)
  {
    for (uint j = 0; j < 4; j++)
    {
      gpio = i + j * rows;
      if (gpio == 187) {
      	 Output("                  ");
         break;
      }

      bank = s3c_int2bank(gpio);
      if (bank==0)
         break;
      
      bank = (bank - 'A');
      B = bank*S3C_BANK_COLS;
      addr = S3C6410_BANKS[B + S3C_BANK_DATREG];
      data = GPIO[addr / 4];
      
      mask = 1 << (gpio - S3C6410_BANKS[B + S3C_BANK_GPIO]);
      data = ((data & mask) > 0);
      
      direction = s3c_gpioGetDir(gpio);
      switch (direction) {
      case -1: dir = '!'; break;
      case  0: dir = 'I'; break;
      case  1: dir = 'O'; break;
      default: dir = S3C6410_BANKS_STATES[bank*8 + direction]; //2-7
      	if (dir == ' ') dir = ('0'+direction);
      }
      
      direction = s3c_gpioGetSleepDir(gpio);
      switch (direction) {
      case -1: sdir = '!'; break;
      case  0: sdir = '0'; break;
      case  1: sdir = '1'; break;
      case  2: sdir = 'I'; break;
      default: sdir = 'P'; //3
      }
      pullupdown = s3c_gpioGetPUD(gpio);
      switch (pullupdown) {
      case -1: pud = '!'; break;
      case  0: pud = ' '; break;
      case  1: pud = 'D'; break;
      case  2: pud = 'U'; break;
      default: pud = '?'; //3
      }
      pullupdown = s3c_gpioGetSleepPUD(gpio);
      switch (pullupdown) {
      case -1: spud = '!'; break;
      case  0: spud = ' '; break;
      case  1: spud = 'D'; break;
      case  2: spud = 'U'; break;
      default: spud = '?'; //3
      }
      
      Output(" %3d %c%02d %c %c %c %c %c%s", gpio, (bank+'A'), gpio - S3C6410_BANKS[B + S3C_BANK_GPIO], 
           data?'1':' ', 
           dir, sdir,
           pud, spud,
           j < 3 ? " |\t" : "");
    }
  }
  
  Output("-------------------+-------------------+-------------------+------------------");
  Output("  V:Value - D:Dir/Mode S:Sleep Dir/State - P:PullUp/Down S:Sleep PullUp/Down ");  
  Output("  Directions       : I:Input, O:Output, X:Ext IRQ, 2-7 (cf. doc)");
  Output("  Sleep Directions : 0:Ouput 0, 1:Output 1, I:Input, P:Previous state");
  
/*for (bank = 0; bank < 17; bank++)
  {  
    B = bank*S3C_BANK_COLS;
    addr = S3C6410_BANKS[B + S3C_BANK_CONREG];  
    Output("  Bank %c %04x - CON0:%08x CON1:%08x", S3C6410_BANKS[B + S3C_BANK_NAME], addr, GPIO[addr / 4], S3C6410_BANKS[B + S3C_BANK_CONSZ]==2 ? GPIO[(addr / 4)+1]:0);
  }*/
  
  
/*
  for (bank = 0; bank < 17; bank++)
  { 
    B = bank*S3C_BANK_COLS;
    Output(" BANK[\"%c\"]:pCtrl = 0x%08x ",bank+'A',S3C6410_BANKS[B + S3C_BANK_CONREG]);
    Output(" BANK[\"%c\"]:nCtrl = %d ",    bank+'A',S3C6410_BANKS[B + S3C_BANK_CONSZ]);
    Output(" BANK[\"%c\"]:pData = 0x%08x ",bank+'A',S3C6410_BANKS[B + S3C_BANK_DATREG]);
    Output(" BANK[\"%c\"]:nPins = 0x%08x ",bank+'A',S3C6410_BANKS[B + S3C_BANK_COUNT]);
    Output(" BANK[\"%c\"]:nPadd = %d     ",bank+'A',S3C6410_BANKS[B + S3C_BANK_CONPAD]);
    Output(" BANK[\"%c\"]:pPull = 0x%08x ",bank+'A',S3C6410_BANKS[B + S3C_BANK_PUDREG]);
    Output(" BANK[\"%c\"]:pSCon = 0x%08x ",bank+'A',S3C6410_BANKS[B + S3C_BANK_SCONREG]);
    Output(" BANK[\"%c\"]:pSPud = 0x%08x ",bank+'A',S3C6410_BANKS[B + S3C_BANK_SPUDREG]);
  }

    for (gpio = 0; gpio < S3C6410_BANKS[B + S3C_BANK_COUNT]; gpio++) {
      Output(" GPIO[\"%c%02d\"]:CtrlPad = %d ",bank+'A',gpio,S3C6410_BANKS[B + S3C_BANK_CONPAD]);
    }
*/    

}
REG_DUMP(testS3C64xx, "GPIOS", s3c_gpioDump,
         "GPIOS\n"
         "  Show GPIO machinery state in a human-readable format.")

// Dump available GPIO outputs
static void
s3c_gpioOutputs(const char *tok, const char *args)
{
  uint32 *GPIO = (uint32 *)memPhysMap (S3C6400_PA_GPIO);
  int direction; char dir, writeable;
  uint B, bank;
  uint32 addr,data,mask,gpio;
  
  Output("GPIO B## D S Writable");
  Output("---------------------");
  for (gpio=0; gpio<187; gpio++)
  {
      bank = s3c_int2bank(gpio);
      if (bank==0)
            break;
      
      bank = (bank - 'A');
      B = bank*S3C_BANK_COLS;
      addr = S3C6410_BANKS[B + S3C_BANK_DATREG];
      data = GPIO[addr / 4];
      
      mask = 1 << (gpio - S3C6410_BANKS[B + S3C_BANK_GPIO]);
      data = ((data & mask) > 0);
      
      dir='O';
      direction = s3c_gpioGetDir(gpio);
      if (direction == 1) {
      	  writeable=' ';
      	  s3c_gpioSetState(gpio,(data == 0));      	  
      	  if ( data != (uint32) s3c_gpioGetState(gpio) ) {
      	  	writeable='W';
      	  	s3c_gpioSetState(gpio,data);
      	  }
          Output(" %3d %c%02d %c %c %c", gpio, (bank+'A'), gpio - S3C6410_BANKS[B + S3C_BANK_GPIO], dir, data?'1':' ', writeable);
      }
  }
}
REG_DUMP(testS3C64xx, "GPIOSOUT", s3c_gpioOutputs,
         "GPIOSOUT\n"
         "  Show GPIOs in output state.")

// Dump GPIO state in a linux-specific format
static void
s3c_gpioDumpState(const char *tok, const char *args)
{
  int i;
  Output("/* GPIO pin direction setup */");
  for (i = 0; i < 187; i++)
    Output("#define GPIO%02d_Dir\t%d",
         i, s3c_gpioGetDir (i));
  Output("\n/* GPIO Pin Init State */");
  for (i = 0; i < 187; i++)
    Output("#define GPIO%02d_LEVEL\t%d",
         i, s3c_gpioGetDir (i) ? s3c_gpioGetState (i) : 0);
  Output("\n/* GPIO Pin Sleep Level */");
  for (i = 0; i < 187; i++)
    Output("#define GPIO%02d_SLEEP_LEVEL\t%d",
         i, s3c_gpioGetSleepDir (i));
}
REG_DUMP(testS3C64xx, "GPIOST", s3c_gpioDumpState,
         "GPIOST\n"
         "  Show GPIO state suitable for include/asm/arch/xxx-init.h")
