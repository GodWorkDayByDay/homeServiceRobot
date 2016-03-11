#include "StdAfx.h"
#include "CvvImage.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CV_INLINE CvvImage::CvvImage()
{
   m_img = 0;
}
CV_INLINE void CvvImage::Destroy()
{
   cvReleaseImage( &m_img );
}
CV_INLINE CvvImage::~CvvImage()
{
   Destroy();
}
CV_INLINE bool  CvvImage::Create( int w, int h, int bpp, int origin )
{
   const unsigned max_img_size = 10000;

   if( (bpp != 8 && bpp != 24 && bpp != 32) ||
      (unsigned)w >=  max_img_size || (unsigned)h >= max_img_size ||
      (origin != IPL_ORIGIN_TL && origin != IPL_ORIGIN_BL))
   {
      assert(0); // most probably, it is a programming error
      return false;
   }
   if( !m_img || Bpp() != bpp || m_img->width != w || m_img->height != h )
   {
      if( m_img && m_img->nSize == sizeof(IplImage))
         Destroy();
     
      m_img = cvCreateImage( cvSize( w, h ), IPL_DEPTH_8U, bpp/8 );
   }
   if( m_img )
      m_img->origin = origin == 0 ? IPL_ORIGIN_TL : IPL_ORIGIN_BL;
   return m_img != 0;
}
CV_INLINE void  CvvImage::CopyOf( CvvImage& image, int desired_color )
{
   IplImage* img = image.GetImage();
   if( img )
   {
      CopyOf( img, desired_color );
   }
}
#define HG_IS_IMAGE(img)                                                  \
   ((img) != 0 && ((const IplImage*)(img))->nSize == sizeof(IplImage) && \
   ((IplImage*)img)->imageData != 0)
CV_INLINE void  CvvImage::CopyOf( IplImage* img, int desired_color )
{
   if( HG_IS_IMAGE(img) )
   {
      int color = desired_color;
      CvSize size = cvGetSize( img ); 
      if( color < 0 )
         color = img->nChannels > 1;
      if( Create( size.width, size.height,
         (!color ? 1 : img->nChannels > 1 ? img->nChannels : 3)*8,
         img->origin ))
      {
         cvConvertImage( img, m_img, 0 );
      }
   }
}
CV_INLINE bool  CvvImage::Load( const char* filename, int desired_color )
{
   IplImage* img = cvLoadImage( filename, desired_color );
   if( !img )
      return false;

   CopyOf( img, desired_color );
   cvReleaseImage( &img );

   return true;
}
CV_INLINE bool  CvvImage::LoadRect( const char* filename,
                   int desired_color, CvRect r )
{
   if( r.width < 0 || r.height < 0 ) return false;

   IplImage* img = cvLoadImage( filename, CV_LOAD_IMAGE_UNCHANGED );
   if( !img )
      return false;
   if( r.width == 0 || r.height == 0 )
   {
      r.width = img->width;
      r.height = img->height;
      r.x = r.y = 0;
   }
   if( r.x > img->width || r.y > img->height ||
      r.x + r.width < 0 || r.y + r.height < 0 )
   {
      cvReleaseImage( &img );
      return false;
   }
   
   if( r.x < 0 )
   {
      r.width += r.x;
      r.x = 0;
   }
   if( r.y < 0 )
   {
      r.height += r.y;
      r.y = 0;
   }
   if( r.x + r.width > img->width )
      r.width = img->width - r.x;

   if( r.y + r.height > img->height )
      r.height = img->height - r.y;
   cvSetImageROI( img, r );
   CopyOf( img, desired_color );
   cvReleaseImage( &img );
   return true;
}
CV_INLINE bool  CvvImage::Save( const char* filename )
{
   if( !m_img )
      return false;
   cvSaveImage( filename, m_img );
   return true;
}
CV_INLINE void  CvvImage::Show( const char* window )
{
   if( m_img )
      cvShowImage( window, m_img );
}
CV_INLINE void  CvvImage::Show( HDC dc, int x, int y, int w, int h, int from_x, int from_y )
{
   if( m_img && m_img->depth == IPL_DEPTH_8U )
   {
      uchar buffer[sizeof(BITMAPINFOHEADER) + 1024];
      BITMAPINFO* bmi = (BITMAPINFO*)buffer;
      int bmp_w = m_img->width, bmp_h = m_img->height;
      FillBitmapInfo( bmi, bmp_w, bmp_h, Bpp(), m_img->origin );
      from_x = MIN( MAX( from_x, 0 ), bmp_w - 1 );
      from_y = MIN( MAX( from_y, 0 ), bmp_h - 1 );
      int sw = MAX( MIN( bmp_w - from_x, w ), 0 );
      int sh = MAX( MIN( bmp_h - from_y, h ), 0 );
      SetDIBitsToDevice(
         dc, x, y, sw, sh, from_x, from_y, from_y, sh,
         m_img->imageData + from_y*m_img->widthStep,
         bmi, DIB_RGB_COLORS );
   }
}
CV_INLINE void  CvvImage::DrawToHDC( HDC hDCDst, RECT* pDstRect ) 
{
   if( pDstRect && m_img && m_img->depth == IPL_DEPTH_8U && m_img->imageData )
   {
      uchar buffer[sizeof(BITMAPINFOHEADER) + 1024];
      BITMAPINFO* bmi = (BITMAPINFO*)buffer;
      int bmp_w = m_img->width, bmp_h = m_img->height;
      CvRect roi = cvGetImageROI( m_img );
      CvRect dst = RectToCvRect( *pDstRect );
      if( roi.width == dst.width && roi.height == dst.height )
      {
         Show( hDCDst, dst.x, dst.y, dst.width, dst.height, roi.x, roi.y );
         return;
      }
      if( roi.width > dst.width )
      {
         SetStretchBltMode(
            hDCDst,           // handle to device context
            HALFTONE );
      }
      else
      {
         SetStretchBltMode(
            hDCDst,           // handle to device context
            COLORONCOLOR );
      }
      FillBitmapInfo( bmi, bmp_w, bmp_h, Bpp(), m_img->origin );
      ::StretchDIBits(
         hDCDst,
         dst.x, dst.y, dst.width, dst.height,
         roi.x, roi.y, roi.width, roi.height,
         m_img->imageData, bmi, DIB_RGB_COLORS, SRCCOPY );
   }
}
CV_INLINE void  CvvImage::Fill( int color )
{
   cvSet( m_img, cvScalar(color&255,(color>>8)&255,(color>>16)&255,(color>>24)&255) );
}