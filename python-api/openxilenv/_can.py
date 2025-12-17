
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


class CAN_ACCEPTANCE_WINDOWS(ct.Structure):
	_fields_ = [('Channel', ct.c_int), ('StartId', ct.c_int), ('EndId', ct.c_int), ('Fill1', ct.c_int)]
	def __repr__(self) -> str:
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