#include "pmx_read.h"

#include <stdio.h>
#include <string.h>

#define PMX_READ_FILE	( ( 'F' << 24 ) | ( 'I' << 16 ) | ( 'L' << 8 ) | 'E' )
#define PMX_READ_BUFFER	( ( 'B' << 24 ) | ( 'U' << 16 ) | ( 'F' << 8 ) | 'F' )

#define PMX_OFFSET_HEADER	( 0 )
#define PMX_OFFSET_INFO		( 8 )
#define PMX_OFFSET_ISIZE	( 11 )
#define PMX_OFFSET_MINFO	( 17 )

enum pmx_read_bits_e {
	PMX_BIT_HEADER			= ( 0x1 << 0 ),
	PMX_BIT_INFO			= ( 0x1 << 1 ),
	PMX_BIT_ISIZE			= ( 0x1 << 2 ),
	PMX_BIT_MINFO			= ( 0x1 << 3 ),

	PMX_BIT_INFO_ALL		= ( PMX_BIT_HEADER | PMX_BIT_INFO | PMX_BIT_ISIZE | PMX_BIT_MINFO ),

	PMX_BIT_OFFSET_VERTEX	= ( 0x1 << 8 ),
	PMX_BIT_OFFSET_FACE		= ( 0x1 << 9 ),
	PMX_BIT_OFFSET_TEXTURE	= ( 0x1 << 10 ),

	PMX_BIT_OFFSET_ALL		= ( PMX_BIT_OFFSET_VERTEX | PMX_BIT_OFFSET_FACE | PMX_BIT_OFFSET_TEXTURE )
};

typedef struct pmx_read_file_s {
	pmx_read	super;
	FILE *		file;
} pmx_read_file;

typedef struct pmx_read_buffer_s {
	pmx_read	super;
	void *		start;
	void *		end;
	void *		cur;
} pmx_read_buffer;

static pmx_int pmx_read_file_fread( pmx_read_file * const _read, void * const _dest, const pmx_int _length ) {
	return ( pmx_int )fread( _dest, 1, ( size_t )_length, _read->file );
}

static pmx_int pmx_read_file_fseek( pmx_read_file * const _read, const pmx_int _offset, const pmx_int _origin ) {
	switch ( _origin ) {
	case PMX_READ_SEEK_SET:
		return ( pmx_int )fseek( _read->file, ( size_t )_offset, SEEK_SET );
	case PMX_READ_SEEK_CUR:
		return ( pmx_int )fseek( _read->file, ( size_t )_offset, SEEK_CUR );
	case PMX_READ_SEEK_END:
		return ( pmx_int )fseek( _read->file, ( size_t )_offset, SEEK_END );
	}
	return -1;
}

static pmx_int pmx_read_file_ftell( pmx_read_file * const _read ) {
	return ( pmx_int )ftell( _read->file );
}

static pmx_int pmx_read_buffer_fread( pmx_read_buffer * const _read, void * const _dest, const pmx_int _length ) {
	const pmx_int length = ( pmx_int )_read->end - ( pmx_int )_read->cur;
	return ( pmx_int )memcpy( _dest, _read->cur, ( size_t )( _length < length ? _length : length ) );
}

static pmx_int pmx_read_buffer_fseek( pmx_read_buffer * const _read, const pmx_int _offset, const pmx_int _origin ) {
	void * newPos;

	switch ( _origin ) {
	case PMX_READ_SEEK_SET:
		newPos = pmx_ptr_add( _read->start, _offset );
	case PMX_READ_SEEK_CUR:
		newPos = pmx_ptr_add( _read->cur, _offset );
	case PMX_READ_SEEK_END:
		newPos = pmx_ptr_add( _read->end, _offset );
	}
	
	if ( newPos <= _read->end && newPos >= _read->start ) {
		_read->cur = newPos;
		return 0;
	}
	return -1;
}

static pmx_int pmx_read_buffer_ftell( pmx_read_buffer * const _read ) {
	return ( pmx_int )pmx_ptr_add( _read->cur, -( pmx_int )_read->start );
}

pmx_byte pmx_read_text( pmx_read * const _read, pmx_text * const _text ) {
	_read->fread_f( _read, &_text->size, sizeof( _text->size ) );
	if ( _text->size ) {
		_text->string.utf8 = ( pmx_byte * )pmx_alloc( PMX_NULL, _text->size );
		_read->fread_f( _read, _text->string.utf8, _text->size );
	}
	
	return 0;
}

pmx_read * pmx_read_create_file( void * const _file ) {
	pmx_read_file * const read = ( pmx_read_file * )pmx_alloc( PMX_NULL, sizeof( pmx_read_file ) );
	memset( read, 0, sizeof( *read ) );

	read->super.type = PMX_READ_FILE;
	read->super.fread_f = ( pmx_read_fread_t )pmx_read_file_fread;
	read->super.fseek_f = ( pmx_read_fseek_t )pmx_read_file_fseek;
	read->super.ftell_f = ( pmx_read_ftell_t )pmx_read_file_ftell;

	read->file = ( FILE * )_file;

	return &read->super;
}

pmx_read * pmx_read_create_buffer( void * const _buffer, const pmx_int _length ) {
	pmx_read_buffer * const read = ( pmx_read_buffer * )pmx_alloc( PMX_NULL, sizeof( pmx_read_buffer ) );
	memset( read, 0, sizeof( *read ) );
	
	read->super.type = PMX_READ_BUFFER;
	read->super.fread_f = ( pmx_read_fread_t )pmx_read_buffer_fread;
	read->super.fseek_f = ( pmx_read_fseek_t )pmx_read_buffer_fseek;
	read->super.ftell_f = ( pmx_read_ftell_t )pmx_read_buffer_ftell;

	read->start = _buffer;
	read->cur = read->start;
	read->end = pmx_ptr_add( read->start, _length );

	return &read->super;
}

void pmx_read_destroy( pmx_read * const _read ) {
	pmx_alloc( _read->minfo.name.local.string.utf8, 0 );
	pmx_alloc( _read->minfo.name.global.string.utf8, 0 );
	
	pmx_alloc( _read->minfo.comment.local.string.utf8, 0 );
	pmx_alloc( _read->minfo.comment.global.string.utf8, 0 );

	pmx_alloc( _read, 0 );
}

pmx_byte pmx_read_header( pmx_read * const _read, pmx_header * const _header ) {
	_read->fseek_f( _read, PMX_OFFSET_HEADER, PMX_READ_SEEK_SET );

	_read->fread_f( _read, _read->header.signature, sizeof( _read->header.signature ) );
	_read->fread_f( _read, &_read->header.version, sizeof( _read->header.version ) );

	if ( pmx_header_check( &_read->header ) == 0 ) {
		pmx_read_info( _read, PMX_NULL );
		pmx_read_isize( _read, PMX_NULL );
		pmx_read_minfo( _read, PMX_NULL );

		_read->offsetVertex = PMX_OFFSET_MINFO + pmx_minfo_size( &_read->minfo );
		_read->offsetFace = _read->offsetVertex + pmx_read_sizeof_vertex( _read, pmx_read_count_vertex( _read ) );
		_read->offsetTexture = _read->offsetFace + pmx_read_sizeof_face( _read, pmx_read_count_face( _read ) );
	}

	if ( _header ) {
		memcpy( _header, &_read->header, sizeof( *_header ) );
	}

	return 0;
}

pmx_byte pmx_read_info( pmx_read * const _read, pmx_info * const _info ) {
	_read->fseek_f( _read, PMX_OFFSET_INFO, PMX_READ_SEEK_SET );

	_read->fread_f( _read, &_read->info.dataCount, sizeof( _read->info.dataCount ) );
	_read->fread_f( _read, &_read->info.textTypeEncoding, sizeof( _read->info.textTypeEncoding ) );
	_read->fread_f( _read, &_read->info.additionalUVCount, sizeof( _read->info.additionalUVCount ) );
	
	if ( _info ) {
		memcpy( _info, &_read->info, sizeof( *_info ) );
	}

	return 0;
}

pmx_byte pmx_read_isize( pmx_read * const _read, pmx_isize * const _isize ) {
	_read->fseek_f( _read, PMX_OFFSET_ISIZE, PMX_READ_SEEK_SET );

	_read->fread_f( _read, &_read->isize.vertex, sizeof( _read->isize.vertex ) );
	_read->fread_f( _read, &_read->isize.texture, sizeof( _read->isize.texture ) );
	_read->fread_f( _read, &_read->isize.material, sizeof( _read->isize.material ) );
	_read->fread_f( _read, &_read->isize.bone, sizeof( _read->isize.bone ) );
	_read->fread_f( _read, &_read->isize.morph, sizeof( _read->isize.morph ) );
	_read->fread_f( _read, &_read->isize.rigidBody, sizeof( _read->isize.rigidBody ) );
	
	if ( _isize ) {
		memcpy( _isize, &_read->isize, sizeof( *_isize ) );
	}

	return 0;
}

pmx_byte pmx_read_minfo( pmx_read * const _read, pmx_minfo * const _minfo ) {
	_read->fseek_f( _read, PMX_OFFSET_MINFO, PMX_READ_SEEK_SET );
	
	pmx_read_text( _read, &_read->minfo.name.local );
	pmx_read_text( _read, &_read->minfo.name.global );
	
	pmx_read_text( _read, &_read->minfo.comment.local );
	pmx_read_text( _read, &_read->minfo.comment.global );
	
	if ( _minfo ) {
		memcpy( _minfo, &_read->minfo, sizeof( *_minfo ) );
	}

	return 0;
}

pmx_int pmx_read_count_vertex( pmx_read * const _read ) {
	pmx_int count = 0;

	_read->fseek_f( _read, _read->offsetVertex, PMX_READ_SEEK_SET );
	_read->fread_f( _read, &count, sizeof( count ) );
	
	_read->countVertex = count;

	return count;
}

pmx_int pmx_read_count_face( pmx_read * const _read ) {
	pmx_int count = 0;
	
	_read->fseek_f( _read, _read->offsetFace, PMX_READ_SEEK_SET );
	_read->fread_f( _read, &count, sizeof( count ) );
	
	_read->countFace = count / 3;

	return _read->countFace;
}

pmx_int pmx_read_count_texture( pmx_read * const _read ) {
	pmx_int count = 0;
	
	_read->fseek_f( _read, _read->offsetTexture, PMX_READ_SEEK_SET );
	_read->fread_f( _read, &count, sizeof( count ) );

	_read->countTexture = count;

	return count;
}

pmx_int pmx_read_sizeof_vertex( pmx_read * const _read, const pmx_int _count ) {
	const pmx_int offset = _read->offsetVertex;
	pmx_int size = 4;
	pmx_int count = _count; // count
	
	if ( count < 0 ) {
		count = pmx_read_count_vertex( _read );
	}

	while ( count-- ) {
		pmx_byte wType;

		size += 32; // pos, norm, uv
		size += 16 * _read->info.additionalUVCount; // additional uv

		_read->fseek_f( _read, offset + size, PMX_READ_SEEK_SET );
		size += _read->fread_f( _read, &wType, 1 );

		switch ( wType ) {
		case 0:
			size += _read->isize.bone;
			break;
		case 1:
			size += _read->isize.bone * 2;
			size += 4;
			break;
		case 2:
			size += _read->isize.bone * 4;
			size += 16;
			break;
		case 3:
			size += _read->isize.bone * 2;
			size += 40;
			break;
		default:
			pmx_print( "Unknown BDEF\n" );
			break;
		}

		size += 4; // edge scale
	}

	return size;
}

pmx_int pmx_read_sizeof_face( pmx_read * const _read, const pmx_int _count ) {
	const pmx_int offset = _read->offsetFace;
	pmx_int size = 4;
	pmx_int count = _count; // count
	
	if ( count < 0 ) {
		count = pmx_read_count_face( _read );
	}

	size += ( count * _read->isize.vertex * 3 );

	return size;
}

pmx_int pmx_read_sizeof_texture( pmx_read * const _read, const pmx_int _count ) {
	const pmx_int offset = _read->offsetTexture;
	pmx_int size = 4;
	pmx_int count = _count; // count

	if ( count < 0 ) {
		count = pmx_read_count_texture( _read );
	}
	
	_read->fseek_f( _read, offset + size, PMX_READ_SEEK_SET );

	while ( count-- ) {
		pmx_text text;

		text.size = 0;
		text.string.utf8 = PMX_NULL;
		pmx_read_text( _read, &text );

		size += text.size + 4;

		pmx_text_print( &text, &_read->info );
		pmx_print( " [%d]\n", text.size );

		pmx_alloc( text.string.utf8, 0 );
		
	}

	return size;
}
