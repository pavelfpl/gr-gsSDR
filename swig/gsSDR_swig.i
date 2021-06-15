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
%}

%include "gsSDR/websocket_transfer_sink.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, websocket_transfer_sink);
%include "gsSDR/http_transfer_sink.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, http_transfer_sink);
%include "gsSDR/http_transfer_source.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, http_transfer_source);
%include "gsSDR/gs_doppler_correction.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, gs_doppler_correction);
