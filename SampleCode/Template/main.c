/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include	"project_config.h"
#include	"fmc_user.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/
#define APROM_TARGET_LEN									(0x00020000UL)

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

uint8_t aprom_buf[FMC_FLASH_PAGE_SIZE];

//#define ENABLE_UPDATE_CHECKSUM

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void  dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

uint8_t calculate_crc32_APROM_checksum(void)
{
    uint32_t chksum_cal, chksum_app , chksum_direct;
	uint32_t chksum_len;
    volatile uint32_t addr, data;
	uint32_t start, size;
	uint32_t i = 0;
	uint32_t target = 0;
	
	start = 0x00000000;
	size = (APROM_TARGET_LEN - FMC_FLASH_PAGE_SIZE);
	chksum_len = APROM_TARGET_LEN - 4; 

    CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
    SYS_UnlockReg();
    FMC_Open();	

    CRC_Open(CRC_32, (CRC_WDATA_RVS | CRC_CHECKSUM_RVS | CRC_CHECKSUM_COM), 0xFFFFFFFF, CRC_CPU_WDATA_32);
    
    for(addr = start; addr < size; addr += 4){
        data = FMC_Read(addr);
        CRC_WRITE_DATA(data);
    }

    chksum_cal = CRC_GetChecksum();

	// read checksum 
	chksum_direct = inp32(chksum_len);
   
    chksum_app = FMC_Read(chksum_len);    

	printf("APROM checksum Caculated : 0x%08X , in APROM : 0x%08X (0x%08X)\r\n" , chksum_cal , chksum_app , chksum_direct);

	for ( i = 0 , target = APROM_TARGET_LEN ; i < 32 ; i += 4 , target -= 4)
	{
		printf("target = 0x%8X ,0x%8X ,\r\n" , target, FMC_Read(target));
	
	}
		
    FMC_Close();
    SYS_LockReg();	
	
    if (chksum_cal == chksum_app) 
	{
        return TRUE;
    } 
	else 
	{
        return FALSE;
    }
}

void update_crc32_APROM_checksum(void)
{	
	#if defined (ENABLE_UPDATE_CHECKSUM)
	uint32_t addr_start_last_page = 0;
	uint32_t addr_end_last_page = 0;
	#endif

	uint32_t checksum_address = APROM_TARGET_LEN - 4;
	uint32_t direct_checksum = 0;
    uint32_t fmc_checksum = 0;	
    uint32_t cal_checksum = 0;
    volatile uint32_t addr, data;
	uint32_t start, size;
	
    CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;	
	SYS_UnlockReg();
	FMC_Open();

	// read checksum 
	direct_checksum = inp32(checksum_address);	
	
	// read checksum 
    FMC_Read_User(checksum_address, &fmc_checksum);
	printf("checksum_address : 0x%8X , readcksum : 0x%8X / 0x%8X \r\n"  , checksum_address , fmc_checksum , direct_checksum);	

    CRC_Open(CRC_32, (CRC_WDATA_RVS | CRC_CHECKSUM_RVS | CRC_CHECKSUM_COM), 0xFFFFFFFF, CRC_CPU_WDATA_32);

	start = 0x00000000;
	size = (APROM_TARGET_LEN - FMC_FLASH_PAGE_SIZE);
	
    for(addr = start; addr < size; addr += 4){
        data = FMC_Read(addr);
        CRC_WRITE_DATA(data);
    }
    cal_checksum = CRC_GetChecksum();
		
	printf("app len : 0x%8X , calcksum : 0x%8X  \r\n"  , size , cal_checksum);

	#if defined (ENABLE_UPDATE_CHECKSUM)
	addr_start_last_page = APROM_TARGET_LEN - FMC_FLASH_PAGE_SIZE;
	addr_end_last_page = APROM_TARGET_LEN;
	FMC_ENABLE_AP_UPDATE();			
	if(cal_checksum != fmc_checksum)	// assume APROM total len 256K
	{
		printf("write calcksum2 : 0x%8X\r\n" , cal_checksum);
		printf("addr : 0x%8X\r\n" , checksum_address);
        ReadData(addr_start_last_page, addr_end_last_page, (uint32_t *)aprom_buf);
        FMC_Erase_User(addr_start_last_page);
		FMC_Write_User(checksum_address, cal_checksum);		
        ReadData(addr_start_last_page, addr_end_last_page, (uint32_t *)aprom_buf);		
        WriteData(addr_start_last_page, addr_end_last_page, (uint32_t *)aprom_buf);

	}
	FMC_DISABLE_AP_UPDATE();	
	#endif

    FMC_Close();
    SYS_LockReg();
	
}


void TMR1_IRQHandler(void)
{
	static uint32_t LOG = 0;
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			PH0 ^= 1;
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res == 'x' || res == 'X')
	{
		NVIC_SystemReset();
	}

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}


void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART0_IRQn);

	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	#endif
}

void Custom_Init(void)
{
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH0MFP_Msk)) | (SYS_GPH_MFPL_PH0MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH1MFP_Msk)) | (SYS_GPH_MFPL_PH1MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH2MFP_Msk)) | (SYS_GPH_MFPL_PH2MFP_GPIO);

	//EVM LED
	GPIO_SetMode(PH,BIT0,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT1,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT2,GPIO_MODE_OUTPUT);
	
}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable External XTAL (4~24 MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk|CLK_PWRCTL_HIRCEN_Msk);

    /* Waiting for 12MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk|CLK_STATUS_HIRCSTB_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk|CLK_PWRCTL_LIRCEN_Msk);

    /* Waiting for 12MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk|CLK_STATUS_LIRCSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_192MHZ);
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    CLK_EnableModuleClock(CRC_MODULE);

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
	
    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	UART0_Init();
	Custom_Init();	
	TIMER1_Init();

	calculate_crc32_APROM_checksum();
	update_crc32_APROM_checksum();

    /* Got no where to go, just loop forever */
    while(1)
    {


    }
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
