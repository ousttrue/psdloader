#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cassert>

class StreamLoader
{
  std::istream &io_;
  std::ios::pos_type start_;

public:
  StreamLoader(std::istream &io)
    : io_(io), start_(io.tellg())
    {}

protected:
  void skip_while_(char skip_letter)
  {
    while(io_){
      char letter=io_.peek();
      if(letter!=skip_letter){
        break;
      }
      else{
        io_.get();
      }
    }
  }

  void skip_to_(std::ios::pos_type end)
  {
    skip_(end-tellg_());
  }

  void skip_(int bytes)
  {
    io_.seekg(bytes, std::ios::cur);
  }

  std::string read_string_()
  {
    std::stringstream ss;
    while(io_){
      char letter=io_.get();
      if(letter==0){
        break;
      }
      ss << letter;
    }
    return ss.str();
  }

  std::string read_string_(int bytes)
  {
    char buf[bytes];
    io_.read(buf, bytes);
    return std::string(buf, buf+bytes);
  }

  unsigned short read_word_()
  {
    char buf[2];
    io_.read(buf, 2);
    return  ((unsigned char)buf[0]<<8)|(unsigned char)buf[1];
  }

  short read_short_()
  {
    char buf[2];
    io_.read(buf, 2);
    short word;
    *((unsigned short*)&word)=((unsigned char)buf[0]<<8)|(unsigned char)buf[1];
    return word;
  }

  unsigned char read_byte_()
  {
    return io_.get();
  }

  unsigned long read_long_()
  {
    char buf[4];
    io_.read(buf, 4);
    return ((unsigned char)buf[0]<<24) + 
      ((unsigned char)buf[1]<<16) + 
      ((unsigned char)buf[2]<<8) +
      ((unsigned char)buf[3]);
  }

  std::ios::pos_type tellg_()
  {
    return io_.tellg()-start_;
  }
};

