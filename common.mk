ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

NAME=Box2D

#===== CCFLAGS - add the flags to the C compiler command line. 
CCFLAGS+=-Y_gpp

#===== EXTRA_SRCVPATH - a space-separated list of directories to search for source files.
EXTRA_SRCVPATH+=$(PROJECT_ROOT)/Box2D/Collision  \
	$(PROJECT_ROOT)/Box2D/Collision/Shapes  \
	$(PROJECT_ROOT)/Box2D/Common  \
	$(PROJECT_ROOT)/Box2D/Dynamics  \
	$(PROJECT_ROOT)/Box2D/Dynamics/Contacts  \
	$(PROJECT_ROOT)/Box2D/Dynamics/Joints  \
	$(PROJECT_ROOT)/Box2D/Rope

include $(MKFILES_ROOT)/qtargets.mk

OPTIMIZE_TYPE_g=none
OPTIMIZE_TYPE=$(OPTIMIZE_TYPE_$(filter g, $(VARIANTS)))