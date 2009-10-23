#include "readstream.h"
#include <vector>

struct PSDLayerChannel
{
  PSDLayerChannel()
    : size(0)
  {}
  unsigned long size;  
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
  PSDLayerChannel channels[5];
  std::string blend_mode;
  unsigned char opacity;
  unsigned char clipping;
  unsigned char flags;
  unsigned char padding;
  unsigned long extra;
  std::string name;
};

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
          layer.channels[PSDLayer::CHANNEL_RED].size=channel_size;
          break;
        case 1:
          layer.channels[PSDLayer::CHANNEL_GREEN].size=channel_size;
          break;
        case 2:
          layer.channels[PSDLayer::CHANNEL_BLUE].size=channel_size;
          break;
        case -1:
          layer.channels[PSDLayer::CHANNEL_ALPHA].size=channel_size;
          break;
        case -2:
          layer.channels[PSDLayer::CHANNEL_MASK].size=channel_size;
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
      unsigned long layer_mask_size=read_long_();
      if(layer_mask_size){
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
    for(layerIterator layer=begin(); layer!=end(); ++layer){
      //InterleavedImage image;
      std::cout << "[" << layer->name << "]" << std::endl;
      // each layer
      for(int i=0; i<5; ++i){
        // each channel
        PSDLayerChannel &channel=layer->channels[i];
        if(channel.size==0){
          continue;
        }
        std::cout << i << ',' << channel.size << " bytes" << std::endl;
      }
    }

    // image

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

