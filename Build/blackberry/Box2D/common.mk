ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

NAME=Box2D

#===== CCFLAGS - add the flags to the C compiler command line. 
CCFLAGS+=-Y_gpp

#===== EXTRA_INCVPATH - a space-separated list of directories to search for include files.
EXTRA_INCVPATH+=$(PRODUCT_ROOT)/../../

#===== EXTRA_SRCVPATH - a space-separated list of directories to search for source files.
EXTRA_SRCVPATH+=$(PRODUCT_ROOT)/../../Box2D/Collision  \
	$(PRODUCT_ROOT)/../../Box2D/Collision/Shapes  \
	$(PRODUCT_ROOT)/../../Box2D/Common  \
	$(PRODUCT_ROOT)/../../Box2D/Dynamics  \
	$(PRODUCT_ROOT)/../../Box2D/Dynamics/Contacts  \
	$(PRODUCT_ROOT)/../../Box2D/Dynamics/Joints  \
	$(PRODUCT_ROOT)/../../Box2D/Rope

include $(MKFILES_ROOT)/qtargets.mk

OPTIMIZE_TYPE_g=none
OPTIMIZE_TYPE=$(OPTIMIZE_TYPE_$(filter g, $(VARIANTS)))