#
#

KMOD=	printer

# WONT_FIX warnings in Broadcom code
EXTRA_CFLAGS=	-Wno-unused-function

KBUILD_EXTRA_SYMBOLS=	$(PWD)/../adc/Module.symvers

export topdir?=../../..
include $(topdir)/Mk/kmod.mk

