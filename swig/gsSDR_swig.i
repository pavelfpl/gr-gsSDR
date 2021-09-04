/* -*- c++ -*- */

#define GSSDR_API

%include "gnuradio.i"           // the common stuff

//load generated python docstrings
%include "gsSDR_swig_doc.i"

%{
#include "gsSDR/websocket_transfer_sink.h"
#include "gsSDR/http_transfer_sink.h"
#include "gsSDR/http_transfer_source.h"
#include "gsSDR/gs_doppler_correction.h"
#include "gsSDR/build_packet_1.h"
#include "gsSDR/packed_to_unpacked.h"
#include "gsSDR/usrp_gain_control.h"
%}

%include "gsSDR/websocket_transfer_sink.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, websocket_transfer_sink);
%include "gsSDR/http_transfer_sink.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, http_transfer_sink);
%include "gsSDR/http_transfer_source.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, http_transfer_source);
%include "gsSDR/gs_doppler_correction.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, gs_doppler_correction);
%include "gsSDR/build_packet_1.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, build_packet_1);
%include "gsSDR/packed_to_unpacked.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, packed_to_unpacked);
%include "gsSDR/usrp_gain_control.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, usrp_gain_control);
