import ctypes as ct

class CAN_ACCEPTANCE_WINDOWS(ct.Structure):
	_fields_ = [('Channel', ct.c_int), ('StartId', ct.c_int), ('EndId', ct.c_int), ('Fill1', ct.c_int)]
	def __repr__(self):
		return ('CAN_ACCEPTANCE_WINDOWS(Channel={}, StartId={}, EndId={})'.format(self.Channel, self.StartId, self.EndId))

class CAN_FIFO_ELEM(ct.Structure):
	_fields_ = [('id', ct.c_uint32), ('data', ct.c_uint8 * 8), ('size', ct.c_uint8), ('ext', ct.c_uint8), ('flag', ct.c_uint8), ('channel', ct.c_uint8), ('node', ct.c_uint8), ('fill',  ct.c_uint8 * 7), ('timestamp', ct.c_uint64)]
	def __repr__(self):
		s = 'CAN_FIFO_ELEM(id={}, size={}, ext={}, flag={}, channel={}, node={}, timestamp={},\n  data='.format(self.id, self.size, self.ext, self.flag, self.channel, self.node, self.timestamp)
		c = 0
		while((c < self.size) and (c < 8)):
			if(c > 0):
				s += ', '
			s += '{}'.format(self.data[c])
			c += 1
		s += ')'
		return s

class CAN_FD_FIFO_ELEM(ct.Structure):
	_fields_ = [('id', ct.c_uint32), ('data', ct.c_uint8 * 64), ('size', ct.c_uint8), ('ext', ct.c_uint8), ('flag', ct.c_uint8), ('channel', ct.c_uint8), ('node', ct.c_uint8), ('fill',  ct.c_uint8 * 7), ('timestamp', ct.c_uint64)]
	def __repr__(self):
		s = 'CAN_FD_FIFO_ELEM(id={}, size={}, ext={}, flag={}, channel={}, node={}, timestamp={},\n  data='.format(self.id, self.size, self.ext, self.flag, self.channel, self.node, self.timestamp)
		c = 0
		while((c < self.size) and (c < 64)):
			if(c > 0):
				s += ', '
			s += '{}'.format(self.data[c])
			c += 1
		s += ')'
		return s

class BB_VARI(ct.Union):
	_fields_ = [('b', ct.c_int8), ('ub', ct.c_uint8), ('w', ct.c_int16), ('uw', ct.c_uint16), ('dw', ct.c_int32), ('udw', ct.c_uint32), ('qw', ct.c_int64), ('uqw', ct.c_uint64), ('f', ct.c_float), ('d', ct.c_double)]

class A2LData:
	__Dll = 0
	__Index = -1
	__LinkNo = -1
	__Data = ct.c_char_p

	def __init__(self, Dll, LinkNo, Index, Data):
		self.__Dll = Dll
		self.__LinkNo = LinkNo
		self.__Index = Index		
		self.__Data = Data

	def c_str(self, string):
		return ct.c_char_p(string.encode('utf-8'))

	def str_c(self, c_string):
		if (c_string == None) :
			return ""
		else :
			return c_string.decode("utf-8")

	def Get(self):
		return self.__Data

	def GetArrayElementValue(self, an, i):
	#	print ("i = ", i)
		DataType = self.__Dll.XilEnv_GetLinkArrayValueDataType(self.__Data, an, i)
		if (DataType == 0): # int
			return self.__Dll.XilEnv_GetLinkArrayValueDataInt(self.__Data, an, i)
		elif (DataType == 1): # uint
			return self.__Dll.XilEnv_GetLinkArrayValueDataUint(self.__Data, an, i)
		elif ((DataType == 2) or (DataType == 3)): # double or physical
			return self.__Dll.XilEnv_GetLinkArrayValueDataDouble(self.__Data, an, i)
		elif (DataType == 4): # text
			return self.str_c(self.__Dll.XilEnv_GetLinkArrayValueDataStringPtr(self.__Data, an, i))
		else:
			return "error"
	
	def GetArrayValues(self, an):
	#	print ("an = ", an)
		dc = self.__Dll.XilEnv_GetLinkArrayValueDimensionCount(self.__Data, an)
		if (dc == 1):
			xd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 0)
			m = [0 for xi in range(xd)]
			xi = 0
			while (xi < xd):
				m[xi] = self.GetArrayElementValue(an, xi)
				xi += 1
			return m
		elif (dc == 2):
			xd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 0)
			yd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 1)
			#print ("Dim = ", yd, xd)
			m = [[0 for yi in range(xd)] for xi in range(yd)]
			yi = 0
			while (yi < yd):
				#print ("yi = ", yi)
				xi = 0
				while (xi < xd):
					#print ("yi xi = ", yi, xi)
					m[yi][xi] = self.GetArrayElementValue(an, yi * xd + xi)
					xi += 1
				yi += 1
			return m
		elif (dc == 3):
			xd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 0)
			yd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 1)
			zd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 3)
			m = [[[0 for xi in range(xd)] for yi in range(yd)]for zi in range(zd)]
			zi = 0
			while (zi < zd):
				yi = 0
				while (yi < yd):
					xi = 0
					while (xi < xd):
						m[zi][yi][xi] = self.GetArrayElementValue(an, (zi * yd + yi) * xd + xi)
						xi += 1
					yi += 1
				zi += 1
			return m
		else:
			return "error"

	def GetValue(self):
		Type = self.__Dll.XilEnv_GetLinkDataType(self.__Data)
		#print ('Type:', Type)
		if ((Type == 0) or (Type == 1)):
			DataType = self.__Dll.XilEnv_GetLinkSingleValueDataType(self.__Data)
			#print ('DataType:', DataType)
			if (DataType == 0): # int
				return self.__Dll.XilEnv_GetLinkSingleValueDataInt(self.__Data)
			elif (DataType == 1): # uint
				return self.__Dll.XilEnv_GetLinkSingleValueDataUint(self.__Data)
			elif ((DataType == 2) or (DataType == 3)): # double or physical
				return self.__Dll.XilEnv_GetLinkSingleValueDataDouble(self.__Data)
			elif (DataType == 4): # text
				return self.str_c(self.__Dll.XilEnv_GetLinkSingleValueDataStringPtr(self.__Data))
			else:
				return "error"
		elif((Type >= 3) and (Type <= 6)):
			ac = self.__Dll.XilEnv_GetLinkDataArrayCount(self.__Data)
			#print ("Arrays = ", ac)
			if (ac == 1):
				return self.GetArrayValues(0)
			else:
				Ret = [] # list of list
				a = 0
				while(a < ac):
					Ret.append(self.GetArrayValues(a))
					a += 1
				return Ret
		else:
			return "error"

# Unit
	def GetArrayElementUnit(self, an, i):
		# print ("i = ", i)
		DataType = self.__Dll.XilEnv_GetLinkArrayValueDataType(self.__Data, an, i)
		# print ('DataType:', DataType)
		if (DataType != 4):  # text
			Flags = self.__Dll.XilEnv_GetLinkArrayValueFlags(self.__Data, an, i)
			# print ('Flags:', Flags)
			if ((Flags & 0x40) == 0x40): # A2L_VALUE_FLAG_HAS_UNIT = 0x40
				Ret = self.str_c(self.__Dll.XilEnv_GetLinkArrayValueUnitPtr(self.__Data, an, i))
				# print (an, Ret);
				return Ret
		return ""

	def GetArrayUnits(self, an):
	#	print ("an = ", an)
		dc = self.__Dll.XilEnv_GetLinkArrayValueDimensionCount(self.__Data, an)
		if (dc == 1):
			xd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 0)
			m = ["" for xi in range(xd)]
			xi = 0
			while (xi < xd):
				m[xi] = self.GetArrayElementUnit(an, xi)
				xi += 1
			return m
		elif (dc == 2):
			xd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 0)
			yd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 1)
			#print ("Dim = ", yd, xd)
			m = [["" for yi in range(xd)] for xi in range(yd)]
			yi = 0
			while (yi < yd):
				#print ("yi = ", yi)
				xi = 0
				while (xi < xd):
					#print ("yi xi = ", yi, xi)
					m[yi][xi] = self.GetArrayElementUnit(an, yi * xd + xi)
					xi += 1
				yi += 1
			return m
		elif (dc == 3):
			xd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 0)
			yd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 1)
			zd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 3)
			m = [[["" for xi in range(xd)] for yi in range(yd)]for zi in range(zd)]
			zi = 0
			while (zi < zd):
				yi = 0
				while (yi < yd):
					xi = 0
					while (xi < xd):
						m[zi][yi][xi] = self.GetArrayElementUnit(an, (zi * yd + yi) * xd + xi)
						xi += 1
					yi += 1
				zi += 1
			return m
		else:
			return "error"

	def GetUnit(self):
		Type = self.__Dll.XilEnv_GetLinkDataType(self.__Data)
		#print ('Type:', Type)
		if ((Type == 0) or (Type == 1)):
			DataType = self.__Dll.XilEnv_GetLinkSingleValueDataType(self.__Data)
			#print ('DataType:', DataType)
			if (DataType != 4):  # text
				Flags = self.__Dll.XilEnv_GetLinkSingleValueFlags(self.__Data)
				#print ('Flags:', Flags)
				if ((Flags & 0x40) == 0x40): # A2L_VALUE_FLAG_HAS_UNIT = 0x40
					return self.str_c(self.__Dll.XilEnv_GetLinkSingleValueUnitPtr(self.__Data))
			return ""
		elif((Type >= 3) and (Type <= 6)):
			ac = self.__Dll.XilEnv_GetLinkDataArrayCount(self.__Data)
			#print ("Arrays = ", ac)
			if (ac == 1):
				return self.GetArrayUnits(0)
			else:
				Ret = [] # list of list
				a = 0
				while(a < ac):
					Ret.append(self.GetArrayUnits(a))
					a += 1
				return Ret
		else:
			return "error"

	def SetArrayElement(self, an, i, Data):
	#	print ("i = ", i)
		DataType = self.__Dll.XilEnv_GetLinkArrayValueDataType(self.__Data, an, i)
		if ((DataType == 0) and (type(Data) != str)): # int
			return self.__Dll.XilEnv_SetLinkArrayValueDataInt(self.__Data, an, i, Data)
		elif ((DataType == 1) and (type(Data) != str)): # uint
			return self.__Dll.XilEnv_SetLinkArrayValueDataUint(self.__Data, an, i, Data)
		elif (((DataType == 2) or (DataType == 3)) and (type(Data) != str)): # double or physical
			return self.__Dll.XilEnv_SetLinkArrayValueDataDouble(self.__Data, an, i, Data)
		elif ((DataType == 4) and (type(Data) == str)): # text
			return self.str_c(self.__Dll.XilEnv_SetLinkArrayValueDataString(self.__Data, an, i, Data))
		else:
			return -1

	def SetArray(self, an, Data):
		#print ("an = ", an)
		dc = self.__Dll.XilEnv_GetLinkArrayValueDimensionCount(self.__Data, an)
		if (dc == 1):
			xd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 0)
			xi = 0
			while (xi < xd):
				if (self.SetArrayElement(an, xi, Data[xi]) != 0):
					return -1
				xi += 1
			return 0
		elif (dc == 2):
			xd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 0)
			yd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 1)
		#	print ("Dim = ", yd, xd)
			yi = 0
			while (yi < yd):
				#print ("yi = ", yi)
				xi = 0
				while (xi < xd):
					#print ("yi xi = ", yi, xi)
					if (self.SetArrayElement(an, yi * xd + xi, Data[yi][xi]) != 0):
						return -1
					xi += 1
				yi += 1
			return 0
		elif (dc == 3):
			xd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 0)
			yd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 1)
			zd = self.__Dll.XilEnv_GetLinkArrayValueDimension(self.__Data, an, 3)
			zi = 0
			while (zi < zd):
				yi = 0
				while (yi < yd):
					xi = 0
					while (xi < xd):
						if(self.SetArrayElement(an, (zi * yd + yi) * xd + xi, Data[zi][yi][xi]) != 0):
							return -1
						xi += 1
					yi += 1
				zi += 1
			return 0
		else:
			return -1

	def SetValue(self, Data):
		Type = self.__Dll.XilEnv_GetLinkDataType(self.__Data)
		#print ('Type:', Type)
		if (((Type == 0) or (Type == 1)) and ((type(Data) == int) or (type(Data) == float) or (type(Data) == str))):
			DataType = self.__Dll.XilEnv_GetLinkSingleValueDataType(self.__Data)
			#print ('DataType:', DataType)
			if ((DataType == 0) and (type(Data) != str)): # int
				return self.__Dll.XilEnv_SetLinkSingleValueDataInt(self.__Data, Data)
			elif ((DataType == 1) and (type(Data) != str)): # uint
				return self.__Dll.XilEnv_SetLinkSingleValueDataUint(self.__Data, Data)
			elif (((DataType == 2) or (DataType == 3)) and (type(Data) != str)): # double or physical
				return self.__Dll.XilEnv_SetLinkSingleValueDataDouble(self.__Data, Data)
			elif ((DataType == 4) and (type(Data) == str)): # text
				return self.__Dll.XilEnv_SetLinkSingleValueDataString(self.__Data, self.c_str(Data))
			else:
				return -1
		elif((Type >= 3) and (Type <= 6) and (type(Data) == list)):
			ac = self.__Dll.XilEnv_GetLinkDataArrayCount(self.__Data)
			#print ("Arrays = ", Arrays)
			if ((ac == 1) and (type(Data[0]) != list)):
				return self.SetArray(0, Data)
			else:
				a = 0
				while(a < ac):
					if (self.SetArray(a, Data[a]) != 0):
						return -1
					a += 1
				return 0
		else:
			return -1

	def WriteBackValue(self):
		Error = ct.c_char_p()
		if (self.__Dll.XilEnv_SetDataToLink(self.__LinkNo, self.__Index, self.__Data,  ct.byref(Error)) != 0):
			print ('Error: ', self.str_c(Error.value))
			return -1
		return 0

class XilEnvRpc:
	__Dll = 0
	__SuccessfulConnected = 0
	__ApiVersion = 0

	def __init__(self, DllPath):
		self.__Dll = ct.windll.LoadLibrary(DllPath)
# Connect
		self.__Dll.XilEnv_GetAPIVersion.restype = ct.c_int
		self.__Dll.XilEnv_GetAPIVersion.argtypes = None
		self.__ApiVersion = self.__Dll.XilEnv_GetAPIVersion()

		self.__Dll.XilEnv_GetAPIModulePath.restype = ct.c_char_p
		self.__Dll.XilEnv_GetAPIModulePath.argtypes = None

		self.__Dll.XilEnv_ConnectTo.restype = ct.c_int
		self.__Dll.XilEnv_ConnectTo.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_ConnectToInstance.restype = ct.c_int
		self.__Dll.XilEnv_ConnectToInstance.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_IsConnectedTo.restype = ct.c_int
		self.__Dll.XilEnv_IsConnectedTo.argtypes = None

		self.__Dll.XilEnv_DisconnectFrom.restype = ct.c_int
		self.__Dll.XilEnv_DisconnectFrom.argtypes = None

		self.__Dll.XilEnv_DisconnectAndClose.restype = ct.c_int
		self.__Dll.XilEnv_DisconnectAndClose.argtypes = [ct.c_int, ct.c_int]

		self.__Dll.XilEnv_GetVersion.restype = ct.c_int
		self.__Dll.XilEnv_GetVersion.argtypes = None
# Other
		self.__Dll.XilEnv_CreateFileWithContent.restype = ct.c_int
		self.__Dll.XilEnv_CreateFileWithContent.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_GetEnvironVar.restype = ct.c_char_p
		self.__Dll.XilEnv_GetEnvironVar.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_SetEnvironVar.restype = ct.c_int
		self.__Dll.XilEnv_SetEnvironVar.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_ChangeSettings.restype = ct.c_int
		self.__Dll.XilEnv_ChangeSettings.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_TextOut.restype = ct.c_int
		self.__Dll.XilEnv_TextOut.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_ErrorTextOut.restype = ct.c_int
		self.__Dll.XilEnv_ErrorTextOut.argtypes = [ct.c_int, ct.c_char_p]

# Scheduler
		self.__Dll.XilEnv_StopScheduler.restype = None
		self.__Dll.XilEnv_StopScheduler.argtypes = None

		self.__Dll.XilEnv_ContinueScheduler.restype = None 
		self.__Dll.XilEnv_ContinueScheduler.argtypes = None

		self.__Dll.XilEnv_IsSchedulerRunning.restype = ct.c_int
		self.__Dll.XilEnv_IsSchedulerRunning.argtypes = None

		self.__Dll.XilEnv_StartProcess.restype = ct.c_int
		self.__Dll.XilEnv_StartProcess.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_StartProcessAndLoadSvl.restype = ct.c_int
		self.__Dll.XilEnv_StartProcessAndLoadSvl.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_StartProcessEx.restype = ct.c_int
		self.__Dll.XilEnv_StartProcessEx.argtypes = [ct.c_char_p, ct.c_int,  ct.c_int, ct.c_short, ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_StartProcessEx2.restype = ct.c_int
		self.__Dll.XilEnv_StartProcessEx2.argtypes = [ct.c_char_p, ct.c_int,  ct.c_int, ct.c_short, ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_uint, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_StopProcess.restype = ct.c_int
		self.__Dll.XilEnv_StopProcess.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_GetNextProcess.restype = ct.c_char_p
		self.__Dll.XilEnv_GetNextProcess.argtypes = [ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_GetProcessState.restype = ct.c_int
		self.__Dll.XilEnv_GetProcessState.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_DoNextCycles.restype = None
		self.__Dll.XilEnv_DoNextCycles.argtypes = [ct.c_int]

		self.__Dll.XilEnv_DoNextCyclesAndWait.restype = None
		self.__Dll.XilEnv_DoNextCyclesAndWait.argtypes = [ct.c_int]

		self.__Dll.XilEnv_AddBeforeProcessEquationFromFile.restype = ct.c_int
		self.__Dll.XilEnv_AddBeforeProcessEquationFromFile.argtypes = [ct.c_int, ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_AddBehindProcessEquationFromFile.restype = ct.c_int
		self.__Dll.XilEnv_AddBehindProcessEquationFromFile.argtypes = [ct.c_int, ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_DelBeforeProcessEquations.restype = None
		self.__Dll.XilEnv_DelBeforeProcessEquations.argtypes = [ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_DelBehindProcessEquations.restype = None
		self.__Dll.XilEnv_DelBehindProcessEquations.argtypes = [ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_WaitUntil.restype = ct.c_int
		self.__Dll.XilEnv_WaitUntil.argtypes = [ct.c_char_p, ct.c_int]

# internal processes
		self.__Dll.XilEnv_StartScript.restype = ct.c_int
		self.__Dll.XilEnv_StartScript.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_StopScript.restype = ct.c_int
		self.__Dll.XilEnv_StopScript.argtypes = None

		self.__Dll.XilEnv_StartRecorder.restype = ct.c_int
		self.__Dll.XilEnv_StartRecorder.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_RecorderAddComment.restype = ct.c_int
		self.__Dll.XilEnv_RecorderAddComment.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_StopRecorder.restype = ct.c_int
		self.__Dll.XilEnv_StopRecorder.argtypes = None

		self.__Dll.XilEnv_StartPlayer.restype = ct.c_int
		self.__Dll.XilEnv_StartPlayer.argtypes = [ct.c_char_p]
		
		self.__Dll.XilEnv_StopPlayer.restype = ct.c_int
		self.__Dll.XilEnv_StopPlayer.argtypes = None

		self.__Dll.XilEnv_StartEquations.restype = ct.c_int
		self.__Dll.XilEnv_StartEquations.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_StopEquations.restype = ct.c_int
		self.__Dll.XilEnv_StopEquations.argtypes = None

		self.__Dll.XilEnv_StartGenerator.restype = ct.c_int
		self.__Dll.XilEnv_StartGenerator.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_StopGenerator.restype = ct.c_int
		self.__Dll.XilEnv_StopGenerator.argtypes = None

# GUI
		self.__Dll.XilEnv_LoadDesktop.restype = ct.c_int
		self.__Dll.XilEnv_LoadDesktop.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_SaveDesktop.restype = ct.c_int
		self.__Dll.XilEnv_SaveDesktop.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_ClearDesktop.restype = ct.c_int
		self.__Dll.XilEnv_ClearDesktop.argtypes = None

		self.__Dll.XilEnv_CreateDialog.restype = ct.c_int
		self.__Dll.XilEnv_CreateDialog.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_AddDialogItem.restype = ct.c_int
		self.__Dll.XilEnv_AddDialogItem.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_ShowDialog.restype = ct.c_int
		self.__Dll.XilEnv_ShowDialog.argtypes = None

		self.__Dll.XilEnv_IsDialogClosed.restype = ct.c_int
		self.__Dll.XilEnv_IsDialogClosed.argtypes = None

		self.__Dll.XilEnv_SelectSheet.restype = ct.c_int
		self.__Dll.XilEnv_SelectSheet.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_AddSheet.restype = ct.c_int
		self.__Dll.XilEnv_AddSheet.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_DeleteSheet.restype = ct.c_int
		self.__Dll.XilEnv_DeleteSheet.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_RenameSheet.restype = ct.c_int
		self.__Dll.XilEnv_RenameSheet.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_OpenWindow.restype = ct.c_int
		self.__Dll.XilEnv_OpenWindow.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_CloseWindow.restype = ct.c_int
		self.__Dll.XilEnv_CloseWindow.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_DeleteWindow.restype = ct.c_int
		self.__Dll.XilEnv_DeleteWindow.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_ImportWindow.restype = ct.c_int
		self.__Dll.XilEnv_ImportWindow.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_ExportWindow.restype = ct.c_int
		self.__Dll.XilEnv_ExportWindow.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p]

# Blackboard
		self.__Dll.XilEnv_AddVari.restype = ct.c_int
		self.__Dll.XilEnv_AddVari.argtypes = [ct.c_char_p, ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_RemoveVari.restype = ct.c_int
		self.__Dll.XilEnv_RemoveVari.argtypes = [ct.c_int]

		self.__Dll.XilEnv_AttachVari.restype = ct.c_int
		self.__Dll.XilEnv_AttachVari.argtypes = [ct.c_char_p]
		
		self.__Dll.XilEnv_Get.restype = ct.c_double
		self.__Dll.XilEnv_Get.argtypes = [ct.c_int]

		self.__Dll.XilEnv_GetPhys.restype = ct.c_double
		self.__Dll.XilEnv_GetPhys.argtypes = [ct.c_int]

		self.__Dll.XilEnv_Set.restype = None
		self.__Dll.XilEnv_Set.argtypes = [ct.c_int, ct.c_double]

		self.__Dll.XilEnv_SetPhys.restype = ct.c_int
		self.__Dll.XilEnv_SetPhys.argtypes = [ct.c_int, ct.c_double]

		self.__Dll.XilEnv_Equ.restype = ct.c_double
		self.__Dll.XilEnv_Equ.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_WrVariEnable.restype = ct.c_int
		self.__Dll.XilEnv_WrVariEnable.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_WrVariDisable.restype = ct.c_int
		self.__Dll.XilEnv_WrVariDisable.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_IsWrVariEnabled.restype = ct.c_int
		self.__Dll.XilEnv_IsWrVariEnabled.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_LoadRefList.restype = ct.c_int
		self.__Dll.XilEnv_LoadRefList.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_AddRefList.restype = ct.c_int
		self.__Dll.XilEnv_AddRefList.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_SaveRefList.restype = ct.c_int
		self.__Dll.XilEnv_SaveRefList.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_ExportRobFile.restype = ct.c_int
		self.__Dll.XilEnv_ExportRobFile.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_GetVariConversionType.restype = ct.c_int
		self.__Dll.XilEnv_GetVariConversionType.argtypes = [ct.c_int]

		self.__Dll.XilEnv_GetVariConversionString.restype = ct.c_char_p
		self.__Dll.XilEnv_GetVariConversionString.argtypes = [ct.c_int]

		self.__Dll.XilEnv_SetVariConversion.restype = ct.c_int
		self.__Dll.XilEnv_SetVariConversion.argtypes = [ct.c_int, ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_GetVariType.restype = ct.c_int
		self.__Dll.XilEnv_GetVariType.argtypes = [ct.c_int]

		self.__Dll.XilEnv_GetVariUnit.restype = ct.c_char_p
		self.__Dll.XilEnv_GetVariUnit.argtypes = [ct.c_int]

		self.__Dll.XilEnv_SetVariUnit.restype = ct.c_int
		self.__Dll.XilEnv_SetVariUnit.argtypes = [ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_GetVariMin.restype = ct.c_double 
		self.__Dll.XilEnv_GetVariMin.argtypes = [ct.c_int]

		self.__Dll.XilEnv_GetVariMax.restype = ct.c_double 
		self.__Dll.XilEnv_GetVariMax.argtypes = [ct.c_int]

		self.__Dll.XilEnv_SetVariMin.restype = ct.c_int
		self.__Dll.XilEnv_SetVariMin.argtypes = [ct.c_int, ct.c_double]

		self.__Dll.XilEnv_SetVariMax.restype = ct.c_int
		self.__Dll.XilEnv_SetVariMax.argtypes = [ct.c_int, ct.c_double]

		self.__Dll.XilEnv_GetNextVari.restype = ct.c_char_p
		self.__Dll.XilEnv_GetNextVari.argtypes = [ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_GetNextVariEx.restype = ct.c_char_p
		self.__Dll.XilEnv_GetNextVariEx.argtypes = [ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_int]

		self.__Dll.XilEnv_GetVariEnum.restype = ct.c_char_p
		self.__Dll.XilEnv_GetVariEnum.argtypes = [ct.c_int, ct.c_char_p, ct.c_double]

		self.__Dll.XilEnv_GetVariDisplayFormatWidth.restype = ct.c_int
		self.__Dll.XilEnv_GetVariDisplayFormatWidth.argtypes = [ct.c_int]

		self.__Dll.XilEnv_GetVariDisplayFormatPrec.restype = ct.c_int
		self.__Dll.XilEnv_GetVariDisplayFormatPrec.argtypes = [ct.c_int]

		self.__Dll.XilEnv_SetVariDisplayFormat.restype = ct.c_int
		self.__Dll.XilEnv_SetVariDisplayFormat.argtypes = [ct.c_int, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_ImportVariProperties.restype = ct.c_int
		self.__Dll.XilEnv_ImportVariProperties.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_EnableRangeControl.restype = ct.c_int
		self.__Dll.XilEnv_EnableRangeControl.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_DisableRangeControl.restype = ct.c_int
		self.__Dll.XilEnv_DisableRangeControl.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_WriteFrame.restype = ct.c_int
		self.__Dll.XilEnv_WriteFrame.argtypes = [ct.POINTER(ct.c_int),ct.POINTER(ct.c_double),ct.c_int]

		self.__Dll.XilEnv_GetFrame.restype = ct.c_int
		self.__Dll.XilEnv_GetFrame.argtypes = [ct.POINTER(ct.c_int),ct.POINTER(ct.c_double),ct.c_int]

		self.__Dll.XilEnv_WriteFrameWaitReadFrame.restype = ct.c_int
		self.__Dll.XilEnv_WriteFrameWaitReadFrame.argtypes = [ct.POINTER(ct.c_int),ct.POINTER(ct.c_double),ct.c_int,ct.POINTER(ct.c_int),ct.POINTER(ct.c_double),ct.c_int]

		self.__Dll.XilEnv_ReferenceSymbol.restype = ct.c_int
		self.__Dll.XilEnv_ReferenceSymbol.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p, ct.c_char_p, ct.c_int, ct.c_char_p, ct.c_double, ct.c_double, ct.c_int, ct.c_int, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_DereferenceSymbol.restype = ct.c_int
		self.__Dll.XilEnv_DereferenceSymbol.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p, ct.c_int]

		self.__Dll.XilEnv_GetRaw.restype = ct.c_int
		self.__Dll.XilEnv_GetRaw.argtypes = [ct.c_int, ct.POINTER(BB_VARI)]

		self.__Dll.XilEnv_SetRaw.restype = ct.c_int
		self.__Dll.XilEnv_SetRaw.argtypes = [ct.c_int, ct.c_int, BB_VARI, ct.c_int]

# Calibration
		self.__Dll.XilEnv_LoadSvl.restype = ct.c_int
		self.__Dll.XilEnv_LoadSvl.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_SaveSvl.restype = ct.c_int
		self.__Dll.XilEnv_SaveSvl.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_SaveSal.restype = ct.c_int
		self.__Dll.XilEnv_SaveSal.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_GetSymbolRaw.restype = ct.c_int
		self.__Dll.XilEnv_GetSymbolRaw.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_int, ct.POINTER(BB_VARI)]

		self.__Dll.XilEnv_SetSymbolRaw.restype = ct.c_int
		self.__Dll.XilEnv_SetSymbolRaw.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_int, ct.c_int, BB_VARI]

# CAN
		self.__Dll.XilEnv_LoadCanVariante.restype = ct.c_int
		self.__Dll.XilEnv_LoadCanVariante.argtypes = [ct.c_char_p, ct.c_int]

		self.__Dll.XilEnv_LoadAndSelCanVariante.restype = ct.c_int
		self.__Dll.XilEnv_LoadAndSelCanVariante.argtypes = [ct.c_char_p, ct.c_int]

		self.__Dll.XilEnv_AppendCanVariante.restype = ct.c_int
		self.__Dll.XilEnv_AppendCanVariante.argtypes = [ct.c_char_p, ct.c_int]

		self.__Dll.XilEnv_DelAllCanVariants.restype = None
		self.__Dll.XilEnv_DelAllCanVariants.argtypes = None

# CCP
		self.__Dll.XilEnv_LoadCCPConfig.restype = ct.c_int
		self.__Dll.XilEnv_LoadCCPConfig.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_StartCCPBegin.restype = ct.c_int
		self.__Dll.XilEnv_StartCCPBegin.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StartCCPAddVar.restype = ct.c_int
		self.__Dll.XilEnv_StartCCPAddVar.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_StartCCPEnd.restype = ct.c_int
		self.__Dll.XilEnv_StartCCPEnd.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StopCCP.restype = ct.c_int
		self.__Dll.XilEnv_StopCCP.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StartCCPCalBegin.restype = ct.c_int
		self.__Dll.XilEnv_StartCCPCalBegin.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StartCCPCalAddVar.restype = ct.c_int
		self.__Dll.XilEnv_StartCCPCalAddVar.argtypes = [ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_StartCCPCalEnd.restype = ct.c_int
		self.__Dll.XilEnv_StartCCPCalEnd.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StopCCPCal.restype = ct.c_int
		self.__Dll.XilEnv_StopCCPCal.argtypes = [ct.c_int]

# XCP
		self.__Dll.XilEnv_LoadXCPConfig.restype = ct.c_int
		self.__Dll.XilEnv_LoadXCPConfig.argtypes = [ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_StartXCPBegin.restype = ct.c_int
		self.__Dll.XilEnv_StartXCPBegin.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StartXCPAddVar.restype = ct.c_int
		self.__Dll.XilEnv_StartXCPAddVar.argtypes = [ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_StartXCPEnd.restype = ct.c_int
		self.__Dll.XilEnv_StartXCPEnd.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StopXCP.restype = ct.c_int
		self.__Dll.XilEnv_StopXCP.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StartXCPCalBegin.restype = ct.c_int
		self.__Dll.XilEnv_StartXCPCalBegin.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StartXCPCalAddVar.restype = ct.c_int
		self.__Dll.XilEnv_StartXCPCalAddVar.argtypes = [ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_StartXCPCalEnd.restype = ct.c_int
		self.__Dll.XilEnv_StartXCPCalEnd.argtypes = [ct.c_int]

		self.__Dll.XilEnv_StopXCPCal.restype = ct.c_int
		self.__Dll.XilEnv_StopXCPCal.argtypes = [ct.c_int]

		self.__Dll.XilEnv_TransmitCAN.restype = ct.c_int
		self.__Dll.XilEnv_TransmitCAN.argtypes = [ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte]

		self.__Dll.XilEnv_TransmitCANFd.restype = ct.c_int
		self.__Dll.XilEnv_TransmitCANFd.argtypes = [ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.POINTER(ct.c_ubyte)]

# CAN-Message-Queues (FIFO's)
		
		self.__Dll.XilEnv_OpenCANQueue.restype = ct.c_int
		self.__Dll.XilEnv_OpenCANQueue.argtypes = [ct.c_int]

		self.__Dll.XilEnv_OpenCANFdQueue.restype = ct.c_int
		self.__Dll.XilEnv_OpenCANFdQueue.argtypes = [ct.c_int, ct.c_int]

		self.__Dll.XilEnv_SetCANAcceptanceWindows.restype = ct.c_int
		self.__Dll.XilEnv_SetCANAcceptanceWindows.argtypes = [ct.c_int, ct.POINTER(CAN_ACCEPTANCE_WINDOWS)]

		self.__Dll.XilEnv_FlushCANQueue.restype = ct.c_int
		self.__Dll.XilEnv_FlushCANQueue.argtypes = [ct.c_int]

		self.__Dll.XilEnv_ReadCANQueue.restype = ct.c_int
		self.__Dll.XilEnv_ReadCANQueue.argtypes = [ct.c_int, ct.POINTER(CAN_FIFO_ELEM)]

		self.__Dll.XilEnv_ReadCANFdQueue.restype = ct.c_int
		self.__Dll.XilEnv_ReadCANFdQueue.argtypes = [ct.c_int, ct.POINTER(CAN_FD_FIFO_ELEM)]

		self.__Dll.XilEnv_TransmitCANQueue.restype = ct.c_int
		self.__Dll.XilEnv_TransmitCANQueue.argtypes = [ct.c_int, ct.POINTER(CAN_FIFO_ELEM)]

		self.__Dll.XilEnv_TransmitCANFdQueue.restype = ct.c_int
		self.__Dll.XilEnv_TransmitCANFdQueue.argtypes = [ct.c_int, ct.POINTER(CAN_FD_FIFO_ELEM)]

		self.__Dll.XilEnv_CloseCANQueue.restype = ct.c_int
		self.__Dll.XilEnv_CloseCANQueue.argtypes = None

# Other part 2
		self.__Dll.XilEnv_SetCanChannelCount.restype = ct.c_int
                self.__Dll.XilEnv_SetCanChannelCount.argtypes = [ct.c_int]

                self.__Dll.XilEnv_SetCanChannelStartupState.restype = ct.c_int
                self.__Dll.XilEnv_SetCanChannelStartupState.argtypes = [ct.c_int, ct.c_int]

# CAN bit error
		self.__Dll.XilEnv_SetCanErr.restype = ct.c_int
		self.__Dll.XilEnv_SetCanErr.argtypes = [ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int64]

		self.__Dll.XilEnv_SetCanErrSignalName.restype = ct.c_int
		self.__Dll.XilEnv_SetCanErrSignalName.argtypes = [ct.c_int, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int64]

		self.__Dll.XilEnv_ClearCanErr.restype = ct.c_int
		self.__Dll.XilEnv_ClearCanErr.argtypes = None

		self.__Dll.XilEnv_SetCanSignalConversion.restype = ct.c_int
		self.__Dll.XilEnv_SetCanSignalConversion.argtypes = [ct.c_int, ct.c_int, ct.c_char_p, ct.c_char_p]

		self.__Dll.XilEnv_ResetCanSignalConversion.restype = ct.c_int
		self.__Dll.XilEnv_ResetCanSignalConversion.argtypes = [ct.c_int, ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_ResetAllCanSignalConversion.restype = ct.c_int
		self.__Dll.XilEnv_ResetAllCanSignalConversion.argtypes = [ct.c_int, ct.c_int]

# CAN Recorder
		self.__Dll.XilEnv_StartCANRecorder.restype = ct.c_int
		self.__Dll.XilEnv_StartCANRecorder.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.POINTER(CAN_ACCEPTANCE_WINDOWS)]

		self.__Dll.XilEnv_StopCANRecorder.restype = ct.c_int
		self.__Dll.XilEnv_StopCANRecorder.argtypes = None

# A2LLink
		self.__Dll.XilEnv_SetupLinkToExternProcess.restype = ct.c_int
		self.__Dll.XilEnv_SetupLinkToExternProcess.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_int]

		self.__Dll.XilEnv_GetLinkToExternProcess.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkToExternProcess.argtypes = [ct.c_char_p]

		self.__Dll.XilEnv_GetIndexFromLink.restype = ct.c_int
		self.__Dll.XilEnv_GetIndexFromLink.argtypes = [ct.c_int, ct.c_char_p, ct.c_int]
	
		self.__Dll.XilEnv_GetNextSymbolFromLink.restype = ct.c_int
		self.__Dll.XilEnv_GetNextSymbolFromLink.argtypes = [ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.POINTER(ct.c_ubyte), ct.c_int]

		self.__Dll.XilEnv_GetDataFromLink.restype = ct.c_void_p
		self.__Dll.XilEnv_GetDataFromLink.argtypes = [ct.c_int, ct.c_int, ct.c_void_p, ct.c_int, ct.POINTER(ct.c_char_p)]

		self.__Dll.XilEnv_SetDataToLink.restype = ct.c_int
		self.__Dll.XilEnv_SetDataToLink.argtypes = [ct.c_int, ct.c_int, ct.c_void_p, ct.POINTER(ct.c_char_p)]

		self.__Dll.XilEnv_ReferenceMeasurementToBlackboard.restype = ct.c_int
		self.__Dll.XilEnv_ReferenceMeasurementToBlackboard = [ct.c_int, ct.c_int, ct.c_int]

# A2LLinks helper
		self.__Dll.XilEnv_GetLinkDataType.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkDataType.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_GetLinkDataArrayCount.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkDataArrayCount.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_GetLinkDataArraySize.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkDataArraySize.argtypes = [ct.c_void_p, ct.c_int]

		self.__Dll.XilEnv_CopyLinkData.restype = ct.c_void_p
		self.__Dll.XilEnv_CopyLinkData.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_FreeLinkData.restype = ct.c_void_p
		self.__Dll.XilEnv_FreeLinkData.argtypes = [ct.c_void_p]

# Single values
		self.__Dll.XilEnv_GetLinkSingleValueDataType.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkSingleValueDataType.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_GetLinkSingleValueTargetDataType.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkSingleValueTargetDataType.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_GetLinkSingleValueFlags.restype = ct.c_uint
		self.__Dll.XilEnv_GetLinkSingleValueFlags.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_GetLinkSingleValueAddress.restype = ct.c_uint64
		self.__Dll.XilEnv_GetLinkSingleValueAddress.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_GetLinkSingleValueDimensionCount.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkSingleValueDimensionCount.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_GetLinkSingleValueDimension.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkSingleValueDimension.argtypes = [ct.c_void_p, ct.c_int]

		self.__Dll.XilEnv_GetLinkSingleValueDataDouble.restype = ct.c_double
		self.__Dll.XilEnv_GetLinkSingleValueDataDouble.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_SetLinkSingleValueDataDouble.restype = ct.c_int
		self.__Dll.XilEnv_SetLinkSingleValueDataDouble.argtypes = [ct.c_void_p, ct.c_double]

		self.__Dll.XilEnv_GetLinkSingleValueDataInt.restype = ct.c_int64
		self.__Dll.XilEnv_GetLinkSingleValueDataInt.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_SetLinkSingleValueDataInt.restype = ct.c_int
		self.__Dll.XilEnv_SetLinkSingleValueDataInt.argtypes = [ct.c_void_p, ct.c_int64]

		self.__Dll.XilEnv_GetLinkSingleValueDataUint.restype = ct.c_uint64
		self.__Dll.XilEnv_GetLinkSingleValueDataUint.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_SetLinkSingleValueDataUint.restype = ct.c_int
		self.__Dll.XilEnv_SetLinkSingleValueDataUint.argtypes = [ct.c_void_p, ct.c_int64]

		self.__Dll.XilEnv_GetLinkSingleValueDataString.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkSingleValueDataString.argtypes = [ct.c_void_p, ct.POINTER(ct.c_char), ct.c_int]

		self.__Dll.XilEnv_GetLinkSingleValueDataStringPtr.restype = ct.c_char_p
		self.__Dll.XilEnv_GetLinkSingleValueDataStringPtr.argtypes = [ct.c_void_p]

		self.__Dll.XilEnv_SetLinkSingleValueDataString.restype = ct.c_int
		self.__Dll.XilEnv_SetLinkSingleValueDataString.argtypes = [ct.c_void_p, ct.c_char_p]

		self.__Dll.XilEnv_GetLinkSingleValueUnit.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkSingleValueUnit.argtypes = [ct.c_void_p, ct.POINTER(ct.c_char), ct.c_int]

		self.__Dll.XilEnv_GetLinkSingleValueUnitPtr.restype = ct.c_char_p
		self.__Dll.XilEnv_GetLinkSingleValueUnitPtr.argtypes = [ct.c_void_p]

# Array of values
		self.__Dll.XilEnv_GetLinkArrayValueDataType.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkArrayValueDataType.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_GetLinkArrayValueTargetDataType.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkArrayValueTargetDataType.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_GetLinkArrayValueFlags.restype = ct.c_uint
		self.__Dll.XilEnv_GetLinkArrayValueFlags.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_GetLinkArrayValueAddress.restype = ct.c_uint64
		self.__Dll.XilEnv_GetLinkArrayValueAddress.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_GetLinkArrayValueDimensionCount.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkArrayValueDimensionCount.argtypes = [ct.c_void_p, ct.c_int]

		self.__Dll.XilEnv_GetLinkArrayValueDimension.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkArrayValueDimension.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_GetLinkArrayValueDataDouble.restype = ct.c_double
		self.__Dll.XilEnv_GetLinkArrayValueDataDouble.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_SetLinkArrayValueDataDouble.restype = ct.c_int
		self.__Dll.XilEnv_SetLinkArrayValueDataDouble.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.c_double]

		self.__Dll.XilEnv_GetLinkArrayValueDataInt.restype = ct.c_int64
		self.__Dll.XilEnv_GetLinkArrayValueDataInt.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_SetLinkArrayValueDataInt.restype = ct.c_int
		self.__Dll.XilEnv_SetLinkArrayValueDataInt.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.c_int64]

		self.__Dll.XilEnv_GetLinkArrayValueDataUint.restype = ct.c_uint64
		self.__Dll.XilEnv_GetLinkArrayValueDataUint.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__Dll.XilEnv_SetLinkArrayValueDataUint.restype = ct.c_int
		self.__Dll.XilEnv_SetLinkArrayValueDataUint.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.c_uint64]

		self.__Dll.XilEnv_GetLinkArrayValueDataString.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkArrayValueDataString.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.POINTER(ct.c_char), ct.c_int]

		self.__Dll.XilEnv_GetLinkArrayValueDataStringPtr.restype = ct.c_char_p
		self.__Dll.XilEnv_GetLinkArrayValueDataStringPtr.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]
	
		self.__Dll.XilEnv_SetLinkArrayValueDataString.restype = ct.c_int
		self.__Dll.XilEnv_SetLinkArrayValueDataString.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.c_char_p]

		self.__Dll.XilEnv_GetLinkArrayValueUnit.restype = ct.c_int
		self.__Dll.XilEnv_GetLinkArrayValueUnit.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.POINTER(ct.c_char), ct.c_int]

		self.__Dll.XilEnv_GetLinkArrayValueUnitPtr.restype = ct.c_char_p
		self.__Dll.XilEnv_GetLinkArrayValueUnitPtr.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]
	
		self.__Dll.XilEnv_PrintLinkData.restype = None
		self.__Dll.XilEnv_PrintLinkData.argtypes = [ct.c_void_p]

# All methods definitions starts here:

	def c_str(self, string):
		return ct.c_char_p(string.encode('utf-8'))

	def str_c(self, c_string):
		if (c_string == None) :
			return ""
		else :
			return c_string.decode("utf-8")

# Connect 

# The following method can be called without a successful connection to softcar
	def GetAPIVersion(self): 
		return self.__Dll.XilEnv_GetAPIVersion()

	def GetAPIAlternativeVersion(self): 
		return self.__Dll.XilEnv_GetAPIAlternativeVersion()

	def GetAPIModulePath(self):
		return self.__Dll.XilEnv_GetAPIModulePath()

	def ConnectTo(self, Address):
		if (self.__SuccessfulConnected == 0):
			Ret = self.__Dll.XilEnv_ConnectTo(self.c_str(Address))
			if (Ret == 0):
				self.__SuccessfulConnected = 1
		else:
			Ret = -1
		return Ret

	def ConnectToInstance(self, Address, Instance):
		if (self.__SuccessfulConnected == 0):
			Ret = self.__Dll.XilEnv_ConnectToInstance(self.c_str(Address), self.c_str(Instance))
			if (Ret == 0):
				self.__SuccessfulConnected = 1
		else:
			Ret = -1
		return Ret

	def IsConnectedTo(self):
		return self.__Dll.XilEnv_IsConnectedTo()

# To call all following method you need a successful connection to softcar (call ConnectToSoftcar before)
	def DisconnectFrom(self):
		if (self.__SuccessfulConnected == 1):
			Ret = self.__Dll.XilEnv_DisconnectFrom()
			if (Ret == 0):
				self.__SuccessfulConnected = 0
		else:
			Ret = -1
		return Ret

	def DisconnectAndClose(self, SetErrorLevelFlag, ErrorLevel):
		if (self.__SuccessfulConnected == 1):
			Ret = self.__Dll.XilEnv_DisconnectAndClose(SetErrorLevelFlag, ErrorLevel)
			if (Ret == 0):
				self.__SuccessfulConnected = 0
		else:
			Ret = -1
		return Ret

	def GetVersion(self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetVersion()
		else:
			return -1


# Other 
	def CreateFileWithContent (self, Filename, Content):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_CreateFileWithContent(self.c_str(Filename), self.c_str(Content))
		else:
			return -1

	def GetEnvironVar (self, EnvironVar):
		if (self.__SuccessfulConnected == 1):
			return self.str_c(self.__Dll.XilEnv_GetEnvironVar(self.c_str(EnvironVar)))
		else:
			return ""

	def SetEnvironVar (self, EnvironVar, EnvironValue):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetEnvironVar(self.c_str(EnvironVar), self.c_str(EnvironValue))
		else:
			return -1

	def ChangeSettings (self, SettingName, SettingValue):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ChangeSettings(self.c_str(SettingName), self.c_str(SettingValue))
		else:
			return -1

	def TextOut (self, TextOut):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_TextOut(self.c_str(TextOut))
		else:
			return -1

	def ErrorTextOut (self, ErrLevel, TextOut):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ErrorTextOut(ErrLevel, self.c_str(TextOut))
		else:
			return -1

# Scheduler
	def StopScheduler (self):
		if (self.__SuccessfulConnected == 1):
			self.__Dll.XilEnv_StopScheduler()

	def ContinueScheduler (self):
		if (self.__SuccessfulConnected == 1):
			self.__Dll.XilEnv_ContinueScheduler()

	def SchedulerRunning (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_IsSchedulerRunning()
		else:
			return -1

	def StartProcess (self, Name):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartProcess(self.c_str(Name))
		else:
			return -1

	def StartProcessAndLoadSvl (self, ProcessName, SvlName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartProcessAndLoadSvl(self.c_str(ProcessName), self.c_str(SvlName))
		else:
			return -1

	def StartProcessEx (self, ProcessName, Prio, Cycle, Delay, Timeout, SVLFile, BBPrefix, UseRangeControl, RangeControlBeforeActiveFlags, RangeControlBehindActiveFlags, RangeControlStopSchedFlag, RangeControlOutput, RangeErrorCounterFlag, RangeErrorCounter, RangeControlVarFlag, RangeControl, RangeControlPhysFlag, RangeControlLimitValues):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartProcessEx(self.c_str(ProcessName), Prio, Cycle, Delay, Timeout, self.c_str(SVLFile), self.c_str(BBPrefix), UseRangeControl, RangeControlBeforeActiveFlags, RangeControlBehindActiveFlags, RangeControlStopSchedFlag, RangeControlOutput, RangeErrorCounterFlag, self.c_str(RangeErrorCounter), RangeControlVarFlag, self.c_str(RangeControl), RangeControlPhysFlag, RangeControlLimitValues)
		else:
			return -1

	def StartProcessEx2 (self, ProcessName, Prio, Cycle, Delay, Timeout, SVLFile, A2LFile, A2LFlags, BBPrefix, UseRangeControl, RangeControlBeforeActiveFlags, RangeControlBehindActiveFlags, RangeControlStopSchedFlag, RangeControlOutput, RangeErrorCounterFlag, RangeErrorCounter, RangeControlVarFlag, RangeControl, RangeControlPhysFlag, RangeControlLimitValues):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartProcessEx(self.c_str(ProcessName), Prio, Cycle, Delay, Timeout, self.c_str(SVLFile), self.c_str(A2LFile), A2LFlags, self.c_str(BBPrefix), UseRangeControl, RangeControlBeforeActiveFlags, RangeControlBehindActiveFlags, RangeControlStopSchedFlag, RangeControlOutput, RangeErrorCounterFlag, self.c_str(RangeErrorCounter), RangeControlVarFlag, self.c_str(RangeControl), RangeControlPhysFlag, RangeControlLimitValues)
		else:
			return -1

	def StopProcess (self, Name):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopProcess(self.c_str(Name))
		else:
			return -1

	def GetNextProcess (self, Flag, Filter):
		if (self.__SuccessfulConnected == 1):
			return self.str_c(self.__Dll.XilEnv_GetNextProcess(Flag, self.c_str(Filter)))
		else:
			return -1

	def GetProcessState (self, Name):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetProcessState(self.c_str(Name))
		else:
			return -1

	def DoNextCycles (self, Cycles):
		if (self.__SuccessfulConnected == 1):
			self.__Dll.XilEnv_DoNextCycles(Cycles)

	def DoNextCyclesAndWait (self, Cycles):
		if (self.__SuccessfulConnected == 1):
			self.__Dll.XilEnv_DoNextCyclesAndWait(Cycles)


	def AddBeforeProcessEquationFromFile (self, Nr, ProcName, EquFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_AddBeforeProcessEquationFromFile(Nr, self.c_str(ProcName), self.c_str(EquFile))
		else:
			return -1

	def AddBehindProcessEquationFromFile (self, Nr, ProcName, EquFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_AddBeforeProcessEquationFromFile(Nr, self.c_str(ProcName), self.c_str(EquFile))
		else:
			return -1

	def DelBeforeProcessEquations (self, Nr, ProcName):
		if (self.__SuccessfulConnected == 1):
			self.__Dll.XilEnv_AddBeforeProcessEquationFromFile(Nr, self.c_str(ProcName))

	def DelBehindProcessEquations (self, Nr, ProcName):
		if (self.__SuccessfulConnected == 1):
			self.__Dll.XilEnv_DelBehindProcessEquations(Nr, self.c_str(ProcName))

	def WaitUntil (self, Equation, Cycles):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_WaitUntil(self.c_str(Equation), Cycles)
		else:
			return -1

# internal processes
	def StartScript (self, ScriptFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartScript(self.c_str(ScriptFile))
		else:
			return -1

	def StopScript (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopScript()
		else:
			return -1

	def StartRecorder (self, CfgFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartRecorder(self.c_str(CfgFile))
		else:
			return -1

	def RecorderAddComment (self, Comment):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_RecorderAddComment(self.c_str(Comment))
		else:
			return -1

	def StopRecorder (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopRecorder()
		else:
			return -1

	def StartPlayer (self, CfgFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartPlayer(self.c_str(CfgFile))
		else:
			return -1

	def StopPlayer (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopPlayer()
		else:
			return -1

	def StartEquations (self, EquFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartEquations(self.c_str(EquFile))
		else:
			return -1

	def StopEquations (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopEquations()
		else:
			return -1

	def StartGenerator (self, GenFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartGenerator(self.c_str(GenFile))
		else:
			return -1

	def StopGenerator (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopGenerator()
		else:
			return -1


# GUI
	def LoadDesktop (self, DskFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_LoadDesktop(self.c_str(DskFile))
		else:
			return -1

	def SaveDesktop (self, DskFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SaveDesktop(self.c_str(DskFile))
		else:
			return -1

	def ClearDesktop (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ClearDesktop()
		else:
			return -1

	def CreateDialog (self, DialogName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_CreateDialog(self.c_str(DialogName))
		else:
			return -1

	def AddDialogItem (self, Description, VariName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_AddDialogItem(self.c_str(Description), self.c_str(VariName))
		else:
			return -1

	def ShowDialog (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ShowDialog()
		else:
			return -1

	def IsDialogClosed (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_IsDialogClosed()
		else:
			return -1

	def SelectSheet (self, SheetName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SelectSheet(self.c_str(SheetName))
		else:
			return -1

	def AddSheet (self, SheetName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_AddSheet(self.c_str(SheetName))
		else:
			return -1

	def DeleteSheet (self, SheetName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_DeleteSheet(self.c_str(SheetName))
		else:
			return -1

	def RenameSheet (self, OldSheetName, NewSheetName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_RenameSheet(self.c_str(OldSheetName), self.c_str(NewSheetName))
		else:
			return -1

	def OpenWindow (self, WindowName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_OpenWindow(self.c_str(WindowName))
		else:
			return -1

	def CloseWindow (self, WindowName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_CloseWindow(self.c_str(WindowName))
		else:
			return -1

	def DeleteWindow (self, WindowName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_DeleteWindow(self.c_str(WindowName))
		else:
			return -1

	def ImportWindow (self, WindowName, FileName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ImportWindow(self.c_str(WindowName), self.c_str(FileName))
		else:
			return -1

	def ExportWindow (self, WindowName, FileName):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ExportWindow(self.c_str(WindowName), self.c_str(FileName))
		else:
			return -1

# Blackboard
	def AddVari (self, Label, Type, Unit):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_AddVari(self.c_str(Label), Type, self.c_str(Unit))
		else:
			return -1

	def RemoveVari (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_RemoveVari(Vid)
		else:
			return -1

	def AttachVari (self, Label):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_AttachVari(self.c_str(Label))
		else:
			return -1

	def Get (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_Get(Vid)
		else:
			return 0

	def GetPhys (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetPhys(Vid)
		else:
			return 0

	def Set (self, Vid, Value):
		if (self.__SuccessfulConnected == 1):
			self.__Dll.XilEnv_Set(Vid, Value)

	def SetPhys (self, Vid, Value):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetPhys(Vid, Value)
		else:
			return -1

	def Equ (self, Equ):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_Equ(self.c_str(Equ))
		else:
			return 0

	def WrVariEnable (self, Label, Process):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_WrVariEnable(self.c_str(Label), self.c_str(Process))
		else:
			return -1

	def WrVariDisable (self, Label, Process):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_WrVariDisable(self.c_str(Label), self.c_str(Process))
		else:
			return -1

	def IsWrVariEnabled (self, Label, Process):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_IsWrVariEnabled(self.c_str(Label), self.c_str(Process))
		else:
			return -1

	def LoadRefList (self, RefList, Process):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_LoadRefList(self.c_str(RefList), self.c_str(Process))
		else:
			return -1

	def AddRefList (self, RefList, Process):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_AddRefList(self.c_str(RefList), self.c_str(Process))
		else:
			return -1

	def SaveRefList (self, RefList, Process):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SaveRefList(self.c_str(RefList), self.c_str(Process))
		else:
			return -1

	def ExportRobFile (self, RobFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ExportRobFile(self.c_str(RobFile))
		else:
			return -1

	def GetVariConversionType (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetVariConversionType(Vid)
		else:
			return -1

	def GetVariConversionString (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.str_c(self.__Dll.XilEnv_GetVariConversionString(Vid))
		else:
			return ""

	def XilEnv_SetVariConversion (self, Vid, ConvertionType, Conversion):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetVariConversion(Vid, ConvertionType, self.c_str(Conversion))
		else:
			return -1

	def GetVariType (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetVariType(Vid)
		else:
			return -1

	def GetVariUnit (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.str_c(self.__Dll.XilEnv_GetVariUnit(Vid))
		else:
			return ""

	def SetVariUnit (self, Vid, Unit):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetVariType(Vid, self.c_str(Unit))
		else:
			return -1

	def GetVariMin (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetVariMin(Vid)
		else:
			return 0

	def GetVariMax (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetVariMax(Vid)
		else:
			return 0

	def SetVariMin (self, Vid, Min):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetVariMin(Vid, Min)
		else:
			return -1

	def SetVariMax (self, Vid, Max):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetVariMax(Vid, Max)
		else:
			return -1

	def GetNextVari (self, Flag, Filter):
		if (self.__SuccessfulConnected == 1):
			return self.str_c(self.__Dll.XilEnv_GetNextVari(Flag, self.c_str(Filter)))
		else:
			return ""

	def GetNextVariEx (self, Flag, Filter, Process, AccessFlags):
		if (self.__SuccessfulConnected == 1):
			return self.str_c(self.__Dll.XilEnv_GetNextVariEx(Flag, self.c_str(Filter), self.c_str(Process), AccessFlags))
		else:
			return ""

	def GetVariEnum (self, Flag, Vid, Value):
		if (self.__SuccessfulConnected == 1):
			return self.str_c(self.__Dll.XilEnv_GetVariEnum(Flag, Vid, Value))
		else:
			return ""

	def GetVariDisplayFormatWidth (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetVariDisplayFormatWidth(Vid)
		else:
			return -1

	def GetVariDisplayFormatPrec (self, Vid):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetVariDisplayFormatPrec(Vid)
		else:
			return -1

	def SetVariDisplayFormat (self, Vid, Width, Prec):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetVariDisplayFormat(Vid, Width, Prec)
		else:
			return -1

	def ImportVariProperties (self, Filename):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ImportVariProperties(self.c_str(Filename))
		else:
			return -1

	def EnableRangeControl (self, ProcessNameFilter, VariableNameFilter):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_EnableRangeControl(self.c_str(ProcessNameFilter), self.c_str(VariableNameFilter))
		else:
			return -1

	def DisableRangeControl (self, ProcessNameFilter, VariableNameFilter):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_DisableRangeControl(self.c_str(ProcessNameFilter), self.c_str(VariableNameFilter))
		else:
			return -1

	def ReferenceSymbol(self, Symbol, DisplayName, Process, Unit, ConversionType, Conversion, Min, Max, Color, Width, Precision, Flags):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ReferenceSymboll(self.c_str(Symbol), self.c_str(Process), self.c_str(Unit), ConversionType, self.c_str(Conversion), Min, Max, Color, Width, Precision, Flags)
		else:
			return -1

	def DereferenceSymbol(self, Symbol, DisplayName, Process, Flags):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_DereferenceSymboll(self.c_str(Symbol), self.c_str(Process), Flags)
		else:
			return -1

	def GetRaw(self, Vid, RetValue):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetRaw(Vid, RetValue)
		else:
			return -1

	def SetRaw(self, Vid, Type, Value, Flag):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetRaw(Vid, Type, Value, Flag)
		else:
			return -1

# Calibration
	def LoadSvl (self, SvlFile, Process):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_LoadSvl(self.c_str(SvlFile), self.c_str(Process))
		else:
			return -1

	def SaveSvl (self, SvlFile, Process, Filter):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SaveSvl(self.c_str(SvlFile), self.c_str(Process), self.c_str(Filter))
		else:
			return -1

	def SaveSal (self, SvlFile, Process, Filter):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SaveSal(self.c_str(SvlFile), self.c_str(Process), self.c_str(Filter))
		else:
			return -1

	def GetSymbolRaw(self, Symbol, Process, Flags, RetValue):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_GetSymbolRaw(self.c_str(Symbol), self.c_str(Process), Flags, RetValue)
		else:
			return -1

	def GetSymbolRawEx(self, Symbol, Process):
		if (self.__SuccessfulConnected == 1):
			Data = (BB_VARI)()
			DataType = self.__Dll.XilEnv_GetSymbolRaw(self.c_str(Symbol), self.c_str(Process), 0x1000, Data) # flags = 0x1000 -> no error message
			if (DataType==0):  # int8_t
				return Data.b
			elif (DataType==1): # uint8_t
				return Data.ub
			elif (DataType==2): # int16_t
				return Data.w
			elif (DataType==3): # uint16_t
				return Data.uw
			elif (DataType==4): # int32_t
				return Data.dw
			elif (DataType==5): # uint32_t
				return Data.udw
			elif (DataType==6): # float
				return Data.f
			elif (DataType==7): # double
				return Data.d
			elif (DataType==36): # int64_t
				return Data.qw
			elif (DataType==37): # uint64_t
				return Data.uqw
			else:
				return None
		else:
			return None

	def SetSymbolRaw(self, Symbol, Process, Flags, DataType, Value):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetSymbolRaw(self.c_str(Symbol), self.c_str(Process), Flags, DataType, Value)
		else:
			return -1

	def SetSymbolRawEx(self, Symbol, Process, Value):
		if (self.__SuccessfulConnected == 1):
			Data = (BB_VARI)()
			DataType = self.__Dll.XilEnv_GetSymbolRaw(self.c_str(Symbol), self.c_str(Process), 0x1000, Data) # flags = 0x1000 -> no error message
			if (DataType==0):  # int8_t
				Data.b = Value
			elif (DataType==1): # uint8_t
				Data.ub =Value
			elif (DataType==2): # int16_t
				Data.w = Value
			elif (DataType==3): # uint16_t
				Data.uw = Value
			elif (DataType==4): # int32_t
				Data.dw =Value
			elif (DataType==5): # uint32_t
				Data.udw = Value
			elif (DataType==6): # float
				Data.f = Value
			elif (DataType==7): # double
				Data.d = Value
			elif (DataType==36): # int64_t
				Data.qw =Value
			elif (DataType==37): # uint64_t
				Data.uqw = Value
			else:
				return -1
			return self.__Dll.XilEnv_SetSymbolRaw(self.c_str(Symbol), self.c_str(Process), 0x1000, DataType, Data) # flags = 0x1000 -> no error message
		else:
			return -1

# CAN
	def LoadCanVariante (self, CanFile, Channel):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_LoadCanVariante(self.c_str(CanFile), Channel)
		else:
			return -1

	def LoadAndSelCanVariante (self, CanFile, Channel):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_LoadAndSelCanVariante(self.c_str(CanFile), Channel)
		else:
			return -1

	def AppendCanVariante (self, CanFile, Channel):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_AppendCanVariante(self.c_str(CanFile), Channel)
		else:
			return -1

	def DelAllCanVariants (self):
		if (self.__SuccessfulConnected == 1):
			self.__Dll.XilEnv_DelAllCanVariants()

	def SetCanChannelCount (self, ChannelCount):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetCanChannelCount(ChannelCount)
		else:
			return -1

        def SetCanChannelStartupState (self, Channel, State):
                if (self.__SuccessfulConnected == 1):
                        return self.__Dll.XilEnv_SetCanChannelStartupState(Channel, State)
                else:
                        return -1


	def TransmitCAN (self, Id, Ext, Size, Data0, Data1, Data2, Data3, Data4, Data5, Data6, Data7):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_TransmitCAN(self, Id, Ext, Size, Data0, Data1, Data2, Data3, Data4, Data5, Data6, Data7)
		else:
			return -1

	def XilEnv_TransmitCANFd (self, Id, Ext, Size, Data):
		if (self.__SuccessfulConnected == 1):
			if (Size > len(Data)):
				Size = len(Data)
			return self.__Dll.XilEnv_TransmitCANFd(self, Id, Ext, Size, Data)
		else:
			return -1

# CAN-Message-Queues (FIFO's)

	def OpenCANQueue (self, Depth):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_OpenCANQueue(Depth)
		else:
			return -1

	def OpenCANFdQueue (self, Depth, FdFlag):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_OpenCANFdQueue(Depth, FdFlag)
		else:
			return -1

	def SetCANAcceptanceWindows (self, Size, AcceptanceWindows):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetCANAcceptanceWindows(Size, AcceptanceWindows)
		else:
			return -1

	def FlushCANQueue (self, Flags):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_FlushCANQueue(Flags)
		else:
			return -1

	def ReadCANQueue (self, ReadMaxElements, Elements):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ReadCANQueue(ReadMaxElements, Elements)
		else:
			return -1

	def ReadCANFdQueue (self, ReadMaxElements, Elements):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ReadCANFdQueue(ReadMaxElements, Elements)
		else:
			return -1

	def TransmitCANQueue (self, WriteElements, Elements):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_TransmitCANQueue(WriteElements, Elements)
		else:
			return -1

	def TransmitCANFdQueue (self, WriteElements, Elements):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_TransmitCANFdQueue(WriteElements, Elements)
		else:
			return -1

	def CloseCANQueue (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_CloseCANQueue()
		else:
			return -1

# CCP
	def LoadCCPConfig (self, Connection, CcpFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_LoadCCPConfig(self.c_str(CcpFile), Connection)
		else:
			return -1

	def StartCCPBegin (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartCCPBegin(Connection)
		else:
			return -1

	def StartCCPAddVar (self, Connection, Variable):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartCCPAddVar(Connection, self.c_str(Variable))
		else:
			return -1

	def StartCCPEnd (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartCCPEnd(Connection)
		else:
			return -1

	def StopCCP (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopCCP(Connection)
		else:
			return -1

	def StartCCPCalBegin (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartCCPCalBegin(Connection)
		else:
			return -1

	def StartCCPCalAddVar (self, Connection, Variable):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartCCPCalAddVar(Connection, self.c_str(Variable))
		else:
			return -1

	def StartCCPCalEnd (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartCCPCalEnd(Connection)
		else:
			return -1

	def StopCCPCal (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopCCPCal(Connection)
		else:
			return -1

# XCP
	def LoadXCPConfig (self, Connection, XcpFile):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_LoadXCPConfig(self.c_str(XcpFile), Connection)
		else:
			return -1

	def StartXCPBegin (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartXCPBegin(Connection)
		else:
			return -1

	def StartXCPAddVar (self, Connection, Variable):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartXCPAddVar(Connection, self.c_str(Variable))
		else:
			return -1

	def StartXCPEnd (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartXCPEnd(Connection)
		else:
			return -1

	def StopXCP (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopXCP(Connection)
		else:
			return -1

	def StartXCPCalBegin (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartXCPCalBegin(Connection)
		else:
			return -1

	def StartXCPCalAddVar (self, Connection, Variable):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartXCPCalAddVar(Connection, self.c_str(Variable))
		else:
			return -1

	def StartXCPCalEnd (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StartXCPCalEnd(Connection)
		else:
			return -1

	def StopXCPCal (self, Connection):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_StopXCPCal(Connection)
		else:
			return -1

# CAN bit error
	def SetCanErr (self, Channel, Id, Startbit, Bitsize, ByteOrder, Cycles, BitErrValue):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetCanErr(Channel, Id, Startbit, Bitsize, self.c_str(ByteOrder), Cycles, BitErrValue)
		else:
			return -1

	def SetCanErrSignalName (self, Channel, Id, Signalname, Cycles, BitErrValue):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetCanErrSignalName(Channel, Id, self.c_str(Signalname), Cycles, BitErrValue)
		else:
			return -1

	def ClearCanErr (self):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ClearCanErr()
		else:
			return -1

	def SetCanSignalConversion (self, Channel, Id, Signalname, Conversion):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_SetCanSignalConversion(Channel, Id, self.c_str(Signalname), self.c_str(Conversion))
		else:
			return -1

	def ResetCanSignalConversion (self, Channel, Id, Signalname):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ResetCanSignalConversion(Channel, Id, self.c_str(Signalname))
		else:
			return -1

	def XilEnv_ResetAllCanSignalConversion (self, Channel, Id):
		if (self.__SuccessfulConnected == 1):
			return self.__Dll.XilEnv_ResetAllCanSignalConversion(Channel, Id)
		else:
			return -1

# CAN Recorder
	def StartCANRecorder (self, FileName, TriggerEqu, DisplayColumnCounterFlag, DisplayColumnTimeAbsoluteFlag, DisplayColumnTimeDiffFlag, DisplayColumnTimeDiffMinMaxFlag, AcceptanceWindows):
		if (self.__SuccessfulConnected == 1):
			Size = len(AcceptanceWindows)
#			for t in AcceptanceWindows:
#			    print(t)
			return self.__Dll.XilEnv_StartCANRecorder(self.c_str(FileName), self.c_str(TriggerEqu), DisplayColumnCounterFlag, DisplayColumnTimeAbsoluteFlag, DisplayColumnTimeDiffFlag, DisplayColumnTimeDiffMinMaxFlag, Size, AcceptanceWindows)
		else:
			return -1

	def StopCANRecorder (self):
		return self.__Dll.XilEnv_StopCANRecorder()

# A2lLink

	def GetA2lLinkNo(self, Process):
		return self.__Dll.XilEnv_GetLinkToExternProcess(self.c_str(Process))

	def FetchA2lData(self, LinkNo, Label, Flags, TypeMask = 0xFFFF):
		Index = self.__Dll.XilEnv_GetIndexFromLink(LinkNo, self.c_str(Label), TypeMask)
		#print ('Index:', Index)
		if (Index < 0):
			return None
		Error = ct.c_char_p()
		Data = self.__Dll.XilEnv_GetDataFromLink(LinkNo, Index, None, Flags, ct.byref(Error))
		if (Data == None):
			print ('Error: ', Label, ' ', self.str_c(Error.value))
			return None
		else:
			Ret = A2LData(self.__Dll, LinkNo, Index, Data)
			return Ret
