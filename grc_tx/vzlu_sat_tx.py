#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: AFSK_audio_tx
# GNU Radio version: 3.8.2.0

from distutils.version import StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

from gnuradio import analog
from gnuradio import blocks
import pmt
from gnuradio import filter
from gnuradio.filter import firdes
from gnuradio import gr
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import uhd
import time
from gnuradio.qtgui import Range, RangeWidget
import gsSDR

from gnuradio import qtgui

class vzlu_sat_tx(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "AFSK_audio_tx")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("AFSK_audio_tx")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "vzlu_sat_tx")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except:
            pass

        ##################################################
        # Variables
        ##################################################
        self.samp_rate_audio = samp_rate_audio = 48000
        self.variable_0 = variable_0 = 0
        self.samp_rate = samp_rate = samp_rate_audio*4
        self.gain_set = gain_set = 38
        self.freq = freq = 437.525e6

        ##################################################
        # Blocks
        ##################################################
        self._gain_set_range = Range(0, 100, 1, 38, 200)
        self._gain_set_win = RangeWidget(self._gain_set_range, self.set_gain_set, 'gain_set', "counter_slider", float)
        self.top_grid_layout.addWidget(self._gain_set_win)
        self.uhd_usrp_sink_0 = uhd.usrp_sink(
            ",".join(("", "")),
            uhd.stream_args(
                cpu_format="fc32",
                args='',
                channels=list(range(0,1)),
            ),
            'packet_len',
        )
        self.uhd_usrp_sink_0.set_subdev_spec('A:A', 0)
        self.uhd_usrp_sink_0.set_center_freq(freq, 0)
        self.uhd_usrp_sink_0.set_gain(gain_set, 0)
        self.uhd_usrp_sink_0.set_antenna('TX/RX', 0)
        self.uhd_usrp_sink_0.set_samp_rate(samp_rate)
        self.uhd_usrp_sink_0.set_time_unknown_pps(uhd.time_spec())
        self.low_pass_filter_0 = filter.fir_filter_ccf(
            1,
            firdes.low_pass(
                1,
                samp_rate,
                12.5e3,
                6.25e3,
                firdes.WIN_BLACKMAN,
                6.76))
        self.gsSDR_build_packet_1_0 = gsSDR.build_packet_1(False, 64, 1, 2, '', [0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0xC3,0xAA,0x66,0x55,0xCE,0xC6,0x25,0x6B,0x58,0x71,0xC0,0x9A,0x26,0x60,0x68,0xA4,0x8B,0x09,0x2A,0xB1,0xFD,0xA4,0x74,0xC0,0x89,0xAC,0x28,0x01,0xA0,0x8A,0xD3,0xB8,0x90,0x94,0xB7,0xA2,0xE0,0xBA,0xE6,0xF2,0xFE,0x2A,0x19,0xB4,0x55,0x55] )
        self.gsSDR_afsk_basic_0 = gsSDR.afsk_basic(samp_rate_audio, 0.0, 1200, 1800, 1200, 'packet_len')
        self.blocks_tagged_stream_multiply_length_0 = blocks.tagged_stream_multiply_length(gr.sizeof_gr_complex*1, 'packet_len', 4)
        self.blocks_message_strobe_0 = blocks.message_strobe(pmt.intern("TEST"), 1000)
        self.analog_nbfm_tx_0 = analog.nbfm_tx(
        	audio_rate=samp_rate_audio,
        	quad_rate=samp_rate,
        	tau=75e-6,
        	max_dev=3e3,
        	fh=-1.0,
                )



        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.blocks_message_strobe_0, 'strobe'), (self.gsSDR_build_packet_1_0, 'pdus'))
        self.msg_connect((self.gsSDR_build_packet_1_0, 'out_pdus'), (self.gsSDR_afsk_basic_0, 'in'))
        self.connect((self.analog_nbfm_tx_0, 0), (self.blocks_tagged_stream_multiply_length_0, 0))
        self.connect((self.blocks_tagged_stream_multiply_length_0, 0), (self.low_pass_filter_0, 0))
        self.connect((self.gsSDR_afsk_basic_0, 0), (self.analog_nbfm_tx_0, 0))
        self.connect((self.low_pass_filter_0, 0), (self.uhd_usrp_sink_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "vzlu_sat_tx")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samp_rate_audio(self):
        return self.samp_rate_audio

    def set_samp_rate_audio(self, samp_rate_audio):
        self.samp_rate_audio = samp_rate_audio
        self.set_samp_rate(self.samp_rate_audio*4)

    def get_variable_0(self):
        return self.variable_0

    def set_variable_0(self, variable_0):
        self.variable_0 = variable_0

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.low_pass_filter_0.set_taps(firdes.low_pass(1, self.samp_rate, 12.5e3, 6.25e3, firdes.WIN_BLACKMAN, 6.76))
        self.uhd_usrp_sink_0.set_samp_rate(self.samp_rate)

    def get_gain_set(self):
        return self.gain_set

    def set_gain_set(self, gain_set):
        self.gain_set = gain_set
        self.uhd_usrp_sink_0.set_gain(self.gain_set, 0)

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.uhd_usrp_sink_0.set_center_freq(self.freq, 0)





def main(top_block_cls=vzlu_sat_tx, options=None):

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    def quitting():
        tb.stop()
        tb.wait()

    qapp.aboutToQuit.connect(quitting)
    qapp.exec_()

if __name__ == '__main__':
    main()
