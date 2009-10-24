#include "readstream.h"

#include <vector>
#include <stdarg.h>
#include <string>

enum CHANNEL_TYPE {
  CHANNEL_ALPHA,
  CHANNEL_RED,
  CHANNEL_GREEN,
  CHANNEL_BLUE,
  CHANNEL_MASK,
};

#define TO_SHORT(buf) (buf[0]<<8|buf[1])

class Image
{
  int width_;
  int height_;
  std::vector<unsigned char> data_;

public:
  Image(int w, int h)
    : width_(w), height_(h)
    {
      data_.resize(w*h*4);
    }

  typedef std::vector<unsigned char>::iterator iterator;

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
    char *current=(char*)&data_[0];
    for(int y=0; y<height_; ++y){
      for(int x=0; x<width_; ++x, current+=4){
        io.write(current, 3);
      }
    }

    return true;
  }

  void set_plane(CHANNEL_TYPE channel, const std::vector<unsigned char> &buf)
  {
    short compressed=TO_SHORT(buf);
    switch(compressed)
    {
    case 0:
      set_plane_(channel, &buf[0]+2, &buf[0]+buf.size());
      break;
    case 1:
      set_compressed_plane_(channel, &buf[0]+2, &buf[0]+buf.size());
      break;

    default:
      std::cout << "unknown compression" << std::endl;
      assert(false);
    }
  }

private:
  void set_compressed_plane_(CHANNEL_TYPE channel, 
      const unsigned char *buf, const unsigned char *end)
  {
    const unsigned char *src=buf;

    unsigned short line_length[height_];
    for(int y=0; y<height_; ++y, src+=2)
    {
      line_length[y]=TO_SHORT(src);
    }

    unsigned char *dst=get_interleaved_head_(channel);
    for(int y=0; y<height_; ++y){
      const unsigned char *line_end=src+line_length[y];
      int x;
      for(x=0; x<width_;){
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
      assert(x==width_);
      src=line_end;
    }
  }

  void set_plane_(CHANNEL_TYPE channel,
      const unsigned char *begin, const unsigned char *end)
  {
    unsigned char *dst=get_interleaved_head_(channel);
    const unsigned char* src=begin;
    for(int y=0; y<height_; ++y){
      for(int x=0; x<width_; ++x, ++src, dst+=4){
        *dst=*src;
      }
    }
  }

  unsigned char *get_interleaved_head_(CHANNEL_TYPE channel)
  {
    switch(channel){
    case CHANNEL_RED:
      return &data_[0];
    case CHANNEL_GREEN:
      return &data_[1];
    case CHANNEL_BLUE:
      return &data_[2];
    case CHANNEL_ALPHA:
      return &data_[3];
    default:
      assert(false);
      return NULL;
    }
  }

};

struct PSDLayer
{
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
    << ' ' << rhs.channels
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
          layer.channel_size[CHANNEL_RED]=channel_size;
          break;
        case 1:
          layer.channel_size[CHANNEL_GREEN]=channel_size;
          break;
        case 2:
          layer.channel_size[CHANNEL_BLUE]=channel_size;
          break;
        case -1:
          layer.channel_size[CHANNEL_ALPHA]=channel_size;
          break;
        case -2:
          layer.channel_size[CHANNEL_MASK]=channel_size;
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
  bool load_rawdata()
  {
    int i=1;
    for(layerIterator layer=begin(); layer!=end(); ++layer){
      std::cout << '[' << i << ']' << *layer;
      // each layer
      if(layer->channels){
        int width=layer->right-layer->left;
        int height=layer->bottom-layer->top;
        Image image(width, height);

        // each channel
        image.set_plane(CHANNEL_ALPHA, read_vector_(
              layer->channel_size[CHANNEL_ALPHA]));

        image.set_plane(CHANNEL_RED, read_vector_(
              layer->channel_size[CHANNEL_RED]));

        image.set_plane(CHANNEL_GREEN, read_vector_(
              layer->channel_size[CHANNEL_GREEN]));

        image.set_plane(CHANNEL_BLUE, read_vector_(
              layer->channel_size[CHANNEL_BLUE]));

        if(width==0){
          continue;
        }
        if(height==0){
          continue;
        }
        // write to file
        char path[1024];
        sprintf(path, "%03d.ppm", i++);
        image.write_ppm(path);
        std::cout << " ==> " << path << std::endl;
      }
    }

    return true;
  }
};

//----------------------------------------------------------------------------//
// entry point
//----------------------------------------------------------------------------//
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

