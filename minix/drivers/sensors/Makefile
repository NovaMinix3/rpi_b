.include <bsd.own.mk>

.if ( (${MACHINE_ARCH} == "earm") || (${MACHINE_ARCH} == "earmv6hf") )
SUBDIR+=	bmp085
SUBDIR+=	sht21
SUBDIR+=	tsl2550
.endif # ${MACHINE_ARCH} == "earm"

.include <bsd.subdir.mk>
