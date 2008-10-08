DIR_SODIUM_BASE = .

include Makefile.cfg

.PHONY: docs solo fs mbr usr kernel

# Compilacion por defecto de los fuentes 
all: tags fs mbr usr kernel solo

docs:
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Generando la documentacion del SODIUM ...
	@$(PRINT) ================================================================================
	@$(MAKE) -C $(DIR_DOCS)

solo:
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Compilando el SOLO...
	@$(PRINT) ================================================================================
	@$(MAKE) -C $(DIR_SOLO)

fs:
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Compilando las rutinas de FS...
	@$(PRINT) ================================================================================
	@$(MAKE) -C $(DIR_FS)

kernel:
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Compilando el KERNEL...
	@$(PRINT) ================================================================================
	@$(MAKE) -C $(DIR_KERNEL)

mbr:
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Compilando el MBR...
	@$(PRINT) ================================================================================
	@$(MAKE) -C $(DIR_MBR)

usr:
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Compilando los programas de usuario...
	@$(PRINT) ================================================================================
	@$(MAKE) -C $(DIR_USR)

tags:
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Creando archivo tags y links simbólicos a él en subdirectorios del proyecto.
	@$(PRINT) ================================================================================
	-ctags -R "$$PWD"
	-find . -name \*.[ch] -exec dirname '{}' \; | sort | uniq | xargs -t -i{} ln -s "$$PWD/tags" {}/tags


# Instalacion del SODIUM.
install: all
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Instalando el SOLO al disquete ...
	@$(PRINT) ================================================================================
	$(MAKE) -s -C $(DIR_SOLO) $@
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Copiando el MBR.bin al disquete ...
	@$(PRINT) ================================================================================
	$(MAKE) -s -C $(DIR_MBR) $@
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Instalando el KERNEL al disquete...
	@$(PRINT) ================================================================================
	$(MAKE) -s -C $(DIR_KERNEL) $@
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Instalando los programas de usuario al disquete...
	@$(PRINT) ================================================================================
	$(MAKE) -s -C $(DIR_USR) $@

sin_svn: clean
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Copiando version de SODIUM sin SVN a /tmp/sodium...
	@$(PRINT) ================================================================================
	@rm -rf /tmp/sodium
	@mkdir /tmp/sodium
	@cp -r * /tmp/sodium
	@find /tmp/sodium -name ".svn" -exec rm -fr {} +

clean:
	@$(PRINT)
	@$(PRINT)
	@$(PRINT) ================================================================================
	@$(PRINT) Borrando los objetos del SODIUM.
	@$(PRINT) ================================================================================
	@-find . -name tags -exec rm '{}' \;
	@-find . -name Makefile.bak -exec rm '{}' \;
	@-rm *.log 
	@$(MAKE) -i -s -C $(DIR_DOCS)   $@ 2>/dev/null
	@$(MAKE) -i -s -C $(DIR_SOLO)   $@ 2>/dev/null
	@$(MAKE) -i -s -C $(DIR_KERNEL) $@ 2>/dev/null
	@$(MAKE) -i -s -C $(DIR_MBR)	$@ 2>/dev/null
	@$(MAKE) -i -s -C $(DIR_USR)	$@ 2>/dev/null
	@$(MAKE) -i -s -C $(DIR_FS)	$@ 2>/dev/null
	@-rm tests/simbolos.txt
