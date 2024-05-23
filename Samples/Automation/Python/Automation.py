import sys
sys.path.append('Path to XilEnv install folder')

from XilEnvRpc import *

print ("Python version:", sys.version)

XilEnv = XilEnvRpc('Path to XilEnv install folder\\XilEnvRpc.dll')

print ("GetAPIVersion", XilEnv.GetAPIVersion())

print ("GetAPIModulePath", XilEnv.GetAPIModulePath())

ConnectionStatus = XilEnv.ConnectTo("")
if (ConnectionStatus == 0):
    print ("Python has been successful connected to XilEnv")
else:
    print ("NOT Successful")
    sys.exit(1)

# Here should be added the tests

print ("now disconnect from XilEnv")
XilEnv.DisconnectFrom()
