#include "soar_predefine.h"
#include "soar_error_code.h"
#include "soar_zerg_frame_malloc.h"
#include "soar_svrd_cfg_fsm.h"
#include "soar_fsm_notify_trans_mgr.h"
#include "soar_fsm_notify_taskbase.h"
#include "soar_fsm_notify_transbase.h"
#include "soar_svrd_app_fsm_notify.h"

Comm_SvrdApp_FSM_Notify::Comm_SvrdApp_FSM_Notify():
    Comm_Svrd_Appliction()
{
};

Comm_SvrdApp_FSM_Notify::~Comm_SvrdApp_FSM_Notify()
{

};

//���ӵ���register_func_cmd
int Comm_SvrdApp_FSM_Notify::on_start(int argc, const char *argv[])
{
    int ret = 0;

    ret = Comm_Svrd_Appliction::on_start(argc,argv);
    if (SOAR_RET::SOAR_RET_SUCC != ret)
    {
        return ret;
    }

    THREADMUTEX_APPFRAME_MALLOCOR::instance()->initialize();

    Server_Config_FSM *svd_config = Server_Config_FSM::instance();
    MT_NOTIFY_TRANS_MANGER *trans_mgr = new MT_NOTIFY_TRANS_MANGER();
    Transaction_Manager::instance(trans_mgr);
    ZCE_Time_Value enqueue_timeout;
    enqueue_timeout.sec(svd_config->framework_config_.task_info_.enqueue_timeout_sec_);
    enqueue_timeout.usec(svd_config->framework_config_.task_info_.enqueue_timeout_usec_);
    //����������ĳ�ʼ��
    trans_mgr->initialize(
        svd_config->framework_config_.trans_info_.trans_cmd_num_,
        svd_config->framework_config_.trans_info_.trans_num_,
        self_services_id_,
        enqueue_timeout,
        ZCE_Timer_Queue::instance(),
        Zerg_MMAP_BusPipe::instance(),
        THREADMUTEX_APPFRAME_MALLOCOR::instance());

    ret = register_notifytrans_cmd();

    if (ret != SOAR_RET::SOAR_RET_SUCC)
    {
        return ret;
    }

    NotifyTrans_TaskBase *clone_task = NULL;
    size_t task_num = 0;
    size_t task_stack_size = 0;

    // task_num task_stack_size���ﲻ��ʹ�ã�ԭ����¶�����
    ret = register_notify_task(clone_task,
                               task_num,
                               task_stack_size);

    // ����app���õĳ�ʼ�����ڿ�ܳ�ʼ���������������register_notify_taskʱ��
    // appʵ���ϻ�û�м������ã����ǵ�task���������ڿ�ܸ�����һЩ��
    // ���ｫTask�������Ƶ���framework.xml������
    // ��ʼ��DB�̣߳�
    ret = trans_mgr->active_notify_task(
              clone_task,
              svd_config->framework_config_.task_info_.task_thread_num_,
              svd_config->framework_config_.task_info_.task_thread_stack_size_);

    if (ret != SOAR_RET::SOAR_RET_SUCC)
    {
        ZLOG_INFO("[framework] InitInstance DBSvrdTransactionManger fail.Ret = %u", ret);
        return ret;
    }

    return SOAR_RET::SOAR_RET_SUCC;
}

//���д���,
int Comm_SvrdApp_FSM_Notify::on_run()
{
    // fix me add log
    ZLOG_INFO("======================================================================================================");
    ZLOG_INFO("[framework] app %s class [%s] run_instance start.",
              get_app_basename(),
              typeid(*this).name());

    //����N�κ�,����SELECT�ĵȴ�ʱ����
    const unsigned int LIGHT_IDLE_SELECT_INTERVAL = 128;
    //����N�κ�,SLEEP��ʱ����
    const unsigned int HEAVY_IDLE_SLEEP_INTERVAL = 10240;

    //microsecond
    // 64λtlinux��idle��ʱ�����̫�̻ᵼ��cpu����
    const int LIGHT_IDLE_INTERVAL_MICROSECOND = 10000;
    const int HEAVY_IDLE_INTERVAL_MICROSECOND = 100000;


    size_t all_proc_frame = 0 , all_gen_trans = 0;
    size_t prcframe_queue = 0 , gentrans_queue = 0, num_timer_expire = 0, num_io_event = 0;
    size_t idle = 0;

    MT_NOTIFY_TRANS_MANGER *notify_trans_mgr = static_cast<MT_NOTIFY_TRANS_MANGER *>(Transaction_Manager::instance());
    ZCE_Time_Value select_interval(0, 0);

    ZCE_Timer_Queue *time_queue = ZCE_Timer_Queue::instance();
    ZCE_Reactor *reactor = ZCE_Reactor::instance();

    for (; app_run_;)
    {
        // ����Ƿ���Ҫ���¼�������


        //��PIPE�����յ�������
        notify_trans_mgr->process_pipe_frame(all_proc_frame, all_gen_trans);
        //��RECV QUEUE��������
        notify_trans_mgr->process_recvqueue_frame(prcframe_queue, gentrans_queue);
        all_proc_frame += prcframe_queue;
        all_gen_trans += gentrans_queue;

        //��ʱ
        num_timer_expire = time_queue->expire();

        // ���������
        reactor->handle_events(&select_interval, &num_io_event);

        if ((all_proc_frame + num_timer_expire + num_io_event) <= 0)
        {
            ++idle;
        }
        else
        {
            idle = 0;
        }

        //���æ�������ɻ�
        if (idle < LIGHT_IDLE_SELECT_INTERVAL)
        {
            select_interval.usec(0);
            continue;
        }
        //������кܶ�,��Ϣһ��,�����ȽϿ��У������SELECT�൱��Sleep��
        else if (idle >= HEAVY_IDLE_SLEEP_INTERVAL)
        {
            select_interval.usec(HEAVY_IDLE_INTERVAL_MICROSECOND );
        }
        //else �൱�� else if (idle >= LIGHT_IDLE_SELECT_INTERVAL)
        else
        {
            select_interval.usec(LIGHT_IDLE_INTERVAL_MICROSECOND );
        }
    }

    ZLOG_INFO("======================================================================================================");
    return SOAR_RET::SOAR_RET_SUCC;
}

//�˳�����
int Comm_SvrdApp_FSM_Notify::on_exit()
{
    //֪ͨ���е��߳��˳�
    MT_NOTIFY_TRANS_MANGER *notify_trans_mgr = static_cast<MT_NOTIFY_TRANS_MANGER *>(Transaction_Manager::instance());
    notify_trans_mgr->stop_notify_task();

    int ret = 0;
    Transaction_Manager::clean_instance();

    //�ȴ����е�Join���߳��˳�
    //ACE_Thread_Manager::instance()->wait();

    ret = Comm_Svrd_Appliction::on_exit();

    if ( SOAR_RET::SOAR_RET_SUCC != ret )
    {
        return ret;
    }

    return SOAR_RET::SOAR_RET_SUCC;
}
