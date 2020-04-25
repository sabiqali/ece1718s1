#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "address_map_arm.h"
#include "ADXL345.h"

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}


volatile int *I2C0_ptr;	            // virtual address for I2C
volatile int *SYSMGR_ptr;           // virtual address for SYSMGR

void Pinmux_Config();
void I2C0_Init();
void ADXL345_REG_WRITE(uint8_t address, uint8_t value);
void ADXL345_REG_READ(uint8_t address, uint8_t *value);
void ADXL345_Init();
bool ADXL345_WasActivityUpdated();
void ADXL345_REG_MULTI_READ(uint8_t address, uint8_t values[], uint8_t len);
void ADXL345_XYZ_Read(int16_t szData16[3]);

int main(void){

    uint8_t devid;
    int16_t mg_per_lsb = 31; //mg per lsb is 31.25mg
    int16_t XYZ[3];

    int fd = -1;

    //To catch CTRL+C
    signal(SIGINT,catchSIGINT);

    //Open file for physical mapping
    if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
    {
        printf ("ERROR: could not open \"/dev/mem\"...\n");
        return (-1);
    }
    //Initialize virtual addresses
    else
    {
        //I2C0_ptr = (int *)map_physical(fd, I2C0_BASE, I2C0_SPAN);
        I2C0_ptr = (int *)mmap (NULL, I2C0_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, I2C0_BASE);
        if (I2C0_ptr == MAP_FAILED)
        {
            printf ("ERROR: mmap() failed...\n");
            close (fd);
            return (NULL);
        }
        //SYSMGR_ptr = (int *)map_physical(fd, SYSMGR_BASE, SYSMGR_SPAN);
        SYSMGR_ptr = (int *)mmap (NULL, SYSMGR_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, SYSMGR_BASE);
        if (SYSMGR_ptr == MAP_FAILED)
        {
            printf ("ERROR: mmap() failed...\n");
            close (fd);
            return (NULL);
        }
        if ((I2C0_ptr == NULL) || (SYSMGR_ptr == NULL)){

            printf("Error: map_physical returned NULL\n");
            return (-1);

        }
    }


    // Configure Pin Muxing
    Pinmux_Config();

    // Initialize I2C0 Controller
    I2C0_Init();

    // 0xE5 is read from DEVID(0x00) if I2C is functioning correctly
    ADXL345_REG_READ(0x00, &devid);

    // Correct Device ID
    if (devid == 0xE5){
        // Initialize accelerometer chip
        ADXL345_Init();

        //Update accel readings while stop is not detected
        while(!stop){
           // if (ADXL345_WasActivityUpdated()){
                ADXL345_XYZ_Read(XYZ);
                printf("X=%d mg, Y=%d mg, Z=%d mg\n", XYZ[0]*mg_per_lsb, XYZ[1]*mg_per_lsb, XYZ[2]*mg_per_lsb);
          //  }
        }
    } else {
        printf("Incorrect device ID\n");
    }

    if (munmap ((void *)I2C0_ptr, I2C0_SPAN) != 0)
	{
		printf ("ERROR: munmap() failed...\n");
		return (-1);
	}
    if (munmap ((void *)SYSMGR_ptr, SYSMGR_SPAN) != 0)
	{
		printf ("ERROR: munmap() failed...\n");
		return (-1);
	}
    close (fd);
    return 0;
}


void Pinmux_Config(){
    // Set up pin muxing (in sysmgr) to connect ADXL345 wires to I2C0
    *(SYSMGR_ptr + SYSMGR_I2C0USEFPGA) = 0;
    *(SYSMGR_ptr + SYSMGR_GENERALIO7) = 1;
    *(SYSMGR_ptr + SYSMGR_GENERALIO8) = 1;
}


void I2C0_Init(){

    // Abort any ongoing transmits and disable I2C0.
    *(I2C0_ptr + I2C0_ENABLE) = 2;

    // Wait until I2C0 is disabled
    while(((*(I2C0_ptr + I2C0_ENABLE_STATUS))&0x1) == 1){}

    // Configure the config reg with the desired setting (act as
    // a master, use 7bit addressing, fast mode (400kb/s)).
    *(I2C0_ptr + I2C0_CON) = 0x65;

    // Set target address (disable special commands, use 7bit addressing)
    *(I2C0_ptr + I2C0_TAR) = 0x53;

    // Set SCL high/low counts (Assuming default 100MHZ clock input to I2C0 Controller).
    // The minimum SCL high period is 0.6us, and the minimum SCL low period is 1.3us,
    // However, the combined period must be 2.5us or greater, so add 0.3us to each.
    *(I2C0_ptr + I2C0_FS_SCL_HCNT) = 60 + 30; // 0.6us + 0.3us
    *(I2C0_ptr + I2C0_FS_SCL_LCNT) = 130 + 30; // 1.3us + 0.3us

    // Enable the controller
    *(I2C0_ptr + I2C0_ENABLE) = 1;

    // Wait until controller is enabled
    while(((*(I2C0_ptr + I2C0_ENABLE_STATUS))&0x1) == 0){}

}


void ADXL345_REG_WRITE(uint8_t address, uint8_t value){

    // Send reg address (+0x400 to send START signal)
    *(I2C0_ptr + I2C0_DATA_CMD) = address + 0x400;

    // Send value
    *(I2C0_ptr + I2C0_DATA_CMD) = value;
}

void ADXL345_REG_READ(uint8_t address, uint8_t *value){

    // Send reg address (+0x400 to send START signal)
    *(I2C0_ptr + I2C0_DATA_CMD) = address + 0x400;

    // Send read signal
    *(I2C0_ptr + I2C0_DATA_CMD) = 0x100;

    // Read the response (first wait until RX buffer contains data)
    while (*(I2C0_ptr + I2C0_RXFLR) == 0){}
    *value = *(I2C0_ptr + I2C0_DATA_CMD);
}


void ADXL345_Init(){

    // +- 16g range, 10 bit resolution
    ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_10BIT) ;

    // Output Data Rate: 12.5 Hz
    ADXL345_REG_WRITE(ADXL345_REG_BW_RATE,  XL345_RATE_12_5);

    // NOTE: The DATA_READY bit is not reliable. It is updated at a much higher rate than the Data Rate
    // Use the Activity and Inactivity interrupts.
    //----- Enabling interrupts -----//
    ADXL345_REG_WRITE(ADXL345_REG_THRESH_ACT, 0x04);	//activity threshold
    ADXL345_REG_WRITE(ADXL345_REG_THRESH_INACT, 0x02);	//inactivity threshold
    ADXL345_REG_WRITE(ADXL345_REG_TIME_INACT, 0x02);	//time for inactivity
    ADXL345_REG_WRITE(ADXL345_REG_ACT_INACT_CTL, 0xFF);	//Enables AC coupling for thresholds
    ADXL345_REG_WRITE(ADXL345_REG_INT_ENABLE, XL345_ACTIVITY | XL345_INACTIVITY );	//enable interrupts
    //-------------------------------//

    // stop measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY);

    // start measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_MEASURE);
}

bool ADXL345_WasActivityUpdated(){
	bool bReady = false;
    uint8_t data8;

    ADXL345_REG_READ(ADXL345_REG_INT_SOURCE,&data8);
    if (data8 & XL345_ACTIVITY)
        bReady = true;

    return bReady;
}

// Read multiple consecutive internal registers
void ADXL345_REG_MULTI_READ(uint8_t address, uint8_t values[], uint8_t len){

    // Send reg address (+0x400 to send START signal)
    *(I2C0_ptr + I2C0_DATA_CMD) = address + 0x400;

    // Send read signal len times
    int i;
    for (i=0;i<len;i++)
        *(I2C0_ptr + I2C0_DATA_CMD) = 0x100;

    // Read the bytes
    int nth_byte=0;
    while (len){
        if ((*(I2C0_ptr + I2C0_RXFLR)) > 0){
            values[nth_byte] = *(I2C0_ptr + I2C0_DATA_CMD);
            nth_byte++;
            len--;
        }
    }
}

// Read acceleration data of all three axes
void ADXL345_XYZ_Read(int16_t szData16[3]){
    uint8_t szData8[6];

    ADXL345_REG_MULTI_READ(0x32, (uint8_t *)&szData8, sizeof(szData8));

    szData16[0] = (szData8[1] << 8) | szData8[0];
    szData16[1] = (szData8[3] << 8) | szData8[2];
    szData16[2] = (szData8[5] << 8) | szData8[4];
}


