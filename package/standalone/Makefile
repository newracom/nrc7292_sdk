PREFIX = make -j4 -f make/Makefile
FILENAME = .build-target
PARAM =

BDFDATA := ./bdf/nrc7292_bd.dat
BDFHDR := ./bdf/nrc7292_bd.h

.PHONY : all bdf

-include $(FILENAME)
ifneq ($(ALIAS), )
	PARAM := ALIAS=$(ALIAS) $(PARAM)
endif

split = $(word $2, $(subst +, , $1))
param = $(strip $(subst target=$(target), , $(MAKEFLAGS)))

######### BUILD ########################################################
all: bdf
	$(PREFIX).$(MAKEFILE) $(PARAM)

clean:
	$(PREFIX).$(MAKEFILE) $(PARAM) ALIAS=$(ALIAS) clean
	rm -f $(BDFHDR)

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

bdf:
	@xxd --include $(BDFDATA) > $(BDFHDR)
	@sed  -i '1i const' $(BDFHDR)
	@sed -i '/unsigned int/i const' $(BDFHDR)

##############################################################################################
