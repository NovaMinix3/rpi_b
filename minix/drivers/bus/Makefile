.include <bsd.own.mk>

.if ${MKIMAGEONLY} == "no"
.  if ${MACHINE_ARCH} == "i386"
SUBDIR+=	ti1225
.  endif # ${MACHINE_ARCH} == "i386"

.  if ((${MACHINE_ARCH} == "earm") || (${MACHINE_ARCH} == "earmv6hf") )
SUBDIR+=	i2c
.  endif # ${MACHINE_ARCH} == "earm"
.endif # ${MKIMAGEONLY} == "no"

.if ${MACHINE_ARCH} == "i386"
SUBDIR+=	pci
.endif # ${MACHINE_ARCH} == "i386"

.include <bsd.subdir.mk>
