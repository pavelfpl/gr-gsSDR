/* -*- c++ -*- */

#define GSSDR_API

%include "gnuradio.i"           // the common stuff

//load generated python docstrings
%include "gsSDR_swig_doc.i"

%{
#include "gsSDR/websocket_transfer_sink.h"
%}

%include "gsSDR/websocket_transfer_sink.h"
GR_SWIG_BLOCK_MAGIC2(gsSDR, websocket_transfer_sink);
