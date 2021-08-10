#!/usr/bin/env python
# -*- coding: utf-8 -*-
# MIT License
#
# Copyright (c) 2021 Pavel Fiala
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#


#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# MIT License
#
# Copyright (c) 2021 Pavel Fiala
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

import queue
import time

import numpy as np
from gnuradio import gr

import pmt,array

class afsk_basic_pdu(gr.sync_block):
    """
    -------------------------------------------------------------------
    A continuous phase FSK modulator for PMT custom packets / like AX25
    -------------------------------------------------------------------
    
    When given an packet, this block converts it to an audio stream with
    the given configured parameters. Two in question:
    - Preamble Len (ms): How long to transmit a clock signal (01010101)
    - IF preamble_len_ms == 0 --> no preamble is added ...
    
    The default values for the mark, space, and baud rate are configurable to
    allow for further experimentation. v.23 modems, for example, use 1300/2100
    tones to generate 1200 baud signals.
    
    Modified for GR 3.8 from: https://github.com/tkuester/gr-bruninga
    Links: https://inst.eecs.berkeley.edu/~ee123/sp15/lab/lab6/Lab6-Part-A-Audio-Frequency-Shift-Keying.html
           https://notblackmagic.com/bitsnpieces/afsk/ 
    
    """
    def __init__(self, samp_rate, preamble_len_ms, mark_freq, space_freq, baud_rate):
        gr.sync_block.__init__(self,
            name="afsk_basic_pdu",
            in_sig=None,
            out_sig=None)
        
        self.samp_rate = samp_rate
        self.mark_freq = mark_freq
        self.space_freq = space_freq
        self.baud_rate = baud_rate
        
        self.preamble_len_bits = 0
        if not (preamble_len_ms == 0):
            self.preamble_len_bits = int((preamble_len_ms / 1000.0) * baud_rate / 2)
            
        self.sps = int(1.0 * self.samp_rate / self.baud_rate)
        
        self.outbox = queue.Queue()
        self.output_buffer = None
        self.opb_idx = 0

        self.message_port_register_in(pmt.intern('in'))
        self.message_port_register_out(pmt.intern("pdus"));
        self.set_msg_handler(pmt.intern('in'), self.handle_msg)

    def handle_msg(self, msg_pmt):
        
        # msg = pmt.to_python(msg_pmt)
        # if not (isinstance(msg, tuple) and len(msg) == 2):
        #    print('Invalid Message: Expected tuple of len 2')
        #    print('Dropping msg of type %s' % type(msg))
        #    return
        
        meta = pmt.car(msg_pmt);
        data_in = pmt.cdr(msg_pmt);
        data = array.array('B', pmt.u8vector_elements(data_in))

        self.outbox.put(data)
        self.output_buffer = self.ax25_to_fsk(self.outbox.get())
        
        burst_vector = pmt.init_f32vector(len(self.output_buffer), self.output_buffer.tolist());
        pdu = pmt.cons(meta, burst_vector);
        self.message_port_pub(pmt.intern("pdus"), pdu);
        
    def ax25_to_fsk(self, msg):

        # Generate message ...
        
        if not (self.preamble_len_bits == 0):
            msg_bits = [0, 1] * self.preamble_len_bits
            msg_bits += msg
        else:
            msg_bits = msg

        # Calculate phase increments ...
        mark_pinc = 2 * np.pi * self.mark_freq / self.samp_rate
        space_pinc = 2 * np.pi * self.space_freq / self.samp_rate
        
        phase = 0
        opb = np.empty(len(msg_bits) * self.sps)
        for i, bit in enumerate(msg_bits):
            pinc = (mark_pinc if bit is 1 else space_pinc)
            phase += pinc

            tmp = np.arange(self.sps) * pinc + phase
            opb[i*self.sps:(i+1)*self.sps] = np.sin(tmp)

            phase = tmp[-1]

        return opb

    def work(self, input_items, output_items):
        in0 = input_items[0]
        out = output_items[0]
        # <+signal processing here+>
        out[:] = in0
        return len(output_items[0])

