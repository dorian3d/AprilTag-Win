// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __DEBAYER_H
#define __DEBAYER_H

void bayer_to_gray(unsigned char *input, int iwidth, int iheight, unsigned char *out, int owidth, int oheight);

#endif
