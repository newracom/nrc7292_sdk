PREFIX = make -j4 -f make/Makefile
FILENAME = .build-target

.PHONY : all lib

-include $(FILENAME)
ifneq ($(ALIAS), )
	PARAM := ALIAS=$(ALIAS) $(PARAM)
endif

split = $(word $2, $(subst +, , $1))
param = $(strip $(subst target=$(target), , $(MAKEFLAGS)))

######### BUILD ########################################################
all:
	$(PREFIX).$(MAKEFILE) $(PARAM)

lib:
	$(PREFIX).$(MAKEFILE) $(PARAM) lib


clean:
	$(PREFIX).$(MAKEFILE) $(PARAM) ALIAS=$(ALIAS) clean

distclean:
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


##############################################################################################
