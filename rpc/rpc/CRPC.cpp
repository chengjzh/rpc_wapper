//
//  CRPC.cpp
//  rpc
//
//  Created by 张成 on 2023/10/16.
//
#include <iostream>
#include "CRPC.h"

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif


CRPC::CRPC() {
    m_SocketRep = zmq::socket_t(m_ZMQContext, ZMQ_REP);
    m_SocketRep.set(zmq::sockopt::rcvtimeo, 1000);  // refresh for a request every 1000 ms
    m_SocketRep.set(zmq::sockopt::linger, 10000);  // wait for 10000 ms before closing if there are pending messages have not been sent
    
    m_SocketPush = zmq::socket_t(m_ZMQContext, ZMQ_PUSH);
    m_SocketPush.set(zmq::sockopt::linger, 10000);
    
    m_bListeningService = false;
    m_bGetPushPort = false;
}

CRPC::~CRPC() {
    ReleaseConnection();
}

bool CRPC::SetRPCSink(IRPCSink* rpcSink) {
    if (!rpcSink) {
        return false;
    }
    m_pSink = rpcSink;
    return true;
}

bool CRPC::StartToListen(int rpc_port) {
    m_bListeningService = true;

    std::string addr;
    if (m_bUseIPC) {
        addr = m_strIPCPath + "/zmq_ipc_" + std::to_string(rpc_port);
    }
    else {
        addr = m_strIPAddress + ":" + std::to_string(rpc_port);
    }

    try {
        m_SocketRep.bind(addr);
    }
    catch (const zmq::error_t& error){
        return false;
    }
    m_strSocketRepPath = addr;
    m_ListenServiceThread = std::thread([this] {HandleListenService();});
    return true;
}

void CRPC::HandleListenService() {
    zmq::message_t recv, send;
#ifdef ANDROID
    LOGI("CRefapp_RPC::HandleListenService");
#endif
    while (m_bListeningService) {
        auto recv_res = m_SocketRep.recv(recv, zmq::recv_flags::none);
        if (!recv_res) {
            // receive timeout
            continue;
        }

        // decode received request
        std::string request, response;
        request = std::string(static_cast<char*>(recv.data()), recv.size());
        // check if the request is used to notify listen port
        if (isConnectionCheck(request)) {
            response = "rpc>>success";
        }
        else if (isListenPortNotification(request)) {
            if (setListenPort(request)) {
                m_bGetPushPort = true;
                response = "rpc>>success";
            }
            else {
                response = "rpc>>failure";
            }
        }
        // handle normal backdoor request
        else {
            if (m_pSink) {
                response = m_pSink->RecvProc(request);
            }
            else {
                break;
            }
        }

        // send response back
        send.rebuild(response.c_str(), response.size());
        auto send_res = m_SocketRep.send(send, zmq::send_flags::none);
        if (!send_res) {
            break;
        }
    }
}

bool CRPC::isConnectionCheck(const std::string &request) {
    return request.rfind("<connection check>", 0) == 0;
}

bool CRPC::isListenPortNotification(const std::string& request) {
    return request.rfind("<listen port>", 0) == 0;
}

bool CRPC::setListenPort(const std::string& request) {
    std::string strPort = request.substr(13);
    int iListenPort = std::stoi(strPort);
    
    std::string last_endpoint = m_SocketPush.get(zmq::sockopt::last_endpoint);
    if (!last_endpoint.empty()) {
        try {
            m_SocketPush.unbind(last_endpoint);
        }
        catch (const zmq::error_t& error) {
        }
    }

    std::string addr;
    if (m_bUseIPC) {
        addr = m_strIPCPath + "/zmq_ipc_" + std::to_string(iListenPort);
    }
    else {
        addr = m_strIPAddress + ":" + std::to_string(iListenPort);
    }

    try {
        m_SocketPush.bind(addr);
    }
    catch (const zmq::error_t& error) {
        return false;
    }
    m_strSocketPushPath = addr;
    return true;
}

bool CRPC::SendData(const std::string& body) {
    if(!m_bGetPushPort){
        return false;
    }
    zmq::message_t send(body.c_str(), body.size());
    auto send_res = m_SocketPush.send(send, zmq::send_flags::none);
    return true;
}

bool CRPC::StopListen() {
    if (m_bListeningService) {
        m_bListeningService = false;
        if (m_ListenServiceThread.joinable()) {
            m_ListenServiceThread.join();
        }
        else {
            return false;
        }
    }
    return true;
}

bool CRPC::ReleaseConnection() {
    if (m_bListeningService) {
        m_bListeningService = false;
        if (m_ListenServiceThread.joinable()) {
            m_ListenServiceThread.join();
        }
        else {
            return false;
        }
    }

    // close all sockets and the context
    m_SocketRep.close();
    m_SocketPush.close();
    m_ZMQContext.close();

    // remove ipc socket files
    if (m_bUseIPC) {
#if defined(_WIN32)
        if (!m_strSocketRepPath.empty()) {
            _unlink(m_strSocketRepPath.substr(6).c_str());
        }
        if (!m_strSocketPushPath.empty()) {
            _unlink(m_strSocketPushPath.substr(6).c_str());
        }
#elif defined(__linux__) || ( defined(__APPLE__) && defined(__MACH__) )
        if (!m_strSocketRepPath.empty()) {
            unlink(m_strSocketRepPath.substr(6).c_str());
        }
        if (!m_strSocketPushPath.empty()) {
            unlink(m_strSocketPushPath.substr(6).c_str());
        }
#endif
    }

    return true;
}

void CRPC::SetIPAddress(const std::string &ip) {
    m_strIPAddress = ip;
}

void CRPC::UseIPC(bool enableIPC) {
    m_bUseIPC = enableIPC;

    if (m_bUseIPC) {
#if defined(_WIN32)
        char const* tmpdir = getenv("TMP");
#elif defined(__linux__) || defined(__APPLE__)
        char const* tmpdir = getenv("TMPDIR");
#endif
        if (tmpdir == 0) {
            tmpdir = "/tmp";
        }
        m_strIPCPath = std::string("ipc://") + tmpdir;
    }
    std::cout << "Refapp_RPC::UseIPC, m_bUseIPC=" << m_bUseIPC << std::endl;
}
