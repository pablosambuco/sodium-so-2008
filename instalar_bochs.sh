#!/bin/sh
echo -n "Logueandose como root..."
sudo echo ""

#echo "Eliminando otras versiones"
#sudo apt-get remove --force-yes bochs 1>/dev/null 2>/dev/null

echo "Verificando paquetes necesarios para compilar"
sudo apt-get install --force-yes byacc 1>/dev/null 2>/dev/null 
# Bochs, baja el codigo fuente, compila e instala
echo -n "Bajando "
wget http://ufpr.dl.sourceforge.net/sourceforge/bochs/bochs-2.3.7.tar.gz 1>/dev/null 2>/dev/null
if test $? -eq 0
then
   echo "OK"
   echo -n "Descomprimiendo "   
   tar -xvvzf bochs-2.3.7.tar.gz 1>/dev/null 2>/dev/null
   if test $? -eq 0 
   then
      echo "OK"
    #  yes | rm bochs-2.3.7.tar.gz
      cd bochs-2.3.7
      echo -n "Configurando "
      sudo sh ./configure --with-x --enable-show-ips --enable-debugger --enable-readline --enable-disasm --enable-idle-hack 1>/dev/null 2>/dev/null
      if test $? -eq 0 
      then
         echo  "OK"
         echo -n "Instalando "
         sudo make install 1>/dev/null 2>/dev/null
         if test $? -eq 0 
         then
            echo "OK"
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
cd ..
yes | rm -r bochs-2.3.7 
echo -n "Instalando paquetes extra "
sudo apt-get install --force-yes vgabios 1>/dev/null 2>/dev/null
if test $? -eq 0
then
   echo "OK"
else
   echo "ERROR"
fi

