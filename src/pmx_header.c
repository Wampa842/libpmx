#include "pmx_header.h"

#define PMX_HEADER_ENTRIES		( 8 )
#define PMX_HEADER_VERSION		( 2.0f )

static pmx_byte pmx_signature_compare( const pmx_byte * const _signature ) {
	pmx_byte errMask = 0x0;

	if ( _signature[0] != 'P' ) {
		errMask |= 0x1;
	}
	if ( _signature[1] != 'M' ) {
		errMask |= 0x2;
	}
	if ( _signature[2] != 'X' ) {
		errMask |= 0x4;
	}
	if ( _signature[3] != ' ' ) {
		errMask |= 0x8;
	}

	return errMask;
}

pmx_header * pmx_header_create() {
	pmx_header * const header = ( pmx_header * )pmx_alloc( PMX_NULL, sizeof( *header ) );

	header->entriesCount = PMX_HEADER_ENTRIES;
	header->signature[0] = 'P';
	header->signature[1] = 'M';
	header->signature[2] = 'X';
	header->signature[3] = ' ';
	header->version = PMX_HEADER_VERSION;

	return header;
}

void pmx_header_destroy( pmx_header * const _header ) {
	pmx_alloc( _header, 0 );
}

pmx_byte pmx_header_check( const pmx_header * const _header ) {
	pmx_byte errMask = 0x0;

	if ( pmx_signature_compare( _header->signature ) != 0 ) {
		errMask |= 0x1;
	}
	if ( _header->version != PMX_HEADER_VERSION ) {
		errMask |= 0x2;
	}
	if ( _header->entriesCount != PMX_HEADER_ENTRIES ) {
		errMask |= 0x4;
	}

	return errMask;
}