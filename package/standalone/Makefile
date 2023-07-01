PREFIX = make -j16 -f make/Makefile
FILENAME = .build-target
USERMAKE = makefile.me
FUNCMAKE = makefile.func
PARAM =

.PHONY : all lib bdf

-include $(FILENAME)
ifneq ($(ALIAS), )
	PARAM := ALIAS=$(ALIAS) $(PARAM)
endif

split = $(word $2, $(subst +, , $1))
param = $(strip $(subst target=$(target), , $(MAKEFLAGS)))

# UCODE PARAM
ifneq ($(MAKEFILE), )
	USYSTEM := $(word 1, $(subst ., ,$(MAKEFILE)))
	UTARGET := $(word 2, $(subst ., ,$(MAKEFILE)))
	UCODEBIN := ./out/$(USYSTEM)/ucode/$(USYSTEM)_ucode.bin
	UCODEHDR := ./system/$(USYSTEM)/ucode.h
	#UCODE ALIAS (cm0)
	ifneq ($(ALIAS), )
		ifeq ($(UTARGET), $(filter $(UTARGET),standalone))
			UPARAM := ALIAS=$(ALIAS)+pscb
		else
			UPARAM := ALIAS=$(ALIAS)
		endif
	else
		ifeq ($(UTARGET), $(filter $(UTARGET),standalone))
			UPARAM := ALIAS=pscb
		endif
	endif
	UMAKEFILE := $(USYSTEM).ucode
        #remove lib in ALIAS for UCODE Build
        UPARAM := $(subst lib,,$(UPARAM))
endif

ifneq ($(MAKEFILE), )
	SYSTEM := $(word 1, $(subst ., ,$(MAKEFILE)))
endif

ifneq ($(MAKEFILE), )
	BLSYSTEM := $(word 1, $(subst ., ,$(MAKEFILE)))
	BLTARGET := $(word 2, $(subst ., ,$(MAKEFILE)))
	BLCODEBIN := ./out/$(BLSYSTEM)/boot/$(BLSYSTEM)_boot.bin
	BLCODEHDR := ./system/$(BLSYSTEM)/second_bl.h
	ifneq ($(ALIAS), )
		BLPARAM := ALIAS=$(ALIAS)
	endif
	BLMAKEFILE := $(BLSYSTEM).boot
        BLPARAM := $(subst xip,,$(BLPARAM))
endif

# Extract the filename using pattern matching and substitution
BDF_INPUT := $(lastword $(filter %.dat,$(subst +, ,$(ALIAS))))
BDFFOLDER := ./bdf
BDFNAME := bd.dat
BDFHEADERNAME := bd.h
ifneq ($(BDF_INPUT), )
	BDFSYSDATA := $(BDFFOLDER)/$(SYSTEM)/$(BDF_INPUT)
else
	BDFSYSDATA := $(BDFFOLDER)/$(SYSTEM)/$(SYSTEM)_$(BDFNAME)
endif
BDFDATA := $(BDFFOLDER)/$(BDFNAME)
BDFHDR := $(BDFFOLDER)/$(BDFHEADERNAME)

ifeq ($(UTARGET), $(filter $(UTARGET),standalone))
$(info =================================================)
$(info BDFSYSDATA : $(BDFSYSDATA))
$(info =================================================)
endif

######### BUILD ########################################################
all: ucode bdf second_bl
	$(PREFIX).$(MAKEFILE) $(PARAM)
lib:
	$(PREFIX).$(MAKEFILE) $(PARAM) lib

pkg:
	$(PREFIX).$(MAKEFILE) $(PARAM) pkg

clean: ucodeclean second_blclean
	$(PREFIX).$(MAKEFILE) $(PARAM) ALIAS=$(ALIAS) clean
	@rm -f $(BDFHDR)
	@rm -f $(BDFDATA)

distclean: ucodeclean second_blclean
	$(PREFIX).$(MAKEFILE) clean
	@rm $(FILENAME)

select::
	@echo "MAKEFILE = $(call split, $(target), 1)" > $(FILENAME)
ifneq ($(call split, $(target), 2), )
	@echo -n "ALIAS = $(call split, $(target), 2)" >> $(FILENAME)
endif
ifneq ($(call split, $(target), 3), )
	@echo -n "+$(call split, $(target), 3)" >> $(FILENAME)
endif
ifneq ($(call split, $(target), 4), )
	@echo -n "+$(call split, $(target), 4)" >> $(FILENAME)
endif
ifneq ($(call split, $(target), 5), )
	@echo -n "+$(call split, $(target), 5)" >> $(FILENAME)
endif
	@echo "" >> $(FILENAME)
	@echo "PARAM := $(param)" >> $(FILENAME)

ucode:
ifeq ($(USYSTEM), $(filter $(USYSTEM),nrc7292 nrc7393 nrc7394 nrc4791))
ifeq ($(UTARGET), $(filter $(UTARGET),cspi standalone))
ifeq ($(findstring noucode, $(ALIAS)), )
ifeq ($(findstring halow, $(ALIAS)), )
	$(PREFIX).$(UMAKEFILE) $(UPARAM)
	@xxd --include $(UCODEBIN) > $(UCODEHDR)
	@sed  -i '1i const' $(UCODEHDR)
endif
endif
endif
endif

ucodeclean:
ifeq ($(USYSTEM), $(filter $(USYSTEM),nrc7292 nrc7393 nrc7394 nrc4791))
ifeq ($(UTARGET), $(filter $(UTARGET),cspi standalone))
ifeq ($(findstring noucode, $(ALIAS)), )
ifeq ($(findstring halow, $(ALIAS)), )
	$(PREFIX).$(UMAKEFILE) $(UPARAM) clean
	@rm -f $(UCODEHDR)
endif
endif
endif
endif

bdf:
	@echo "BDFILE = $(BDFSYSDATA)"
	@cp -f $(BDFSYSDATA) $(BDFDATA)
	@xxd --include $(BDFDATA) > $(BDFHDR)
	@sed  -i '1i const' $(BDFHDR)
	@sed -i '/unsigned int/i const' $(BDFHDR)
	@sed -i '1i #ifndef __BD_H__' $(BDFHDR)
	@sed -i '2i #define __BD_H__' $(BDFHDR)
	@echo "#endif" >> $(BDFHDR)

second_bl:
ifeq ($(BLSYSTEM), $(filter $(BLSYSTEM),nrc7292 nrc7393 nrc7394 nrc5292))
ifeq ($(BLTARGET), $(filter $(BLTARGET),boot))
ifneq ($(findstring xip, $(ALIAS)), )
	$(PREFIX).$(BLMAKEFILE) $(BLPARAM)
	@xxd --include $(BLCODEBIN) > $(BLCODEHDR)
	@sed  -i '1i const' $(BLCODEHDR)
endif
endif
endif

second_blclean:
ifeq ($(BLSYSTEM), $(filter $(BLSYSTEM),nrc7292 nrc7393 nrc7394 nrc5292))
ifeq ($(BLTARGET), $(filter $(BLTARGET),boot))
ifneq ($(findstring xip, $(ALIAS)), )
	$(PREFIX).$(BLMAKEFILE) $(BLPARAM) clean
	@rm -f $(BLCODEHDR)
endif
endif
endif


##############################################################################################
