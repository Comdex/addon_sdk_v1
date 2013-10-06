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
	"method":"dollcatch.steer",
	"params":"U"\"D" \"L"\"R" \"F"\"B"
}

*/

#define  NB_NAME "dollcatch"


//���ﶨ��������Ҫʹ����Щ���� �Լ�һЩΪ���������ĺ궨��

#define IO1	WIFIIO_GPIO_17
#define IO2	WIFIIO_GPIO_18
#define IO3	WIFIIO_GPIO_19
#define IO4	WIFIIO_GPIO_20
#define IO5	WIFIIO_GPIO_21
#define IO6	WIFIIO_GPIO_22


#define STEER_Y_UP()	{api_io.low(IO1);api_io.high(IO2);}
#define STEER_Y_DOWN()	{api_io.high(IO1);api_io.low(IO2);}
#define STEER_Y_MID()	{api_io.low(IO1);api_io.low(IO2);}

#define STEER_X_RIGHT()	{api_io.low(IO3);api_io.high(IO4);}
#define STEER_X_LEFT()	{api_io.high(IO3);api_io.low(IO4);}
#define STEER_X_MID()	{api_io.low(IO3);api_io.low(IO4);}

#define STEER_Z_BACK()	{api_io.low(IO5);api_io.high(IO6);}
#define STEER_Z_FRONT()	{api_io.high(IO5);api_io.low(IO6);}
#define STEER_Z_MID()	{api_io.low(IO5);api_io.low(IO6);}


#define STEER_OP_TIME_LAST_MAX 1000

typedef struct{
	char steer;
}invoke_msg_opt_dollcatch_t;




///////////////////////////////////////////
// �ú���Ϊ��ʱ����������Ԥ��ʱ�䱻dollcatch
// ���̵��á�
///////////////////////////////////////////

int dollcatch_timer(nb_info_t* pnb, nb_timer_t* ptmr, void *ctx)
{
	if((void*)'R' == ctx || (void*)'L' == ctx)
		STEER_X_MID()
	if((void*)'U' == ctx || (void*)'D' == ctx)
		STEER_Y_MID()
	if((void*)'F' == ctx || (void*)'B' == ctx)
		STEER_Z_MID()
	else{
		STEER_X_MID();
		STEER_Y_MID();
		STEER_Z_MID();
	}
	return NB_TIMER_RELEASE;
}

////////////////////////////////////////
//�ú�����dollcatch�������У�ʵ�� timer����
////////////////////////////////////////
int timer_adjest_safe(nb_info_t*pnb, nb_invoke_msg_t* msg)
{
	//��ȡ������ߵ���ͼ
	invoke_msg_opt_dollcatch_t* opt = api_nb.invoke_msg_opt(msg);
	nb_timer_t* ptmr;



	if('U' == opt->steer){
		STEER_Y_UP();
	}
	else if('D' == opt->steer){
		STEER_Y_DOWN();
	}
	else if('R' == opt->steer){
		STEER_X_RIGHT();
	}
	else if('L' == opt->steer){
		STEER_X_LEFT();
	}
	else if('F' == opt->steer){
		STEER_Z_FRONT();
	}
	else if('B' == opt->steer){
		STEER_Z_BACK();
	}
	else {
		return STATE_OK;
	}


	//�����Ѿ����ţ���timer�ڵȴ��У���λ��timer
	ptmr = api_nb.timer_by_ctx(pnb, (void*)((u32_t)opt->steer));	


	if(NULL == ptmr){	//��û�о������µ�
		LOG_INFO("dollcatch:tmr alloced.\r\n");
		ptmr = api_nb.timer_attach(pnb, STEER_OP_TIME_LAST_MAX, dollcatch_timer, (void*)((u32_t)opt->steer), 0);	//����alloc
	}

	
	else{	//������ ���¶��������
		LOG_INFO("dollcatch:tmr restarted.\r\n");
		api_nb.timer_restart(pnb, STEER_OP_TIME_LAST_MAX, dollcatch_timer);
	}


	return STATE_OK;
}







////////////////////////////////////////
//�ú��������Ĳ��� dollcatch����
//�޷���֤�޾��������ʹ��nb����ṩ��
//invoke���ƣ���dollcatch����"ע��"���к���
////////////////////////////////////////

int dollcatch_steer_arrange(nb_info_t* pnb, char steer)
{

	LOG_INFO("DELEGATE:%c.\r\n",steer);

	//����һ��invoke������Ҫ������(��Ϣ)
	nb_invoke_msg_t* msg = api_nb.invoke_msg_alloc(timer_adjest_safe, sizeof(invoke_msg_opt_dollcatch_t), NB_MSG_DELETE_ONCE_USED);
	invoke_msg_opt_dollcatch_t* opt = api_nb.invoke_msg_opt(msg);

	opt->steer = steer;

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
// �ӿ�����Ϊ dollcatch.steer
////////////////////////////////////////

int __ADDON_EXPORT__ JSON_DELEGATE(steer)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)	//�̳���fp_json_delegate_start
{
	int ret = STATE_OK;
	char* err_msg = NULL;
	nb_info_t* pnb;
	if(NULL == (pnb = api_nb.find(NB_NAME))){		//�жϷ����Ƿ��Ѿ�����
		ret = STATE_ERROR;
		err_msg = "Service unavailable.";
		LOG_WARN("dollcatch:%s.\r\n", err_msg);
		goto exit_err;
	}

	LOG_INFO("DELEGATE dollcatch.steer.\r\n");


	char steer[4];
	if(JSMN_STRING != pjn->tkn->type || 
	     STATE_OK != jsmn.tkn2val_str(pjn->js, pjn->tkn, steer, sizeof(steer), NULL)){
		ret = STATE_ERROR;
		err_msg = "Params error.";
		LOG_WARN("dollcatch:%s.\r\n", err_msg);
		goto exit_err;
	}


	if('U' == steer[0] || 'D' == steer[0] || 'R' == steer[0] || 'L' == steer[0] || 'F' == steer[0] || 'B' == steer[0]){
		dollcatch_steer_arrange(pnb, steer[0]);
	}
	else {
		ret = STATE_ERROR;
		err_msg = "Params invalid.";
		LOG_WARN("dollcatch:%s.\r\n", err_msg);
		goto exit_err;
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
	 api_io.init(WIFIIO_GPIO_17, OUT_PUSH_PULL);
	 api_io.init(WIFIIO_GPIO_18, OUT_PUSH_PULL);
	 api_io.init(WIFIIO_GPIO_19, OUT_PUSH_PULL);
	 api_io.init(WIFIIO_GPIO_20, OUT_PUSH_PULL);
	 api_io.init(WIFIIO_GPIO_21, OUT_PUSH_PULL);
	 api_io.init(WIFIIO_GPIO_22, OUT_PUSH_PULL);

	 api_io.low(WIFIIO_GPIO_17);
	 api_io.low(WIFIIO_GPIO_18);
	 api_io.low(WIFIIO_GPIO_19);
	 api_io.low(WIFIIO_GPIO_20);
	 api_io.low(WIFIIO_GPIO_21);
	 api_io.low(WIFIIO_GPIO_22);

	LOG_INFO("dollcatch:IO initialized.\r\n");

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

static const nb_if_t nb_dollcatch_if = {enter, before_exit, exit};





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


	if(NULL == (pnb = api_nb.info_alloc(0)))	//����nb�ṹ
		goto err_exit;

	api_nb.info_preset(pnb, NB_NAME, &nb_dollcatch_if);	//���ṹ


	if(STATE_OK != api_nb.start(pnb)){
		LOG_WARN("main:Start error.\r\n");
		api_nb.info_free(pnb);
		goto err_exit;
	}

	LOG_INFO("main:Service starting...\r\n");


	return ADDON_LOADER_GRANTED;
err_exit:
	return ADDON_LOADER_ABORT;
}




