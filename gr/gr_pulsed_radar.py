#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Not titled yet
# Author: Chris Wozny
# Copyright: 2022
# GNU Radio version: 3.10.1.1

from packaging.version import Version as StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

from PyQt5 import Qt
from gnuradio import qtgui
from gnuradio.filter import firdes
import sip
from gnuradio import blocks
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import uhd
import time
from gnuradio.qtgui import Range, RangeWidget
from PyQt5 import QtCore



from gnuradio import qtgui

class gr_pulsed_radar(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Not titled yet", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Not titled yet")
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

        self.settings = Qt.QSettings("GNU Radio", "gr_pulsed_radar")

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
        self.pw_sec = pw_sec = 1.3e-6
        self.samp_rate = samp_rate = 50e6
        self.pri_sec = pri_sec = 100e-6
        self.chip_width_sec = chip_width_sec = pw_sec/13
        self.tx_gain = tx_gain = 1
        self.rx_gain = rx_gain = 1
        self.pw_samples = pw_samples = (int)(pw_sec*samp_rate)
        self.pri_samples = pri_samples = (int)(pri_sec*samp_rate)
        self.chip_width_samples = chip_width_samples = (int)(chip_width_sec*samp_rate)
        self.carrier_freq = carrier_freq = 2440e6

        ##################################################
        # Blocks
        ##################################################
        self._tx_gain_range = Range(1, 70, 1, 1, 200)
        self._tx_gain_win = RangeWidget(self._tx_gain_range, self.set_tx_gain, "'tx_gain'", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_layout.addWidget(self._tx_gain_win)
        self._rx_gain_range = Range(1, 70, 1, 1, 200)
        self._rx_gain_win = RangeWidget(self._rx_gain_range, self.set_rx_gain, "'rx_gain'", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_layout.addWidget(self._rx_gain_win)
        self._carrier_freq_range = Range(70e6, 6000e6, 1e6, 2440e6, 200)
        self._carrier_freq_win = RangeWidget(self._carrier_freq_range, self.set_carrier_freq, "'carrier_freq'", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_layout.addWidget(self._carrier_freq_win)
        self.uhd_usrp_source_0 = uhd.usrp_source(
            ",".join(("", '')),
            uhd.stream_args(
                cpu_format="fc32",
                otw_format="sc12",
                args='',
                channels=list(range(0,1)),
            ),
        )
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0.set_time_unknown_pps(uhd.time_spec(0))

        self.uhd_usrp_source_0.set_center_freq(carrier_freq, 0)
        self.uhd_usrp_source_0.set_antenna("RX2", 0)
        self.uhd_usrp_source_0.set_rx_agc(False, 0)
        self.uhd_usrp_source_0.set_gain(rx_gain, 0)
        self.uhd_usrp_sink_0 = uhd.usrp_sink(
            ",".join(("", '')),
            uhd.stream_args(
                cpu_format="fc32",
                otw_format="sc12",
                args='',
                channels=list(range(0,1)),
            ),
            "",
        )
        self.uhd_usrp_sink_0.set_samp_rate(samp_rate)
        self.uhd_usrp_sink_0.set_time_unknown_pps(uhd.time_spec(0))

        self.uhd_usrp_sink_0.set_center_freq(carrier_freq, 0)
        self.uhd_usrp_sink_0.set_antenna("TX/RX", 0)
        self.uhd_usrp_sink_0.set_gain(tx_gain, 0)
        self.qtgui_time_sink_x_0_0 = qtgui.time_sink_f(
            int(pri_sec*samp_rate), #size
            samp_rate, #samp_rate
            "", #name
            1, #number of inputs
            None # parent
        )
        self.qtgui_time_sink_x_0_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0_0.set_y_axis(0, 1)

        self.qtgui_time_sink_x_0_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0_0.enable_tags(True)
        self.qtgui_time_sink_x_0_0.set_trigger_mode(qtgui.TRIG_MODE_AUTO, qtgui.TRIG_SLOPE_POS, 0.5, 0, 0, "")
        self.qtgui_time_sink_x_0_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0_0.enable_grid(True)
        self.qtgui_time_sink_x_0_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0_0.enable_control_panel(True)
        self.qtgui_time_sink_x_0_0.enable_stem_plot(False)


        labels = ['Signal 1', 'Signal 2', 'Signal 3', 'Signal 4', 'Signal 5',
            'Signal 6', 'Signal 7', 'Signal 8', 'Signal 9', 'Signal 10']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ['blue', 'red', 'green', 'black', 'cyan',
            'magenta', 'yellow', 'dark red', 'dark green', 'dark blue']
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]
        styles = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1]


        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_0_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_0_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_0_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_0_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_0_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_0_0_win = sip.wrapinstance(self.qtgui_time_sink_x_0_0.qwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_time_sink_x_0_0_win)
        self.blocks_vector_source_x_1_0 = blocks.vector_source_f(5*chip_width_samples*[1,]+2*chip_width_samples*[-1,]+2*chip_width_samples*[1,]+1*chip_width_samples*[-1,]+1*chip_width_samples*[1,]+1*chip_width_samples*[-1,]+1*chip_width_samples*[1,]+pri_samples*[0,], True, 1, [])
        self.blocks_vector_source_x_1 = blocks.vector_source_f(pw_samples*[1,]+pri_samples*[0,], True, 1, [])
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*1)
        self.blocks_float_to_complex_0 = blocks.float_to_complex(1)
        self.blocks_complex_to_magphase_1 = blocks.complex_to_magphase(1)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_complex_to_magphase_1, 1), (self.blocks_null_sink_0, 0))
        self.connect((self.blocks_complex_to_magphase_1, 0), (self.qtgui_time_sink_x_0_0, 0))
        self.connect((self.blocks_float_to_complex_0, 0), (self.uhd_usrp_sink_0, 0))
        self.connect((self.blocks_vector_source_x_1, 0), (self.blocks_float_to_complex_0, 0))
        self.connect((self.blocks_vector_source_x_1_0, 0), (self.blocks_float_to_complex_0, 1))
        self.connect((self.uhd_usrp_source_0, 0), (self.blocks_complex_to_magphase_1, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "gr_pulsed_radar")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_pw_sec(self):
        return self.pw_sec

    def set_pw_sec(self, pw_sec):
        self.pw_sec = pw_sec
        self.set_chip_width_sec(self.pw_sec/13)
        self.set_pw_samples((int)(self.pw_sec*self.samp_rate))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_chip_width_samples((int)(self.chip_width_sec*self.samp_rate))
        self.set_pri_samples((int)(self.pri_sec*self.samp_rate))
        self.set_pw_samples((int)(self.pw_sec*self.samp_rate))
        self.qtgui_time_sink_x_0_0.set_samp_rate(self.samp_rate)
        self.uhd_usrp_sink_0.set_samp_rate(self.samp_rate)
        self.uhd_usrp_source_0.set_samp_rate(self.samp_rate)

    def get_pri_sec(self):
        return self.pri_sec

    def set_pri_sec(self, pri_sec):
        self.pri_sec = pri_sec
        self.set_pri_samples((int)(self.pri_sec*self.samp_rate))

    def get_chip_width_sec(self):
        return self.chip_width_sec

    def set_chip_width_sec(self, chip_width_sec):
        self.chip_width_sec = chip_width_sec
        self.set_chip_width_samples((int)(self.chip_width_sec*self.samp_rate))

    def get_tx_gain(self):
        return self.tx_gain

    def set_tx_gain(self, tx_gain):
        self.tx_gain = tx_gain
        self.uhd_usrp_sink_0.set_gain(self.tx_gain, 0)

    def get_rx_gain(self):
        return self.rx_gain

    def set_rx_gain(self, rx_gain):
        self.rx_gain = rx_gain
        self.uhd_usrp_source_0.set_gain(self.rx_gain, 0)

    def get_pw_samples(self):
        return self.pw_samples

    def set_pw_samples(self, pw_samples):
        self.pw_samples = pw_samples
        self.blocks_vector_source_x_1.set_data(self.pw_samples*[1,]+self.pri_samples*[0,], [])

    def get_pri_samples(self):
        return self.pri_samples

    def set_pri_samples(self, pri_samples):
        self.pri_samples = pri_samples
        self.blocks_vector_source_x_1.set_data(self.pw_samples*[1,]+self.pri_samples*[0,], [])
        self.blocks_vector_source_x_1_0.set_data(5*self.chip_width_samples*[1,]+2*self.chip_width_samples*[-1,]+2*self.chip_width_samples*[1,]+1*self.chip_width_samples*[-1,]+1*self.chip_width_samples*[1,]+1*self.chip_width_samples*[-1,]+1*self.chip_width_samples*[1,]+self.pri_samples*[0,], [])

    def get_chip_width_samples(self):
        return self.chip_width_samples

    def set_chip_width_samples(self, chip_width_samples):
        self.chip_width_samples = chip_width_samples
        self.blocks_vector_source_x_1_0.set_data(5*self.chip_width_samples*[1,]+2*self.chip_width_samples*[-1,]+2*self.chip_width_samples*[1,]+1*self.chip_width_samples*[-1,]+1*self.chip_width_samples*[1,]+1*self.chip_width_samples*[-1,]+1*self.chip_width_samples*[1,]+self.pri_samples*[0,], [])

    def get_carrier_freq(self):
        return self.carrier_freq

    def set_carrier_freq(self, carrier_freq):
        self.carrier_freq = carrier_freq
        self.uhd_usrp_sink_0.set_center_freq(self.carrier_freq, 0)
        self.uhd_usrp_source_0.set_center_freq(self.carrier_freq, 0)




def main(top_block_cls=gr_pulsed_radar, options=None):

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
