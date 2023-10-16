//
//  rpc.h
//  rpc
//
//  Created by 张成 on 2023/9/21.
//


#ifndef IMPT_IPC_h
#define IMPT_IPC_h
#include <string>
#include "mpt_rpc_define.pb.h"

#ifndef IRefapp_RPC_h
#define IRefapp_RPC_h

#include <string>

class IRPCSink {
public:
    virtual std::string RecvProc(const std::string& request) = 0;
};

class IRPC {
public:
    /*set rpc sink, then it will notify the command*/
    virtual bool SetRPCSink(IRPCSink* rpcSink) = 0;
    
    /*start to listen the request from python*/
    virtual bool StartToListen(int rpc_port) = 0;
    
    /*send json string*/
    virtual bool SendData(const std::string& body) = 0;
    
    /*stop listening to requests*/
    virtual bool StopListen() = 0;
    
    /*release all the connection*/
    virtual bool ReleaseConnection() = 0;
    
    /*set a specific ip address*/
    virtual void SetIPAddress(const std::string& ip) = 0;
};

#if defined(_WIN32) || defined(_WIN64)
    #define RPC_EXPORT __declspec(dllexport)
#else
    #define RPC_EXPORT
#endif

extern "C" {
RPC_EXPORT bool Create_RPC(IRPC** ppRPC){
    if (ppRPC) {
        CRPC* rpc = new CRPC;
        if (rpc) {
            *ppRPC = rpc;
            return true;
        }
    }
    return false;
}

RPC_EXPORT bool Destroy_RPC(IRPC* pRPC){
    if (pRPC) {
        CRPC* rpc = dynamic_cast<CRPC*>(rpc);
        if (rpc) {
            delete rpc;
            return true;
        }
    }
}

#endif /* Header_h */


