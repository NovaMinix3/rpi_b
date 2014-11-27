#	$NetBSD: Makefile,v 1.18 2012/06/14 04:14:36 riz Exp $

.if ${MACHINE_ARCH} == "earm"
SUBDIR+=	beaglebone
.endif
.if ${MACHINE_ARCH} == "earmv6hf"
SUBDIR+=	rpi
.endif # ${MACHINE_ARCH} == "earmv6hf"

.include <bsd.subdir.mk>
