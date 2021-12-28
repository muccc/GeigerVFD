/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "rotary_encoder.h"

/* esp32 GPIO pin mappings */
#define PIN_LED    32
#define PIN_PWRLED  4

#define PIN_LOAD   17
#define PIN_BLANK  16
#define PIN_CLK    18
#define PIN_DIN    19

#define PIN_BRIGHT 27

#define PIN_ROT1   14
#define PIN_ROT2   25

/* OUT pins of MAX6934 */
#define OUT(x) (1<<(31-x))

/* VFD - G pins */
#define G01 OUT(27)  // symbols
#define G02 OUT(26)  // rightmost
#define G03 OUT(25)
#define G04 OUT(24)
#define G05 OUT(23)
#define G06 OUT(22)
#define G07 OUT(21)
#define G08 OUT(20)
#define G09 OUT(19)
#define G10 OUT(18)
#define G11 OUT(17)  // leftmost

unsigned int grids[]={G11,G10,G09,G08,G07,G06,G05,G04,G03,G02,G01};  // left to right

/* VFD - P pins */
#define P01 OUT(28)  /* s1 */
#define P02 OUT(29)  /* s2 */
#define P03 OUT(30)  /* a  */
#define P04 OUT(31)  /* b  */
#define P05 OUT( 0)  /* f  */
#define P06 OUT( 2)  /* k  */
#define P07 OUT( 1)  /* j  */
#define P08 OUT( 4)  /* h  */
#define P09 OUT( 3)  /* m  */
#define P10 OUT( 6)  /* g  */
#define P11 OUT( 5)  /* n  */
#define P12 OUT( 8)  /* p  */
#define P13 OUT( 7)  /* r  */
#define P14 OUT(10)  /* c  */
#define P15 OUT( 9)  /* e  */
#define P16 OUT(12)  /* d  */
#define P17 OUT(11)  /* s3 */

/* P locations according to VFD datasheet:
    ---    a           3
   |\|/| fhjkb   5  8  7  6  4
    - -   g m      10     9
   |/|\| erpnc  15 13 12 11 14
    ---    d          16
 */

const int ascii[] = {
	P03|P05|P04|P10|P09|P15|P14,  /* a */
//	P05|P08|P10|P15|P13,          /* b */
//	P05|P15|P03|P16|P06|P11|P10,  /* b v2 */
	P05|P15|P10|P09|P14|P16,      /* b small */
	P03|P05|P15|P16,              /* c */
//	P05|P08|P15|P13,              /* d */
	P15|P10|P09|P16|P04|P14,      /* d small */
	P03|P05|P10|P15|P16,          /* e */
	P03|P05|P10|P15,              /* f */
	P03|P05|P15|P16|P11,          /* g */
	P05|P15|P10|P09|P04|P14,      /* h */
	P07|P12,                      /* i */
//	P07|P12|P16|P15,              /* j */
	P04|P14|P16|P15,              /* j v2 */
//	P07|P06|P12|P11,              /* k */
	P05|P15|P10|P06|P11,          /* k new */
	P05|P15|P16,                  /* l */
	P05|P15|P03|P07|P04|P14,      /* m */
	P05|P15|P03|P04|P14,          /* n */
	P03|P05|P04|P15|P14|P16,      /* o */
	P03|P05|P04|P15|P10|P09,      /* p */
	P03|P05|P04|P15|P14|P16|P11,  /* q */
	P03|P05|P15|P04|P09|P10|P11,  /* r */
//	P03|P07|P04|P09|P11|P12,      /* r v2 */
	P03|P05|P10|P09|P14|P16,      /* s */
	P03|P07|P12,                  /* t */
	P05|P15|P16|P04|P14,          /* u */
	P05|P15|P13|P06,              /* v */
	P05|P15|P16|P04|P14|P12,      /* w */
	P08|P06|P13|P11,              /* x */
	P08|P06|P12,                  /* y */
//	P03|P06|P13|P16,              /* z */
	P03|P06|P13|P16|P10|P09,      /* z v2 */
};


const int digit[] = {
	P03|P05|P04|P14|P15|P16,         // 0
	P04|P14,                         // 1
	P03|P04|P10|P09|P15|P16,         // 2
	P03|P04|P10|P09|P14|P16,         // 3
	P05|P10|P09|P04|P14,             // 4
	P03|P05|P10|P09|P14|P16,         // 5
	P03|P05|P10|P09|P15|P14|P16,     // 6
	P03|P04|P14,                     // 7
	P03|P05|P04|P10|P09|P15|P14|P16, // 8
	P03|P05|P04|P10|P09|P14|P16,     // 9
};


/* special symbols
             s1 s2 s3
	11G  <  >
	10G  bd
	09G  B  C
	08G
	07G  [] [] :
	06G  [] []
	05G  [] []
	04G  [] []
	03G  [] []
	02G  <  <
	01G  CCC -> <- ( ) analog digital (p11 - p17)
 */

#define SYM_CCC        G01|P11
#define SYM_LEFTARROW  G01|P12
#define SYM_RIGHTARROW G01|P13
#define SYM_LEFTPAREN  G01|P14
#define SYM_RIGHTPAREN G01|P15
#define SYM_ANALOG     G01|P16
#define SYM_DIGITAL    G01|P17
#define SYM_COLON      G07|P17
#define SYM_LEFT       G11|P01
#define SYM_RIGHT      G11|P02
#define SYM_DOLBY      G10|P01
#define SYM_B          G09|P01
#define SYM_C          G09|P02
#define SYM_PIP0       G07|P02
#define SYM_PIP1       G07|P01
#define SYM_PIP2       G06|P02
#define SYM_PIP3       G06|P01
#define SYM_PIP4       G05|P02
#define SYM_PIP5       G05|P01
#define SYM_PIP6       G04|P02
#define SYM_PIP7       G04|P01
#define SYM_PIP8       G03|P02
#define SYM_PIP9       G03|P01
#define SYM_REV_L      G02|P01
#define SYM_REV_R      G02|P02

unsigned int const pips[]={ SYM_PIP0, SYM_PIP1, SYM_PIP2, SYM_PIP3, SYM_PIP4, SYM_PIP5, SYM_PIP6, SYM_PIP7, SYM_PIP8, SYM_PIP9};

/* display high level */
#define DISPLAY_LEN 11
#define DISPLAY_TEXT_LEN (DISPLAY_LEN-1)
unsigned int outbuf[DISPLAY_LEN] = {0,0,0,0,0,0,0,0,0,0,0};

void vfd_write(char *str, unsigned char flags){
	int out=0;
	int in=0;

	// clear display
	for (out=0;out<DISPLAY_LEN;out++)
		outbuf[out]=0;

	out=DISPLAY_TEXT_LEN-strlen(str); // right-justify by default
	assert(out>=0);
	if (out<0)
		out=0;

	while ( str[in]!=0 && (in+out)<DISPLAY_TEXT_LEN ){
		char c=str[in];
		if (c>='0' && c<='9')
			outbuf[in+out]=digit[c-'0'];
		else if (c>='a' && c<='z')
			outbuf[in+out]=ascii[c-'a'];
		else if (c=='_')
			outbuf[in+out]=P16;
		else if (c=='-')
			outbuf[in+out]=P10|P09;
		else if (c=='+')
			outbuf[in+out]=P10|P09|P07|P12;
		else if (c=='<')
			outbuf[in+out]=P06|P11;
		else if (c=='>')
			outbuf[in+out]=P08|P13;
		else if (c==' ')
			outbuf[in+out]=0;
		else
			outbuf[in+out]=P10; /* error indicator */
		in++;
	};
};

void vfd_symbol(int sym){
	int x;
	
	for(x=0;x<DISPLAY_LEN;x++){
		if (sym&grids[x])
			outbuf[x]|= sym & ~grids[x];
	};
};

/* display low level */
static void display_timer_callback(void* arg);

spi_device_handle_t spi; // global spi handle
spi_transaction_t spi_t;

void spi_post_transfer_callback(spi_transaction_t *t) {
	int dc=(int)t->user;
	if (dc==1){
		ets_delay_us(1);
		gpio_set_level(PIN_LOAD, 1);
		ets_delay_us(1);
		gpio_set_level(PIN_LOAD, 0);
	};
}

void setup_display_spi(){
	esp_err_t ret;
	spi_bus_config_t buscfg={
		.miso_io_num=-1,
		.mosi_io_num=PIN_DIN,
		.sclk_io_num=PIN_CLK,
		.quadwp_io_num=-1,
		.quadhd_io_num=-1,
		.max_transfer_sz=32,
	};
	spi_device_interface_config_t devcfg={
		.clock_speed_hz=1*1000*1000,         // Clock out at 1 MHz (max=5MHz?)
		.mode=0,                             // SPI mode 0
		.spics_io_num=-1,                    // no CS pin
		.queue_size=7,                       // We want to be able to queue 7 transactions at a time
		.post_cb=spi_post_transfer_callback, // callback to enable LOAD
	};

	//Initialize non-SPI GPIOs
	gpio_set_direction(PIN_LOAD,  GPIO_MODE_OUTPUT);
	gpio_set_direction(PIN_BLANK, GPIO_MODE_OUTPUT);
	gpio_set_level(PIN_LOAD,  0);
	gpio_set_level(PIN_BLANK, 0);

	//Initialize the SPI bus
	ret=spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
	ESP_ERROR_CHECK(ret);
	//Attach the LCD to the SPI bus
	ret=spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);

	/* prepare spi transaction */
	memset(&spi_t, 0, sizeof(spi_t));
	spi_t.length=32;
	spi_t.flags = SPI_TRANS_USE_TXDATA;
	spi_t.user = (void*)1;
}

void setup_display_timer(){
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = &display_timer_callback,
		/* name is optional, but may help identify the timer when debugging */
		.name = "vfd_refresh"
	};

	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));

	printf("Started timers, time since boot: %lld us\n", esp_timer_get_time());

	/* Print debugging information about timers to console every 2 seconds */
//	for (int i = 0; i < 5; ++i) {
//		ESP_ERROR_CHECK(esp_timer_dump(stdout));
//		usleep(2000000);
//	}

	/* Remove timer */
//	ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
//	ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
}

void setup_display_pwm(){
	gpio_set_direction(PIN_BRIGHT, GPIO_MODE_OUTPUT);

	// should we use LEDC_HIGH_SPEED_MODE ?

	ledc_timer_config_t ledc_timer = {
		.duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
		.freq_hz = 60000,                    // frequency of PWM signal
		.speed_mode = LEDC_LOW_SPEED_MODE,   // timer mode
		.timer_num = LEDC_TIMER_0,           // timer index
		.clk_cfg = LEDC_AUTO_CLK,            // Auto select the source clock
	};
	ledc_timer_config(&ledc_timer);

	ledc_channel_config_t ledc_channel = {
		.channel    = LEDC_CHANNEL_0,
		.duty       = 0,
		.gpio_num   = PIN_BRIGHT,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.hpoint     = 0,
		.timer_sel  = LEDC_TIMER_0
	};
	ledc_channel_config(&ledc_channel);

#define LEDC_TEST_DUTY         (1<<5)
	ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, LEDC_TEST_DUTY);
	ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}


static void display_timer_callback(void* arg){
	static unsigned int cur_g =0;
	esp_err_t ret;

	/* write next segment */
	cur_g=(cur_g+1)%DISPLAY_LEN;

//	printf("[timer] g: %2d (%8x), out: %8x\n",cur_g,grids[cur_g],outbuf[cur_g]);

	// ESP32 is little endian, so the bytes have to be swapped....
	unsigned int vfd=grids[cur_g] | outbuf[cur_g];
	spi_t.tx_data[0]=(vfd>>24)&0xff;
	spi_t.tx_data[1]=(vfd>>16)&0xff;
	spi_t.tx_data[2]=(vfd>>8) &0xff;
	spi_t.tx_data[3]=(vfd)    &0xff;

	// fire & forget
	ret = spi_device_queue_trans(spi, &spi_t, 0);
//    ret = spi_device_polling_transmit(spi, &spi_t);

	assert( ret == ESP_OK );
	ESP_ERROR_CHECK(ret);
}

/* rotary encoder */
rotary_encoder_t *encoder = NULL;

void setup_rotary(){
	// Rotary encoder underlying device is represented by a PCNT unit in this example
	uint32_t pcnt_unit = 0;

	// Create rotary encoder instance
	rotary_encoder_config_t config = ROTARY_ENCODER_DEFAULT_CONFIG((rotary_encoder_dev_t)pcnt_unit, PIN_ROT1, PIN_ROT2);
	ESP_ERROR_CHECK(rotary_encoder_new_ec11(&config, &encoder));

	// Filter out glitch (1us)
//    ESP_ERROR_CHECK(encoder->set_glitch_filter(encoder, 1));

	// Start encoder
	ESP_ERROR_CHECK(encoder->start(encoder));
}

/* utility functions */
void print_esp_info(){
	/* Print chip information */
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
			CONFIG_IDF_TARGET,
			chip_info.cores,
			(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
			(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

	printf("silicon revision %d, ", chip_info.revision);

	printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
			(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
}

/* */

void app_main(void)
{
	printf("Startup...\n");
	print_esp_info();
//	vTaskDelay(1000 / portTICK_PERIOD_MS);

	printf("GO!\n");
	setup_display_spi();
	setup_display_pwm();

	printf("Hello world!\n");
	vfd_write("<hello>",0);
	vfd_symbol(SYM_CCC);
	setup_display_timer();
	usleep(3000000);

	setup_rotary();

	gpio_set_direction(PIN_LED,    GPIO_MODE_OUTPUT);
	gpio_set_direction(PIN_PWRLED, GPIO_MODE_OUTPUT);

	char outstr[99];
	int i=0;
	int evo=0;
	while(1){
		i++;

		//printf("Running loop %d ...\n", i);
		vTaskDelay(20 / portTICK_PERIOD_MS);
		gpio_set_level(PIN_LED, i%2);
		gpio_set_level(PIN_PWRLED, i%2);

		/* Proper euklidean definitions */
#define MOD(a,b) ((a)%(b)+(((a)%(b)<0)?(b):0))
#define DIV(a,b) ((a)<0 ? ((a)-(b)+1)/(b) : (a)/(b))

		int ev=encoder->get_counter_value(encoder);
		int ev4=DIV((ev+1),4); // round up

		sprintf(outstr,"%d %c %3d",ev4,'a'+MOD(DIV(ev4,10),26),i);
		vfd_write(outstr,0);

		int evm=MOD(ev4,10);
		if (evm!=evo){
			printf("ev: %d, ev4: %d, evm: %d\n",ev,ev4,evm);
			evo=evm;
		};
		vfd_symbol(pips[evm]);
	};

	printf("Restarting now.\n");
	fflush(stdout);
	esp_restart();
}
