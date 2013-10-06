#include "include.h"

#define IR_SEND_PIN WIFIIO_GPIO_01

void IR_send_38K(int circle)
{
	for(;circle > 0; circle--){
		api_io.high(IR_SEND_PIN);
		api_tim.tim6_poll(100);
		api_io.low(IR_SEND_PIN);
		api_tim.tim6_poll(163);
	}
}

int main(int argc, char* argv[])
{
	api_io.init(IR_SEND_PIN, OUT_PUSH_PULL);
	api_io.low(IR_SEND_PIN);

	api_tim.tim6_init(10000000);	//初始化定时器6 0.1us为单位

	while(1)
	{
		api_os.tick_sleep(1000);

		IR_send_38K(38000);
	}

//	return ADDON_LOADER_GRANTED;
//err_exit:
	return ADDON_LOADER_ABORT;		//release 
}


