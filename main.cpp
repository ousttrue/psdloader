#include "psdloader.h"

#ifdef USE_PNG
#include <png.h>
#endif


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

  bool write_ppm(const char *path)
  {
    std::ofstream io(path, std::ios::binary);
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

#ifdef USE_PNG
  bool write_png(const char *path, bool isInterlaced=false)
  {
    // output
    FILE* pFile=fopen(path, "wb");
    if(pFile==NULL){
      return false;
    }

    // png
    png_struct *pPngStruct=png_create_write_struct(PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL);
    png_info *pPngInfo=png_create_info_struct(pPngStruct);

    // setjmp
    png_byte** ppRowTable=0;
    if(setjmp(pPngStruct->jmpbuf)){
      if(ppRowTable){
        delete[] ppRowTable;
      }
      png_destroy_write_struct(&pPngStruct, &pPngInfo);
      fclose(pFile);
      return false;
    }

    png_init_io(pPngStruct, pFile);
    png_set_compression_level(pPngStruct, Z_BEST_COMPRESSION);
    png_set_IHDR(pPngStruct, pPngInfo,
        width_, height_,
        8, PNG_COLOR_TYPE_RGB_ALPHA,
        isInterlaced ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // gamma
    double CRT_exponent=2.2;
    double LUT_exponent=1.0;
    double default_display_exponent=LUT_exponent*CRT_exponent;
    double default_gamma=1.0/default_display_exponent;
    png_set_gAMA(pPngStruct, pPngInfo, default_gamma);

    png_write_info(pPngStruct, pPngInfo);
    png_set_bgr(pPngStruct);

    if(isInterlaced){
      ppRowTable=new png_byte*[height_];
      for(int y=0; y<height_; ++y){
        ppRowTable[y]=&data_[y*width_*4];
      }
      png_write_image(pPngStruct, ppRowTable);
    }
    else{
      for(int y=0; y<height_; ++y){
        png_write_row(pPngStruct, &data_[y*width_*4]);
      }
    }

    png_write_end(pPngStruct, NULL);

    if(ppRowTable){
      delete[] ppRowTable;
    }
    png_destroy_write_struct(&pPngStruct, &pPngInfo);
    fclose(pFile);

    return true;
  }
#endif
};

unsigned char* channel_rgba(unsigned char* buf, CHANNEL_TYPE channel_id)
{
  switch(channel_id)
  {
  case CHANNEL_RED:
    return buf+0;
  case CHANNEL_GREEN:
    return buf+1;
  case CHANNEL_BLUE:
    return buf+2;
  case CHANNEL_ALPHA:
    return buf+3;
  default:
    assert(false);
    return NULL;
  }
}

unsigned char* channel_bgra(unsigned char* buf, CHANNEL_TYPE channel_id)
{
  switch(channel_id)
  {
  case CHANNEL_RED:
    return buf+2;
  case CHANNEL_GREEN:
    return buf+1;
  case CHANNEL_BLUE:
    return buf+0;
  case CHANNEL_ALPHA:
    return buf+3;
  default:
    assert(false);
    return NULL;
  }
}

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
      layer->store_to_interleaved_image(image.get(), channel_bgra);
      image.write_png(path);
#else
      sprintf(path, "%03d.ppm", write_count++);
      layer->store_to_interleaved_image(image.get(), channel_rgba);
      image.write_ppm(path);
#endif
      std::cout << " ==> " << path;
    }
    std::cout << std::endl;
  }

  return 0;
}

