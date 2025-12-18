
# Copyright (c) 2024 Contributors to the Eclipse Foundation
#
# This program and the accompanying materials are made available under the
# terms of the Apache License, Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0.
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0

import ctypes as ct


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