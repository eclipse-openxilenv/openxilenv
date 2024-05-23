 
target_sources(LinuxRemoteMasterCore PRIVATE 
    CpuClock.c
    MemoryAllocation.c
    RealtimeProcessEquations.c
    RealtimeScheduler.c
    RemoteMasterDecoder.c
    RemoteMasterFile.c
    RemoteMasterLock.c
    RemoteMasterMessage.c
    RemoteMasterModelInterface.c
    RemoteMasterReadWriteMemory.c 
    RemoteMasterReqToClient.c 
    RemoteMasterServer.c
    Stups.c) 

target_sources(LinuxRemoteMaster PRIVATE 
    RemoteMasterLibrary.c) 
    
target_sources(LinuxRemoteMaster.Out PRIVATE 
    RemoteMasterLibrary.c
    RemoteMasterEmptyModel.c) 

add_subdirectory(Drivers)    
#add_subdirectory(Dsp56301)    
add_subdirectory(Equations)    
add_subdirectory(Ramps)    
