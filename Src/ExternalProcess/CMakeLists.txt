 
target_sources(${XilEnvExtProcTargetName} 
    PRIVATE
    ExtpBaseMessages.c
    ExtpKillExternProcessEvent.c
    ExtpBlackboard.c
    ExtpMain.c
    ExtpReferenceVariables.c
    ExtpBlackboardCopyLists.c
    ExtpMemoryAllocation.c
    ExtpSocketMessages.c
    ExtpMemoryAccess.c
    ExtpUnixDomainSocketMessages.c
    ExtpError.c
    ExtpParseCmdLine.c
    ExtpVirtualNetwork.c
    ExtpExtError.c
    ExtpVirtualNetworkCan.c
    ExtpXcpCopyPages.cpp
) 

if(UNIX)
    target_sources(${XilEnvExtProcTargetName}
        PRIVATE ExtpUnixDomainSocketMessages.c)
elseif(WIN32)
    target_sources(${XilEnvExtProcTargetName}
        PRIVATE ExtpPipeMessages.c OpenXilEnvExtp.def)
endif()
    
