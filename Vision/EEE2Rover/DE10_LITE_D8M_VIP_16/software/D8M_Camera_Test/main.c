
#include <stdio.h>
#include "I2C_core.h"
#include "terasic_includes.h"
#include "mipi_camera_config.h"
#include "mipi_bridge_config.h"

#include "auto_focus.h"

#include <fcntl.h>
#include <unistd.h>

//EEE_IMGPROC defines
#define EEE_IMGPROC_MSG_START_R ('R'<<16 | 'B'<<8 | 'B')
#define EEE_IMGPROC_MSG_START_G ('G'<<16 | 'B'<<8 | 'B')
#define EEE_IMGPROC_MSG_START_B ('B'<<16 | 'B'<<8 | 'B')
#define EEE_IMGPROC_MSG_START_Y ('Y'<<16 | 'B'<<8 | 'B')
#define EEE_IMGPROC_MSG_START_P ('P'<<16 | 'B'<<8 | 'B')

//offsets
#define EEE_IMGPROC_STATUS 0
#define EEE_IMGPROC_MSG 1
#define EEE_IMGPROC_ID 2
#define EEE_IMGPROC_BBCOL 3

#define EXPOSURE_INIT 0x002000
#define EXPOSURE_STEP 0x1000
#define GAIN_INIT 0xff00
#define GAIN_STEP 0xf
#define DEFAULT_LEVEL 3

#define MIPI_REG_PHYClkCtl		0x0056
#define MIPI_REG_PHYData0Ctl	0x0058
#define MIPI_REG_PHYData1Ctl	0x005A
#define MIPI_REG_PHYData2Ctl	0x005C
#define MIPI_REG_PHYData3Ctl	0x005E
#define MIPI_REG_PHYTimDly		0x0060
#define MIPI_REG_PHYSta			0x0062
#define MIPI_REG_CSIStatus		0x0064
#define MIPI_REG_CSIErrEn		0x0066
#define MIPI_REG_MDLSynErr		0x0068
#define MIPI_REG_FrmErrCnt		0x0080
#define MIPI_REG_MDLErrCnt		0x0090

int blue_hl = 200, blue_hu = 230;
int blue_sl = 0, blue_su = 0xff;
int blue_vl = 0, blue_vu = 0xff;

int green_hl = 150, green_hu = 190;
int green_sl = 0, green_su = 0xff;
int green_vl = 0, green_vu = 0xff;

int red_hl = 0, red_hu = 40;
int red_sl = 0, red_su = 0xff;
int red_vl = 0, red_vu = 0xff;

int yellow_hl = 60, yellow_hu = 90;
int yellow_sl = 0, yellow_su = 0xff;
int yellow_vl = 0, yellow_vu = 0xff;

int pink_hl = 260, pink_hu = 320;
int pink_sl = 0, pink_su = 0xff;
int pink_vl = 0, pink_vu = 0xff;

int filtered = 0;
int mode = 0;


void mipi_clear_error(void) {
	MipiBridgeRegWrite(MIPI_REG_CSIStatus, 0x01FF); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLSynErr, 0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_FrmErrCnt, 0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLErrCnt, 0x0000); // clear error

	MipiBridgeRegWrite(0x0082, 0x00);
	MipiBridgeRegWrite(0x0084, 0x00);
	MipiBridgeRegWrite(0x0086, 0x00);
	MipiBridgeRegWrite(0x0088, 0x00);
	MipiBridgeRegWrite(0x008A, 0x00);
	MipiBridgeRegWrite(0x008C, 0x00);
	MipiBridgeRegWrite(0x008E, 0x00);
	MipiBridgeRegWrite(0x0090, 0x00);
}

void mipi_show_error_info(void) {

	alt_u16 PHY_status, SCI_status, MDLSynErr, FrmErrCnt, MDLErrCnt;

	PHY_status = MipiBridgeRegRead(MIPI_REG_PHYSta);
	SCI_status = MipiBridgeRegRead(MIPI_REG_CSIStatus);
	MDLSynErr = MipiBridgeRegRead(MIPI_REG_MDLSynErr);
	FrmErrCnt = MipiBridgeRegRead(MIPI_REG_FrmErrCnt);
	MDLErrCnt = MipiBridgeRegRead(MIPI_REG_MDLErrCnt);
	printf(
			"PHY_status=%xh, CSI_status=%xh, MDLSynErr=%xh, FrmErrCnt=%xh, MDLErrCnt=%xh\r\n",
			PHY_status, SCI_status, MDLSynErr, FrmErrCnt, MDLErrCnt);
}

void mipi_show_error_info_more(void) {
	printf("FrmErrCnt = %d\n", MipiBridgeRegRead(0x0080));
	printf("CRCErrCnt = %d\n", MipiBridgeRegRead(0x0082));
	printf("CorErrCnt = %d\n", MipiBridgeRegRead(0x0084));
	printf("HdrErrCnt = %d\n", MipiBridgeRegRead(0x0086));
	printf("EIDErrCnt = %d\n", MipiBridgeRegRead(0x0088));
	printf("CtlErrCnt = %d\n", MipiBridgeRegRead(0x008A));
	printf("SoTErrCnt = %d\n", MipiBridgeRegRead(0x008C));
	printf("SynErrCnt = %d\n", MipiBridgeRegRead(0x008E));
	printf("MDLErrCnt = %d\n", MipiBridgeRegRead(0x0090));
	printf("FIFOSTATUS = %d\n", MipiBridgeRegRead(0x00F8));
	printf("DataType = 0x%04x\n", MipiBridgeRegRead(0x006A));
	printf("CSIPktLen = %d\n", MipiBridgeRegRead(0x006E));
}

bool MIPI_Init(void) {
	bool bSuccess;

	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_MIPI_BASE, 50 * 1000 * 1000,
			400 * 1000); //I2C: 400K
	if (!bSuccess)
		printf("failed to init MIPI- Bridge i2c\r\n");

	usleep(50 * 1000);
	MipiBridgeInit();

	usleep(500 * 1000);

//	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_CAMERA_BASE, 50*1000*1000,400*1000); //I2C: 400K
//	if (!bSuccess)
//		printf("failed to init MIPI- Camera i2c\r\n");

	MipiCameraInit();
	MIPI_BIN_LEVEL(DEFAULT_LEVEL);
//    OV8865_FOCUS_Move_to(340);

//    oc_i2c_uninit(I2C_OPENCORES_CAMERA_BASE);  // Release I2C bus , due to two I2C master shared!

	usleep(1000);

//    oc_i2c_uninit(I2C_OPENCORES_MIPI_BASE);

	return bSuccess;
}

int main() {

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

	printf("DE10-LITE D8M VGA Demo\n");
	printf("Imperial College EEE2 Project version\n");
	IOWR(MIPI_PWDN_N_BASE, 0x00, 0x00);
	IOWR(MIPI_RESET_N_BASE, 0x00, 0x00);

	usleep(2000);
	IOWR(MIPI_PWDN_N_BASE, 0x00, 0xFF);
	usleep(2000);
	IOWR(MIPI_RESET_N_BASE, 0x00, 0xFF);

	printf("Image Processor ID: %x\n", IORD(0x42000, EEE_IMGPROC_ID));
	//printf("Image Processor ID: %x\n",IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_ID)); //Don't know why this doesn't work - definition is in system.h in BSP

	usleep(2000);

	// MIPI Init
	if (!MIPI_Init()) {
		printf("MIPI_Init Init failed!\r\n");
	} else {
		printf("MIPI_Init Init successfully!\r\n");
	}

//   while(1){
	mipi_clear_error();
	usleep(50 * 1000);
	mipi_clear_error();
	usleep(1000 * 1000);
	mipi_show_error_info();
//	    mipi_show_error_info_more();
	printf("\n");
//   }

#if 0  // focus sweep
	printf("\nFocus sweep\n");
	alt_u16 ii= 350;
	alt_u8 dir = 0;
	while(1) {
		if(ii< 50) dir = 1;
		else if (ii> 1000) dir =0;

		if(dir) ii += 20;
		else ii -= 20;

		printf("%d\n",ii);
		OV8865_FOCUS_Move_to(ii);
		usleep(50*1000);
	}
#endif

	//////////////////////////////////////////////////////////
	alt_u16 bin_level = DEFAULT_LEVEL;
	alt_u8 manual_focus_step = 10;
	alt_u16 current_focus = 300;
	int boundingBoxColour = 0;
	alt_u32 exposureTime = EXPOSURE_INIT;
	alt_u16 gain = GAIN_INIT;

	OV8865SetExposure(exposureTime);
	OV8865SetGain(gain);
	Focus_Init();

	FILE* ser = fopen("/dev/uart", "wb+");
	if (ser) {
		printf("Opened UART\n");
	} else {
		printf("Failed to open UART\n");
		while (1)
			;
	}

	while (1) {

		// touch KEY0 to trigger Auto focus
		if ((IORD(KEY_BASE,0) & 0x03) == 0x02) {

			current_focus = Focus_Window(320, 240);
		}
		// touch KEY1 to ZOOM
		if ((IORD(KEY_BASE,0) & 0x03) == 0x01) {
			if (bin_level == 3)
				bin_level = 1;
			else
				bin_level++;
			printf("set bin level to %d\n", bin_level);
			MIPI_BIN_LEVEL(bin_level);
			usleep(500000);

		}

#if 0
		if((IORD(KEY_BASE,0)&0x0F) == 0x0E) {

			current_focus = Focus_Window(320,240);
		}

		// touch KEY1 to trigger Manual focus  - step
		if((IORD(KEY_BASE,0)&0x0F) == 0x0D) {

			if(current_focus > manual_focus_step) current_focus -= manual_focus_step;
			else current_focus = 0;
			OV8865_FOCUS_Move_to(current_focus);

		}

		// touch KEY2 to trigger Manual focus  + step
		if((IORD(KEY_BASE,0)&0x0F) == 0x0B) {
			current_focus += manual_focus_step;
			if(current_focus >1023) current_focus = 1023;
			OV8865_FOCUS_Move_to(current_focus);
		}

		// touch KEY3 to ZOOM
		if((IORD(KEY_BASE,0)&0x0F) == 0x07) {
			if(bin_level == 3 )bin_level = 1;
			else bin_level ++;
			printf("set bin level to %d\n",bin_level);
			MIPI_BIN_LEVEL(bin_level);
			usleep(500000);

		}
#endif

		//Read messages from the image processor and print them on the terminal
		int stage = 0;
		char color;
		int x_min,x_max,y_min,y_max;
		int x_diff,y_diff,x_dis,y_dis;
		int x_center,y_center,angle;
		while ((IORD(0x42000,EEE_IMGPROC_STATUS) >> 8) & 0x01ff) { //Find out if there are words to read
			int word = IORD(0x42000, EEE_IMGPROC_MSG); //Get next word from message buffer
			if (fwrite(&word, 4, 1, ser) != 1)
				printf("Error writing to UART");

			if( mode == 1){
				continue;
			}else if (word == EEE_IMGPROC_MSG_START_R){
				color = 'r';
				stage = 1;
				printf("red   : ");
			}else if(word == EEE_IMGPROC_MSG_START_G){
				color = 'g';
				stage = 1;

				printf("green : ");
			}else if(word == EEE_IMGPROC_MSG_START_B){
				color = 'b';
				stage = 1;

				printf("blue  : ");
			}else if(word == EEE_IMGPROC_MSG_START_Y){
				color = 'y';
				stage = 1;

				printf("yellow: ");
			}else if(word == EEE_IMGPROC_MSG_START_P){
				color = 'p';
				stage = 1;

				printf("pink  : ");
			}else if(stage == 1){
				x_min = (word & 0x07FF0000) >> 16 ;
				x_max = word & 0x000007FF;
				x_diff = x_max-x_min;
				x_center = (x_min+x_max)/2;
				printf("x_min: %3d ",x_min);
				printf("x_max: %3d ",x_max);
				printf("Size: %4d ",x_diff);

				angle = (x_center-320)/13;//(x_center-320)/320*25; // percentage of screen converted into degrees;
				if (x_diff < 0){
					x_dis = 0;
					angle = 0;
				}else if ( x_diff > 50 && x_diff < 580){
					x_dis = 4900/x_diff;
					//(x_center-320)/320*25; // percentage of screen converted into degrees;
					fprintf(ser,"%i,%d,%d\n",color,angle,x_dis);
				}
				printf("angle: %3d ",angle);
				printf("distance: %3d \n",x_dis);

				stage = 2;
			}else if(stage == 2){
				y_min = (word & 0x07FF0000) >> 16;
				y_max = word & 0x000007FF;
				y_diff = y_max-y_min;
				y_center = (x_min+x_max)/2;
//				printf("y_min: %3d ",y_min);
//				printf("y_max: %3d ",y_max);
//				printf("Size: %3d \n",y_diff);

				stage = 0;
			}

//			fprintf(ser,"\n");
//			fprintf(ser,"%x : ", (IORD(0x42000,EEE_IMGPROC_STATUS) >> 8) & 0x1ff);
//			fprintf(ser,"%08x ", word);
		}

		//Update the bounding box colour
		boundingBoxColour = ((boundingBoxColour + 1) & 0xff);
		IOWR(0x42000, EEE_IMGPROC_BBCOL,
				(boundingBoxColour << 8) | (0xff - boundingBoxColour));



		//Process input commands
		int hue_step = 5;
		int in = getchar();
		switch (in) {
		case 'J': {
			exposureTime += EXPOSURE_STEP;
			OV8865SetExposure(exposureTime);
			printf("\nExposure = %x ", exposureTime);
			break;
		}
		case 'j': {
			exposureTime -= EXPOSURE_STEP;
			OV8865SetExposure(exposureTime);
			printf("\nExposure = %x ", exposureTime);
			break;
		}
		case 'K': {
			gain += GAIN_STEP;
			OV8865SetGain(gain);
			printf("\nGain = %x ", gain);
			break;
		}
		case 'k': {
			gain -= GAIN_STEP;
			OV8865SetGain(gain);
			printf("\nGain = %x ", gain);
			break;
		}
		case 'L': {
			current_focus += manual_focus_step;
			if (current_focus > 1023)
				current_focus = 1023;
			OV8865_FOCUS_Move_to(current_focus);
			printf("\nFocus = %x ", current_focus);
			break;
		}
		case 'l': {
			if (current_focus > manual_focus_step)
				current_focus -= manual_focus_step;
			OV8865_FOCUS_Move_to(current_focus);
			printf("\nFocus = %x ", current_focus);
			break;
		}

		//BLUE HSV COLOR MAPPING
		case 'U': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0001);
			blue_hl = blue_hl + hue_step;;
			printf("\nBlue Lower Hue increased to %d", blue_hl);
			break;
		}
		case 'u': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0002);
			blue_hl -= hue_step;
			printf("\nBlue Lower Hue decreased to %d", blue_hl);
			break;
		}
		case '&': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0003);
			blue_hu += hue_step;
			printf("\nBlue Upper Hue increased to %d", blue_hu);
			break;
		}
		case '7': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0004);
			blue_hu -= hue_step;
			printf("\nBlue Upper Hue decreased to %d", blue_hu);
			break;
		}
		case 'i': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0005);
			blue_sl += hue_step;
			printf("\nBlue Lower Saturation increased to %d", blue_sl);
			break;
		}
		case 'I': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0006);
			blue_sl -= hue_step;
			printf("\nBlue Lower Saturation decreased to %d", blue_sl);
			break;
		}
		case '*': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0007);
			blue_su += hue_step;
			printf("\nBlue Upper Saturation increased to %d", blue_su);
			break;
		}
		case '8': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0008);
			blue_su -= hue_step;
			printf("\nBlue Upper Saturation decreased to %d", blue_su);
			break;
		}
		case 'O': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0009);
			blue_vl += hue_step;
			printf("\nBlue Lower Value increased to %d", blue_vl);
			break;
		}
		case 'o': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x000a);
			blue_vl -= hue_step;
			printf("\nBlue Lower Value decreased to %d", blue_vl);
			break;
		}
		case '(': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x000b);
			blue_vu += hue_step;
			printf("\nBlue Lower Value increased to %d", blue_vu);
			break;
		}
		case '9': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x000c);
			blue_vu -= hue_step;
			printf("\nBlue Lower Value decreased to %d", blue_vu);
			break;
		}

		//GREEN HSV COLOR MAPPING
		case 'R': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x1001);
			green_hl += hue_step;
			printf("\nGreen Lower Hue increased to %d", green_hl);
			break;
		}
		case 'r': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x1002);
			green_hl -= hue_step;
			printf("\nGreen Lower Hue decreased to %d", green_hl);
			break;
		}
		case '$': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x1003);
			green_hu += hue_step;
			printf("\nGreen upper Hue increased to %d", green_hu);
			break;
		}
		case '4': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x1004);
			green_hu -= hue_step;
			printf("\nGreen upper Hue decreased to %d", green_hu);
			break;
		}
		case 'T': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x1005);
			green_sl += hue_step;
			printf("\ngreen Lower Saturation increased to %d", green_sl);
			break;
		}
		case 't': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x1006);
			green_sl -= hue_step;
			printf("\ngreen Lower Saturation decreased to %d", green_sl);
			break;
		}
		case '%': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x1007);
			green_su += hue_step;
			printf("\ngreen Lower Saturation increased to %d", green_su);
			break;
		}
		case '5': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x1008);
			green_su -= hue_step;
			printf("\ngreen Lower Saturation decreased to %d", green_su);
			break;
		}
		case 'Y': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x1009);
			green_vl += hue_step;
			printf("\ngreen Lower Value increased to %d", green_vl);
			break;
		}
		case 'y': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x100a);
			green_vl -= hue_step;
			printf("\ngreen Lower Value decreased to %d", green_vl);
			break;
		}
		case '^': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x100b);
			green_vu += hue_step;
			printf("\ngreen Lower Value increased to %d", green_vu);
			break;
		}
		case '6': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x100c);
			green_vu -= hue_step;
			printf("\ngreen Lower Value decreased to %d", green_vu);
			break;
		}


		//RED HSV COLOR MAPPING
		case 'Q': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x2001);
			red_hl += hue_step;
			printf("\nRed Lower Hue increased to %d", red_hl);
			break;
		}
		case 'q': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x2002);
			red_hl -= hue_step;
			printf("\nRed Lower Hue decreased to %d", red_hl);
			break;
		}
		case '!': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x2003);
			red_hu += hue_step;
			printf("\nRed upper Hue increased to %d", red_hu);
			break;
		}
		case '1': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x2004);
			red_hu -= hue_step;
			printf("\nRed upper Hue decreased to %d", red_hu);
			break;
		}
		case 'W': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x2005);
			red_sl += hue_step;
			printf("\nred Lower Saturation increased to %d", red_sl);
			break;
		}
		case 'w': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x2006);
			red_sl -= hue_step;
			printf("\nred Lower Saturation decreased to %d", red_sl);
			break;
		}
		case '@': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x2007);
			red_su += hue_step;
			printf("\nred Upper Saturation increased to %d", red_su);
			break;
		}
		case '2': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x2008);
			red_su -= hue_step;
			printf("\nred Upper Saturation decreased to %d", red_su);
			break;
		}
		case 'E': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x2009);
			red_vl += hue_step;
			printf("\nred Lower Value increased to %d", red_vl);
			break;
		}
		case 'e': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x200a);
			red_vl -= hue_step;
			printf("\nred Lower Value decreased to %d", red_vl);
			break;
		}
		case '#': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x200b);
			red_vu += hue_step;
			printf("\nred Upper Value increased to %d", red_vu);
			break;
		}
		case '3': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x200c);
			red_vu -= hue_step;
			printf("\nred Upper Saturation decreased to %d", red_vu);
			break;
		}

		//YELLOW HSV COLOR MAPPING
		case 'Z': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x3001);
			yellow_hl += hue_step;
			printf("\nYellow Lower Hue increased to %d", yellow_hl);
			break;
		}
		case 'z': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x3002);
			yellow_hl -= hue_step;
			printf("\nYellow Lower Hue decreased to %d", yellow_hl);
			break;
		}
		case 'A': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x3003);
			yellow_hu += hue_step;
			printf("\nYellow upper Hue increased to %d", yellow_hu);
			break;
		}
		case 'a': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x3004);
			yellow_hu -= hue_step;
			printf("\nYellow upper Hue decreased to %d", yellow_hu);
			break;
		}
		case 'X': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x3005);
			yellow_sl += hue_step;
			printf("\nyellow Lower Saturation increased to %d", yellow_sl);
			break;
		}
		case 'x': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x3006);
			yellow_sl -= hue_step;
			printf("\nyellow Lower Saturation decreased to %d", yellow_sl);
			break;
		}
		case 'S': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x3007);
			yellow_su += hue_step;
			printf("\nyellow Upper Saturation increased to %d", yellow_su);
			break;
		}
		case 's': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x3008);
			yellow_su -= hue_step;
			printf("\nyellow Upper Saturation decreased to %d", yellow_su);
			break;
		}
		case 'C': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x3009);
			yellow_vl += hue_step;
			printf("\nyellow Lower Value increased to %d", yellow_vl);
			break;
		}
		case 'c': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x300a);
			yellow_vl -= hue_step;
			printf("\nyellow Lower Value decreased to %d", yellow_vl);
			break;
		}
		case 'D': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x300b);
			yellow_vu += hue_step;
			printf("\nyellow Upper Value increased to %d", yellow_vu);
			break;
		}
		case 'd': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x300c);
			yellow_vu -= hue_step;
			printf("\nyellow Upper Value decreased to %d", yellow_vu);
			break;
		}

		//PINK HSV COLOR MAPPING
		case 'V': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x4001);
			pink_hl += hue_step;
			printf("\nPink Lower Hue increased to %d", pink_hl);
			break;
		}
		case 'v': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x4002);
			pink_hl -= hue_step;
			printf("\nPink Lower Hue decreased to %d", pink_hl);
			break;
		}
		case 'F': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x4003);
			pink_hu += hue_step;
			printf("\nPink upper Hue increased to %d", pink_hu);
			break;
		}
		case 'f': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x4004);///
			pink_hu -= hue_step;
			printf("\nPink upper Hue decreased to %d", pink_hu);
			break;
		}
		case 'B': {
				IOWR(0x42000, EEE_IMGPROC_MSG, 0x4005);
			pink_sl += hue_step;
			printf("\npink Lower Saturation increased to %d", pink_sl);
			break;
		}
		case 'b': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x4006);
			pink_sl -= hue_step;
			printf("\npink Lower Saturation decreased to %d", pink_sl);
			break;
		}
		case 'G': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x4007);
			pink_su += hue_step;
			printf("\npink Upper Saturation increased to %d", pink_su);
			break;
		}
		case 'g': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x4008);
			pink_su -= hue_step;
			printf("\npink Upper Saturation decreased to %d", pink_su);
			break;
		}
		case 'N': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x4009);
			pink_vl += hue_step;
			printf("\npink Lower Value increased to %d", pink_vl);
			break;
		}
		case 'n': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x400a);
			pink_vl -= hue_step;
			printf("\npink Lower Value decreased to %d", pink_vl);
			break;
		}
		case 'H': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x400b);
			pink_vu += hue_step;
			printf("\npink Upper Value increased to %d", pink_vu);
			break;
		}
		case 'h': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x400c);
			pink_vu -= hue_step;
			printf("\npink Upper Value decreased to %d", pink_vu);
			break;
		}

		//Vision Modes
		case '/': {
			IOWR(0x42000, EEE_IMGPROC_MSG, 0x0100);
			filtered+=1;
			if(filtered%2) {
				printf("\nFilter OFF\n");
			} else {
				printf("\nFilter ON\n");
			}
			break;
		}case ';': {
			if(mode == 0){
				mode = 1;
				printf("\nColor config mode ON\n" );
			}else{
				mode = 0;
				printf("\nColor config mode OFF\n" );
			}
			break;
		}

		}

		//Main loop delay
		usleep(10000);

	};
	return 0;
}
