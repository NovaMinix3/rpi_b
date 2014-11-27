.include <bsd.own.mk>

.if ((${MACHINE_ARCH} == "earm") || (${MACHINE_ARCH} == "earmv6hf") )
SUBDIR+=	usbd
.endif # ${MACHINE_ARCH} == "earm"

SUBDIR+=	usb_storage usb_hub

.include <bsd.subdir.mk>
