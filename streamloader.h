#ifndef STREAM_LOADER_H
#define STREAM_LOADER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cassert>
#include <vector>

class StreamLoader
{
  std::istream &io_;
  std::ios::pos_type start_;
  std::vector<unsigned char> buf_;

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

  void skip_4_bound_()
  {
    int remain=tellg_()%4;
    if(remain){
      skip_(4-remain);
    }
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
    buf_.resize(bytes);
    io_.read((char*)&buf_[0], bytes);
    return std::string(buf_.begin(), buf_.end());
  }

  const std::vector<unsigned char> &read_vector_(int bytes)
  {
    buf_.resize(bytes);
    io_.read((char*)&buf_[0], bytes);
    return buf_;
  }

  void read_buf_(unsigned char* buf, size_t size)
  {
    io_.read((char*)buf, size);
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

#endif // STREAM_LOADER_H

