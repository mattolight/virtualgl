/* Copyright (C)2011 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include "pbpm.h"
#include "fakerconfig.h"

pbpm::pbpm(Display *dpy, Pixmap pm, Visual *v) : pbdrawable(dpy, pm)
{
	rrcs::safelock l(_mutex);
	_prof_pmblit.setname("PMap Blit ");
	errifnot(_fb=new rrfb(dpy, pm, v));
}

pbpm::~pbpm()
{
	rrcs::safelock l(_mutex);
	if(_fb) {delete _fb;  _fb=NULL;}
}

void pbpm::readback(void)
{
	fconfig_reloadenv();

	rrcs::safelock l(_mutex);
	int pbw=_pb->width(), pbh=_pb->height();
	bool usepbo=(fconfig.readback==RRREAD_PBO);
	int desiredformat=_pb->format();
	static bool alreadywarned=false;

	rrframeheader hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.height=hdr.frameh=pbh;
	hdr.width=hdr.framew=pbw;
	hdr.x=hdr.y=0;
	_fb->init(hdr);

	_fb->_flags|=RRBMP_BOTTOMUP;
	int format;
	unsigned char *bits=_fb->_bits;
	switch(_fb->_pixelsize)
	{
		case 1:  format=GL_COLOR_INDEX;  break;
		case 3:
			format=GL_RGB;
			#ifdef GL_BGR_EXT
			if(_fb->_flags&RRBMP_BGR) format=GL_BGR_EXT;
			#endif
			break;
		case 4:
			format=GL_RGBA;
			#ifdef GL_BGRA_EXT
			if(_fb->_flags&RRBMP_BGR && !(_fb->_flags&RRBMP_ALPHAFIRST))
				format=GL_BGRA_EXT;
			#endif
			if(_fb->_flags&RRBMP_BGR && _fb->_flags&RRBMP_ALPHAFIRST)
			{
				#ifdef GL_ABGR_EXT
				format=GL_ABGR_EXT;
				#elif defined(GL_BGRA_EXT)
				format=GL_BGRA_EXT;  bits=_fb->_bits+1;
				#endif
			}
			if(!(_fb->_flags&RRBMP_BGR) && _fb->_flags&RRBMP_ALPHAFIRST)
			{
				format=GL_RGBA;  bits=_fb->_bits+1;
			}
			break;
		default:
			_throw("Unsupported pixel format");
	}
	if(usepbo && format!=desiredformat)
	{
		usepbo=false;
		if(fconfig.verbose && !alreadywarned)
		{
			alreadywarned=true;
			rrout.println("[VGL] NOTICE: Pixel format of 2D X server does not match pixel format of");
			rrout.println("[VGL}    Pbuffer.  Disabling PBO's.");
		}
	}
	readpixels(0, 0, min(pbw, _fb->_h.framew), _fb->_pitch,
		min(pbh, _fb->_h.frameh), format, _fb->_pixelsize, bits, GL_FRONT, usepbo,
		false);

	_fb->redraw();
}