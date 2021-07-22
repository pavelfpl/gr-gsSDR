#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: gr-lucky test GUI
# Author: Pavel Fiala
# GNU Radio version: v3.8.2.0-100-gd3da99e9

from gnuradio import analog
from gnuradio import blocks
from gnuradio import gr
from gnuradio.filter import firdes
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import uhd
import time
import gsSDR
import satellites.components.deframers
import satellites.components.demodulators


class gr_lucky_test(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "gr-lucky test GUI")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 192000
        self.gain_set = gain_set = 40
        self.freq = freq = 437.525e6

        ##################################################
        # Blocks
        ##################################################
        self.uhd_usrp_source_0 = uhd.usrp_source(
            ",".join(("", "")),
            uhd.stream_args(
                cpu_format="fc32",
                args='',
                channels=list(range(0,1)),
            ),
        )
        self.uhd_usrp_source_0.set_subdev_spec('A:A', 0)
        self.uhd_usrp_source_0.set_center_freq(freq, 0)
        self.uhd_usrp_source_0.set_gain(gain_set, 0)
        self.uhd_usrp_source_0.set_antenna('RX2', 0)
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0.set_time_unknown_pps(uhd.time_spec())
        self.satellites_lucky7_deframer_0 = satellites.components.deframers.lucky7_deframer(syncword_threshold = 2, options="")
        self.satellites_fsk_demodulator_0 = satellites.components.demodulators.fsk_demodulator(baudrate = 4800, samp_rate = samp_rate, iq = True, subaudio = False, options="")
        self.gsSDR_http_transfer_sink_0 = gsSDR.http_transfer_sink('localhost', '8080', '/gs/tm', 0, 1, '', '')
        self.gsSDR_gs_doppler_correction_0 = gsSDR.gs_doppler_correction('localhost', '4534', 1, freq)
        self.blocks_multiply_xx_0 = blocks.multiply_vcc(1)
        self.blocks_message_debug_1 = blocks.message_debug()
        self.analog_sig_source_x_0 = analog.sig_source_c(samp_rate, analog.GR_COS_WAVE, 0, 1, 0, 0)


        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.gsSDR_gs_doppler_correction_0, 'out_rx'), (self.analog_sig_source_x_0, 'freq'))
        self.msg_connect((self.gsSDR_gs_doppler_correction_0, 'out_rx'), (self.blocks_message_debug_1, 'print'))
        self.msg_connect((self.satellites_lucky7_deframer_0, 'out'), (self.gsSDR_http_transfer_sink_0, 'in'))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_multiply_xx_0, 1))
        self.connect((self.blocks_multiply_xx_0, 0), (self.satellites_fsk_demodulator_0, 0))
        self.connect((self.satellites_fsk_demodulator_0, 0), (self.satellites_lucky7_deframer_0, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.blocks_multiply_xx_0, 0))


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)
        self.uhd_usrp_source_0.set_samp_rate(self.samp_rate)

    def get_gain_set(self):
        return self.gain_set

    def set_gain_set(self, gain_set):
        self.gain_set = gain_set
        self.uhd_usrp_source_0.set_gain(self.gain_set, 0)

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.uhd_usrp_source_0.set_center_freq(self.freq, 0)





def main(top_block_cls=gr_lucky_test, options=None):
    tb = top_block_cls()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        sys.exit(0)

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    tb.start()

    tb.wait()


if __name__ == '__main__':
    main()
