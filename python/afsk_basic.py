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

class afsk_basic(gr.sync_block):
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
    def __init__(self, samp_rate, preamble_len_ms, mark_freq, space_freq, baud_rate, stream_tag):
        gr.sync_block.__init__(self,
            name="afsk_basic",
            in_sig=None,
            out_sig=[np.float32])
        
        self.samp_rate = samp_rate
        self.mark_freq = mark_freq
        self.space_freq = space_freq
        self.baud_rate = baud_rate
        self.stream_tag = stream_tag
        
        self.preamble_len_bits = 0
        if not (preamble_len_ms == 0):
            self.preamble_len_bits = int((preamble_len_ms / 1000.0) * baud_rate / 2)
            
        self.sps = int(1.0 * self.samp_rate / self.baud_rate)
        
        self.outbox = queue.Queue()
        self.output_buffer = None
        self.opb_idx = 0

        self.message_port_register_in(pmt.intern('in'))
        self.set_msg_handler(pmt.intern('in'), self.handle_msg)

    def handle_msg(self, msg_pmt):
        
        # msg = pmt.to_python(msg_pmt)
        # if not (isinstance(msg, tuple) and len(msg) == 2):
        #    print('Invalid Message: Expected tuple of len 2')
        #    print('Dropping msg of type %s' % type(msg))
        #    return
        
        data_in = pmt.cdr(msg_pmt);
        data = array.array('B', pmt.u8vector_elements(data_in))

        self.outbox.put(data)
        
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
        
        out = output_items[0]
        idx = 0

        # TODO: Transmit cooldown period
        if self.output_buffer is None:
            if self.outbox.empty():
                # TODO: This is a bit of a hack to work around the ALSA Audio
                #       Sink being unhappy with underflows ...
                if(len(self.stream_tag)==0):
                    out[0:] = 0
                    return len(out)
                else:
                    return 0

            self.output_buffer = self.ax25_to_fsk(self.outbox.get())
            self.opb_idx = 0

            # print(len(self.output_buffer))
            
            key = pmt.intern(self.stream_tag)
            value = pmt.from_long(len(self.output_buffer))
                 
            self.add_item_tag(0,                         # Write to output port 0 ...
                          self.nitems_written(0),        # Index of the tag in absolute terms ...
                          key,                           # Key of the tag ...
                          value                          # Value of the tag ...
            )
            
            # key = pmt.intern("stream_tag_stop")
            # value = pmt.intern(str(len(self.output_buffer)))
                 
            # self.add_item_tag(0,                                                # Write to output port 0 ...
            #              self.nitems_written(0)+len(self.output_buffer),        # Index of the tag in absolute terms ...
            #              key,                                                   # Key of the tag ...
            #              value                                                  # Value of the tag ...
            # )


        # How many samples do we have left for each buffer ?
        opb_left = len(self.output_buffer) - self.opb_idx
        out_left = len(out) - idx

        # Take the minimum, and copy them to out
        cnt = min(opb_left, out_left)
        out[idx:idx+cnt] = self.output_buffer[self.opb_idx:self.opb_idx+cnt]

        # Update counters
        idx += cnt
        self.opb_idx += cnt

        # If we run out of samples in the output buffer, we're done ...
        if self.opb_idx >= len(self.output_buffer):
            self.output_buffer = None

        if(len(self.stream_tag)==0):
            # Fill the remaining buffer with zeros. Hack to help the ALSA audio sink ...
            # be happy.
            if idx < len(out):
                out[idx:] = 0
            return len(out)
        else:
            return idx
