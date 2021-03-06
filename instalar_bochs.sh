#!/bin/sh
LOG=salida.log
ERROR=errores.log

echo -n "Logueandose como root..."
sudo echo "" 1>${LOG} 2>${ERROR}

#echo "Eliminando otras versiones"
sudo apt-get remove --force-yes bochs 1>>${LOG} 2>>${ERROR}

echo "Verificando paquetes necesarios para compilar"
sudo apt-get install --force-yes byacc libwxgtk2.6-dev wx2.6-headers wx-common libxmu-dev libxmuu-dev 1>>${LOG} 2>>${ERROR} 

# Bochs, baja el codigo fuente, compila e instala
echo -n "Bajando "
wget http://ufpr.dl.sourceforge.net/sourceforge/bochs/bochs-2.3.7.tar.gz 1>>${LOG} 2>>${ERROR} 
if test $? -eq 0
then
   echo "OK"
   echo -n "Descomprimiendo "   
   tar -xvvzf bochs-2.3.7.tar.gz  1>>${LOG} 2>>${ERROR}
   if test $? -eq 0 
   then
      echo "OK"
      yes | rm bochs-2.3.7.tar.gz
      cd bochs-2.3.7
      echo -n "Configurando "
      sudo sh ./configure --with-x --enable-show-ips --enable-gdb-stub --enable-readline --enable-disasm --enable-idle-hack 1>>${LOG} 2>>${ERROR} 
      if test $? -eq 0 
      then
         echo  "OK"
         echo -n "Instalando "
         sudo make install 1>>${LOG} 2>>${ERROR} 
         if test $? -eq 0 
         then
            echo "OK"
            rm ${LOG} ${ERROR}
            cd ..
            yes | rm -r bochs-2.3.7
         else 
            echo "ERROR"
         fi
      else
         echo "ERROR"
      fi
   else
      echo "ERROR"
   fi
else
   echo "ERROR"
fi

echo -n "Instalando paquetes extra "
sudo apt-get install --force-yes vgabios 1>>${LOG} 2>>${ERROR} 
if test $? -eq 0
then
   echo "OK"
   rm ${LOG} ${ERROR}
else
   echo "ERROR"
fi

