/*
	�ó�����ʾ���ʹ��noneblock��� ��д���Ա��ƶ˻��߱�����������Ƶĵ����Ӧ�á�
	��ʵ�ֿ������õƾ߶�ʱ���� �Լ���ʱ�����Ч����
	����������nb����еĶ�ʱ��ʵ�� �ԵƾߵĿ��ƣ�����Ч��Ҳ�����ö�ʱ����������ʵ�֡�
	��������ʾ��:
	1��nb��ܽ�����������
	2��ͨ��nb��ܵ�invoke���� ����������һ������(������)������
	3��nb�Ķ�ʱ��Ӧ�ã�
	4��pwm����
	5��jsonί�нӿڵ�ʹ��(jsonί�нӿ���httpd��otd����Ĭ�ϵ��ⲿ����ӿ�)
*/


#include "include.h"

/*
ģ��:

�����ʽ: 
{
	"method":"schedule",
	"params":[
		[val, time, period, 1],
		...
		[val, time, period, 2,3],
	]
}

val: ���� 0~99
time: ��ʱ����ʱ�䣬��λs 0~0xFFFFFFFF
period: ����ʱ�� ��λs 0~60

ʾ����
���ص� 1 2 3
[
	[99/0, 0, 0, 1,2,3],
]

���صƣ�3s���� 1 2
[
	[99/0, 0, 3, 1,2],
]


10s�󿪹صƣ�3s���� 1 2
[
	[99/0, 10, 3, 1,2],
]


*/

#define  NB_NAME "lighting"
#define BRIGHTNESS(lv) (0.11f*math.pow(1.07f,lv))	//google "y = 0.11*1.07^x"



typedef struct{
	u8_t dev;
	wifiIO_pwm_t pwm_dev;
	u8_t Vdest;	//Ŀ��ֵ

	u32_t dT;		//��ʱ����ʱ��
	u32_t dt;		//�������ʱ��

}timer_opt_lighting_t;

typedef struct {
	u8_t lv[4];
}lighting_opt_t;



///////////////////////////////////////////
// �ú���Ϊ��ʱ����������Ԥ��ʱ�䱻lighting
// ���̵��á�
///////////////////////////////////////////

int lighting_timer(nb_info_t* pnb, nb_timer_t* ptmr, void *ctx)
{
	lighting_opt_t *dev_opt = (lighting_opt_t*) api_nb.info_opt(pnb);
	timer_opt_lighting_t* opt = (timer_opt_lighting_t*)api_nb.timer_opt(ptmr);
	u32_t now = api_os.tick_now();


	if(now < opt->dT){		//�Ⱥ�׶�
		//�����Ⱥ�
		LOG_INFO("Timer:waiting.\r\n");
		return opt->dT - now;
	}

	else if(now <= opt->dt){	//����׶�

		int dt, dv;

//		u16_t v = api_pwm.t_read(opt->pwm_dev);
		u16_t v = dev_opt->lv[opt->dev];	//��ǰֵ

		dt = opt->dt - now;	//���ʱ��
		dv = opt->Vdest - v;	//���ֵ


		api_console.printf(dv>0?"+":"-");

		if(dv == 0)	//�Ѿ����
			//�ͷŸ�timer
			return NB_TIMER_RELEASE;

		else	if(dt <= 100){	//ʱ���С��100ms

			dev_opt->lv[opt->dev] = v+dv;
			api_pwm.t_set(opt->pwm_dev, BRIGHTNESS(dev_opt->lv[opt->dev]));	//���

			//�ͷŸ�timer
			return NB_TIMER_RELEASE;
		}

		else{
			int div = 1, _dv = ABS(dv);

			//������ݽ���ʱ�� �� ��ֵ ����һ������ʱ��Ͳ���ֵ

			//����ʱ�������100ms
			while(dt/div > 100 && _dv/div >= 1)div++;
			div--;

			//���µ�ǰֵ

			dev_opt->lv[opt->dev] = v+dv/div;
			api_pwm.t_set(opt->pwm_dev, BRIGHTNESS(dev_opt->lv[opt->dev]));	//���


			//ָ����һ��timer��ʱʱ��
			return dt/div;
		}

	}

	else{	//����ԭ�򣬳�ʱ
		api_console.printf("#");
		LOG_INFO("Timer:%u->%u->%u.(%u)#\r\n",opt->dT,opt->dt,now, opt->Vdest);

		dev_opt->lv[opt->dev] = opt->Vdest;
		api_pwm.t_set(opt->pwm_dev, BRIGHTNESS(dev_opt->lv[opt->dev]));	//ֱ���������ֵ


		//�ͷŸ�timer
		return NB_TIMER_RELEASE;
	}		


	return NB_TIMER_RELEASE;
}

////////////////////////////////////////
//�ú�����lighting�������У�ʵ�� timer����
////////////////////////////////////////
int timer_adjest_safe(nb_info_t*pnb, nb_invoke_msg_t* msg)
{
	//��ȡ������ߵ���ͼ
	timer_opt_lighting_t* opt = api_nb.invoke_msg_opt(msg);

	int wait = opt->dT - api_os.tick_now();	//�ȼ��㻹��Ҫ�Ⱥ��ʱ����
	if(wait < 0)wait = 0;


	//���氲�ź��ʵ�timer�������ĳ���ƾߵĿ���
	nb_timer_t* ptmr;

	//�����Ѿ����ţ���timer�ڵȴ��У���λ��timer
	ptmr = api_nb.timer_by_ctx(pnb, (void*)((u32_t)opt->dev));	


	if(NULL == ptmr){	//��û�о������µ�
		LOG_INFO("lighting:tmr alloced.\r\n");
		ptmr = api_nb.timer_attach(pnb, wait, lighting_timer, (void*)((u32_t)opt->dev), sizeof(timer_opt_lighting_t));	//����alloc
	}

	
	else{	//������ ���¶��������
		LOG_INFO("lighting:tmr restarted.\r\n");
		api_nb.timer_restart(pnb, wait, lighting_timer);
	}

	//timer�����Ķ�������
	timer_opt_lighting_t* tmr_opt = (timer_opt_lighting_t*)api_nb.timer_opt(ptmr);
	string.memcpy(tmr_opt, opt, sizeof(timer_opt_lighting_t));	//����Ҫ�Ĳ��� ��ֵ��timer

	return STATE_OK;
}







////////////////////////////////////////
//�ú��������Ĳ��� lighting����
//�޷���֤�޾��������ʹ��nb����ṩ��
//invoke���ƣ���lighting����"ע��"���к���
////////////////////////////////////////

int lighting_pwm_arrange(nb_info_t* pnb, int dev, u8_t val, u32_t time, u8_t period)
{

	LOG_INFO("DELEGATE:dev%u val:%u,time:%u,period:%u.\r\n",dev,val,time,period);

	//����һ��invoke������Ҫ������(��Ϣ)
	nb_invoke_msg_t* msg = api_nb.invoke_msg_alloc(timer_adjest_safe, sizeof(timer_opt_lighting_t), NB_MSG_DELETE_ONCE_USED);
	timer_opt_lighting_t* opt = api_nb.invoke_msg_opt(msg);

	//��д��Ӧ��Ϣ
	opt->dev = dev;
	if(0 == dev){
		opt->pwm_dev = WIFIIO_PWM_05CH1;
	}
	else	if(1 == dev){
		opt->pwm_dev = WIFIIO_PWM_05CH2;
	}
	else	if(2 == dev){
		opt->pwm_dev = WIFIIO_PWM_09CH1;
	}
	else	if(3 == dev){
		opt->pwm_dev = WIFIIO_PWM_09CH2;
	}
	opt->Vdest = val;

	u32_t now = api_os.tick_now();
	opt->dT = now + time*1000;
	opt->dt = opt->dT + period*1000;

	//���͸�lighting���� ʹ������ timer_adjest_safe �������
	api_nb.invoke_start(pnb , msg);
	return STATE_OK;
}






////////////////////////////////////////
//ί�нӿڵĶ������:
// JSON_DELEGATE() ��Ϊ������Ӻ�׺����
// __ADDON_EXPORT__ �꽫�÷��Ŷ���Ϊ��¶��
// ������ʽ �� ��������һ��json��������һ��
// ִ����ɺ�Ļص� ���丽��������
//
// �ú��������������� �ǵ��ø�ί�нӿڵĽ���
// httpd��otd �����Ե��øýӿ�
// �ӿ�����Ϊ lighting.schedule
////////////////////////////////////////

int JSON_RPC(schedule)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)	//�̳���fp_json_delegate_start
{
	int ret = STATE_OK;
	char* err_msg = NULL;
	nb_info_t* pnb;
	if(NULL == (pnb = api_nb.find(NB_NAME))){		//�жϷ����Ƿ��Ѿ�����
		ret = STATE_ERROR;
		err_msg = "Service unavailable.";
		LOG_WARN("lighting:%s.\r\n", err_msg);
		goto exit_err;
	}

	LOG_INFO("DELEGATE lighting.schedule.\r\n");


	jsmntok_t* jt = pjn->tkn;	//ָ������������[[],..[]]

	if(JSMN_ARRAY != jt->type){
		ret = STATE_ERROR;
		err_msg = "Cmd array needed.";
		LOG_WARN("lighting:%s.\r\n", err_msg);
		goto exit_err;
	}

	int cmd_num = jt->size;
	LOG_INFO("%u cmds.\r\n", cmd_num);

	jt += 1; 	//skip [
	while(cmd_num--){
		int dev_num = 0;
		jsmntok_t* j = jt;
		u8_t val,period;
		u32_t time;
	
		if(JSMN_ARRAY != j->type || j->size <= 3){
			ret = STATE_PARAM;
			err_msg = "Cmd invalid.";
			LOG_WARN("lighting:%s. %u %u\r\n", err_msg, j->type, j->size);
			goto exit_err;
		}
		dev_num = j->size - 3;	//�������鵥Ԫ���� �õ�pwm���Ƶ�Ԫ������
		j += 1;	//skip [


		jsmn.tkn2val_u8(pjn->js, j, &val);	//��1����Ԫ�� val
		j++;
		jsmn.tkn2val_uint(pjn->js, j, &time);	//��2����Ԫ�� time
		j++;
		jsmn.tkn2val_u8(pjn->js, j, &period);	//��3����Ԫ�� period
		j++;


		//�����ÿ��pwm��������
		while(dev_num--){
			u8_t dev;
			jsmn.tkn2val_u8(pjn->js, j, &dev);
			lighting_pwm_arrange(pnb, dev, val, time, period);
			j++;
		}

		jt = jsmn.next(jt);	//��Ϊÿ����λ����primitive������˲���ʹ��+1���л�
	}
	

//exit_safe:
	if(ack)
		jsmn.delegate_ack_result(ack, ctx, ret);
	return ret;

exit_err:
	if(ack)
		jsmn.delegate_ack_err(ack, ctx, ret, err_msg);
	return ret;


	
}


////////////////////
//����nb��ܵ��ûص�
////////////////////


	///////////////////////////
	//����nb��Ϣѭ��֮ǰ������
	///////////////////////////
 int enter(nb_info_t* pnb)
 {
	 api_pwm.init(WIFIIO_PWM_05CH1, 600, 100, 0);
	 api_pwm.init(WIFIIO_PWM_05CH2, 600, 100, 0);
	 api_pwm.init(WIFIIO_PWM_09CH1, 600, 100, 0);
	 api_pwm.init(WIFIIO_PWM_09CH2, 600, 100, 0);


	lighting_opt_t *opt = (lighting_opt_t*) api_nb.info_opt(pnb);

	 api_pwm.init(WIFIIO_PWM_05CH1, 600, 100, BRIGHTNESS(opt->lv[0]));
	 api_pwm.init(WIFIIO_PWM_05CH2, 600, 100, BRIGHTNESS(opt->lv[1]));
	 api_pwm.init(WIFIIO_PWM_09CH1, 600, 100, BRIGHTNESS(opt->lv[2]));
	 api_pwm.init(WIFIIO_PWM_09CH2, 600, 100, BRIGHTNESS(opt->lv[3]));

	 api_pwm.start(WIFIIO_PWM_05CH1);
	 api_pwm.start(WIFIIO_PWM_05CH2);
	 api_pwm.start(WIFIIO_PWM_09CH1);
	 api_pwm.start(WIFIIO_PWM_09CH2);
	LOG_INFO("lighting:PWM initialized.\r\n");

	//////////////////////////////////////////////////////
	//����STATE_OK֮���ֵ������������Ϣѭ����ֱ���˳�����
	//////////////////////////////////////////////////////
 	return STATE_OK;
 }
 
	///////////////////////////
	//nb���յ��˳���Ϣʱ������
	///////////////////////////
 int before_exit(nb_info_t* pnb)
 {
 	return STATE_OK;
 }
 
	///////////////////////////
	//�˳�nb��Ϣѭ��֮�󱻵���
	///////////////////////////
 int exit(nb_info_t* pnb)
 {
 	return STATE_OK;
 }

////////////////////
//�ص�������ϳɽṹ
////////////////////

static const nb_if_t nb_lighting_if = {enter, before_exit, exit};





////////////////////////////////////////
//ÿһ��addon����main���ú����ڼ��غ�����
// �����ʺ�:
// addon�������л�����飻
// ��ʼ��������ݣ�

//������ �� ADDON_LOADER_GRANTED �����º������غ�
//��ж��

//�ú��������������� ��wifiIO����
////////////////////////////////////////

int main(int argc, char* argv[])
{
	nb_info_t* pnb;

	if(NULL != api_nb.find(NB_NAME)){		//�жϷ����Ƿ��Ѿ�����
		LOG_WARN("main:Service exist,booting abort.\r\n");
		goto err_exit;
	}



	////////////////////////////////////////
	//�������ǽ���һ��nb��ܳ���Ľ��̣�
	//����ΪNB_NAME 
	////////////////////////////////////////


	if(NULL == (pnb = api_nb.info_alloc(sizeof(lighting_opt_t))))	//����nb�ṹ
		goto err_exit;

	lighting_opt_t *opt = (lighting_opt_t*) api_nb.info_opt(pnb);
	opt->lv[0] = 0;
	opt->lv[1] = 0;
	opt->lv[2] = 0;
	opt->lv[3] = 0;


	api_nb.info_preset(pnb, NB_NAME, &nb_lighting_if);	//���ṹ


	if(STATE_OK != api_nb.start(pnb)){		//������
		LOG_WARN("main:Start error.\r\n");
		api_nb.info_free(pnb);
		goto err_exit;
	}

	LOG_INFO("main:Service starting...\r\n");


	return ADDON_LOADER_GRANTED;
err_exit:
	return ADDON_LOADER_ABORT;
}




//httpd�ӿ�  ��� json
/*
GET http://192.168.1.105/logic/wifiIO/invoke?target=lighting.status

{"state":loaded}

*/


int __ADDON_EXPORT__ JSON_FACTORY(status)(char*arg, int len, fp_consumer_generic consumer, void *ctx)
{
	nb_info_t* pnb;
	char buf[128];
	int n, size = sizeof(buf);

	if(NULL == (pnb = api_nb.find(NB_NAME))){		//�жϷ����Ƿ��Ѿ�����
		goto exit_noload;
	}

	lighting_opt_t *opt = (lighting_opt_t*) api_nb.info_opt(pnb);


	n = 0;
	n += utl.snprintf(buf + n, size-n, "{\"state\":\"loaded\",\"r\":%u,\"g\":%u,\"b\":%u}", opt->lv[1], opt->lv[2], opt->lv[3]);
	return  consumer(ctx, (u8_t*)buf, n);

exit_noload:
	n = 0;
	n += utl.snprintf(buf + n, size-n, "{\"state\":\"not loaded\"}");

	return  consumer(ctx, (u8_t*)buf, n);
}






