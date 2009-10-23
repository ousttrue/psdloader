#include "readstream.h"

bool load_resolution(std::istream &io)
{
  skip(io, 16);
  return true;
}

bool load_psd_header(std::istream &io)
{
  //------------------------------------------------------------//
  // header
  //------------------------------------------------------------//
  std::cout << "[psd header(26bytes)]" << std::endl;

  // file id
  std::string id=read_string(io, 4);
  if(id!="8BPS"){
    std::cout << "invalid File ID(8BPS)" << std::endl;
    return false;
  }

  // version
  unsigned short version=read_word(io);
  std::cout << "version " << version << std::endl;

  skip(io, 6);

  // channels
  unsigned short channels=read_word(io);
  std::cout << "channels " << channels << std::endl;

  // height
  unsigned long height=read_long(io);
  // width
  unsigned long width=read_long(io);
  // depth
  unsigned short depth=read_word(io);
  std::cout << width << 'x' << height << ' ' << depth << std::endl;

  // mode
  unsigned short mode=read_word(io);
  std::cout << "mode " << mode << std::endl;

  //------------------------------------------------------------//
  // color mode data block
  //------------------------------------------------------------//
  //std::cout << "[color mode data block]" << std::endl;
  switch(mode)
  {
  case 2:
    // index
    assert(false);
    break;
  case 8:
    // duo tone
    assert(false);
    break;
  default:    
    skip(io, 4);
  }

  unsigned long header_size=read_long(io);
  std::cout << "header size: " << header_size << std::endl;

  while(io){
    if(io.tellg()%2){
      skip(io, 1);
    }

    if(io.peek()!='8'){
      break;
    }

    //------------------------------------------------------------//
    // image resource block
    //------------------------------------------------------------//
    //std::cout << "[image reosurce block]" << std::endl;
    if(read_string(io, 4)!="8BIM"){
      std::cout << "invalid resource block" << std::endl;
      return false;
    }

    unsigned short res_id=read_word(io);
    //std::cout << "resouce id 0x" << std::hex << res_id << std::dec << std::endl;

    std::string name=read_string(io);
    if(name==""){
      skip(io, 1);
    }

    unsigned long res_size=read_long(io);
    //std::cout << "size " << res_size << std::endl;

    switch(res_id)
    {
      case 0x03ed:
        // resolution information
        assert(res_size==16);
        load_resolution(io);
        break;

      default:
        //std::cout << "unknown resouce id" << std::endl;
        skip(io, res_size);
    }
  }

  return true;
}

bool load_psd_layer(std::istream &io)
{
  std::cout << "[layer and mask information block]" << std::endl;
  unsigned long layer_and_mask_size=read_long(io);
  unsigned long layer_block_size=read_long(io);
  std::cout << "layer and mask(" << layer_and_mask_size << ")"
    << "layers size(" << layer_block_size << ")" << std::endl;

  //  layers
  short layer_count=read_short(io);
  if(layer_count<0){
    layer_count*=-1;
  }
  std::cout << layer_count << " layers" << std::endl;

  for(int i=0; io && i<layer_count; ++i){
    std::cout << "(" << i << ")";
    // each layer
    unsigned long top=read_long(io);
    unsigned long left=read_long(io);
    unsigned long bottom=read_long(io);
    unsigned long right=read_long(io);
    std::cout 
      << "[" << left << ", " << top << "]"
      << " - "
      << "[" << right << ", " << bottom << "]"
      ;

    unsigned short channels=read_word(io);
    for(int j=0; io && j<channels; ++j){
      // each channel
      unsigned short channel_id=read_word(io);
      unsigned long channel_size=read_long(io);
    }

    // blend mode
    if(read_string(io, 4)!="8BIM"){
      std::cout << "invalid blend mode signature" << std::endl;
      return false;
    }
    std::string key=read_string(io, 4);
    unsigned char opacity=read_byte(io);
    unsigned char clipping=read_byte(io);
    unsigned char flags=read_byte(io);
    unsigned char padding=read_byte(io);
    unsigned long extra=read_long(io);

    // layer mask section
    unsigned long layer_mask_size=read_long(io);
    if(layer_mask_size){
      unsigned long top=read_long(io);
      unsigned long left=read_long(io);
      unsigned long bottom=read_long(io);
      unsigned long right=read_long(io);
      unsigned char color=read_byte(io);
      unsigned char flags=read_byte(io);
      unsigned short padding=read_byte(io);
      std::cout << "layer mask "
        << left << ", " << top
        << " - " << right << ", " << bottom
        ;
    }

    // blending range
    unsigned long blending_size=read_long(io);
    skip(io, blending_size);

    // layer name
    std::string name=read_string(io);
    if(name[0]=='\n' || name[0]=='r'){
      std::cout << name.c_str()+1 << '#' << key;
    }
    else{
      std::cout << name << '#' << key;
    }
    if(io.tellg()%2){
      skip(io, 1);
    }

    while(io){
      if(io.peek()!='8'){
        break;
      }
      if(read_string(io, 4)!="8BIM"){
        std::cout << "not found 8BIM" << std::endl;
        return false;
      }
      std::string key=read_string(io, 4);
      std::cout << "[" << key << "]";
      unsigned long size=read_long(io);
      skip(io, size);
    }
    std::cout << std::endl;
  }

  return true;
}

bool load_psd_rawdata(std::istream &io)
{
  unsigned short compressed=read_word(io);
  std::cout << "compressed: " << compressed << std::endl;

  return true;
}

bool load_psd(std::istream &io)
{
  if(!load_psd_header(io)){
    return false;
  }

  if(!load_psd_layer(io)){
    return false;
  }

  if(!load_psd_rawdata(io)){
    return false;
  }

  return true;
}

int main(int argc, char** argv)
{
  if(argc<2){
    std::cout << "usage: " <<argv[0] << " {psd}" << std::endl;
    return 1;
  }

  std::ifstream io(argv[1], std::ios::binary);
  if(!io){
    std::cout << "fail to open " << argv[1] << std::endl;
    return 2;
  }

  if(!load_psd(io)){
    std::cout << "fail to load psd" << std::endl;
    return 3;
  }

  std::cout << "success" << std::endl;
  return 0;
}

