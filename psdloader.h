#ifndef PSD_LOADER_H
#define PSD_LOADER_H

#include "streamloader.h"

#define TO_SHORT(buf) (buf[0]<<8|buf[1])

enum CHANNEL_TYPE {
  CHANNEL_ALPHA,
  CHANNEL_RED,
  CHANNEL_GREEN,
  CHANNEL_BLUE,
  CHANNEL_MASK,
};

struct PSDChannel
{
  CHANNEL_TYPE id;
  std::vector<unsigned char> data;
  
  PSDChannel(CHANNEL_TYPE _id, size_t size)
    : id(_id), data(size)
    {}
};

struct PSDLayer
{
  unsigned long top;
  unsigned long left;
  unsigned long bottom;
  unsigned long right;

  std::vector<PSDChannel> channels;

  std::string blend_mode;
  unsigned char opacity;
  unsigned char clipping;
  unsigned char flags;
  unsigned char padding;
  std::string name;

  PSDLayer()
    : top(0), left(0), bottom(0), right(0)
    {}

  int get_width()const{ return right-left; }
  int get_height()const{ return bottom-top; }

  PSDChannel *begin(){ 
    return channels.empty() ? NULL : &channels[0]; 
  }
  PSDChannel *end(){ 
    return channels.empty() ? NULL : &channels[0]+channels.size(); 
  }
  const PSDChannel *begin()const{ 
    return channels.empty() ? NULL : &channels[0]; 
  }
  const PSDChannel *end()const{ 
    return channels.empty() ? NULL : &channels[0]+channels.size(); 
  }

  //------------------------------------------------------------//
  // plane channels to interleaved image
  //------------------------------------------------------------//
  typedef unsigned char* (*CHANNEL_FUNC)(unsigned char* buf, CHANNEL_TYPE type);
  void store_to_interleaved_image(unsigned char *buf, CHANNEL_FUNC func)
  {
    for(const PSDChannel *channel=begin(); channel!=end(); ++channel){
      unsigned char *dst=func(buf, channel->id);

      // each channel
      short compressed=TO_SHORT(channel->data);
      switch(compressed)
      {
      case 0:
        set_plane_(dst,
            &channel->data[0]+2, &channel->data[0]+channel->data.size());
        break;
      case 1:
        set_compressed_plane_(dst,
            &channel->data[0]+2, &channel->data[0]+channel->data.size());
        break;

      default:
        std::cout << "unknown compression" << std::endl;
        assert(false);
      }
    }
  }

private:
  //------------------------------------------------------------//
  // RLE compress plane data to interleaved
  //------------------------------------------------------------//
  void set_compressed_plane_(unsigned char* dst,
      const unsigned char *begin, const unsigned char *end)
  {
    int width=get_width();
    int height=get_height();
    const unsigned char *src=begin;

    unsigned short line_length[get_height()];
    for(int y=0; y<height; ++y, src+=2)
    {
      line_length[y]=TO_SHORT(src);
    }

    for(int y=0; y<height; ++y){
      const unsigned char *line_end=src+line_length[y];
      int x;
      for(x=0; x<width;){
        if(src>=line_end){
          break;
        }
        if(*src & 0x80){
          // continuos
          int pack_length=-((char)*src)+1;
          ++src;
          assert(pack_length<=128);
          unsigned char value=*src;
          ++src;
          assert(src<=line_end);
          for(int i=0; i<pack_length; ++i, dst+=4, ++x){
            *dst=value;                  
          }
        }
        else{
          // not continious
          int pack_length=*src+1;
          ++src;
          assert(pack_length<=128);
          for(int i=0; i<pack_length; ++i, dst+=4, ++src, ++x){
            *dst=*src;
            assert(src<=line_end);
          }
        }
      }
      assert(x==width);
      src=line_end;
    }
  }

  //------------------------------------------------------------//
  // palane data to interleaved
  //------------------------------------------------------------//
  void set_plane_(unsigned char *dst,
      const unsigned char *begin, const unsigned char *end)
  {
    int width=get_width();
    int height=get_height();
    const unsigned char *src=begin;
    for(int y=0; y<height; ++y){
      for(int x=0; x<width; ++x, ++src, dst+=4){
        *dst=*src;
      }
    }
  }
};
inline std::ostream& operator<<(std::ostream &os, const PSDLayer &rhs)
{
  os
    << "{" 
    << rhs.left << ',' << rhs.top
    << ' ' << rhs.get_width() << 'x' << rhs.get_height()
    << ' ' << rhs.channels.size()
    << "}"
    ;
}

class PSDLoader : public StreamLoader
{
  enum STATUS {
    STATUS_UNKNOWN,
    STATUS_INVALID_8BPS,
    STATUS_INVALID_BLEND_MODE_SIGNATURE,
  };
  STATUS status_;

  // first 26 bytes
  unsigned short version_;
  unsigned short channels_;

  unsigned long height_;
  unsigned long width_;
  unsigned short depth_;
  unsigned short mode_;

  std::vector<PSDLayer> layers_;
public:
  PSDLoader(std::istream &io)
    : StreamLoader(io), status_(STATUS_UNKNOWN)
    {
      if(!load_header_()){
        return;
      }

      if(!load_color_mode_data_block_()){
        return;
      }

      if(!load_image_resource_blocks_()){
        return;
      }

      if(!load_layers_()){
        return;
      }

      if(!load_rawdata_()){
        return;
      }
    }

  PSDLayer *begin(){ 
    return layers_.empty() ? NULL : &layers_[0]; 
  }
  PSDLayer *end(){ 
    return layers_.empty() ? NULL : &layers_[0]+layers_.size(); 
  }

private:
  //------------------------------------------------------------//
  // psd header(26 bytes)
  //------------------------------------------------------------//
  bool load_header_()
  {
    // file id
    std::string id=read_string_(4);
    if(id!="8BPS"){
      status_=STATUS_INVALID_8BPS;
      return false;
    }

    // version
    version_=read_word_();

    skip_(6);

    // channels
    channels_=read_word_();

    // height
    height_=read_long_();
    // width
    width_=read_long_();
    // depth
    depth_=read_word_();
    // mode
    mode_=read_word_();

    return true;
  }

  //------------------------------------------------------------//
  // color mode data block
  //------------------------------------------------------------//
  bool load_color_mode_data_block_()
  {
    //std::cout << "[color mode data block]" << std::endl;
    switch(mode_)
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
      skip_(4);
    }

    return true;
  }

  //------------------------------------------------------------//
  // image resource blocks
  //------------------------------------------------------------//
  bool load_image_resource_blocks_()
  {
    unsigned long header_size=read_long_();
    skip_(header_size);
    return true;
  }

  //------------------------------------------------------------//
  // layer and mask information blocks
  //------------------------------------------------------------//
  bool load_layers_()
  {
    unsigned long layer_and_mask_size=read_long_();
    unsigned long layer_block_size=read_long_();

    //  layers
    short layer_count=read_short_();
    if(layer_count<0){
      layer_count*=-1;
    }

    for(int i=0; i<layer_count; ++i){
      // each layer
      PSDLayer layer;
      layer.top=read_long_();
      layer.left=read_long_();
      layer.bottom=read_long_();
      layer.right=read_long_();

      unsigned short channels=read_word_();
      assert(channels<=5);
      for(int j=0; j<channels; ++j){
        // each channel
        short channel_id=read_short_();
        unsigned long channel_size=read_long_();

        switch(channel_id)
        {
        case 0:
          layer.channels.push_back(PSDChannel(CHANNEL_RED, channel_size));
          break;
        case 1:
          layer.channels.push_back(PSDChannel(CHANNEL_GREEN, channel_size));
          break;
        case 2:
          layer.channels.push_back(PSDChannel(CHANNEL_BLUE, channel_size));
          break;
        case -1:
          layer.channels.push_back(PSDChannel(CHANNEL_ALPHA, channel_size));
          break;
        case -2:
          layer.channels.push_back(PSDChannel(CHANNEL_MASK, channel_size));
          break;
        default:
          std::cout << "unknown channel id: " << channel_id << std::endl;
          assert(false);
        }
      }

      // blend mode
      if(read_string_(4)!="8BIM"){
        status_=STATUS_INVALID_BLEND_MODE_SIGNATURE;
        return false;
      }
      layer.blend_mode=read_string_(4);
      layer.opacity=read_byte_();
      layer.clipping=read_byte_();
      layer.flags=read_byte_();
      layer.padding=read_byte_();
      unsigned long extra=read_long_();
      std::ios::pos_type layer_end=tellg_()
        +static_cast<std::ios::pos_type>(extra);

      // layer mask section
      unsigned long mask_size=read_long_();
      if(mask_size){
        unsigned long top=read_long_();
        unsigned long left=read_long_();
        unsigned long bottom=read_long_();
        unsigned long right=read_long_();
        unsigned char color=read_byte_();
        unsigned char flags=read_byte_();
        unsigned short padding=read_byte_();
      }

      // blending range
      unsigned long blending_size=read_long_();
      skip_(blending_size);

      // layer name
      layer.name=read_string_();
      // skip odd byte
      if(tellg_()%2){
        skip_(1);
      }

      skip_to_(layer_end);

      layers_.push_back(layer);
    }
    return true;
  }

  //------------------------------------------------------------//
  // raw data
  //------------------------------------------------------------//
  bool load_rawdata_()
  {
    int write_count=1;
    int i=0;
    for(PSDLayer *layer=begin(); layer!=end(); ++layer, ++i){
      // each layer
      for(PSDChannel *channel=layer->begin();
          channel!=layer->end();
          ++channel){
        // each channel
        read_buf_(&channel->data[0], channel->data.size());
      }
    }

    // image data

    return true;
  }

};

#endif // PSD_LOADER_H
