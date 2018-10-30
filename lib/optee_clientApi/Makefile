#Copyright 2012 The Chromium Authors
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += OpteeClientMem.o
obj-y += OpteeClientApiLib.o
obj-y += OpteeClientInterface.o
obj-y += OpteeClientSMC.o
obj-y += OpteeClientRPC.o
obj-y += tee_smc-arm64.o
obj-y += OpteeClientRkFs.o

ifdef CONFIG_OPTEE_V1
obj-y += c11fe8ac-b997-48cf-a28de2a55e5240ef.o
lib/optee_clientApi/c11fe8ac-b997-48cf-a28de2a55e5240ef.o: lib/optee_clientApi/c11fe8ac-b997-48cf-a28de2a55e5240ef.c
lib/optee_clientApi/c11fe8ac-b997-48cf-a28de2a55e5240ef.c: lib/optee_clientApi/c11fe8ac-b997-48cf-a28de2a55e5240ef.ta
	$(srctree)/lib/optee_clientApi/tabinary_to_cfile.py --prefix widevine_keybox --TA $< --out $@
endif

