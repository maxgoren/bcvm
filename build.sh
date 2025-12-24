#!/bin/sh
mgclex parse/ghost.mlex parse/lexer_matrix.h
g++ -g glaux.cpp -o glaux
sudo mv glaux /usr/local/bin
