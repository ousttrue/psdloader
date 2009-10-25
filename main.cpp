#include "psdloader.h"
#include "rgbimage.h"

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

  int write_count=1;
  int i=0;
  for(PSDLayer *layer=loader.begin(); 
      layer!=loader.end(); 
      ++layer, ++i){
    // each layer
    std::cout << '(' << i << ')' << *layer;
    int width=layer->get_width();
    int height=layer->get_height();
    char path[1024];
    if(width>0 && height>0){
      RGBAImage image(width, height);
#ifdef USE_PNG
      sprintf(path, "%03d.png", write_count++);
      layer->store_to_interleaved_image(image.get(), ChannelMap_BRGA());
      image.write_png(path);
#else
      sprintf(path, "%03d.ppm", write_count++);
      layer->store_to_interleaved_image(image.get(), ChannelMap_RGBA());
      image.write_ppm(path);
#endif
      std::cout << " ==> " << path;
    }
    std::cout << std::endl;
  }

  return 0;
}

