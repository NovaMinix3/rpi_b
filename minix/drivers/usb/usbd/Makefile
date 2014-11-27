# Makefile for usb host controllers
.include <bsd.own.mk>

.if ((${MACHINE_ARCH} == "earm") || (${MACHINE_ARCH} == "earmv6hf") )
SUBDIR= .WAIT base/earm
.endif

.include <bsd.subdir.mk>
