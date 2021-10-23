PREFIX = make -j4 -f make/Makefile
FILENAME = .build-target
USERMAKE = makefile.me
FUNCMAKE = makefile.func
PARAM =


.PHONY : all lib

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
	#UCODE ALIAS (cm0 or cm3)
	ifneq ($(ALIAS), )
		ifneq (,$(findstring cm3, $(ALIAS)))
		UPARAM = ALIAS=$(ALIAS)
		else
		ifneq (,$(findstring cm0, $(ALIAS)))
		UPARAM = ALIAS=$(ALIAS)
		else
		ifeq ($(UTARGET), $(filter $(UTARGET),standalone))
		UPARAM = ALIAS=$(ALIAS)+cm0
		else
		UPARAM = ALIAS=$(ALIAS)
		endif
		endif
		endif
	else
		# Standalone (CM0 build if no alias)
		ifeq ($(UTARGET), $(filter $(UTARGET),standalone))
		UPARAM = ALIAS=cm0
		endif
	endif
	UMAKEFILE := $(USYSTEM).ucode
        #remove lib in ALIAS for UCODE Build
        UPARAM := $(subst lib,,$(UPARAM))
endif

######### BUILD ########################################################
all: ucode
	$(PREFIX).$(MAKEFILE) $(PARAM)
lib:
	$(PREFIX).$(MAKEFILE) $(PARAM) lib

pkg:
	$(PREFIX).$(MAKEFILE) $(PARAM) pkg

clean: ucodeclean
	$(PREFIX).$(MAKEFILE) $(PARAM) ALIAS=$(ALIAS) clean

distclean: ucodeclean
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
ifeq ($(USYSTEM), $(filter $(USYSTEM),nrc7292 nrc7392 nrc4791))
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
ifeq ($(USYSTEM), $(filter $(USYSTEM),nrc7292 nrc7392 nrc4791))
ifeq ($(UTARGET), $(filter $(UTARGET),cspi standalone))
ifeq ($(findstring noucode, $(ALIAS)), )
ifeq ($(findstring halow, $(ALIAS)), )
	$(PREFIX).$(UMAKEFILE) $(UPARAM) clean
	@rm -f $(UCODEHDR)
endif
endif
endif
endif


##############################################################################################
