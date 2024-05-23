 
target_sources(LinuxRemoteMasterCore PRIVATE
    IrqCpuAffinity.c SearchAndInitHardware.c) 

#add_subdirectory(Flexcard)    
#add_subdirectory(Goldammer)    
#add_subdirectory(Ixxat)    
add_subdirectory(SocketCan)    
