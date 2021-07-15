# M480BSP_SRecord_Checksum
 M480BSP_SRecord_Checksum

update @ 2021/07/15

1. by using CRC32  ,to generate application code checksum

2. use SRecord tool , to attach checksum at last 4 bytes in application code , and generate a full binary (0x20000 , 128K)

	perform generateChecksum.bat , will execute 2 cmd : generateChecksum.cmd , generateCRCbinary.cmd

![image](https://github.com/released/M480BSP_SRecord_Checksum/blob/main/setting.jpg)


	generateChecksum.cmd
	
		load file , and declare 0x0000 to 0x1F000 (assume MCU flash is 0x20000 , last page (0x20000 - 0x1000) will store checksum)
	
		fill 0xFF in to the filed (0x0000 to 0x1F000)
		
		use CRC32 , and calculate checksum , put under region : 0x1FFFC 0x20000
		
		display checksum hex result , under KEIL compile window
		
![image](https://github.com/released/M480BSP_SRecord_Checksum/blob/main/compile.jpg)
		
		
	generateCRCbinary.cmd
	
		process same as previos checksum flow 
		
		generate the final binary , with checksum in last 4 bytes

3. need to use ICP tool , to programming the complete binary (128K) to MCU

4. below is log message screen , 

![image](https://github.com/released/M480BSP_SRecord_Checksum/blob/main/log.jpg)


