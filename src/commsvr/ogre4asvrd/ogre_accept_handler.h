/******************************************************************************************
Copyright           : 2000-2004, FXL Technology (Shenzhen) Company Limited.
FileName            : ogre_accepthandler.h
Author              : Jovi(HuangHao),Sail(ZENGXING)
Version             :
Date Of Creation    : 2006��11��7��
Description         : ACCPET��Handler����

Others              :
Function List       :
    1.  ......
Modification History:
    1.Date  :
      Author  :
      Modification  :
******************************************************************************************/

#ifndef OGRE_TCP_ACCEPT_HANDLER_H_
#define OGRE_TCP_ACCEPT_HANDLER_H_

/****************************************************************************************************
class  OgreTCPAcceptHandler TCP Accept ������EventHandler
****************************************************************************************************/
class Ogre4aIPRestrictMgr;
class OgreTCPAcceptHandler : public ZEN_Event_Handler
{

public:
    //���캯��
    OgreTCPAcceptHandler(const ZEN_Sockaddr_In &listen_addr,
                         const char *recv_mod_file,
                         ZEN_Reactor *reactor = ZEN_Reactor::instance());
protected:
    ~OgreTCPAcceptHandler();
public:
    //����Bind��IP��ַ
    void set_peer_bindaddr(ZEN_Sockaddr_In &addr);

    //���������Ķ˿�
    int create_listenpeer();

    //�����ж˿�
    virtual int handle_input(ZEN_HANDLE );

    //�رմ���
    virtual int handle_close ();

    //�õ�Acceptor�ľ��
    ZEN_SOCKET get_handle(void) const;

protected:

    //ACCPET PEER
    ZEN_Socket_Acceptor        peer_acceptor_;

    //��ĵ�ַ
    ZEN_Sockaddr_In            accept_bind_addr_;

    //��ȡ���ݵ�ģ����ļ�����
    std::string                recv_mod_file_;
    //TCP��ȡ���ݵ�ģ��HANDLER
    ZEN_SHLIB_HANDLE           recv_mod_handler_;

    //ģ���ܽ�ĺ�������
    FPJudgeRecvWholeFrame      fp_judge_whole_frame_;

    //IP���ƹ�����
    Ogre4aIPRestrictMgr       *ip_restrict_;

};

#endif //_OGRE_TCP_ACCEPT_HANDLER_H_
