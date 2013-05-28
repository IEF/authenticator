#!/usr/bin/env python

# genKeyLine code by Hordur Heidarsson
# rest Oliver Matthews

import base64
import sys
from binascii import unhexlify


secrets = []
labels = []
lengths = []
code_lengths=[]
max_code_length = 6 

time_zone = "+0"

def decode( code ):
  key_b32 = code.replace(' ','').upper()
  key_b32 = key_b32+'='*(32%len(key_b32))
  key     = base64.b32decode(key_b32)
  return map(ord,key)

def battle_decode ( code ):
  code = unhexlify( code )
  key = map(ord,code)
  return key

def genKeyLine( key_bytes ):
  print key_bytes
  lengths.append( len(key_bytes) )
  key_hex = ["0x%02X" % x for x in key_bytes]
  return "{ " + ', '.join(key_hex) + " },"


f = open( 'configuration.txt','r' )

for line in f:
  line = line.strip()
  if( line.startswith('#') or not ':' in line ): continue
  key,value = line.split(':')
  if( key.lower() == "tz" ):
    time_zone = value
  else:
    labels.append( key )
    if( key.lower() == "battle" ):
      code_lengths.append( "8" )
      max_code_length = 8
      secrets.append( genKeyLine(battle_decode(value)) )
    else:
      secrets.append( genKeyLine(decode(value)) )
      code_lengths.append( "6" )
f.close()

f = open( "src/configuration.h","w" )
f.write( "#ifndef _CONFIGURATION_H_\n#define _CONFIGURATION_H_\n" )
f.write( "#define NUM_SECRETS %i\n" % len(labels) )
f.write( "#define DEFAULT_TIME_ZONE %s\n" % time_zone )
f.write( "#define TOKEN_TEXT \"" )
for _ in range( max_code_length ):
  f.write( "X" )
f.write( "\"\n")
f.write( "#define DIGITS_TRUNCATE 1" )
for _ in range( max_code_length ):
  f.write( "0" )
f.write( "\n" )

#TODO:: calculate length properly
f.write( "char otplabels[NUM_SECRETS][%s] = {\n    " % 20)
for label in labels:
  f.write( "\"%s\"," % label )
f.write( "\n};\n" )

f.write( "unsigned char otpkeys[NUM_SECRETS][41] = {\n    " )
for secret in secrets:
  f.write( "%s\n" % secret )
f.write( "};\n" )

f.write ("int otpsizes[NUM_SECRETS] = { ")
for length in lengths:
  f.write( "%s," % length )
f.write( "};\n")

f.write( "int otplengths[NUM_SECRETS] = { ")
for length in code_lengths:
  f.write( "%s," % length )
f.write( "};\n")

f.write("#endif\n" )

f.close()
