#include "psdloader.h"

class RGBAImage
{
  int width_;
  int height_;
  std::vector<unsigned char> data_;

public:
  RGBAImage()
    : width_(0), height_(0)
    {}

  RGBAImage(int w, int h)
  {
    resize(w, h);
  }

  void resize(int w, int h)
  {
    width_=w;
    height_=h;
    data_.resize(w*h*4);
  }

  unsigned char* get()
  {
    return data_.empty() ? NULL : &data_[0];
  }

  void write_ppm(std::ostream &io)
  {
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

  int write_count=1;
  int i=0;
  for(PSDLayer *layer=loader.begin(); 
      layer!=loader.end(); 
      ++layer, ++i){
    // each layer
    std::cout << '(' << i << ')' << *layer;
    int width=layer->get_width();
    int height=layer->get_height();
    if(width>0 && height>0){
      RGBAImage image(width, height);
      layer->store_to_interleaved_image(image.get());
      char path[1024];
      sprintf(path, "%03d.ppm", write_count++);
      std::ofstream io(path, std::ios::binary);
      image.write_ppm(io);
      std::cout << " ==> " << path;
    }
    std::cout << std::endl;
  }

  return 0;
}

