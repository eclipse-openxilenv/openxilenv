target_sources(XilEnvRpc
    PRIVATE
    RpcClientSocket.c
    RpcFuncClientA2lLinks.c
    RpcFuncClient.c
    XilEnvRpc.def
#    PUBLIC FILE_SET HEADERS
#    BASE_DIRS ${PROJECT_SOURCE_DIR}
#    FILES XilEnvRpc.h)
)

set(CommonFileList
    RpcServerFuncCan.c
    RpcServerFuncInternalProcesses.c
    RpcServerFuncXcp.c
    RpcServerFuncCcp.c
    RpcServerFuncLogin.c
    RpcControlProcess.c
    RpcServerFuncBlackboard.c
    #RpcServerFuncFlexray.c
    RpcServerFuncMisc.c
    RpcSocketServer.c
    RpcServerFuncCalibration.c
    RpcServerFuncGui.c
    RpcServerFuncSched.c
)

target_sources(XilEnv  PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})
