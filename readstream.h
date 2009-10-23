#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cassert>

void skip_while(std::istream &io, char skip_letter)
{
  while(io){
    char letter=io.peek();
    if(letter!=skip_letter){
      break;
    }
    else{
      io.get();
    }
  }
}

void skip(std::istream &io, int bytes)
{
  io.seekg(bytes, std::ios::cur);
}

std::string read_string(std::istream &io)
{
  std::stringstream ss;
  while(io){
    char letter=io.get();
    if(letter==0){
      break;
    }
    ss << letter;
  }
  return ss.str();
}

std::string read_string(std::istream &io, int bytes)
{
  char buf[bytes];
  io.read(buf, bytes);
  return std::string(buf, buf+bytes);
}

unsigned short read_word(std::istream &io)
{
  char buf[2];
  io.read(buf, 2);
  return  ((unsigned char)buf[0]<<8)|(unsigned char)buf[1];
}

short read_short(std::istream &io)
{
  char buf[2];
  io.read(buf, 2);
  short word;
  *((unsigned short*)&word)=((unsigned char)buf[0]<<8)|(unsigned char)buf[1];
  return word;
}

unsigned char read_byte(std::istream &io)
{
  return io.get();
}

unsigned long read_long(std::istream &io)
{
  char buf[4];
  io.read(buf, 4);
  return ((unsigned char)buf[0]<<24) + 
    ((unsigned char)buf[1]<<16) + 
    ((unsigned char)buf[2]<<8) +
    ((unsigned char)buf[3]);
}

