#!/bin/bash

# Comprueba si existe el archivo ejecutable mytar
if [ -x mytar ] ; then
    # Comprueba si existe un directorio tmp, y si es asi, lo borra
    if [ -d tmp ] ; then
        rm -r tmp
    fi
    # Crea un directorio tmp y accede a el
    mkdir tmp
    cd tmp
    # Crea tres ficheros:
    echo "Hello world!" > file1.txt
    head -10 /etc/passwd > file2.txt
    head -c 1024 /dev/urandom > file3.dat
    # Ejecuta el programa mytar para comprimir los tres ficheros en uno
    ../mytar -c -f filetar.mtar file1.txt file2.txt file3.dat
    # Crea un directorio out y copia filetar.mtar en el
    mkdir out
    cp filetar.mtar out
    # Accede a out y ejecuta el programa mytar para extraer el fichero
    cd out
    ../../mytar -x -f filetar.mtar
    # Compara los archivos extraidos con los originales
    if (diff file1.txt ../file1.txt) && (diff file2.txt ../file2.txt) && (diff file3.dat ../file3.dat); then
        echo "Correct"
        exit 0
    else
        echo "ERROR: Unsuccesful process of compression and extraction"
        exit 1
    fi
    cd ../..
else
    echo "ERROR: Program mytar doesn't exist or can't be executed."
    exit 1
fi