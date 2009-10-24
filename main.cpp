#include "readstream.h"

#include <vector>
#include <stdarg.h>
#include <string>

std::string string_format(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  char buf[1024];
  vsprintf(buf, format, ap);
  va_end(ap);

  return buf;
}

struct RGBA
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

class Image
{
  int width_;
  int height_;
  std::vector<RGBA> data_;

public:
  Image(int w, int h)
    : width_(w), height_(h)
    {
      data_.resize(w*h);
    }

  typedef std::vector<unsigned char>::iterator iterator;

  void set_plane(int channel, const std::vector<unsigned char> &buf)
  {
    if(buf[0] || buf[1]){
      set_compressed_plane_((unsigned short*)&buf[2], 
          (buf.size()-2)/sizeof(unsigned short));
    }
    else{
      set_plane_(&buf[2], buf.size()-2);
    }
  }

  bool write_ppm(const std::string &path)
  {
    std::ofstream io(path.c_str(), std::ios::binary);
    if(!io){
      return false;
    }

    // header
    io
      << "P6\n"
      << width_ << ' ' << height_ << '\n'
      << 255 << '\n'
      ;

    // raw data
    io.write((char*)&data_[0], data_.size());

    return true;
  }

private:
  void set_compressed_plane_(const unsigned short *buf, size_t size)
  {
  }

  void set_plane_(const unsigned char *buf, size_t size)
  {
  }
};

struct PSDLayer
{
  enum CHANNEL_TYPE {
    CHANNEL_RED,
    CHANNEL_GREEN,
    CHANNEL_BLUE,
    CHANNEL_ALPHA,
    CHANNEL_MASK,
  };
  unsigned long top;
  unsigned long left;
  unsigned long bottom;
  unsigned long right;
  unsigned short channels;
  unsigned long channel_size[5];
  std::string blend_mode;
  unsigned char opacity;
  unsigned char clipping;
  unsigned char flags;
  unsigned char padding;
  std::string name;

  PSDLayer()
    : top(0), left(0), bottom(0), right(0), channels(0)
    {
      channel_size[0]=0;
      channel_size[1]=0;
      channel_size[2]=0;
      channel_size[3]=0;
      channel_size[4]=0;
    }
};
inline std::ostream& operator<<(std::ostream &os, const PSDLayer &rhs)
{
  os
    << "{" 
    << rhs.left << ',' << rhs.top
    << ' ' << (rhs.right-rhs.left) << 'x' << (rhs.bottom-rhs.top)
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

      if(!load_rawdata()){
        return;
      }
    }

  typedef std::vector<PSDLayer>::iterator layerIterator;
  layerIterator begin(){ return layers_.begin(); }
  layerIterator end(){ return layers_.end(); }

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

std::cout 
  << layer_and_mask_size 
  << ',' << layer_block_size 
  << ',' << layer_count
  << std::endl
  ;

    for(int i=0; i<layer_count; ++i){
      // each layer
      PSDLayer layer;
      layer.top=read_long_();
      layer.left=read_long_();
      layer.bottom=read_long_();
      layer.right=read_long_();
      layer.channels=read_word_();

      assert(layer.channels<=5);
      for(int j=0; j<layer.channels; ++j){
        // each channel
        short channel_id=read_short_();
        unsigned long channel_size=read_long_();

        switch(channel_id)
        {
        case 0:
          layer.channel_size[PSDLayer::CHANNEL_RED]=channel_size;
          break;
        case 1:
          layer.channel_size[PSDLayer::CHANNEL_GREEN]=channel_size;
          break;
        case 2:
          layer.channel_size[PSDLayer::CHANNEL_BLUE]=channel_size;
          break;
        case -1:
          layer.channel_size[PSDLayer::CHANNEL_ALPHA]=channel_size;
          break;
        case -2:
          layer.channel_size[PSDLayer::CHANNEL_MASK]=channel_size;
          break;
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
  bool load_rawdata()
  {
    int i=1;
    for(layerIterator layer=begin(); layer!=end(); ++layer){
std::cout << *layer << std::endl;
      // each layer
      if(layer->channels){
        Image image(layer->right-layer->left, 
            layer->bottom-layer->top);

        // each channel
        image.set_plane(PSDLayer::CHANNEL_RED, read_vector_(
              layer->channel_size[PSDLayer::CHANNEL_RED]));
        image.set_plane(PSDLayer::CHANNEL_GREEN, read_vector_(
              layer->channel_size[PSDLayer::CHANNEL_GREEN]));
        image.set_plane(PSDLayer::CHANNEL_BLUE, read_vector_(
              layer->channel_size[PSDLayer::CHANNEL_BLUE]));
        image.set_plane(PSDLayer::CHANNEL_ALPHA, read_vector_(
              layer->channel_size[PSDLayer::CHANNEL_ALPHA]));

        // write to file
        image.write_ppm(string_format("%02d.ppm", i++));

        if(i>2){
          break;
        }
      }
    }

    return true;
  }
};

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

  PSDLoader loader(io);

  return 0;
}

