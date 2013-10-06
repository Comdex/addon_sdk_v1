



#include "include.h"





#define  PCF8591_ADDR 0x90    //PCF8591 ��ַ
#define  PCF8591_W 0x00
#define  PCF8591_R 0x01
#define  PCF8591_ADDR_W (PCF8591_ADDR|PCF8591_W)
#define  PCF8591_ADDR_R (PCF8591_ADDR|PCF8591_R)

#define  PCF8591_CTRL_CH0 0x00
#define  PCF8591_CTRL_CH1 0x01
#define  PCF8591_CTRL_CH2 0x02
#define  PCF8591_CTRL_CH3 0x03

#define  PCF8591_CTRL_AUTO 0x04


#define  PCF8591_CTRL_SINGLE 0x00
#define  PCF8591_CTRL_DIF3 0x10
#define  PCF8591_CTRL_MIX 0x20
#define  PCF8591_CTRL_DIF2 0x30

#define  PCF8591_CTRL_DA_EN 0x40



i2c_sim_dev_t i2c_dev;


/*
u8_t i2c_tx(i2c_sim_dev_t *dev, u8_t addr , u8_t* dat, int len)
{
	if(STATE_OK != i2c_sim.start(dev))			//��������
		return FALSE;
	i2c_sim.tx(dev, addr);		//����������ַ
	if(STATE_OK != i2c_sim.get_ack(dev)){
		i2c_sim.stop(dev); 
		return FALSE;
	}
	while(len-- > 0){
		i2c_sim.tx(dev, *dat++);			//��������
		if(STATE_OK != i2c_sim.get_ack(dev)){
			i2c_sim.stop(dev); 
			return FALSE;
		}
	}
	i2c_sim.stop(dev);			//��������
	return TRUE;
}
u8_t i2c_rx(i2c_sim_dev_t *dev, u8_t addr, u8_t* dat, int len)
{
	if(STATE_OK != i2c_sim.start(dev))			//��������
		return FALSE;
	i2c_sim.tx(dev, addr);		//����������ַ
	if(STATE_OK != i2c_sim.get_ack(dev)){
		i2c_sim.stop(dev); 
		return FALSE;
	}
	while(len-- > 0){
		*dat++ = i2c_sim.rx(dev);	//��ȡ����
		if(len > 0)
			i2c_sim.ack(dev);		//���ͷǾʹ�λ
		else
			i2c_sim.nack(dev);		//���ͷǾʹ�λ
	}
	i2c_sim.stop(dev);		//��������
	return TRUE;
}

*/

int main(int argc, char* argv[])
{
	int ret = STATE_OK;
	//step1: init iic device

	//��ʼ��ģ��iic
	i2c_sim.init(&i2c_dev,WIFIIO_GPIO_01 ,WIFIIO_GPIO_02 ,10);

	//��ʼ��PCF8591оƬ�����˲�����ͨ������
	do{
		u8_t tx_byte[4];
		tx_byte[0] = PCF8591_CTRL_DA_EN | PCF8591_CTRL_AUTO | PCF8591_CTRL_CH0;
		ret = i2c_sim.write(&i2c_dev, PCF8591_ADDR_W, tx_byte, 1);

		//ret = i2c_tx(&i2c_dev, PCF8591_ADDR_W, tx_byte, 1);

		LOG_INFO("pcf8591:init ret %d.\r\n", ret);

	}while(0);


//	if(STATE_OK== ret)
//		return ADDON_LOADER_GRANTED;
//	else
		return ADDON_LOADER_ABORT;

}


