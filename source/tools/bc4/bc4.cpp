#include "qcommon/base.h"
#include "qcommon/span2d.h"
#include "client/renderer/dds.h"

#include "rgbcx/rgbcx.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb/stb_image.h"

#include "stb/stb_image_resize.h"

static u32 BlockFormatMipLevels( u32 w, u32 h ) {
	u32 dim = Max2( w, h );
	u32 levels = 0;

	while( dim >= 4 ) {
		dim /= 2;
		levels++;
	}

	return levels;
}

static void MipDims( u32 * mip_w, u32 * mip_h, u32 w, u32 h, u32 level ) {
	*mip_w = Max2( w >> level, u32( 1 ) );
	*mip_h = Max2( h >> level, u32( 1 ) );
}

static u32 MipSize( u32 w, u32 h, u32 level ) {
	MipDims( &w, &h, w, h, level );
	return w * h;
}

static void * Alloc( size_t size ) {
	void * p = malloc( size );
	if( p == NULL ) {
		printf( "malloc failed\n" );
		abort();
	}
	return p;
}

template< typename T >
T * AllocMany( size_t n ) {
	return ( T * ) Alloc( n * sizeof( T ) );
}

template< typename T >
Span< T > AllocSpan( size_t n ) {
	return Span< T >( AllocMany< T >( n ), n );
}

int main( int argc, char ** argv ) {
	if( argc != 2 ) {
		printf( "Usage: bc4 <single channel image.png>\n" );
		return 1;
	}

	int w, h, comp;
	u8 * pixels = stbi_load( argv[ 1 ], &w, &h, &comp, 1 );
	if( pixels == NULL ) {
		printf( "Can't load image: %s\n", stbi_failure_reason() );
		return 1;
	}
	defer { stbi_image_free( pixels ); };

	if( !IsPowerOf2( w ) || !IsPowerOf2( h ) ) {
		printf( "Image must be power of 2 dimensions\n" );
		return 1;
	}

	if( comp != 1 ) {
		printf( "Image must be single channel\n" );
		return 1;
	}

	u32 num_levels = BlockFormatMipLevels( w, h );
	u32 total_size = 0;
	for( u32 i = 0; i < num_levels; i++ ) {
		total_size += MipSize( w, h, i );
	}

	constexpr u32 BC4BitsPerPixel = 4;
	constexpr u32 BC4BlockSize = ( 4 * 4 * BC4BitsPerPixel ) / 8;

	u32 bc4_bytes = ( total_size * BC4BitsPerPixel ) / 8;
	Span< u8 > bc4 = AllocSpan< u8 >( bc4_bytes );
	u32 bc4_cursor = 0;

	printf( "total size %u -> %u\n", total_size, bc4_bytes );

	u8 * src_level = AllocMany< u8 >( w * h );

	defer { free( bc4.ptr ); };
	defer { free( src_level ); };

	rgbcx::init();

	for( u32 i = 0; i < num_levels; i++ ) {
		u32 mip_w, mip_h;
		MipDims( &mip_w, &mip_h, w, h, i );

		int ok = stbir_resize_uint8(
			pixels, w, h, 0,
			src_level, mip_w, mip_h, 0,
			1
		);

		if( ok == 0 ) {
			printf( "stb_image_resize died lol\n" );
			return 1;
		}

		Span2D< const u8 > mip = Span2D< u8 >( pixels, mip_w, mip_h );

		for( u32 row = 0; row < mip_h / 4; row++ ) {
			for( u32 col = 0; col < mip_w / 4; col++ ) {
				Span2D< const u8 > src = mip.slice( col * 4, row * 4, 4, 4 );
				Span< u8 > dst = bc4.slice( bc4_cursor, bc4_cursor + BC4BlockSize );
				rgbcx::encode_bc4( dst.ptr, src.ptr, src.row_stride );
				bc4_cursor += BC4BlockSize;
			}
		}
	}

	DDSHeader dds_header = { };
	dds_header.magic = DDSMagic;
	dds_header.height = h;
	dds_header.width = w;
	dds_header.mipmap_count = num_levels;
	dds_header.format = DDSTextureFormat_BC4;

	FILE * dds = fopen( "mipped.dds", "wb" );
	if( dds == NULL ) {
		printf( "shit\n" );
		return 1;
	}

	fwrite( &dds_header, sizeof( dds_header ), 1, dds );
	fwrite( bc4.ptr, 1, bc4_bytes, dds );
	fclose( dds );

	return 0;
}
