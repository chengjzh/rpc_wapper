//
//  CRPC.h
//  rpc
//
//  Created by 张成 on 2023/10/16.
//

#ifndef CRPC_h
#define CRPC_h
#include "IRPC.h"
#include "zmq.hpp"
#include <thread>
#include <atomic>

class CRPC : public IRPC {
public:
    CRPC();
    virtual ~CRPC();
    virtual bool SetRPCSink(IRefapp_RPCSink* rpcSink);
    virtual bool StartToListen(int rpc_port);
    virtual bool SendData(const std::string& body);
    virtual bool StopListen();
    virtual bool ReleaseConnection();
    virtual void SetIPAddress(const std::string &ip);

protected:
    IRPCSink* m_pSink = nullptr;
    zmq::context_t m_ZMQContext;
    zmq::socket_t m_SocketRep;
    zmq::socket_t m_SocketPush;
    std::thread m_ListenServiceThread;
    std::string m_strIPAddress = "tcp://*";
    std::atomic_bool m_bListeningService;
    std::atomic_bool m_bGetPushPort;

    bool m_bUseIPC = false;
    std::string m_strIPCPath;
    std::string m_strSocketRepPath;
    std::string m_strSocketPushPath;

    void HandleListenService();
    bool isConnectionCheck(const std::string& request);
    bool isListenPortNotification(const std::string& request);
    bool setListenPort(const std::string& request);
};

#endif /* CRPC_h */
