#!/bin/bash

# Nome do arquivo
FILE="disk.dat"

# Diretório de origem
SOURCE_DIR="diskOriginal"

# Diretório atual (destino)
DEST_DIR="."

# Remover o arquivo disk.dat do diretório atual, se existir
if [ -f "$DEST_DIR/$FILE" ]; then
    rm "$DEST_DIR/$FILE"
    echo "$FILE removido do diretório atual."
else
    echo "$FILE não encontrado no diretório atual."
fi

# Copiar o arquivo disk.dat do diretório diskOriginal para o diretório atual
if [ -f "$SOURCE_DIR/$FILE" ]; then
    cp "$SOURCE_DIR/$FILE" "$DEST_DIR"
    echo "$FILE copiado de $SOURCE_DIR para $DEST_DIR."
else
    echo "$FILE não encontrado no diretório $SOURCE_DIR."
fi
