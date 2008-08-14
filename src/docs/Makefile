# Path relativo al directorio principal del proyecto
DIR_SODIUM_BASE = ..

include $(DIR_SODIUM_BASE)/Makefile.cfg
docs:
	-$(Q)doxygen Doxyfile $(if $(VERBOSE:1=),1>/dev/null 2>/dev/null)
clean:
	-$(Q)rm -r paginas_man
	-$(Q)rm -r html
	-$(Q)rm DoxyWarns.log
